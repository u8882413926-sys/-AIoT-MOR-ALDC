/*
 * sensor_source.h
 * ===========================================================================
 * Unified Sensor Source Abstraction — AIoT-MOR-ALDC
 * Renesas EK-RA8P1 | ARM Cortex-M85 @ 480 MHz
 *
 * Supports RUNTIME mode selection:
 *   SENSOR_MODE_LIVE_DHT11   (1) — Real physical DHT11 sensor on GPIO P410
 *   SENSOR_MODE_DATA_DRIVEN  (2) — Laptop injection (USB RTT/UART) / sensor_data.txt
 *
 * HOW MODES ARE SELECTED:
 *   - At startup: On-screen selection menu + Hardware Buttons (S1=Live, S2=Data-Driven)
 *                 or Console input ('1' or '2'), with auto-timeout.
 *   - At runtime: Press S1/S2 on the EK-RA8P1 board at any time to switch modes!
 * ===========================================================================
 */

#ifndef SENSOR_SOURCE_H
#define SENSOR_SOURCE_H

#include <stdint.h>
#include <stdbool.h>

/* ── Operational sensor modes ────────────────────────────────────────────── */
typedef enum {
    SENSOR_MODE_LIVE_DHT11   = 1,   /* Real physical DHT11 sensor on GPIO P410 */
    SENSOR_MODE_DATA_DRIVEN  = 2,   /* Laptop injected dataset via USB RTT/UART */
} sensor_mode_t;

/* Backward-compatibility aliases */
#define SENSOR_MODE_DHT11       SENSOR_MODE_LIVE_DHT11
#define SENSOR_MODE_RTT_INJECT  SENSOR_MODE_DATA_DRIVEN
#define SENSOR_MODE_SIMULATE    SENSOR_MODE_DATA_DRIVEN

/* ── Measurement result type ─────────────────────────────────────────────── */
typedef struct {
    float    temperature_c;   /* Degrees Celsius                */
    float    humidity_pct;    /* Relative humidity percent      */
    bool     is_injected;     /* true if value came from laptop */
    bool     is_simulated;    /* true if value is synthetic     */
    bool     is_sensor;       /* true if value came from DHT11  */
} sensor_reading_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise sensor subsystem. Call once at startup.
 */
void sensor_source_init(void);

/**
 * @brief  Set active operational mode (Live Sensor or Data-Driven).
 */
void sensor_source_set_mode(sensor_mode_t mode);

/**
 * @brief  Get active operational mode.
 */
sensor_mode_t sensor_source_get_mode(void);

/**
 * @brief  Acquire one temperature + humidity reading using the active mode.
 */
bool sensor_source_read(sensor_reading_t *out);

/**
 * @brief  Returns human-readable name of the active mode for UI display.
 */
const char *sensor_source_mode_name(void);

/**
 * @brief  Returns short badge name (e.g. "LIVE (DHT11)" or "DATA-DRIVEN").
 */
const char *sensor_source_mode_badge(void);

/**
 * @brief  Check if a mode switch character ('1' for Live, '2' for Data-Driven) is in RTT buffer.
 * @return 1 for Live mode, 2 for Data-Driven mode, 0 otherwise.
 */
int sensor_source_check_input_cmd(void);

#endif /* SENSOR_SOURCE_H */
