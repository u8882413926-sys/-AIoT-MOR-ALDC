/*
 * lv_port_indev.h
 * ===========================================================================
 * LVGL Input Device Port — Renesas RK-RA8P1
 *
 * Provides touch / button input device registration for LVGL 9.x.
 * For the AIoT-MOR-ALDC project the 7-inch display is display-only;
 * touch input is optional.  This file provides a stub implementation
 * that registers a "no input" device so hal_entry.c compiles without
 * modification.  If your board has a touch controller (e.g. GT911 via IIC),
 * replace the stub in lv_port_indev.c with real driver calls.
 * ===========================================================================
 */

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialise and register LVGL input devices.
 *
 * Call ONCE from display_task after lv_port_display_init().
 * In the stub implementation this function registers a pointer-type
 * input device that always reports "not pressed", so LVGL's internal
 * input handling remains functional without a physical touch panel.
 */
void lv_port_indev_init(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_INDEV_H */
