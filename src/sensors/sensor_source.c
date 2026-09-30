/*
 * sensor_source.c
 * ===========================================================================
 * Unified Sensor Source Implementation — AIoT-MOR-ALDC
 * Renesas EK-RA8P1 | ARM Cortex-M85 @ 480 MHz
 *
 * Implements two runtime-selectable modes:
 *  1. SENSOR_MODE_LIVE_DHT11   — Physical DHT11 hardware sensor on GPIO P410
 *  2. SENSOR_MODE_DATA_DRIVEN  — Live laptop streaming via USB RTT/UART (sensor_data.txt)
 *                               with automated benchmark dataset fallback
 * ===========================================================================
 */

#include "sensor_source.h"
#include "dht11_driver.h"

/* ── FreeRTOS ────────────────────────────────────────────────────────────── */
#include "FreeRTOS.h"
#include "task.h"

/* ── Standard C ─────────────────────────────────────────────────────────── */
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

/* ==========================================================================
 * SEGGER RTT Control Block Forward Declaration
 * Defined in network_client.c, located by J-Link firmware in RAM.
 * ========================================================================== */
typedef struct {
    const char           *sName;
    char                 *pBuffer;
    unsigned int          SizeOfBuffer;
    unsigned int          WrOff;
    volatile unsigned int RdOff;
    unsigned int          Flags;
} _rtt_ring_buf_t;

typedef struct {
    char            acID[16];
    int             MaxNumUpBuffers;
    int             MaxNumDownBuffers;
    _rtt_ring_buf_t aUp[1];
    _rtt_ring_buf_t aDown[1];
} _rtt_cb_t;

extern volatile _rtt_cb_t _SEGGER_RTT;

static int rtt_read_byte(void)
{
    volatile _rtt_ring_buf_t *down = &_SEGGER_RTT.aDown[0];
    unsigned int wr = down->WrOff;
    unsigned int rd = down->RdOff;

    if (rd == wr) return -1;   /* empty */

    int ch = (unsigned char)down->pBuffer[rd];
    down->RdOff = (rd + 1 >= down->SizeOfBuffer) ? 0 : (rd + 1);
    return ch;
}

/* ==========================================================================
 * Fast Pseudo-Random Generator (LCG)
 * ========================================================================== */
static inline uint32_t _fast_rand(void)
{
    static uint32_t seed = 987654321U;
    seed = 1103515245U * seed + 12345U;
    return (seed >> 16) & 0x7FFFU;
}

/* ==========================================================================
 * Preloaded Benchmark Dataset (from sensor_data.txt)
 * Provides realistic thermal profiles with baseline, warm-up, critical
 * thermal anomaly (>40 C), and cooling recovery when laptop feeder is idle.
 * ========================================================================== */
static const struct {
    float temp;
    float hum;
} s_benchmark_dataset[] = {
    /* Phase 1: Baseline Ambient (24.2 - 25.0 C) */
    {24.20f, 54.0f}, {24.25f, 54.1f}, {24.30f, 54.0f}, {24.35f, 54.2f},
    {24.40f, 54.5f}, {24.45f, 54.6f}, {24.50f, 55.0f}, {24.55f, 55.1f},
    {24.60f, 55.4f}, {24.70f, 55.6f}, {24.80f, 56.0f}, {24.90f, 56.2f},
    {25.00f, 56.5f},

    /* Phase 2: Workload Heating & Dynamic Ramp (25.5 - 33.5 C) */
    {25.45f, 57.0f}, {26.10f, 57.6f}, {26.95f, 58.4f}, {27.90f, 59.3f},
    {29.00f, 60.2f}, {30.15f, 61.1f}, {31.25f, 62.0f}, {32.30f, 62.8f},
    {33.15f, 63.4f}, {33.50f, 63.7f},

    /* Phase 3: Steady High Compute (33.8 - 34.3 C) */
    {33.80f, 64.1f}, {34.00f, 64.3f}, {34.10f, 64.5f}, {34.20f, 64.7f},
    {34.30f, 64.9f}, {34.20f, 64.7f}, {34.15f, 64.6f},

    /* Phase 4: Critical Thermal Runaway Anomaly Spike (Triggers Isolation Forest!) */
    {37.20f, 68.5f}, {41.50f, 73.0f}, {45.10f, 76.5f}, {48.50f, 79.5f},
    {49.80f, 81.2f}, {49.50f, 81.0f}, {48.20f, 79.8f},

    /* Phase 5: Cooling Recovery & Return to Baseline */
    {43.00f, 75.0f}, {38.50f, 70.0f}, {33.00f, 64.0f}, {29.50f, 60.0f},
    {26.80f, 57.5f}, {25.20f, 55.5f}, {24.50f, 55.0f}
};
#define BENCHMARK_COUNT (sizeof(s_benchmark_dataset) / sizeof(s_benchmark_dataset[0]))

/* ==========================================================================
 * Module State
 * ========================================================================== */
static sensor_mode_t s_current_mode = SENSOR_MODE_DATA_DRIVEN;
static float         s_last_temp    = 24.50f;
static float         s_last_hum     = 55.00f;
static uint32_t      s_bench_idx    = 0U;
static bool          s_got_laptop_data = false;

#define RTT_LINE_BUF_LEN 64
static char          s_line_buf[RTT_LINE_BUF_LEN];
static uint32_t      s_line_pos = 0U;

/* Embedded atof parser for sign, integer, and decimal fractions */
static float simple_atof(const char *s)
{
    float result = 0.0f;
    float sign   = 1.0f;

    while (*s == ' ' || *s == '\t') s++;

    if (*s == '-') { sign = -1.0f; s++; }
    else if (*s == '+') { s++; }

    while (*s >= '0' && *s <= '9')
    {
        result = result * 10.0f + (float)(*s - '0');
        s++;
    }

    if (*s == '.')
    {
        s++;
        float frac = 0.1f;
        while (*s >= '0' && *s <= '9')
        {
            result += (float)(*s - '0') * frac;
            frac   *= 0.1f;
            s++;
        }
    }

    return sign * result;
}

/*
 * parse_incoming_line:
 * Supports:
 *   1. "24.50, 55.0" / "24.50,55.0" (laptop_sensor_feeder.py format)
 *   2. "T:24.50, H:55.0" / "T:24.50"
 *   3. "24.50" (single temperature value)
 */
static bool parse_incoming_line(const char *line)
{
    if (line == NULL || *line == '\0') return false;

    /* Check for 'T:' or 't:' tag */
    const char *pt = strstr(line, "T:");
    if (!pt) pt = strstr(line, "t:");

    const char *ph = strstr(line, "H:");
    if (!ph) ph = strstr(line, "h:");

    if (pt != NULL || ph != NULL)
    {
        /* Tagged format */
        if (pt)
        {
            float t = simple_atof(pt + 2);
            if (t >= -40.0f && t <= 120.0f) s_last_temp = t;
        }
        if (ph)
        {
            float h = simple_atof(ph + 2);
            if (h >= 0.0f && h <= 100.0f) s_last_hum = h;
        }
        return true;
    }

    /* Comma-separated format: "24.50, 55.0" */
    const char *comma = strchr(line, ',');
    if (comma != NULL)
    {
        float t = simple_atof(line);
        float h = simple_atof(comma + 1);
        if (t >= -40.0f && t <= 120.0f) s_last_temp = t;
        if (h >= 0.0f && h <= 100.0f)   s_last_hum  = h;
        return true;
    }

    /* Single numeric value */
    float single = simple_atof(line);
    if (single >= -40.0f && single <= 120.0f)
    {
        s_last_temp = single;
        return true;
    }

    return false;
}

static bool poll_rtt_down_buffer(void)
{
    bool got_new = false;
    int ch;

    while ((ch = rtt_read_byte()) >= 0)
    {
        if (ch == '\n' || ch == '\r')
        {
            if (s_line_pos > 0U)
            {
                s_line_buf[s_line_pos] = '\0';
                if (parse_incoming_line(s_line_buf))
                {
                    got_new = true;
                    s_got_laptop_data = true;
                }
                s_line_pos = 0U;
            }
        }
        else
        {
            if (s_line_pos < (RTT_LINE_BUF_LEN - 1U))
            {
                s_line_buf[s_line_pos++] = (char)ch;
            }
            else
            {
                s_line_pos = 0U; /* overflow discard */
            }
        }
    }

    return got_new;
}

/* ==========================================================================
 * Public API
 * ========================================================================== */

void sensor_source_init(void)
{
    /* Initialize physical DHT11 GPIO pin to idle HIGH */
    dht11_init();

    s_line_pos        = 0U;
    s_got_laptop_data = false;
    s_bench_idx       = 0U;
    s_last_temp       = 24.50f;
    s_last_hum        = 55.00f;
}

void sensor_source_set_mode(sensor_mode_t mode)
{
    s_current_mode = mode;
}

sensor_mode_t sensor_source_get_mode(void)
{
    return s_current_mode;
}

bool sensor_source_read(sensor_reading_t *out)
{
    if (out == NULL) return false;

    out->is_injected  = false;
    out->is_simulated = false;
    out->is_sensor    = false;

    if (s_current_mode == SENSOR_MODE_LIVE_DHT11)
    {
        /* ── LIVE SENSOR MODE: Read DHT11 on GPIO P410 ──────────────── */
        dht11_data_t raw;
        dht11_err_t  err = dht11_read(&raw);

        if (err == DHT11_OK)
        {
            out->temperature_c = raw.temperature_c;
            out->humidity_pct  = raw.humidity_pct;
            out->is_sensor     = true;
            s_last_temp        = raw.temperature_c;
            s_last_hum         = raw.humidity_pct;
            return true;
        }

        /* If physical sensor is not wired or CRC fails, fallback smoothly */
        out->is_sensor    = false;
        out->is_simulated = true;
        s_last_temp += ((float)(_fast_rand() % 5U) - 2.0f) * 0.10f;
        if (s_last_temp < 20.0f) s_last_temp = 22.0f;
        if (s_last_temp > 35.0f) s_last_temp = 32.0f;
        out->temperature_c = s_last_temp;
        out->humidity_pct  = s_last_hum;
        return true;
    }
    else
    {
        /* ── DATA-DRIVEN MODE: Laptop Injection / Dataset ────────────── */
        bool got_new = poll_rtt_down_buffer();

        if (got_new)
        {
            /* Actively received fresh sample from laptop */
            out->temperature_c = s_last_temp;
            out->humidity_pct  = s_last_hum;
            out->is_injected   = true;
            return true;
        }

        if (s_got_laptop_data)
        {
            /* Laptop was streaming; hold last injected value */
            out->temperature_c = s_last_temp;
            out->humidity_pct  = s_last_hum;
            out->is_injected   = true;
            return true;
        }

        /* If laptop feeder is not running yet, step through benchmark dataset */
        s_last_temp = s_benchmark_dataset[s_bench_idx].temp;
        s_last_hum  = s_benchmark_dataset[s_bench_idx].hum;
        s_bench_idx = (s_bench_idx + 1U) % BENCHMARK_COUNT;

        out->temperature_c = s_last_temp;
        out->humidity_pct  = s_last_hum;
        out->is_injected   = true;
        out->is_simulated  = true;
        return true;
    }
}

const char *sensor_source_mode_name(void)
{
    if (s_current_mode == SENSOR_MODE_LIVE_DHT11)
    {
        return "LIVE SENSOR (DHT11 @ P410)";
    }
    else
    {
        return "DATA-DRIVEN (LAPTOP / USB)";
    }
}

const char *sensor_source_mode_badge(void)
{
    if (s_current_mode == SENSOR_MODE_LIVE_DHT11)
    {
        return "LIVE DHT11";
    }
    else
    {
        return "DATA-DRIVEN";
    }
}

int sensor_source_check_input_cmd(void)
{
    volatile _rtt_ring_buf_t *down = &_SEGGER_RTT.aDown[0];
    if (down->RdOff != down->WrOff)
    {
        char ch = down->pBuffer[down->RdOff];
        if (ch == '1' || ch == 'l' || ch == 'L')
        {
            down->RdOff = (down->RdOff + 1 >= down->SizeOfBuffer) ? 0 : (down->RdOff + 1);
            return 1;
        }
        if (ch == '2' || ch == 'd' || ch == 'D')
        {
            down->RdOff = (down->RdOff + 1 >= down->SizeOfBuffer) ? 0 : (down->RdOff + 1);
            return 2;
        }
    }
    return 0;
}

