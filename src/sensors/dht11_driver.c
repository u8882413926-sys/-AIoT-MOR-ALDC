/*
 * dht11_driver.c
 * ===========================================================================
 * DHT11 Single-Wire Sensor Driver — Renesas RA8P1 (FSP v6.5+)
 *
 * Protocol timing (DHT11 datasheet Rev1.3):
 *
 *  Host start signal:
 *    DATA low  >= 18 ms   (we use 18 ms via vTaskDelay — safe inside RTOS)
 *    DATA high ~  30 us   (R_BSP_SoftwareDelay)
 *    Release DATA line (pin -> input, 10k pull-up holds HIGH)
 *
 *  DHT11 response:
 *    Pulls DATA low  80 us
 *    Pulls DATA high 80 us
 *
 *  Each of 40 data bits:
 *    Low  50 us  (bit start)
 *    High 26-28 us = bit "0"
 *    High 70 us    = bit "1"
 *    We sample at 40 us: still HIGH at 40 us -> bit 1, else bit 0
 *
 *  Bit order: MSB first
 *  Byte order: [0] hum_int [1] hum_dec [2] tmp_int [3] tmp_dec [4] checksum
 *  DHT11: hum_dec and tmp_dec are always 0x00 (integer sensors)
 *
 * Implementation notes:
 *  - The 18 ms start pulse is generated with vTaskDelay (gives CPU back).
 *  - All subsequent waits are spin-loops with a timeout counter.
 *  - No timer peripheral needed — timing is sufficient on 480 MHz M85.
 *  - R_BSP_SoftwareDelay accuracy is +/- 1 us at 480 MHz, adequate for DHT11.
 * ===========================================================================
 */

#include "dht11_driver.h"
#include "FreeRTOS.h"
#include "task.h"

/* ==========================================================================
 * Low-level GPIO pin helpers
 * ========================================================================== */

/* Configure pin as push-pull OUTPUT */
static inline fsp_err_t pin_set_output(void)
{
    R_BSP_PinAccessEnable();
    fsp_err_t err = R_IOPORT_PinCfg(&g_ioport_ctrl,
                                    DHT11_DATA_PIN,
                                    IOPORT_CFG_PORT_DIRECTION_OUTPUT);
    R_BSP_PinAccessDisable();
    return err;
}

/* Configure pin as INPUT (10k external pull-up keeps it HIGH) */
static inline fsp_err_t pin_set_input(void)
{
    R_BSP_PinAccessEnable();
    fsp_err_t err = R_IOPORT_PinCfg(&g_ioport_ctrl,
                                    DHT11_DATA_PIN,
                                    IOPORT_CFG_PORT_DIRECTION_INPUT);
    R_BSP_PinAccessDisable();
    return err;
}

static inline void pin_write_high(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, DHT11_DATA_PIN, BSP_IO_LEVEL_HIGH);
}

static inline void pin_write_low(void)
{
    R_IOPORT_PinWrite(&g_ioport_ctrl, DHT11_DATA_PIN, BSP_IO_LEVEL_LOW);
}

static inline bsp_io_level_t pin_read(void)
{
    bsp_io_level_t lvl = BSP_IO_LEVEL_LOW;
    R_IOPORT_PinRead(&g_ioport_ctrl, DHT11_DATA_PIN, &lvl);
    return lvl;
}

/* ==========================================================================
 * dht11_init
 * ========================================================================== */
dht11_err_t dht11_init(void)
{
    fsp_err_t rc;

    /* Drive the line HIGH — this is the idle state for DHT11 */
    rc = pin_set_output();
    if (rc != FSP_SUCCESS) return DHT11_ERR_GPIO;

    pin_write_high();

    return DHT11_OK;
}

/* ==========================================================================
 * dht11_read
 * ========================================================================== */
dht11_err_t dht11_read(dht11_data_t *out)
{
    uint8_t  raw[5] = {0, 0, 0, 0, 0};   /* 40 bits = 5 bytes             */
    uint32_t timeout;

    /* ── Step 1: Send start signal ──────────────────────────────────── */
    /* Ensure pin is in output mode */
    pin_set_output();
    pin_write_high();
    R_BSP_SoftwareDelay(2U, BSP_DELAY_UNITS_MILLISECONDS);

    /* Pull LOW for >= 18 ms (mandatory per datasheet) */
    pin_write_low();
    vTaskDelay(pdMS_TO_TICKS(DHT11_START_LOW_MS));   /* 18 ms — give CPU back */

    /* Release HIGH for 30 us */
    pin_write_high();
    R_BSP_SoftwareDelay(DHT11_START_HIGH_US, BSP_DELAY_UNITS_MICROSECONDS);

    /* Switch to input — 10k pull-up holds line HIGH until DHT11 responds */
    pin_set_input();

    /* ── Step 2: Wait for DHT11 response — LOW ──────────────────────── */
    timeout = DHT11_TIMEOUT_LOOPS;
    while (pin_read() == BSP_IO_LEVEL_HIGH)
    {
        if (--timeout == 0U) return DHT11_ERR_TIMEOUT;
    }

    /* ── Step 3: Wait for DHT11 response — HIGH ─────────────────────── */
    timeout = DHT11_TIMEOUT_LOOPS;
    while (pin_read() == BSP_IO_LEVEL_LOW)
    {
        if (--timeout == 0U) return DHT11_ERR_TIMEOUT;
    }

    /* Wait for the response HIGH to end (DHT11 ready to send bits) */
    timeout = DHT11_TIMEOUT_LOOPS;
    while (pin_read() == BSP_IO_LEVEL_HIGH)
    {
        if (--timeout == 0U) return DHT11_ERR_TIMEOUT;
    }

    /* ── Step 4: Read 40 data bits ──────────────────────────────────── */
    for (int bit = 0; bit < 40; bit++)
    {
        /* Wait for the bit-start LOW to end (50 us low period) */
        timeout = DHT11_TIMEOUT_LOOPS;
        while (pin_read() == BSP_IO_LEVEL_LOW)
        {
            if (--timeout == 0U) return DHT11_ERR_TIMEOUT;
        }

        /* Delay 40 us into the HIGH period:
         *   If pin is still HIGH  -> bit = 1  (HIGH was 70 us)
         *   If pin is already LOW -> bit = 0  (HIGH was 26-28 us) */
        R_BSP_SoftwareDelay(DHT11_BIT_SAMPLE_US, BSP_DELAY_UNITS_MICROSECONDS);

        /* Shift in the bit (MSB first) */
        raw[bit / 8] <<= 1;
        if (pin_read() == BSP_IO_LEVEL_HIGH)
        {
            raw[bit / 8] |= 0x01U;
        }

        /* Wait for the HIGH to finish before next bit */
        timeout = DHT11_TIMEOUT_LOOPS;
        while (pin_read() == BSP_IO_LEVEL_HIGH)
        {
            if (--timeout == 0U) return DHT11_ERR_TIMEOUT;
        }
    }

    /* ── Step 5: Verify checksum ─────────────────────────────────────── */
    /* Checksum = sum of first 4 bytes, truncated to 8 bits */
    uint8_t checksum = (uint8_t)(raw[0] + raw[1] + raw[2] + raw[3]);
    if (checksum != raw[4])
    {
        return DHT11_ERR_CHECKSUM;
    }

    /* ── Step 6: Decode sensor values ───────────────────────────────── */
    /* DHT11 integer format:
     *   raw[0] = humidity integer part
     *   raw[1] = humidity decimal part (always 0 for DHT11)
     *   raw[2] = temperature integer part
     *   raw[3] = temperature decimal part (always 0 for DHT11)    */
    out->humidity_pct   = (float)raw[0];          /* e.g. 55.0 %           */
    out->temperature_c  = (float)raw[2];          /* e.g. 24.0 C           */

    /* Return pin to idle HIGH */
    pin_set_output();
    pin_write_high();

    return DHT11_OK;
}
