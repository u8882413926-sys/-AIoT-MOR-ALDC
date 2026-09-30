/*
 * dht11_driver.h
 * ===========================================================================
 * DHT11 Temperature & Humidity Sensor Driver for Renesas RA8P1
 *
 * Protocol  : Single-wire (proprietary 1-Wire — NOT I2C, NOT SPI)
 * Interface : One GPIO data pin + external 10 kOhm pull-up resistor
 *
 * DHT11 Specifications:
 *   Temperature : 0-50 C, resolution 1 C, accuracy +-2 C
 *   Humidity    : 20-90% RH, resolution 1%, accuracy +-5%
 *   Sample rate : 1 Hz maximum (one reading per second)
 *   VCC         : 3.3 V to 5.5 V (both work on RK-RA8P1)
 *
 * WIRING (Arduino-compatible header J18 on RK-RA8P1):
 * -------------------------------------------------------
 *  DHT11 VCC  -> J18 pin 2  (3.3V)
 *  DHT11 GND  -> J18 pin 7  (GND)
 *  DHT11 DATA -> J18 pin 6  (D6 = P410)   <-- single data wire
 *  10 kOhm resistor between DATA and VCC  <-- REQUIRED
 *
 * NOTE: Change DHT11_DATA_PIN below if you use a different GPIO pin.
 *       Verify against your board schematic before wiring.
 * ===========================================================================
 */

#ifndef DHT11_DRIVER_H
#define DHT11_DRIVER_H

#include "hal_data.h"
#include <stdint.h>
#include <stdbool.h>

/* ── Data pin — update to match your actual wiring ─────────────────────── */
/* Default: P410 = Arduino D6 on RK-RA8P1 J18 header                       */
#define DHT11_DATA_PIN      BSP_IO_PORT_04_PIN_10

/* ── Timing (per DHT11 datasheet Rev1.3) ───────────────────────────────── */
#define DHT11_START_LOW_MS      18U    /* Host pull-low: >= 18 ms           */
#define DHT11_START_HIGH_US     30U    /* Host release before tristate      */
#define DHT11_BIT_SAMPLE_US     40U    /* Sample 40 us into bit HIGH:
                                          >40 us remaining = bit 1
                                          already LOW at 40 us  = bit 0    */
#define DHT11_TIMEOUT_LOOPS    100000U   /* Spin-wait timeout (prevents hang) */

/* ── Return codes ───────────────────────────────────────────────────────── */
typedef enum {
    DHT11_OK            =  0,
    DHT11_ERR_TIMEOUT   = -1,   /* Sensor did not respond              */
    DHT11_ERR_CHECKSUM  = -2,   /* 40-bit checksum failed              */
    DHT11_ERR_GPIO      = -3,   /* FSP IOPORT call returned error      */
} dht11_err_t;

/* ── Measurement result ─────────────────────────────────────────────────── */
typedef struct {
    float temperature_c;    /* 0-50 C,  1 C resolution  (DHT11 integers) */
    float humidity_pct;     /* 20-90 %, 1 % resolution  (DHT11 integers) */
} dht11_data_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Configure the DHT11 data pin as output-high (idle state).
 *         Call ONCE at startup before any dht11_read() calls.
 * @return DHT11_OK on success.
 */
dht11_err_t dht11_init(void);

/**
 * @brief  Trigger a DHT11 measurement and read back 40 bits.
 *         Blocks for ~20 ms (start pulse) + ~5 ms (data).
 *         Call from a FreeRTOS task — uses vTaskDelay for the 18 ms start.
 *         Subsequent GPIO waits are busy-loops (<5 ms total).
 *
 * @param  out  Pointer filled with temperature_c and humidity_pct on success.
 * @return DHT11_OK on success, negative error code otherwise.
 */
dht11_err_t dht11_read(dht11_data_t *out);

#endif /* DHT11_DRIVER_H */
