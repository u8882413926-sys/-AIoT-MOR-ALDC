/*
 * lv_port_display.c
 * ===========================================================================
 * LVGL 9.x Display Port — Renesas EK-RA8P1 (480 × 854 MIPI-DSI)
 *
 * Matches the EXACT init sequence from the working Renesas mipi_dsi example:
 *
 *   1. R_GLCDC_Open()         → opens GLCDC + DSI hardware
 *   2. Overwrite ctrl callback → safe: g_display0_ctrl.p_callback is NOT const
 *   3. touch_screen_reset()   → GPIO reset of display panel
 *   4. mipi_dsi_push_table()  → send full LCD init (thread context, spin-wait)
 *   5. R_GLCDC_Start()        → start DSI video mode
 *   6. LVGL display setup     → manual (flush + vsync callbacks)
 *   7. Backlight ON           → GPIO BSP_IO_PORT_05_PIN_14
 *
 * We do NOT use RM_LVGL_PORT_Open() because it bundles Open+Start
 * with zero gap for LCD init commands.
 * ===========================================================================
 */

#include "lv_port_display.h"
#include "lvgl.h"
#include "hal_data.h"
#include "r_glcdc.h"
#include "r_ioport.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>

/* ── EK-RA8P1 display backlight pin ───────────────────────────────────── */
#define PIN_DISPLAY_BACKLIGHT  (BSP_IO_PORT_05_PIN_14)
#define PIN_DISPLAY_RST        (BSP_IO_PORT_06_PIN_06)

/* ── External: from mipi_dsi_ep.c ─────────────────────────────────────── */
typedef struct
{
    unsigned char        size;
    unsigned char        buffer[10];
    mipi_cmd_id_t        cmd_id;
    mipi_dsi_cmd_flag_t  flags;
} lcd_table_setting_t;

extern const lcd_table_setting_t g_lcd_init_focuslcd[];
extern void mipi_dsi_push_table(const lcd_table_setting_t *table);
extern void touch_screen_reset(void);

/* ── Diagnostics (visible in debugger Expressions view) ──────────────── */
volatile uint32_t  g_boot_stage         = 0;
volatile fsp_err_t g_boot_err           = FSP_SUCCESS;
volatile uint32_t  g_display_init_stage = 0;
volatile fsp_err_t g_display_init_err   = FSP_SUCCESS;
volatile uint32_t  g_glcdc_vsync_count  = 0;
volatile uint32_t  g_sdram_test_val     = 0;
volatile bool      g_sdram_test_pass    = false;
volatile uint32_t  g_mipi_dsi_vmset0r   = 0;
volatile uint32_t  g_mipi_dsi_vmsr      = 0;

#define SET_STAGE(s) do { g_boot_stage = (s); g_display_init_stage = (s); } while(0)
#define SET_ERROR(s, e) do { g_boot_stage = (s); g_display_init_stage = (s); g_boot_err = (e); g_display_init_err = (e); } while(0)

/* ── VSYNC semaphore ──────────────────────────────────────────────────── */
static SemaphoreHandle_t g_vsync_sem;
static StaticSemaphore_t g_vsync_sem_buf;

/* ── Global VSYNC flag for direct hardware scan loop ─────────────────── */
volatile bool g_hardware_vsync_flag = false;

/* ── Our own GLCDC callback (replaces _rm_lvgl_port_display_callback) ── */
static void my_glcdc_callback(display_callback_args_t *p_args)
{
    if (DISPLAY_EVENT_LINE_DETECTION == p_args->event)
    {
        g_glcdc_vsync_count++;
        g_hardware_vsync_flag = true;

        /* Keep live view of DSI hardware status for debugger Expressions view */
        g_mipi_dsi_vmset0r = R_MIPI_DSI->VMSET0R;
        g_mipi_dsi_vmsr    = R_MIPI_DSI->VMSR;

        if (g_vsync_sem)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xSemaphoreGiveFromISR(g_vsync_sem, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/* ── External: Dave2D dynamic renderer control variable ─────────────── */
extern lv_subject_t dave2d_enable;

/* ── FreeRTOS millisecond tick callback for LVGL ──────────────────────── */
static uint32_t my_tick_get_cb(void)
{
    return (uint32_t)(pdTICKS_TO_MS(xTaskGetTickCount()));
}

/* ── Helpers to pre-draw dashboard visual structure directly to SDRAM ─── */
static void draw_fb_rect(uint16_t *fb, uint32_t x0, uint32_t y0, uint32_t rw, uint32_t rh, uint16_t color)
{
    const uint32_t w = DISPLAY_HSIZE_INPUT0;
    const uint32_t h = DISPLAY_VSIZE_INPUT0;
    const uint32_t stride = DISPLAY_BUFFER_STRIDE_PIXELS_INPUT0;

    for (uint32_t y = y0; (y < (y0 + rh)) && (y < h); y++)
    {
        for (uint32_t x = x0; (x < (x0 + rw)) && (x < w); x++)
        {
            fb[y * stride + x] = color;
        }
    }
}

static void __attribute__((unused)) draw_fb_card(uint16_t *fb, uint32_t x0, uint32_t y0, uint32_t rw, uint32_t rh, uint16_t border_col, uint16_t fill_col)
{
    draw_fb_rect(fb, x0, y0, rw, rh, border_col);
    if ((rw > 4U) && (rh > 4U))
    {
        draw_fb_rect(fb, x0 + 2U, y0 + 2U, rw - 4U, rh - 4U, fill_col);
    }
}

static void pre_draw_dashboard_layout(uint16_t *fb)
{
    const uint32_t w = DISPLAY_HSIZE_INPUT0;
    const uint32_t h = DISPLAY_VSIZE_INPUT0;

    /* Fill entire background with CLR_BG (0x10C4 = Rich dark slate) */
    draw_fb_rect(fb, 0, 0, w, h, 0x10C4U);

    /* 1. Header banner (y=0..50): Vibrant Teal 0x0716 (#00E5B8) */
    draw_fb_rect(fb, 0, 0, w, 50U, 0x0716U);
}

/* ── LVGL flush callback ─────────────────────────────────────────────── */
static void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    FSP_PARAMETER_NOT_USED(area);

    if (lv_display_flush_is_last(disp))
    {
#if BSP_CFG_DCACHE_ENABLED
        SCB_CleanInvalidateDCache_by_Addr(px_map,
            (int32_t)(DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0));
#endif
        fsp_err_t err;
        do {
            err = R_GLCDC_BufferChange(g_display0.p_ctrl,
                                       px_map,
                                       DISPLAY_FRAME_LAYER_1);
        } while (FSP_ERR_INVALID_UPDATE_TIMING == err);
    }
}

static void my_flush_wait_cb(lv_display_t *disp)
{
    if (lv_display_flush_is_last(disp))
    {
        /* In accordance with Renesas rm_lvgl_port reference:
         * Clear any stale semaphore and wait for the next VSYNC pulse */
        xSemaphoreTake(g_vsync_sem, 0);
        xSemaphoreTake(g_vsync_sem, pdMS_TO_TICKS(50));
    }
}

/* Static configuration structures in RAM to guarantee dsi_mode = true (Master mode)
 * and prevent LP transitions during horizontal blanking porches (no FIFO underflow). */
static mipi_phy_cfg_t          s_fixed_mipi_phy0_cfg;
static mipi_phy_instance_t     s_fixed_mipi_phy0;
static mipi_dsi_extended_cfg_t s_fixed_mipi_dsi0_extended_cfg;
static mipi_dsi_cfg_t          s_fixed_mipi_dsi0_cfg;
static mipi_dsi_instance_t     s_fixed_mipi_dsi0;
static glcdc_extended_cfg_t    s_fixed_display0_extend_cfg;
static display_cfg_t           s_fixed_display0_cfg;

/* ==========================================================================
 * lv_port_display_init
 *
 * Replicates the EXACT sequence from the reference mipi_dsi example:
 *   R_GLCDC_Open → touch_screen_reset → mipi_dsi_push_table → R_GLCDC_Start
 * Then sets up LVGL manually and ensures backlight is ON.
 *
 * IMPORTANT: lv_init() MUST be called before this function.
 * ========================================================================== */
void lv_port_display_init(void)
{
    fsp_err_t err;
    SET_STAGE(0);

    /* ── Verify SDRAM read/write at 0x68000000 ──────────────────────── */
    volatile uint32_t *p_sdram_check = (volatile uint32_t *)&fb_background[0];
    p_sdram_check[0] = 0xA5A55A5AU;
    p_sdram_check[1] = 0x12345678U;
#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanDCache_by_Addr((uint32_t *)p_sdram_check, 32);
    SCB_InvalidateDCache_by_Addr((uint32_t *)p_sdram_check, 32);
#endif
    g_sdram_test_val = p_sdram_check[0];
    g_sdram_test_pass = (g_sdram_test_val == 0xA5A55A5AU) && (p_sdram_check[1] == 0x12345678U);

    if (!g_sdram_test_pass)
    {
        /* Re-initialize SDRAM if initial check failed */
        R_BSP_SdramInit(true);
        p_sdram_check[0] = 0xA5A55A5AU;
        p_sdram_check[1] = 0x12345678U;
#if BSP_CFG_DCACHE_ENABLED
        SCB_CleanDCache_by_Addr((uint32_t *)p_sdram_check, 32);
        SCB_InvalidateDCache_by_Addr((uint32_t *)p_sdram_check, 32);
#endif
        g_sdram_test_val = p_sdram_check[0];
        g_sdram_test_pass = (g_sdram_test_val == 0xA5A55A5AU) && (p_sdram_check[1] == 0x12345678U);
    }

    /* ── Create VSYNC semaphore ─────────────────────────────────────── */
    g_vsync_sem = xSemaphoreCreateBinaryStatic(&g_vsync_sem_buf);

    /* ── Disable D/AVE 2D hardware renderer ──────────────────────────── *
     * Force pure software rendering (lv_draw_sw) on Cortex-M85 (480MHz). *
     * Completely bypasses DRW interrupt semaphore deadlocks and fixes   *
     * missing text label flushes in Renesas LVGL 9 port.                */
    R_BSP_MODULE_START(FSP_IP_DRW, 0);
    lv_subject_set_int(&dave2d_enable, 0);

    /* ── Pre-draw complete dashboard layout into both framebuffers ──── *
     * Ensures instant visual feedback with zero blank/black screen:     *
     * Vibrant Teal title bar, cyan card outlines, and slate panels.     */
    pre_draw_dashboard_layout((uint16_t *)&fb_background[0]);
    pre_draw_dashboard_layout((uint16_t *)&fb_background[1]);

#if BSP_CFG_DCACHE_ENABLED
    SCB_CleanInvalidateDCache_by_Addr((uint32_t *)&fb_background[0],
        (int32_t)(sizeof(fb_background)));
#endif

    /* ── Configure backlight firmly ON (matches Renesas reference) ───── *
     * Backlight is kept continuously ON throughout startup.             */
    R_BSP_PinAccessEnable();
    R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_DISPLAY_BACKLIGHT,
                    IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_BACKLIGHT, BSP_IO_LEVEL_HIGH);
    R_BSP_PinAccessDisable();

    /* ── STEP 1: R_GLCDC_Open ───────────────────────────────────────── *
     * Opens GLCDC + internally opens MIPI-DSI.                        *
     * Ensure MIPI PHY is in DSI Master mode (dsi_mode = true).         *
     * Without dsi_mode = true, the D-PHY PLL is never started by       *
     * r_mipi_phy_open(), causing R_MIPI_DSI_Open() -> dsi_enter_reset()*
     * to hang indefinitely at line 708 waiting for RSTSR reset bits.  */
    const glcdc_extended_cfg_t * p_orig_glcdc_extend = (const glcdc_extended_cfg_t *)g_display0.p_cfg->p_extend;
    const mipi_dsi_instance_t *  p_orig_dsi          = (const mipi_dsi_instance_t *)p_orig_glcdc_extend->phy_layer;
    const mipi_phy_instance_t *  p_orig_phy          = p_orig_dsi->p_cfg->p_mipi_phy_instance;

    /* 1. Copy PHY config and force dsi_mode = true so D-PHY PLL starts */
    memcpy(&s_fixed_mipi_phy0_cfg, p_orig_phy->p_cfg, sizeof(s_fixed_mipi_phy0_cfg));
    s_fixed_mipi_phy0_cfg.dsi_mode = true;

    s_fixed_mipi_phy0 = *p_orig_phy;
    s_fixed_mipi_phy0.p_cfg = &s_fixed_mipi_phy0_cfg;

    /* 2. Copy DSI extended config to disable VM interrupt resets */
    memcpy(&s_fixed_mipi_dsi0_extended_cfg, p_orig_dsi->p_cfg->p_extend, sizeof(s_fixed_mipi_dsi0_extended_cfg));
    s_fixed_mipi_dsi0_extended_cfg.dsi_vmie = 0; /* Suppress VBUFUDF/VBUFOVF ISR resets */

    /* 3. Copy DSI config pointing to our fixed PHY and fixed extended config */
    memcpy(&s_fixed_mipi_dsi0_cfg, p_orig_dsi->p_cfg, sizeof(s_fixed_mipi_dsi0_cfg));
    s_fixed_mipi_dsi0_cfg.p_mipi_phy_instance = &s_fixed_mipi_phy0;
    s_fixed_mipi_dsi0_cfg.p_extend            = &s_fixed_mipi_dsi0_extended_cfg;

    /* Prevent Low-Power (LP) transitions during HSA and HBP porches.
     * FocusLCD horizontal sync is only 2 cycles (66 ns) and back porch is 5 cycles (166 ns).
     * D-PHY requires >400 ns to enter and exit LP mode; attempting LP during these short
     * periods causes video buffer underflow / D-PHY desync at line boundaries. */
    s_fixed_mipi_dsi0_cfg.hsa_no_lp = true;
    s_fixed_mipi_dsi0_cfg.hbp_no_lp = true;
    s_fixed_mipi_dsi0_cfg.hfp_no_lp = true;

    s_fixed_mipi_dsi0 = *p_orig_dsi;
    s_fixed_mipi_dsi0.p_cfg = &s_fixed_mipi_dsi0_cfg;

    /* 4. Copy GLCDC extended config pointing to our fixed DSI */
    memcpy(&s_fixed_display0_extend_cfg, p_orig_glcdc_extend, sizeof(s_fixed_display0_extend_cfg));
    s_fixed_display0_extend_cfg.phy_layer = (void *)&s_fixed_mipi_dsi0;

    /* 5. Copy GLCDC display config pointing to our fixed extended config */
    memcpy(&s_fixed_display0_cfg, g_display0.p_cfg, sizeof(s_fixed_display0_cfg));
    s_fixed_display0_cfg.p_extend = (void *)&s_fixed_display0_extend_cfg;
    s_fixed_display0_cfg.p_callback = my_glcdc_callback;

    err = R_GLCDC_Open(g_display0.p_ctrl, &s_fixed_display0_cfg);
    if (FSP_SUCCESS != err)
    {
        SET_ERROR(90, err);
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    /* CRITICAL SAFETY: Prevent FSP's mipi_dsi_vin1_isr from resetting DSI peripheral.
     * R_MIPI_DSI_Open enables VECTOR_NUMBER_MIPIDSI_VIN1 in NVIC. If a video buffer underflow
     * occurs at startup, mipi_dsi_vin1_isr calls dsi_enter_reset() which permanently halts video mode.
     * Disabling this IRQ and clearing VMIER ensures continuous uninterrupted video mode. */
    R_BSP_IrqDisable(VECTOR_NUMBER_MIPIDSI_VIN1);
    NVIC_ClearPendingIRQ(VECTOR_NUMBER_MIPIDSI_VIN1);
    R_MIPI_DSI->VMIER = 0U;
    SET_STAGE(1);
    SET_STAGE(2);

    /* ── STEP 3: GPIO reset of display panel ────────────────────────── *
     * Copied from reference: resets display + touch controller.       *
     * This is done AFTER R_GLCDC_Open, BEFORE push_table.            *
     *                                                                 *
     * CRITICAL: R_BSP_PinAccessEnable() MUST be called here so that   *
     * R_IOPORT_PinCfg/PinWrite inside touch_screen_reset() can        *
     * actually modify the PFS registers. Without this, GPIO writes    *
     * are silently ignored and the display never gets a reset pulse.   */
    R_BSP_PinAccessEnable();
    touch_screen_reset();
    R_BSP_PinAccessDisable();
    SET_STAGE(3);

    /* ── STEP 4: Send full LCD init sequence ─────────────────────────── *
     * ~100 DCS commands including power, gamma, GIP timing,           *
     * Sleep Out + 120ms delay + Display ON + pixel format.            *
     * Sent from thread context with spin-wait for each completion.    */
    mipi_dsi_push_table(g_lcd_init_focuslcd);
    SET_STAGE(4);

    /* Allow LCD controller to settle after Display ON command.         *
     * The init table ends with Sleep Out (120ms) + Display ON (20ms). *
     * An extra 20ms here ensures the panel is fully ready before       *
     * the GLCDC starts sending video-mode pixel data.                  */
    R_BSP_SoftwareDelay(20, BSP_DELAY_UNITS_MILLISECONDS);

    /* ── STEP 5: R_GLCDC_Start ──────────────────────────────────────── *
     * Starts GLCDC output. Internally calls R_MIPI_DSI_Start().       *
     * DSI video mode begins. Panel is fully initialized and ready.    */
    err = R_GLCDC_Start(g_display0.p_ctrl);
    if (FSP_SUCCESS != err)
    {
        SET_ERROR(95, err);
        while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }
    SET_STAGE(5);

    /* Note: GLCDC is already outputting fb_background[0] (pre-filled with
     * color bars). LVGL will safely render frames via my_flush_cb. */

    /* Ensure Video Mode error interrupt remains disabled so underflow cannot halt DSI */
    R_BSP_IrqDisable(VECTOR_NUMBER_MIPIDSI_VIN1);
    NVIC_ClearPendingIRQ(VECTOR_NUMBER_MIPIDSI_VIN1);
    R_MIPI_DSI->VMIER = 0U;

    /* Record live hardware register values right after start for diagnostics */
    g_mipi_dsi_vmset0r = R_MIPI_DSI->VMSET0R;
    g_mipi_dsi_vmsr    = R_MIPI_DSI->VMSR;

    /* ── STEP 6: Ensure backlight is firmly ON ──────────────────────── */
    R_BSP_PinAccessEnable();
    R_IOPORT_PinCfg(&g_ioport_ctrl, PIN_DISPLAY_BACKLIGHT,
                    IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_HIGH);
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_BACKLIGHT, BSP_IO_LEVEL_HIGH);
    R_BSP_PinAccessDisable();
    SET_STAGE(6);

    /* ── STEP 7: Create LVGL display + register callbacks ───────────── */
    {
        lv_display_t *disp = lv_display_create(
            DISPLAY_HSIZE_INPUT0, DISPLAY_VSIZE_INPUT0);
        if (NULL == disp)
        {
            SET_ERROR(92, FSP_ERR_OUT_OF_MEMORY);
            while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
        }

        lv_tick_set_cb(my_tick_get_cb);

        lv_display_set_flush_cb(disp, my_flush_cb);
        lv_display_set_flush_wait_cb(disp, my_flush_wait_cb);
        lv_display_set_buffers_with_stride(
            disp,
            &fb_background[0],
            &fb_background[1],
            (DISPLAY_BUFFER_STRIDE_BYTES_INPUT0 * DISPLAY_VSIZE_INPUT0),
            DISPLAY_BUFFER_STRIDE_BYTES_INPUT0,
            LV_DISPLAY_RENDER_MODE_DIRECT);
    }
    SET_STAGE(7);
}

/* ==========================================================================
 * lv_port_display_deinit
 * ========================================================================== */
void lv_port_display_deinit(void)
{
    R_BSP_PinAccessEnable();
    R_IOPORT_PinWrite(&g_ioport_ctrl, PIN_DISPLAY_BACKLIGHT, BSP_IO_LEVEL_LOW);
    R_BSP_PinAccessDisable();
}
