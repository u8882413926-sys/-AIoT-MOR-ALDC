void app_main(void);
/*
 * hal_entry.c
 * ===========================================================================
 * AIoT-MOR-ALDC — Main Application Entry Point
 * RK-RA8P1  |  FSP v6.5+  |  FreeRTOS
 *
 * TASK ARCHITECTURE:
 * ──────────────────────────────────────────────────────────────────────────
 *  Task Name          Priority   Stack   Period      What it does
 *  ─────────────────  ────────   ─────   ──────      ────────────
 *  sensor_task        High (4)   1024B   1 s         Read AHT21, buffer 100 readings
 *  ai_task            Normal(3)  4096B   per window  Run AI pipeline, compress
 *  display_task       Low  (2)   8192B   500 ms      Update LVGL screen
 *  network_task       Low  (2)   4096B   per result  Send JSON to server
 * ──────────────────────────────────────────────────────────────────────────
 *
 * DATA FLOW:
 *  AHT21 sensor → sensor_task → [ring buffer] → ai_task
 *                                                   ↓
 *                                             ai_result + compression
 *                                                   ↓
 *                                     ┌─────────────┴──────────────┐
 *                                  display_task              network_task
 *                                  (LVGL screen)         (TCP → laptop)
 *
 * INTER-TASK COMMUNICATION:
 *   - g_sensor_queue : sensor_task  → ai_task      (100 floats per message)
 *   - g_result_queue : ai_task      → display_task  (ai_result_t)
 *   - g_result_queue : ai_task      → network_task  (ai_result_t)
 * ===========================================================================
 */

#include "hal_data.h"

/* ── FreeRTOS ─────────────────────────────────────────────────────────── */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* ── Application modules ──────────────────────────────────────────────── */
#include "sensors/sensor_source.h"   /* unified DHT11 / simulate / RTT-inject */
#include "ai/ai_pipeline.h"
#include "compression/residual_compress.h"
#include "display/display_ui.h"
#include "network/network_client.h"

/* ── LVGL (from FSP LVGL port) ────────────────────────────────────────── */
#include "lvgl.h"
#include "lv_port_display.h"   /* Renesas RA8P1 LVGL display port          */
#include "lv_port_indev.h"     /* Touch input (optional — for touch events) */

/* ── RTC (FSP r_rtc driver) ────────────────────────────────────────────── */
/* The r_rtc.h header is pulled in via hal_data.h when the RTC peripheral   */
/* is enabled in the FSP Configurator. We include string.h for memset().    */
#include <string.h>
#include <stdlib.h>

/* ── Hardware Push Buttons on EK-RA8P1 ────────────────────────────────── */
#ifndef USER_SW1
#define USER_SW1 (BSP_IO_PORT_00_PIN_09)   /* S1: Live Sensor Mode (P410) */
#endif
#ifndef USER_SW2
#define USER_SW2 (BSP_IO_PORT_00_PIN_08)   /* S2: Data-Driven Mode        */
#endif
#ifndef USER_SW_CFG_INT
#define USER_SW_CFG_INT (BSP_IO_PORT_00_PIN_00) /* S3 / USER_SW push button */
#endif

/* ==========================================================================
 * FreeRTOS stack overflow hook — called when configCHECK_FOR_STACK_OVERFLOW
 * detects a task has overflowed its stack. Inspect these in the debugger.
 *
 * WARNING: Do NOT use __BKPT() here — halting the CPU in debug mode also
 * stops the GLCDC peripheral, which makes the display go BLACK.
 * Instead we just spin with the diagnostic variables set.
 * ========================================================================== */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
void HardFault_Handler(void);

volatile TaskHandle_t g_overflow_task = NULL;
volatile const char  *g_overflow_name = NULL;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    g_overflow_task = xTask;
    g_overflow_name = (const char *)pcTaskName;
    /* Do NOT use __BKPT(0) — it halts CPU which stops GLCDC → black screen */
    while (1) { __NOP(); }
}

/* ==========================================================================
 * HardFault handler — capture fault info without halting GLCDC
 * ========================================================================== */
volatile uint32_t g_fault_lr = 0;
volatile uint32_t g_fault_pc = 0;

void HardFault_Handler(void)
{
    /* Capture the link register for debugging */
    __asm volatile ("MOV %0, LR" : "=r" (g_fault_lr));
    /* Do NOT halt — keep GLCDC running so display stays on for diagnostics */
    while (1) { __NOP(); }
}

/* ==========================================================================
 * Override BSP __assert_func — the default calls __BKPT(0) which halts
 * the CPU and stops the GLCDC → display goes BLACK.
 * This override keeps the display alive so you can see the last frame.
 * ========================================================================== */
volatile const char *g_assert_file = NULL;
volatile int         g_assert_line = 0;

void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    (void)func; (void)expr;
    g_assert_file = file;
    g_assert_line = line;
    /* Do NOT call __BKPT — it stops GLCDC */
    while (1) { __NOP(); }
}

/* ==========================================================================
 * Constants
 * ========================================================================== */
#define SENSOR_WINDOW_SIZE      AI_WINDOW_SIZE   /* 100 readings per window  */
#define SENSOR_PERIOD_MS        1000U            /* 1 reading per second     */
#define DISPLAY_REFRESH_MS      500U             /* LVGL timer tick          */
#define LVGL_TICK_MS            5U               /* LVGL tick period         */

/* ── Set to 1 to enable worker tasks, 0 to run display-only mode ───────── */
#define ENABLE_WORKER_TASKS     1

/* ==========================================================================
 * Shared data structures passed between tasks
 * ========================================================================== */

/* Full message from AI task to display/network tasks */
typedef struct {
    float           temp_c;
    float           humidity;
    ai_result_t     ai;
    float           cr;
    uint32_t        orig_bytes;
    uint32_t        comp_bytes;
    uint32_t        time_ms;
    uint32_t        pkt_id;
    rtc_time_t      timestamp;    /* RTC wall-clock time at window capture    */
    uint32_t        uptime_sec;   /* Seconds since boot (for rate-of-change)  */
} result_message_t;

/* Sensor window message: 100 float readings + humidity snapshot */
typedef struct {
    float window[SENSOR_WINDOW_SIZE];
    float humidity;
    float latest_temp;
} sensor_window_t;



/* ==========================================================================
 * LVGL tick timer (calls lv_tick_inc every LVGL_TICK_MS)
 * Uses a FreeRTOS software timer.
 * ========================================================================== */
#include "timers.h"
__attribute__((unused)) static void lvgl_tick_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    lv_tick_inc(LVGL_TICK_MS);
}

/* ==========================================================================
 * TASK 1: sensor_task
 *
 * Reads the DHT11 sensor once per second (1 Hz max — DHT11 hardware limit).
 * After collecting SENSOR_WINDOW_SIZE readings, sends the window to ai_task.
 * ========================================================================== */
/* ==========================================================================
 * Fast, thread-safe pseudo-random generator
 * Avoids newlib-nano rand() which invokes __assert_func in bare-metal RTOS
 * ========================================================================== */
static inline uint32_t fast_rand(void)
{
    static uint32_t s_seed = 123456789U;
    s_seed = (1103515245U * s_seed + 12345U);
    return (s_seed >> 16) & 0x7FFFU;
}



/* ==========================================================================
 * rtc_set_compile_time
 *
 * Seeds the RA8P1 RTC with the firmware's build timestamp on cold boot.
 * __DATE__ = "Jul 15 2026"   __TIME__ = "14:18:36"
 *
 * Accuracy note: the RTC drifts ~±20 ppm (~1.7 s/day). For sensor-event
 * logging (minutes resolution) this is perfectly adequate. For GPS-locked
 * accuracy, replace with an NTP sync from tcp_server.py on connect.
 * ========================================================================== */
static void rtc_set_compile_time(void)
{
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *d = __DATE__;    /* "Jul 15 2026" */
    const char *t = __TIME__;    /* "14:18:36"    */

    rtc_time_t ts;
    memset(&ts, 0, sizeof(ts));

    /* Month (search in 3-char month table) */
    char mon_buf[4] = { d[0], d[1], d[2], '\0' };
    const char *p   = strstr(month_names, mon_buf);
    ts.tm_mon       = p ? (int)((p - month_names) / 3) : 0;

    /* Day ("Jul  5 2026" has a leading space for single-digit days) */
    ts.tm_mday = ((d[4] != ' ') ? (d[4] - '0') * 10 : 0) + (d[5] - '0');

    /* Year */
    int yr     = (d[7]-'0')*1000 + (d[8]-'0')*100 +
                 (d[9]-'0')*10   + (d[10]-'0');
    ts.tm_year = yr - 1900;

    /* Time */
    ts.tm_hour = (t[0]-'0')*10 + (t[1]-'0');
    ts.tm_min  = (t[3]-'0')*10 + (t[4]-'0');
    ts.tm_sec  = (t[6]-'0')*10 + (t[7]-'0');

    R_RTC_CalendarTimeSet(&g_rtc0_ctrl, &ts);
}

/* ==========================================================================
 * hal_entry  — FSP calls this instead of main()
 * ========================================================================== */
void app_main(void)
{
    /* ── Open the RTC and seed it with the build timestamp ──────────── */
    R_RTC_Open(&g_rtc0_ctrl, &g_rtc0_cfg);
    rtc_set_compile_time();   /* Sets RTC to firmware compile time        */



    /* ── Initialise display, LVGL, and UI immediately on boot ─────────── */
    lv_init();
    lv_port_display_init();

    g_boot_stage = 9;
    g_display_init_stage = 9;

    /* ── Transition to LVGL Edge AI Dashboard ────────────────────────── */
    lv_port_indev_init();

    /* Initialize sensor subsystem and configure hardware push buttons */
    R_BSP_PinAccessEnable();
    R_IOPORT_PinCfg(&g_ioport_ctrl, USER_SW1,
                    IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);
    R_IOPORT_PinCfg(&g_ioport_ctrl, USER_SW2,
                    IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);
    R_IOPORT_PinCfg(&g_ioport_ctrl, USER_SW_CFG_INT,
                    IOPORT_CFG_PORT_DIRECTION_INPUT | IOPORT_CFG_PULLUP_ENABLE);
    sensor_source_init();
    R_BSP_PinAccessDisable();

    /* Initialize Edge AI pipeline */
    ai_pipeline_init();

    /* Pre-fill initial window with baseline temperature */
    float window[SENSOR_WINDOW_SIZE];
    for (uint32_t i = 0; i < SENSOR_WINDOW_SIZE; i++)
    {
        window[i] = 24.5f + ((float)(fast_rand() % 7) - 3.0f) * 0.08f;
    }

    /* Initialize UI layout (creates header, the two mode boxes, and all telemetry cards) */
    display_ui_init();

    /* ── STARTUP MODE SELECTION PROMPT ON BOARD (20-Second Countdown) ──── *
     * S1 (P009) -> Live Sensor Mode (DHT11 @ P410)                         *
     * S2 (P008) -> Data-Driven Mode (Laptop / sensor_data.txt)             *
     * USER_SW (P000) -> Toggles mode                                      *
     * Console input ('1' or '2') also works. Default: Data-Driven          *
     * ──────────────────────────────────────────────────────────────────── */
    sensor_mode_t chosen_mode = SENSOR_MODE_DATA_DRIVEN;
    sensor_source_set_mode(chosen_mode);
    display_ui_set_active_mode((int)chosen_mode);

    display_ui_show_startup_modal(20, sensor_source_mode_name());

    const uint32_t countdown_total_ms = 20000U;
    TickType_t select_start = xTaskGetTickCount();
    uint32_t last_remaining_s = 99;
    bool user_made_selection = false;

    while (!user_made_selection && (xTaskGetTickCount() - select_start < pdMS_TO_TICKS(countdown_total_ms)))
    {
        uint32_t elapsed_ms = pdTICKS_TO_MS(xTaskGetTickCount() - select_start);
        uint32_t remaining_s = (countdown_total_ms > elapsed_ms) ?
                               ((countdown_total_ms - elapsed_ms + 999U) / 1000U) : 0U;

        if (remaining_s != last_remaining_s)
        {
            last_remaining_s = remaining_s;
            display_ui_show_startup_modal(remaining_s, sensor_source_mode_name());
        }

        /* Check Hardware Button S1 (P009) -> Live Sensor Mode */
        bsp_io_level_t s1_val = BSP_IO_LEVEL_HIGH;
        R_IOPORT_PinRead(&g_ioport_ctrl, USER_SW1, &s1_val);
        if (s1_val == BSP_IO_LEVEL_LOW)
        {
            chosen_mode = SENSOR_MODE_LIVE_DHT11;
            sensor_source_set_mode(chosen_mode);
            user_made_selection = true;
            display_ui_set_active_mode((int)chosen_mode);
            display_ui_show_startup_modal(0, sensor_source_mode_name());
            vTaskDelay(pdMS_TO_TICKS(700));
            break;
        }

        /* Check Hardware Button S2 (P008) -> Data-Driven Mode */
        bsp_io_level_t s2_val = BSP_IO_LEVEL_HIGH;
        R_IOPORT_PinRead(&g_ioport_ctrl, USER_SW2, &s2_val);
        if (s2_val == BSP_IO_LEVEL_LOW)
        {
            chosen_mode = SENSOR_MODE_DATA_DRIVEN;
            sensor_source_set_mode(chosen_mode);
            user_made_selection = true;
            display_ui_set_active_mode((int)chosen_mode);
            display_ui_show_startup_modal(0, sensor_source_mode_name());
            vTaskDelay(pdMS_TO_TICKS(700));
            break;
        }

        /* Check User Switch (P000) -> Toggles selection */
        bsp_io_level_t sw_val = BSP_IO_LEVEL_HIGH;
        R_IOPORT_PinRead(&g_ioport_ctrl, USER_SW_CFG_INT, &sw_val);
        if (sw_val == BSP_IO_LEVEL_LOW)
        {
            chosen_mode = (chosen_mode == SENSOR_MODE_LIVE_DHT11) ?
                          SENSOR_MODE_DATA_DRIVEN : SENSOR_MODE_LIVE_DHT11;
            sensor_source_set_mode(chosen_mode);
            user_made_selection = true;
            display_ui_set_active_mode((int)chosen_mode);
            display_ui_show_startup_modal(0, sensor_source_mode_name());
            vTaskDelay(pdMS_TO_TICKS(700));
            break;
        }

        /* Check Console / RTT Command ('1' or '2') */
        int cmd = sensor_source_check_input_cmd();
        if (cmd == 1)
        {
            chosen_mode = SENSOR_MODE_LIVE_DHT11;
            sensor_source_set_mode(chosen_mode);
            user_made_selection = true;
            display_ui_set_active_mode((int)chosen_mode);
            display_ui_show_startup_modal(0, sensor_source_mode_name());
            vTaskDelay(pdMS_TO_TICKS(700));
            break;
        }
        else if (cmd == 2)
        {
            chosen_mode = SENSOR_MODE_DATA_DRIVEN;
            sensor_source_set_mode(chosen_mode);
            user_made_selection = true;
            display_ui_set_active_mode((int)chosen_mode);
            display_ui_show_startup_modal(0, sensor_source_mode_name());
            vTaskDelay(pdMS_TO_TICKS(700));
            break;
        }

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    /* Hide startup modal and reveal full dashboard */
    display_ui_hide_startup_modal();
    display_ui_set_active_mode((int)chosen_mode);

    /* Run initial AI inference on baseline window and update UI */
    ai_result_t init_ai;
    memset(&init_ai, 0, sizeof(init_ai));
    ai_pipeline_run(window, &init_ai);

    rtc_time_t init_ts;
    memset(&init_ts, 0, sizeof(init_ts));
    R_RTC_CalendarTimeGet(&g_rtc0_ctrl, &init_ts);
    display_ui_update(24.5f, 55.0f, &init_ai, 2.14f, 400, 187, 3, 1, &init_ts, 0, sensor_source_mode_badge());

    /* Force immediate first frame render so full dashboard appears right away */
    lv_timer_handler();

    g_boot_stage = 10;
    g_display_init_stage = 10;

    /* ── Initialise Ethernet TCP link to laptop (non-blocking) ────────── *
     * If no cable or server is present, this returns false and the board *
     * continues running normally with display-only mode.                 */
    bool net_ok = network_client_init();
    (void)net_ok;

    /* ── ROBUST MAIN OPERATIONAL LOOP ────────────────────────────────── */
    TickType_t last_update = xTaskGetTickCount();
    uint32_t sim_pkt = 1;
    static uint8_t comp_buf[COMPRESS_MAX_OUT_BYTES];

    while (1)
    {
        TickType_t now = xTaskGetTickCount();

        /* Check runtime button presses to allow instant mode toggling */
        bsp_io_level_t rt_s1 = BSP_IO_LEVEL_HIGH;
        bsp_io_level_t rt_s2 = BSP_IO_LEVEL_HIGH;
        bsp_io_level_t rt_sw = BSP_IO_LEVEL_HIGH;
        R_IOPORT_PinRead(&g_ioport_ctrl, USER_SW1, &rt_s1);
        R_IOPORT_PinRead(&g_ioport_ctrl, USER_SW2, &rt_s2);
        R_IOPORT_PinRead(&g_ioport_ctrl, USER_SW_CFG_INT, &rt_sw);

        if (rt_s1 == BSP_IO_LEVEL_LOW && sensor_source_get_mode() != SENSOR_MODE_LIVE_DHT11)
        {
            sensor_source_set_mode(SENSOR_MODE_LIVE_DHT11);
            display_ui_set_active_mode(1);
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        else if (rt_s2 == BSP_IO_LEVEL_LOW && sensor_source_get_mode() != SENSOR_MODE_DATA_DRIVEN)
        {
            sensor_source_set_mode(SENSOR_MODE_DATA_DRIVEN);
            display_ui_set_active_mode(2);
            vTaskDelay(pdMS_TO_TICKS(250));
        }
        else if (rt_sw == BSP_IO_LEVEL_LOW)
        {
            sensor_mode_t next_m = (sensor_source_get_mode() == SENSOR_MODE_LIVE_DHT11) ?
                                   SENSOR_MODE_DATA_DRIVEN : SENSOR_MODE_LIVE_DHT11;
            sensor_source_set_mode(next_m);
            display_ui_set_active_mode((int)next_m);
            vTaskDelay(pdMS_TO_TICKS(350));
        }

        int rt_cmd = sensor_source_check_input_cmd();
        if (rt_cmd == 1 && sensor_source_get_mode() != SENSOR_MODE_LIVE_DHT11)
        {
            sensor_source_set_mode(SENSOR_MODE_LIVE_DHT11);
            display_ui_set_active_mode(1);
        }
        else if (rt_cmd == 2 && sensor_source_get_mode() != SENSOR_MODE_DATA_DRIVEN)
        {
            sensor_source_set_mode(SENSOR_MODE_DATA_DRIVEN);
            display_ui_set_active_mode(2);
        }

        /* Every 1 second (1 Hz), read sensor / AI / compression and update UI */
        if ((now - last_update) >= pdMS_TO_TICKS(1000))
        {
            last_update = now;
            sim_pkt++;

            /* 1. Acquire sensor data */
            sensor_reading_t reading;
            sensor_source_read(&reading);

            /* Slide window */
            for (uint32_t i = 0; i < SENSOR_WINDOW_SIZE - 1; i++)
            {
                window[i] = window[i + 1];
            }
            window[SENSOR_WINDOW_SIZE - 1] = reading.temperature_c;

            /* 2. Run Edge AI Pipeline */
            TickType_t t_start = xTaskGetTickCount();
            ai_result_t ai_res;
            ai_pipeline_run(window, &ai_res);

            /* 3. Run Lossless Compression */
            uint32_t orig_bytes = 0;
            uint32_t comp_bytes = 0;
            float cr = 1.0f;
            if (ai_res.do_compress)
            {
                cr = compress_signal(window, SENSOR_WINDOW_SIZE, comp_buf, &comp_bytes, &orig_bytes);
            }
            else
            {
                orig_bytes = SENSOR_WINDOW_SIZE * sizeof(float);
                comp_bytes = orig_bytes;
                cr = 1.0f;
            }
            TickType_t t_end = xTaskGetTickCount();
            uint32_t infer_ms = pdTICKS_TO_MS(t_end - t_start);
            if (infer_ms == 0) infer_ms = 3;

            /* 4. Get RTC Time */
            rtc_time_t ts;
            R_RTC_CalendarTimeGet(&g_rtc0_ctrl, &ts);

            /* 5. Update UI */
            display_ui_update(
                reading.temperature_c,
                reading.humidity_pct,
                &ai_res,
                cr,
                orig_bytes,
                comp_bytes,
                infer_ms,
                sim_pkt,
                &ts,
                (uint32_t)(now / configTICK_RATE_HZ),
                sensor_source_mode_badge()
            );

            /* 6. Send data to laptop over Ethernet TCP / RTT */
            {
                net_packet_t net_pkt;
                net_pkt.packet_id         = sim_pkt;
                net_pkt.temp_c            = reading.temperature_c;
                net_pkt.humidity          = reading.humidity_pct;
                net_pkt.signal_class      = ai_res.signal_class;
                net_pkt.signal_class_name = ai_res.signal_class_name;
                net_pkt.is_anomaly        = ai_res.is_anomaly;
                net_pkt.do_compress       = ai_res.do_compress;
                net_pkt.cr                = cr;
                net_pkt.orig_bytes        = orig_bytes;
                net_pkt.comp_bytes        = comp_bytes;
                net_pkt.time_ms           = infer_ms;
                net_pkt.uptime_sec        = (uint32_t)(now / configTICK_RATE_HZ);
                net_pkt.src_injected      = reading.is_injected;
                net_pkt.src_simulated     = reading.is_simulated;
                net_pkt.src_sensor        = reading.is_sensor;
                memcpy(&net_pkt.timestamp, &ts, sizeof(struct tm));
                memcpy(net_pkt.features, ai_res.features, sizeof(net_pkt.features));

                network_client_send(&net_pkt);
            }
        }

        /* 7. Run LVGL rendering */
        lv_timer_handler();

        /* 8. Yield briefly for 10ms (smooth frame rate) */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void hal_entry(void)
{
    app_main();
}
