/*
 * lv_port_display.h
 * ===========================================================================
 * LVGL Display Port — Renesas RK-RA8P1
 * MCU  : RA8P1  (Cortex-M85)
 * LCD  : 480 × 854 TFT panel, RGB565
 * GPU  : Dave2D (managed internally by FSP rm_lvgl_port middleware)
 * GLCDC: Renesas Graphics LCD Controller (managed by FSP rm_lvgl_port)
 *
 * Usage:
 *   Call lv_init() first, then call lv_port_display_init() once.
 *   The FSP rm_lvgl_port middleware registers all necessary LVGL callbacks
 *   (flush, wait) and starts GLCDC automatically.
 *
 * Screen dimensions are taken from the FSP-generated macros:
 *   LVGL_DISPLAY_HSIZE_INPUT (480)
 *   LVGL_DISPLAY_VSIZE_INPUT (854)
 * The macros below mirror those values for use by application code.
 * ===========================================================================
 */

#ifndef LV_PORT_DISPLAY_H
#define LV_PORT_DISPLAY_H

#include "bsp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Screen dimensions — must match the GLCDC layer configuration in FSP.
 * These are kept in sync with LVGL_DISPLAY_HSIZE_INPUT /
 * LVGL_DISPLAY_VSIZE_INPUT defined in ra_gen/common_data.h.
 * -------------------------------------------------------------------------- */
#define LV_PORT_DISP_HOR_RES   (480)
#define LV_PORT_DISP_VER_RES   (854)

/* --------------------------------------------------------------------------
 * Color depth — must match lv_conf.h and the GLCDC pixel format.
 *   RGB565  → LV_COLOR_DEPTH 16  (current project setting)
 *   ARGB8888 → LV_COLOR_DEPTH 32 (not used in this project)
 * -------------------------------------------------------------------------- */
#ifndef LV_PORT_DISP_COLOR_DEPTH
  #if LV_COLOR_DEPTH == 32
    #define LV_PORT_DISP_COLOR_DEPTH  32U
  #else
    #define LV_PORT_DISP_COLOR_DEPTH  16U
  #endif
#endif

/* --------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

/**
 * @brief  Initialise the display port via the FSP rm_lvgl_port middleware.
 *
 * Call ONCE from your display task, immediately after lv_init().
 * Internally calls RM_LVGL_PORT_Open() which:
 *   1. Opens and starts the GLCDC peripheral.
 *   2. Assigns the two SDRAM frame buffers (double-buffering).
 *   3. Creates an lv_display_t and registers flush / wait callbacks.
 *   4. Enables VSYNC-locked buffer swapping.
 *
 * Dave2D acceleration is managed by the LVGL Dave2D draw backend —
 * no separate R_DRW_Open() call is required.
 */
void lv_port_display_init(void);

/**
 * @brief  De-initialise the display port.
 *         Currently a no-op (rm_lvgl_port has no close API in FSP v6.x).
 */
void lv_port_display_deinit(void);

/* Diagnostics visible in debugger Expressions view */
extern volatile uint32_t  g_boot_stage;
extern volatile fsp_err_t g_boot_err;
extern volatile uint32_t  g_display_init_stage;
extern volatile fsp_err_t g_display_init_err;

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_DISPLAY_H */
