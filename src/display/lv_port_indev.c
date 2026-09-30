/*
 * lv_port_indev.c
 * ===========================================================================
 * LVGL 9.x Input Device Port — Renesas RK-RA8P1
 *
 * Stub implementation — registers a "no input" pointer device.
 *
 * TO ADD REAL TOUCH SUPPORT (e.g. GT911 capacitive touch controller):
 *   1. Enable the GT911 IIC driver in your project.
 *   2. Replace touch_read_cb() below with real GT911 read calls.
 *   3. Set TOUCH_ENABLED 1 in this file.
 *
 * The stub ensures hal_entry.c compiles and lv_port_indev_init() is safe
 * to call even when no touch hardware is present.
 * ===========================================================================
 */

#include "lv_port_indev.h"
#include "lvgl.h"

/* Set to 1 if you have a physical touch controller connected */
#define TOUCH_ENABLED  0

/* ==========================================================================
 * Touch read callback
 *
 * LVGL calls this callback repeatedly (driven by lv_timer_handler).
 * Fill in p_data->point and p_data->state to report touch position.
 * ========================================================================== */
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *p_data)
{
    (void)indev;

#if TOUCH_ENABLED
    /*
     * ── Real GT911 / FT5336 touch read (example skeleton) ──────────────
     * Replace the block below with actual IIC read calls to your touch IC.
     *
     * uint16_t x, y;
     * bool     pressed;
     * gt911_read(&x, &y, &pressed);
     *
     * if (pressed)
     * {
     *     p_data->point.x = (lv_coord_t)x;
     *     p_data->point.y = (lv_coord_t)y;
     *     p_data->state   = LV_INDEV_STATE_PRESSED;
     * }
     * else
     * {
     *     p_data->state = LV_INDEV_STATE_RELEASED;
     * }
     */
    p_data->state = LV_INDEV_STATE_RELEASED;  /* placeholder until real driver added */
#else
    /* Stub: always report "not pressed" */
    p_data->state = LV_INDEV_STATE_RELEASED;
#endif
}

/* ==========================================================================
 * lv_port_indev_init
 * ========================================================================== */
void lv_port_indev_init(void)
{
    /* Create a pointer-type input device and attach the read callback.
     * LVGL will call touch_read_cb() from lv_timer_handler(). */
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);
}
