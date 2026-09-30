/*
 * mipi_dsi_ep.c
 * ===========================================================================
 * MIPI-DSI callback + LCD init for EK-RA8P1
 *
 * Matches the Renesas reference mipi_dsi example EXACTLY:
 *   - Callback only handles SEQUENCE_0 (flag) and PHY
 *   - NO POST_OPEN handling (commands sent from thread, not callback)
 *   - push_table sends commands with spin-wait + timeout
 * ===========================================================================
 */

#include "common_data.h"
#include "r_mipi_dsi.h"
#include "r_ioport.h"
#include "bsp_api.h"

/* ── EK-RA8P1 display GPIO pins (from reference) ─────────────────────── */
#define PIN_DISPLAY_RST        (BSP_IO_PORT_06_PIN_06)
#define PIN_DISPLAY_BACKLIGHT  (BSP_IO_PORT_05_PIN_14)
#define PIN_DISPLAY_INT        (BSP_IO_PORT_01_PIN_11)

/* ── Flags set by ISR, consumed by push_table ─────────────────────────── */
volatile bool                  g_message_sent = false;
volatile mipi_dsi_phy_status_t g_phy_status   = 0;

/* ── Diagnostics ──────────────────────────────────────────────────────── */
volatile uint32_t g_push_table_cmd_count = 0;
volatile uint32_t g_push_table_errors    = 0;

/* ── LCD init table type ──────────────────────────────────────────────── */
typedef struct
{
    unsigned char        size;
    unsigned char        buffer[10];
    mipi_cmd_id_t        cmd_id;
    mipi_dsi_cmd_flag_t  flags;
} lcd_table_setting_t;

#define MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG   ((mipi_cmd_id_t) 0xFE)
#define MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE ((mipi_cmd_id_t) 0xFD)
void touch_screen_reset(void);
void mipi_dsi_push_table(const lcd_table_setting_t *table);

/* ==========================================================================
 * FocusLCD E45RA-MW276-C init table
 * Copied from working Renesas mipi_dsi example (same panel on EK-RA8P1)
 * ========================================================================== */
const lcd_table_setting_t g_lcd_init_focuslcd[] =
{
    {6,  {0xFF, 0xFF, 0x98, 0x06, 0x04, 0x01}, MIPI_CMD_ID_DCS_LONG_WRITE,          MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x08, 0x10}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x21, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x30, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x31, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x40, 0x14}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x41, 0x33}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x42, 0x02}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x43, 0x09}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x44, 0x06}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x50, 0x70}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x51, 0x70}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x52, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x53, 0x48}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x60, 0x07}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x61, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x62, 0x08}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x63, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa0, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa1, 0x03}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa2, 0x09}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa3, 0x0d}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa4, 0x06}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa5, 0x16}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa6, 0x09}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa7, 0x08}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa8, 0x03}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xa9, 0x07}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xaa, 0x06}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xab, 0x05}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xac, 0x0d}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xad, 0x2c}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xae, 0x26}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xaf, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc0, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc1, 0x04}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc2, 0x0b}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc3, 0x0f}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc4, 0x09}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc5, 0x18}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc6, 0x07}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc7, 0x08}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc8, 0x05}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xc9, 0x09}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xca, 0x07}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xcb, 0x05}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xcc, 0x0c}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xcd, 0x2d}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xce, 0x28}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xcf, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},

    {6,  {0xFF, 0xFF, 0x98, 0x06, 0x04, 0x06}, MIPI_CMD_ID_DCS_LONG_WRITE,          MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x00, 0x21}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x01, 0x09}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x02, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x03, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x04, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x05, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x06, 0x80}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x07, 0x05}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x08, 0x02}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x09, 0x80}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x0a, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x0b, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x0c, 0x0a}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x0d, 0x0a}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x0e, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x0f, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x10, 0xe0}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x11, 0xe4}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x12, 0x04}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x13, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x14, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x15, 0xc0}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x16, 0x08}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x17, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x18, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x19, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x1a, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x1b, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x1c, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x1d, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x20, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x21, 0x23}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x22, 0x45}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x23, 0x67}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x24, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x25, 0x23}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x26, 0x45}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x27, 0x67}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x30, 0x01}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x31, 0x11}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x32, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x33, 0xee}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x34, 0xff}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x35, 0xcb}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x36, 0xda}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x37, 0xad}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x38, 0xbc}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x39, 0x76}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x3a, 0x67}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x3b, 0x22}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x3c, 0x22}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x3d, 0x22}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x3e, 0x22}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x3f, 0x22}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x40, 0x22}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x53, 0x10}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x54, 0x10}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},

    {6,  {0xFF, 0xFF, 0x98, 0x06, 0x04, 0x07}, MIPI_CMD_ID_DCS_LONG_WRITE,          MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x18, 0x1d}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x26, 0xb2}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x02, 0x77}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0xe1, 0x79}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},
    {2,  {0x17, 0x22}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER},

    /* Change to Page 0 CMD for Normal command */
    {6,  {0xFF, 0xFF, 0x98, 0x06, 0x04, 0x00}, MIPI_CMD_ID_DCS_LONG_WRITE,          MIPI_DSI_CMD_FLAG_LOW_POWER},
    /* Sleep out command may not be issued within 120 ms of GPIO HW reset. Wait to ensure timing maintained. */
    {120, {0}, MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG, (mipi_dsi_cmd_flag_t)0},
    {2,  {0x11, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER}, /* Sleep-Out */
    {5,   {0}, MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG, (mipi_dsi_cmd_flag_t)0},          /* Delay 5msec */
    {2,  {0x29, 0x00}, MIPI_CMD_ID_DCS_SHORT_WRITE_0_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER}, /* Display on */
    {2,  {0x3a, 0x70}, MIPI_CMD_ID_DCS_SHORT_WRITE_1_PARAM, MIPI_DSI_CMD_FLAG_LOW_POWER}, /* 24-bit/pixel (RGB888) */

    {0x00, {0}, MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE, (mipi_dsi_cmd_flag_t)0}, /* End of table */
};

/* ==========================================================================
 * touch_screen_reset — GPIO reset of display panel
 * Copied EXACTLY from reference mipi_dsi example
 * ========================================================================== */
void touch_screen_reset(void)
{
    R_BSP_PinAccessEnable();

    /* Reset touch chip by setting GPIO reset pin low */
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_RST, BSP_IO_LEVEL_LOW);
    R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_DISPLAY_INT, IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MICROSECONDS);

    /* Start delay to set the device slave address to 0x28/0x29 */
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_INT, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MICROSECONDS);

    /* Release touch chip from reset */
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_RST, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);

    /* Set GPIO INT pin low */
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_INT, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(100, BSP_DELAY_UNITS_MILLISECONDS);

    /* Release touch chip interrupt pin for control */
    R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_DISPLAY_INT,
                    IOPORT_CFG_PORT_DIRECTION_INPUT
                    | IOPORT_CFG_EVENT_RISING_EDGE
                    | IOPORT_CFG_IRQ_ENABLE);

    R_BSP_PinAccessDisable();
}

/* ==========================================================================
 * mipi_dsi_push_table — Send LCD init commands (matches reference example)
 * ========================================================================== */
void mipi_dsi_push_table(const lcd_table_setting_t *table)
{
    fsp_err_t err;
    const lcd_table_setting_t *p_entry = table;
    g_push_table_cmd_count = 0;
    g_push_table_errors    = 0;

    while (MIPI_DSI_DISPLAY_CONFIG_DATA_END_OF_TABLE != p_entry->cmd_id)
    {
        mipi_dsi_cmd_t msg =
        {
            .channel     = 0,
            .cmd_id      = p_entry->cmd_id,
            .flags       = p_entry->flags,
            .tx_len      = p_entry->size,
            .p_tx_buffer = (uint8_t *)p_entry->buffer,
        };

        if (MIPI_DSI_DISPLAY_CONFIG_DATA_DELAY_FLAG == msg.cmd_id)
        {
            R_BSP_SoftwareDelay(p_entry->size, BSP_DELAY_UNITS_MILLISECONDS);
        }
        else
        {
            /* Wait until DSI sequence engine is idle before issuing next command */
            uint32_t wait_idle = 200000U;
            while ((R_MIPI_DSI->LINKSR_b.SQ0RUN || R_MIPI_DSI->LINKSR_b.SQ1RUN) && (--wait_idle > 0))
            {
                __NOP();
            }

            g_message_sent = false;

            /* Send command with retry if busy */
            uint32_t retries = 50U;
            do {
                err = R_MIPI_DSI_Command(&g_mipi_dsi0_ctrl, &msg);
                if (FSP_SUCCESS == err)
                {
                    break;
                }
                R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            } while (--retries > 0);

            if (FSP_SUCCESS != err)
            {
                g_push_table_errors++;
            }
            else
            {
                /* Wait for ISR completion */
                uint32_t timeout = 5000000U;
                while (!g_message_sent && (--timeout > 0))
                {
                    __NOP();
                }

                if (g_message_sent)
                {
                    g_push_table_cmd_count++;
                }
                else
                {
                    g_push_table_errors++;
                }
            }
        }
        p_entry++;
    }
}

/* ==========================================================================
 * Diagnostics visible in debugger Expressions view
 * ========================================================================== */
volatile uint32_t g_isr_seq0_count             = 0;
volatile uint32_t g_last_tx_status             = 0;
volatile uint32_t g_dsi_err_flags              = 0;
volatile uint32_t g_mipi_dsi_video_count       = 0;
volatile uint32_t g_mipi_dsi_last_video_status = 0;
volatile uint32_t g_mipi_dsi_fatal_count       = 0;

/* ==========================================================================
 * mipi_dsi0_callback — MIPI-DSI interrupt callback
 *
 * Uses bitmask comparison on tx_status because hardware sets multiple flags
 * (such as ACTIONS_FINISHED and DESCRIPTORS_FINISHED) in SQCH0SR simultaneously.
 * ========================================================================== */
void mipi_dsi0_callback(mipi_dsi_callback_args_t *p_args)
{
    switch (p_args->event)
    {
        case MIPI_DSI_EVENT_SEQUENCE_0:
        case MIPI_DSI_EVENT_SEQUENCE_1:
        {
            g_isr_seq0_count++;
            g_last_tx_status = (uint32_t)p_args->tx_status;
            if (p_args->tx_status & (MIPI_DSI_SEQUENCE_STATUS_TX_INTERNAL_BUS_ERROR |
                                     MIPI_DSI_SEQUENCE_STATUS_SIZE_ERROR |
                                     MIPI_DSI_SEQUENCE_STATUS_DESCRIPTOR_ABORT))
            {
                g_dsi_err_flags |= (uint32_t)p_args->tx_status;
            }
            if (p_args->tx_status & (MIPI_DSI_SEQUENCE_STATUS_DESCRIPTORS_FINISHED |
                                     MIPI_DSI_SEQUENCE_STATUS_ACTIONS_FINISHED))
            {
                g_message_sent = true;
            }
            break;
        }
        case MIPI_DSI_EVENT_VIDEO:
        {
            g_mipi_dsi_video_count++;
            g_mipi_dsi_last_video_status = (uint32_t)p_args->video_status;
            break;
        }
        case MIPI_DSI_EVENT_FATAL:
        {
            g_mipi_dsi_fatal_count++;
            break;
        }
        case MIPI_DSI_EVENT_PHY:
        {
            g_phy_status |= p_args->phy_status;
            break;
        }
        default:
        {
            break;
        }
    }
}
