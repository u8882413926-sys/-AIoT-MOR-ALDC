/*
 * display_ui.h
 * ===========================================================================
 * LVGL Display UI — AIoT-MOR-ALDC Temperature Change Tracker Dashboard
 * Target: 4.3-inch 480x854 portrait TFT with LVGL 9.x port for RA8P1
 * ===========================================================================
 */

#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include "ai_pipeline.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifndef rtc_time_t
typedef struct tm rtc_time_t;
#endif

/* ── Screen dimensions (EK-RA8P1 4.3-inch portrait display) ───────────── */
#define DISPLAY_WIDTH    480
#define DISPLAY_HEIGHT   854

/**
 * @brief  Initialise LVGL and build the full dashboard UI.
 */
void display_ui_init(void);

/**
 * @brief  Display the interactive startup mode selection modal.
 *
 * @param  seconds_left       Seconds remaining on the auto-start countdown.
 * @param  current_mode_name  Name of currently selected mode (Live Sensor or Data-Driven).
 */
void display_ui_show_startup_modal(uint32_t seconds_left, const char *current_mode_name);

/**
 * @brief  Dismiss the startup mode selection modal and reveal the full dashboard.
 */
void display_ui_hide_startup_modal(void);

/**
 * @brief  Update the active mode badge on the title header.
 */
void display_ui_set_mode_badge(const char *mode_badge);

/**
 * @brief  Highlight the selected mode box on the dashboard (1 = Live DHT11, 2 = Data-Driven).
 */
void display_ui_set_active_mode(int mode_id);

/**
 * @brief  Refresh all dashboard panels with the latest measurement.
 */
void display_ui_update(float              temp_c,
                       float              humidity,
                       const ai_result_t *result,
                       float              cr,
                       uint32_t           orig_bytes,
                       uint32_t           comp_bytes,
                       uint32_t           infer_ms,
                       uint32_t           pkt_id,
                       const rtc_time_t  *ts,
                       uint32_t           uptime_sec,
                       const char        *mode_name);

#endif /* DISPLAY_UI_H */
