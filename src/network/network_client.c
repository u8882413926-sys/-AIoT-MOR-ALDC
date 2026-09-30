/*
 * network_client.c
 * ===========================================================================
 * Board-to-Laptop Data Link Engine — USB Serial (UART) & SEGGER RTT
 *
 * Transmits real-time AIoT-MOR-ALDC data packets from EK-RA8P1 to the laptop
 * over the standard USB connection (Virtual COM / UART and SEGGER RTT).
 *
 * NO EXTRA HARDWARE NEEDED:
 * Simply connect the USB debug cable (J10) between the EK-RA8P1 and laptop.
 * The data is streamed simultaneously via:
 *   1. SEGGER RTT (Real-Time Transfer) over the debug USB connection.
 *   2. Hardware UART / Virtual COM port (SCI_B UART if enabled in FSP).
 *   3. Standard I/O (printf).
 *
 * JSON FORMAT (one line per packet, newline-delimited):
 * {"pkt":1,"temp":24.50,"hum":55.00,"class":"SMOOTH","class_id":5,
 *  "anomaly":false,"compress":true,"cr":2.14,"orig":400,"comp":187,
 *  "ms":3,"ts":"2026-07-15T14:18:36","uptime":120,
 *  "feat":[24.50,0.12,24.10,24.90,24.30,24.70,0.01,0.08]}
 * ===========================================================================
 */

#include "network_client.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* ========================================================================== *
 *  1. EMBEDDED SEGGER RTT ENGINE (Zero-wire debug USB streaming)             *
 * ========================================================================== */

#define RTT_UP_BUF_SIZE     4096
#define RTT_DOWN_BUF_SIZE   64

typedef struct {
    const char           *sName;
    char                 *pBuffer;
    unsigned int          SizeOfBuffer;
    unsigned int          WrOff;
    volatile unsigned int RdOff;
    unsigned int          Flags;
} rtt_ring_buf_t;

typedef struct {
    char            acID[16];
    int             MaxNumUpBuffers;
    int             MaxNumDownBuffers;
    rtt_ring_buf_t  aUp[1];
    rtt_ring_buf_t  aDown[1];
} rtt_control_block_t;

static char s_rtt_up_buf[RTT_UP_BUF_SIZE];
static char s_rtt_down_buf[RTT_DOWN_BUF_SIZE];

/* The J-Link firmware searches RAM for this exact struct name & ID signature */
volatile rtt_control_block_t _SEGGER_RTT = {
    .acID = "SEGGER RTT\0\0\0\0\0",
    .MaxNumUpBuffers   = 1,
    .MaxNumDownBuffers = 1,
    .aUp = {
        {
            .sName        = "Terminal",
            .pBuffer      = s_rtt_up_buf,
            .SizeOfBuffer = RTT_UP_BUF_SIZE,
            .WrOff        = 0,
            .RdOff        = 0,
            .Flags        = 2 /* NO_BLOCK_TRIM: trim on overflow, never hang */
        }
    },
    .aDown = {
        {
            .sName        = "Terminal",
            .pBuffer      = s_rtt_down_buf,
            .SizeOfBuffer = RTT_DOWN_BUF_SIZE,
            .WrOff        = 0,
            .RdOff        = 0,
            .Flags        = 0
        }
    }
};

static void rtt_write_data(const char *data, unsigned int len)
{
    unsigned int wr = _SEGGER_RTT.aUp[0].WrOff;
    unsigned int rd = _SEGGER_RTT.aUp[0].RdOff;
    unsigned int size = _SEGGER_RTT.aUp[0].SizeOfBuffer;
    char *buf = _SEGGER_RTT.aUp[0].pBuffer;

    for (unsigned int i = 0; i < len; i++)
    {
        unsigned int next = wr + 1;
        if (next >= size) next = 0;
        if (next == rd)
        {
            /* Ring buffer full: trim rest of packet to prevent stalling MCU */
            break;
        }
        buf[wr] = data[i];
        wr = next;
    }
    _SEGGER_RTT.aUp[0].WrOff = wr;
}

/* ========================================================================== *
 *  2. HARDWARE UART SUPPORT (SCI_B UART if present in FSP)                   *
 * ========================================================================== */

#if defined(__has_include) && __has_include("r_uart_api.h")
#include "r_uart_api.h"
extern const uart_instance_t g_uart0 __attribute__((weak));
static bool s_uart_available = false;

static void uart_init_if_available(void)
{
    if (&g_uart0 != NULL && g_uart0.p_api != NULL)
    {
        fsp_err_t err = g_uart0.p_api->open(g_uart0.p_ctrl, g_uart0.p_cfg);
        if (err == FSP_SUCCESS || err == FSP_ERR_ALREADY_OPEN)
        {
            s_uart_available = true;
        }
    }
}

static void uart_write_data(const char *data, unsigned int len)
{
    if (s_uart_available && &g_uart0 != NULL && g_uart0.p_api != NULL)
    {
        g_uart0.p_api->write(g_uart0.p_ctrl, (uint8_t const *)data, len);
    }
}
#else
static void uart_init_if_available(void) {}
static void uart_write_data(const char *data, unsigned int len)
{
    (void)data;
    (void)len;
}
#endif

/* ========================================================================== *
 *  3. HIGH-SPEED EMBEDDED JSON FORMATTING                                    *
 * ========================================================================== */

static int float_to_str(char *buf, float val)
{
    int neg = 0;
    if (val < 0.0f) { neg = 1; val = -val; }

    int whole = (int)val;
    int frac  = (int)((val - (float)whole) * 100.0f + 0.5f);
    if (frac >= 100) { whole++; frac -= 100; }

    int n = 0;
    if (neg) buf[n++] = '-';

    char tmp[12];
    int  ti = 0;
    if (whole == 0) { tmp[ti++] = '0'; }
    else { while (whole > 0) { tmp[ti++] = (char)('0' + (whole % 10)); whole /= 10; } }
    for (int i = ti - 1; i >= 0; i--) buf[n++] = tmp[i];

    buf[n++] = '.';
    buf[n++] = (char)('0' + (frac / 10));
    buf[n++] = (char)('0' + (frac % 10));
    buf[n]   = '\0';
    return n;
}

static int uint_to_str(char *buf, uint32_t val)
{
    char tmp[12];
    int ti = 0;
    if (val == 0) { tmp[ti++] = '0'; }
    else { while (val > 0) { tmp[ti++] = (char)('0' + (val % 10)); val /= 10; } }

    for (int i = ti - 1; i >= 0; i--) buf[i] = tmp[ti - 1 - i];
    buf[ti] = '\0';
    return ti;
}

static int append_str(char *dst, int pos, const char *src)
{
    while (*src) dst[pos++] = *src++;
    return pos;
}

static int build_json(char *json, int max_len, const net_packet_t *pkt)
{
    (void)max_len;
    int p = 0;

    /* Opening brace + packet_id */
    p = append_str(json, p, "{\"pkt\":");
    p += uint_to_str(json + p, pkt->packet_id);

    /* Temperature */
    p = append_str(json, p, ",\"temp\":");
    p += float_to_str(json + p, pkt->temp_c);

    /* Humidity */
    p = append_str(json, p, ",\"hum\":");
    p += float_to_str(json + p, pkt->humidity);

    /* Signal class */
    p = append_str(json, p, ",\"class\":\"");
    p = append_str(json, p, pkt->signal_class_name ? pkt->signal_class_name : "UNKNOWN");
    p = append_str(json, p, "\"");

    p = append_str(json, p, ",\"class_id\":");
    p += uint_to_str(json + p, (uint32_t)pkt->signal_class);

    /* Anomaly flag */
    p = append_str(json, p, ",\"anomaly\":");
    p = append_str(json, p, pkt->is_anomaly ? "true" : "false");

    /* Compression flag + ratio */
    p = append_str(json, p, ",\"compress\":");
    p = append_str(json, p, pkt->do_compress ? "true" : "false");

    p = append_str(json, p, ",\"cr\":");
    p += float_to_str(json + p, pkt->cr);

    /* Byte sizes */
    p = append_str(json, p, ",\"orig\":");
    p += uint_to_str(json + p, pkt->orig_bytes);

    p = append_str(json, p, ",\"comp\":");
    p += uint_to_str(json + p, pkt->comp_bytes);

    /* Inference time */
    p = append_str(json, p, ",\"ms\":");
    p += uint_to_str(json + p, pkt->time_ms);

    /* RTC Timestamp: "YYYY-MM-DDThh:mm:ss" */
    p = append_str(json, p, ",\"ts\":\"");
    int yr = pkt->timestamp.tm_year + 1900;
    p += uint_to_str(json + p, (uint32_t)yr);
    json[p++] = '-';
    int mon = pkt->timestamp.tm_mon + 1;
    if (mon < 10) json[p++] = '0';
    p += uint_to_str(json + p, (uint32_t)mon);
    json[p++] = '-';
    if (pkt->timestamp.tm_mday < 10) json[p++] = '0';
    p += uint_to_str(json + p, (uint32_t)pkt->timestamp.tm_mday);
    json[p++] = 'T';
    if (pkt->timestamp.tm_hour < 10) json[p++] = '0';
    p += uint_to_str(json + p, (uint32_t)pkt->timestamp.tm_hour);
    json[p++] = ':';
    if (pkt->timestamp.tm_min < 10) json[p++] = '0';
    p += uint_to_str(json + p, (uint32_t)pkt->timestamp.tm_min);
    json[p++] = ':';
    if (pkt->timestamp.tm_sec < 10) json[p++] = '0';
    p += uint_to_str(json + p, (uint32_t)pkt->timestamp.tm_sec);
    p = append_str(json, p, "\"");

    /* Uptime */
    p = append_str(json, p, ",\"uptime\":");
    p += uint_to_str(json + p, pkt->uptime_sec);

    /* Features array */
    p = append_str(json, p, ",\"feat\":[");
    for (int i = 0; i < 8; i++)
    {
        if (i > 0) json[p++] = ',';
        p += float_to_str(json + p, pkt->features[i]);
    }
    json[p++] = ']';

    /* Sensor source tag: "DHT11", "SIMULATE", or "RTT-INJECT" */
    p = append_str(json, p, ",\"src\":\"");
    if (pkt->src_sensor)        p = append_str(json, p, "DHT11");
    else if (pkt->src_injected) p = append_str(json, p, "RTT-INJECT");
    else                        p = append_str(json, p, "SIMULATE");
    json[p++] = '"';

    /* Closing brace + newline delimiter */
    json[p++] = '}';
    json[p++] = '\n';
    json[p]   = '\0';

    return p;
}

/* ========================================================================== *
 *  4. PUBLIC API                                                             *
 * ========================================================================== */

static bool s_initialized = false;

bool network_client_init(void)
{
    /* Initialize RTT buffer pointers and offsets */
    _SEGGER_RTT.aUp[0].WrOff = 0;
    _SEGGER_RTT.aUp[0].RdOff = 0;

    /* Initialize UART if available */
    uart_init_if_available();

    /* Send initial startup handshake line */
    const char *banner = "{\"status\":\"ready\",\"device\":\"EK-RA8P1\",\"link\":\"USB_SERIAL_RTT\"}\n";
    rtt_write_data(banner, (unsigned int)strlen(banner));
    uart_write_data(banner, (unsigned int)strlen(banner));

    s_initialized = true;
    return true;
}

bool network_client_send(const net_packet_t *pkt)
{
    if (pkt == NULL) return false;

    if (!s_initialized)
    {
        network_client_init();
    }

    static char json_buf[NET_JSON_BUF_SIZE];
    int len = build_json(json_buf, sizeof(json_buf), pkt);
    if (len <= 0) return false;

    /* 1. Transmit via SEGGER RTT (high speed over debug USB cable) */
    rtt_write_data(json_buf, (unsigned int)len);

    /* 2. Transmit via hardware UART / Virtual COM port if configured */
    uart_write_data(json_buf, (unsigned int)len);

    return true;
}

void network_client_close(void)
{
    s_initialized = false;
}

bool network_client_is_connected(void)
{
    return s_initialized;
}
