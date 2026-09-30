/*
 * display_ui.c
 * ===========================================================================
 * AIoT-MOR-ALDC — High-Visibility Research-Grade Dashboard UI
 * Dedicated Target: Renesas EK-RA8P1 (480 x 854 Portrait MIPI-DSI)
 *
 * System Features:
 *   [1] Real-Time Sensor Telemetry (Dual-Mode: Live DHT11 or Data-Driven Dataset)
 *   [2] Thermal Change Tracker with Elapsed Time & Drift Analysis
 *   [3] On-Device Edge AI Pipeline (Pattern Classification & Anomaly Detection)
 *   [4] MOR-ALDC Adaptive Residual + RLE Compression Engine
 *   [5] Interactive Startup Sensor Mode Selection Modal (S1: Live, S2: Data-Driven)
 * ===========================================================================
 */

#include "display_ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ==========================================================================
 * Colour Palette — GitHub-Dark & Modern Lab Instrument Grade
 * ========================================================================== */
#define CLR_BG          lv_color_hex(0x0F172A)   /* Deep slate-navy background */
#define CLR_HEADER      lv_color_hex(0x131E30)   /* Header surface             */
#define CLR_PANEL       lv_color_hex(0x1E293B)   /* Card surface               */
#define CLR_CARD_BORDER lv_color_hex(0x334155)   /* Subtle slate card border   */
#define CLR_ACCENT      lv_color_hex(0x00E5B8)   /* Vibrant electric teal      */
#define CLR_WARM        lv_color_hex(0xFF7A30)   /* Vibrant coral-orange temp  */
#define CLR_TEXT        lv_color_hex(0xFFFFFF)   /* Pure white primary text    */
#define CLR_TEXT_DIM    lv_color_hex(0x94A3B8)   /* Light slate secondary text */
#define CLR_GREEN       lv_color_hex(0x22C55E)   /* Bright emerald green       */
#define CLR_RED         lv_color_hex(0xEF4444)   /* Vivid alarm red            */
#define CLR_YELLOW      lv_color_hex(0xF59E0B)   /* Amber warning              */
#define CLR_BLUE        lv_color_hex(0x38BDF8)   /* Vivid sky blue             */

#define TEMP_CHANGE_THRESHOLD_C   1.0f

/* ==========================================================================
 * Widget Handles
 * ========================================================================== */
/* Top Header Bar */
static lv_obj_t *g_lbl_mode_badge;
static lv_obj_t *g_lbl_pkt;

/* Top Dual Mode Selector Cards ("The Two Boxes") */
static lv_obj_t *g_box_live       = NULL;
static lv_obj_t *g_lbl_live_title = NULL;
static lv_obj_t *g_lbl_live_sub   = NULL;
static lv_obj_t *g_lbl_live_badge = NULL;

static lv_obj_t *g_box_data       = NULL;
static lv_obj_t *g_lbl_data_title = NULL;
static lv_obj_t *g_lbl_data_sub   = NULL;
static lv_obj_t *g_lbl_data_badge = NULL;

/* Card 1: Sensor Telemetry */
static lv_obj_t *g_lbl_temp;
static lv_obj_t *g_lbl_trend;
static lv_obj_t *g_lbl_humidity;
static lv_obj_t *g_lbl_time;
static lv_obj_t *g_lbl_date;
static lv_obj_t *g_lbl_rate;
static lv_obj_t *g_lbl_sess_min;
static lv_obj_t *g_lbl_sess_max;
static lv_obj_t *g_lbl_src_mode;

/* Card 2: Thermal Change Tracker */
static lv_obj_t *g_lbl_chg_vals;
static lv_obj_t *g_lbl_chg_at;
static lv_obj_t *g_lbl_chg_elaps;

/* Card 3: Edge AI Pipeline */
static lv_obj_t *g_lbl_pattern;
static lv_obj_t *g_lbl_anomaly;
static lv_obj_t *g_lbl_compress;
static lv_obj_t *g_lbl_infer_time;

/* Card 4: Compression Benchmarks */
static lv_obj_t *g_lbl_payload;
static lv_obj_t *g_lbl_cr;
static lv_obj_t *g_lbl_network;

/* Startup Mode Selection Modal */
static lv_obj_t *g_modal_obj     = NULL;
static lv_obj_t *g_modal_cd_lbl  = NULL;
static lv_obj_t *g_modal_sel_lbl = NULL;

/* ==========================================================================
 * Persistent State
 * ========================================================================== */
static float      s_prev_temp      = -999.0f;
static float      s_sess_min       =  9999.0f;
static float      s_sess_max       = -9999.0f;
static float      s_chg_from       =     0.0f;
static float      s_chg_to         =     0.0f;
static rtc_time_t s_chg_ts         =    {0};
static bool       s_has_chg        =    false;
static float      s_rate_prev_temp = -999.0f;
static uint32_t   s_rate_prev_sec  =      0U;

/* ==========================================================================
 * Helper Functions
 * ========================================================================== */

/* Create clean, styled card without asymmetric borders or rogue lines */
static lv_obj_t *create_card(lv_obj_t *parent,
                             lv_coord_t x, lv_coord_t y,
                             lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_bg_color(c, CLR_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(c, CLR_CARD_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(c, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(c, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(c, 12, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(c, LV_SCROLLBAR_MODE_OFF);
    return c;
}

static lv_obj_t *create_label(lv_obj_t *parent,
                              const char *text,
                              lv_coord_t x, lv_coord_t y,
                              lv_color_t colour,
                              const lv_font_t *font)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_pos(lbl, x, y);
    lv_obj_set_style_text_color(lbl, colour, LV_PART_MAIN);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    return lbl;
}

static void fmt_hms(const rtc_time_t *ts, char *buf, size_t len)
{
    int h12 = ts->tm_hour % 12;
    const char *ap = (ts->tm_hour < 12) ? "AM" : "PM";
    if (h12 == 0) h12 = 12;
    snprintf(buf, len, "%02d:%02d:%02d %s", h12, ts->tm_min, ts->tm_sec, ap);
}

static void fmt_date(const rtc_time_t *ts, char *buf, size_t len)
{
    snprintf(buf, len, "%02d/%02d/%04d",
             ts->tm_mday, ts->tm_mon + 1, ts->tm_year + 1900);
}

static uint32_t elapsed_sec(const rtc_time_t *from, const rtc_time_t *to)
{
    int32_t sec_from = from->tm_hour * 3600 + from->tm_min * 60 + from->tm_sec;
    int32_t sec_to   = to->tm_hour * 3600 + to->tm_min * 60 + to->tm_sec;
    if (sec_to >= sec_from) return (uint32_t)(sec_to - sec_from);
    else return (uint32_t)(sec_to + 86400 - sec_from);
}

static void fmt_elapsed(uint32_t sec, char *buf, size_t len)
{
    uint32_t h = sec / 3600U;
    uint32_t m = (sec % 3600U) / 60U;
    uint32_t s = sec % 60U;
    if (h > 0U)
        snprintf(buf, len, "%lu hr %lu min %lu s", (unsigned long)h, (unsigned long)m, (unsigned long)s);
    else if (m > 0U)
        snprintf(buf, len, "%lu min %lu s", (unsigned long)m, (unsigned long)s);
    else
        snprintf(buf, len, "%lu seconds", (unsigned long)s);
}

/* ==========================================================================
 * display_ui_init — Build full dashboard layout (480 x 854)
 * ========================================================================== */
void display_ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, CLR_BG, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    /* ── 1. TOP HEADER BAR (y=0..50) ────────────────────────────────── */
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, DISPLAY_WIDTH, 50);
    lv_obj_set_style_bg_color(bar, CLR_HEADER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, CLR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(bar, 2, LV_PART_MAIN);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(bar, LV_SCROLLBAR_MODE_OFF);

    create_label(bar, "AIoT-MOR-ALDC", 16, 6, CLR_TEXT, &lv_font_montserrat_16);
    create_label(bar, "EK-RA8P1 EDGE AI OS", 16, 26, CLR_TEXT_DIM, &lv_font_montserrat_10);

    g_lbl_mode_badge = create_label(bar, "DATA-DRIVEN", 250, 10, CLR_ACCENT, &lv_font_montserrat_12);
    lv_obj_set_style_bg_color(g_lbl_mode_badge, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_lbl_mode_badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_lbl_mode_badge, CLR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_lbl_mode_badge, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(g_lbl_mode_badge, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(g_lbl_mode_badge, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(g_lbl_mode_badge, 2, LV_PART_MAIN);

    g_lbl_pkt = create_label(bar, "Pkt #0001", 380, 26, CLR_TEXT_DIM, &lv_font_montserrat_12);

    /* ── 2. TWO OPERATIONAL MODE SELECTOR BOXES (y=54, h=78) ────────── */
    /* Box 1: [S1] LIVE SENSOR (x=12, w=222, h=78) */
    g_box_live = lv_obj_create(scr);
    lv_obj_set_pos(g_box_live, 12, 54);
    lv_obj_set_size(g_box_live, 222, 78);
    lv_obj_set_style_bg_color(g_box_live, CLR_PANEL, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_box_live, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_box_live, CLR_CARD_BORDER, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_box_live, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(g_box_live, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_box_live, 8, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(g_box_live, LV_SCROLLBAR_MODE_OFF);

    g_lbl_live_title = create_label(g_box_live, "[ S1 ] LIVE SENSOR", 0, 0, CLR_TEXT_DIM, &lv_font_montserrat_14);
    g_lbl_live_sub   = create_label(g_box_live, "Physical DHT11 (P410)", 0, 22, CLR_TEXT_DIM, &lv_font_montserrat_10);
    g_lbl_live_badge = create_label(g_box_live, "STANDBY (Press S1)", 0, 42, CLR_TEXT_DIM, &lv_font_montserrat_10);

    /* Box 2: [S2] DATA-DRIVEN (x=246, w=222, h=78) */
    g_box_data = lv_obj_create(scr);
    lv_obj_set_pos(g_box_data, 246, 54);
    lv_obj_set_size(g_box_data, 222, 78);
    lv_obj_set_style_bg_color(g_box_data, lv_color_hex(0x0C4A6E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_box_data, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(g_box_data, CLR_BLUE, LV_PART_MAIN);
    lv_obj_set_style_border_width(g_box_data, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(g_box_data, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(g_box_data, 8, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(g_box_data, LV_SCROLLBAR_MODE_OFF);

    g_lbl_data_title = create_label(g_box_data, "[ S2 ] DATA-DRIVEN", 0, 0, CLR_TEXT, &lv_font_montserrat_14);
    g_lbl_data_sub   = create_label(g_box_data, "Laptop USB / Dataset", 0, 22, CLR_ACCENT, &lv_font_montserrat_10);
    g_lbl_data_badge = create_label(g_box_data, "[ \xe2\x97\x8f ACTIVE (USB) ]", 0, 40, CLR_BLUE, &lv_font_montserrat_12);

    /* ── 3. CARD 1: SENSOR TELEMETRY (y=138, h=194, w=456) ──────────── */
    lv_obj_t *tc = create_card(scr, 12, 138, 456, 194);

    create_label(tc, "[1] REAL-TIME SENSOR TELEMETRY", 0, 0, CLR_ACCENT, &lv_font_montserrat_14);

    g_lbl_temp  = create_label(tc, "24.50", 0, 22, CLR_WARM, &lv_font_montserrat_36);
    create_label(tc, "\xc2\xb0""C", 112, 32, CLR_TEXT_DIM, &lv_font_montserrat_22);
    g_lbl_trend = create_label(tc, "STABLE", 155, 32, CLR_GREEN, &lv_font_montserrat_14);

    create_label(tc, "HUMIDITY", 275, 20, CLR_TEXT_DIM, &lv_font_montserrat_10);
    g_lbl_humidity = create_label(tc, "55.0 %RH", 275, 34, CLR_BLUE, &lv_font_montserrat_20);

    /* Column 1: Time, Date, Drift Rate */
    g_lbl_time = create_label(tc, "Time: 12:00:00 PM", 0, 78, CLR_TEXT, &lv_font_montserrat_12);
    g_lbl_date = create_label(tc, "Date: 30/09/2026",  0, 98, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_rate = create_label(tc, "Thermal Drift: +0.000 \xc2\xb0""C/min", 0, 118, CLR_GREEN, &lv_font_montserrat_12);

    /* Column 2: Min, Max, Active Source */
    g_lbl_sess_min = create_label(tc, "Session Min: 24.50 \xc2\xb0""C", 225, 78, CLR_BLUE, &lv_font_montserrat_12);
    g_lbl_sess_max = create_label(tc, "Session Max: 24.50 \xc2\xb0""C", 225, 98, CLR_RED, &lv_font_montserrat_12);
    g_lbl_src_mode = create_label(tc, "Src: DATA-DRIVEN", 225, 118, CLR_ACCENT, &lv_font_montserrat_12);

    create_label(tc, "Sampling: 100 readings @ 1 Hz | Change Thresh: \xc2\xb1""1.0\xc2\xb0""C",
                 0, 146, CLR_TEXT_DIM, &lv_font_montserrat_10);

    /* ── 4. CARD 2: THERMAL CHANGE TRACKER (y=338, h=142, w=456) ────── */
    lv_obj_t *cc = create_card(scr, 12, 338, 456, 142);

    create_label(cc, "[2] THERMAL CHANGE TRACKER", 0, 0, CLR_ACCENT, &lv_font_montserrat_14);
    create_label(cc, "Threshold: \xc2\xb1""1.0\xc2\xb0""C", 270, 2, CLR_TEXT_DIM, &lv_font_montserrat_10);

    create_label(cc, "Last Event:", 0, 24, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_chg_vals = create_label(cc, "Baseline Initialized (Awaiting change)", 84, 24, CLR_TEXT, &lv_font_montserrat_12);

    create_label(cc, "Event Time:", 0, 48, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_chg_at = create_label(cc, "System Startup", 84, 48, CLR_TEXT, &lv_font_montserrat_12);

    create_label(cc, "Time Since:", 0, 72, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_chg_elaps = create_label(cc, "0 seconds", 84, 72, CLR_YELLOW, &lv_font_montserrat_12);

    create_label(cc, "Thermal tracking records drift rate to detect anomalous heat rise",
                 0, 102, CLR_TEXT_DIM, &lv_font_montserrat_10);

    /* ── 5. CARD 3: ON-DEVICE EDGE AI INFERENCE (y=486, h=172, w=456) ── */
    lv_obj_t *ac = create_card(scr, 12, 486, 456, 172);

    create_label(ac, "[3] ON-DEVICE EDGE AI PIPELINE", 0, 0, CLR_ACCENT, &lv_font_montserrat_14);
    create_label(ac, "RF + IF + Decision Tree", 270, 2, CLR_TEXT_DIM, &lv_font_montserrat_10);

    create_label(ac, "Signal Pattern:", 0, 22, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_pattern = create_label(ac, "SMOOTH (Normal Profile)", 110, 20, CLR_TEXT, &lv_font_montserrat_14);

    create_label(ac, "Anomaly Status:", 0, 50, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_anomaly = create_label(ac, "NORMAL \xe2\x80\x94 Clean Profile (No Anomaly)", 110, 50, CLR_GREEN, &lv_font_montserrat_12);

    create_label(ac, "Codec Policy:", 0, 78, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_compress = create_label(ac, "Adaptive Residual Codec (Active)", 110, 78, CLR_GREEN, &lv_font_montserrat_12);

    create_label(ac, "Latency:", 0, 104, CLR_TEXT_DIM, &lv_font_montserrat_12);
    g_lbl_infer_time = create_label(ac, "3 ms (ARM Cortex-M85 @ 480 MHz)", 110, 104, CLR_BLUE, &lv_font_montserrat_12);

    create_label(ac, "Extracts 8 statistical features across 100-sample sliding window",
                 0, 132, CLR_TEXT_DIM, &lv_font_montserrat_10);

    /* ── 6. CARD 4: COMPRESSION BENCHMARKS (y=664, h=180, w=456) ────── */
    lv_obj_t *sc = create_card(scr, 12, 664, 456, 180);

    create_label(sc, "[4] MOR-ALDC COMPRESSION BENCHMARKS", 0, 0, CLR_ACCENT, &lv_font_montserrat_14);
    create_label(sc, "Multi-Order Residual + RLE", 255, 2, CLR_TEXT_DIM, &lv_font_montserrat_10);

    g_lbl_payload = create_label(sc, "Payload: 400 Bytes (Raw) -> 187 Bytes (Comp)", 0, 22, CLR_TEXT, &lv_font_montserrat_12);
    g_lbl_cr = create_label(sc, "Compression Ratio: 2.14x  |  Bandwidth Saved: 53.3%", 0, 48, CLR_GREEN, &lv_font_montserrat_14);

    g_lbl_network = create_label(sc, "Stream Link: Active (SEGGER RTT / TCP 192.168.10.100)", 0, 76, CLR_TEXT_DIM, &lv_font_montserrat_12);

    create_label(sc, "[S1] Press S1 for Live Sensor (P410)  |  [S2] Press S2 for Data-Driven",
                 0, 106, CLR_ACCENT, &lv_font_montserrat_12);

    create_label(sc, "Edge compression reduces RF transmit energy & prolongs battery life",
                 0, 136, CLR_TEXT_DIM, &lv_font_montserrat_10);
}

/* ==========================================================================
 * Startup Mode Selection Modal
 * ========================================================================== */
void display_ui_show_startup_modal(uint32_t seconds_left, const char *current_mode_name)
{
    lv_obj_t *scr = lv_scr_act();
    char buf[128];

    if (g_modal_obj == NULL)
    {
        /* Create centered modal container */
        g_modal_obj = lv_obj_create(scr);
        lv_obj_set_pos(g_modal_obj, 16, 120);
        lv_obj_set_size(g_modal_obj, 448, 560);
        lv_obj_set_style_bg_color(g_modal_obj, lv_color_hex(0x131E30), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(g_modal_obj, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_modal_obj, CLR_ACCENT, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_modal_obj, 2, LV_PART_MAIN);
        lv_obj_set_style_radius(g_modal_obj, 12, LV_PART_MAIN);
        lv_obj_set_style_pad_all(g_modal_obj, 16, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(g_modal_obj, LV_SCROLLBAR_MODE_OFF);

        create_label(g_modal_obj, "AIoT-MOR-ALDC SYSTEM BOOT", 16, 10, CLR_ACCENT, &lv_font_montserrat_18);
        create_label(g_modal_obj, "Select Sensor Input Operational Mode", 16, 36, CLR_TEXT_DIM, &lv_font_montserrat_12);

        /* ── Option 1: Live Sensor Mode Card ────────────────────────── */
        lv_obj_t *opt1 = lv_obj_create(g_modal_obj);
        lv_obj_set_pos(opt1, 8, 70);
        lv_obj_set_size(opt1, 400, 110);
        lv_obj_set_style_bg_color(opt1, CLR_PANEL, LV_PART_MAIN);
        lv_obj_set_style_border_color(opt1, CLR_GREEN, LV_PART_MAIN);
        lv_obj_set_style_border_width(opt1, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(opt1, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(opt1, 10, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(opt1, LV_SCROLLBAR_MODE_OFF);

        create_label(opt1, "[ S1 ]  LIVE SENSOR MODE", 0, 0, CLR_GREEN, &lv_font_montserrat_16);
        create_label(opt1, "- Real DHT11 hardware sensor on GPIO pin P410", 0, 28, CLR_TEXT, &lv_font_montserrat_12);
        create_label(opt1, "- Acquires live physical ambient temperature & humidity", 0, 52, CLR_TEXT_DIM, &lv_font_montserrat_12);

        /* ── Option 2: Data-Driven Mode Card ────────────────────────── */
        lv_obj_t *opt2 = lv_obj_create(g_modal_obj);
        lv_obj_set_pos(opt2, 8, 195);
        lv_obj_set_size(opt2, 400, 110);
        lv_obj_set_style_bg_color(opt2, CLR_PANEL, LV_PART_MAIN);
        lv_obj_set_style_border_color(opt2, CLR_BLUE, LV_PART_MAIN);
        lv_obj_set_style_border_width(opt2, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(opt2, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(opt2, 10, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(opt2, LV_SCROLLBAR_MODE_OFF);

        create_label(opt2, "[ S2 ]  DATA-DRIVEN MODE", 0, 0, CLR_BLUE, &lv_font_montserrat_16);
        create_label(opt2, "- Stream from Laptop (sensor_data.txt) via USB cable", 0, 28, CLR_TEXT, &lv_font_montserrat_12);
        create_label(opt2, "- Or runs automated dynamic thermal test scenarios", 0, 52, CLR_TEXT_DIM, &lv_font_montserrat_12);

        /* ── Selected Mode & Countdown Label ────────────────────────── */
        g_modal_sel_lbl = create_label(g_modal_obj, "", 16, 325, CLR_TEXT, &lv_font_montserrat_14);
        g_modal_cd_lbl  = create_label(g_modal_obj, "", 16, 355, CLR_YELLOW, &lv_font_montserrat_16);

        create_label(g_modal_obj, "How to Select:", 16, 395, CLR_ACCENT, &lv_font_montserrat_14);
        create_label(g_modal_obj, "1. Press Button S1 (P009) or S2 (P008) on board", 16, 420, CLR_TEXT, &lv_font_montserrat_12);
        create_label(g_modal_obj, "2. Or type '1' (Live) / '2' (Data-Driven) in Console", 16, 442, CLR_TEXT, &lv_font_montserrat_12);
        create_label(g_modal_obj, "3. Or wait for countdown to auto-start with default", 16, 464, CLR_TEXT_DIM, &lv_font_montserrat_12);
    }

    snprintf(buf, sizeof(buf), "Active Selection:  %s", current_mode_name);
    lv_label_set_text(g_modal_sel_lbl, buf);

    if (seconds_left > 0)
    {
        snprintf(buf, sizeof(buf), "Auto-starting in:  %lu seconds ...", (unsigned long)seconds_left);
        lv_label_set_text(g_modal_cd_lbl, buf);
        lv_obj_set_style_text_color(g_modal_cd_lbl, CLR_YELLOW, LV_PART_MAIN);
    }
    else
    {
        lv_label_set_text(g_modal_cd_lbl, ">>> MODE CONFIRMED! LAUNCHING OS <<<");
        lv_obj_set_style_text_color(g_modal_cd_lbl, CLR_GREEN, LV_PART_MAIN);
    }

    lv_timer_handler();
}

void display_ui_hide_startup_modal(void)
{
    if (g_modal_obj != NULL)
    {
        lv_obj_del(g_modal_obj);
        g_modal_obj     = NULL;
        g_modal_cd_lbl  = NULL;
        g_modal_sel_lbl = NULL;
        lv_timer_handler();
    }
}

void display_ui_set_mode_badge(const char *mode_badge)
{
    if (g_lbl_mode_badge != NULL && mode_badge != NULL)
    {
        lv_label_set_text(g_lbl_mode_badge, mode_badge);
    }
}

void display_ui_set_active_mode(int mode_id)
{
    if (g_box_live == NULL || g_box_data == NULL) return;

    if (mode_id == 1) /* Live DHT11 Sensor */
    {
        /* Highlight Box 1 (Green) */
        lv_obj_set_style_bg_color(g_box_live, lv_color_hex(0x064E3B), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(g_box_live, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_box_live, CLR_GREEN, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_box_live, 2, LV_PART_MAIN);
        if (g_lbl_live_title) lv_obj_set_style_text_color(g_lbl_live_title, CLR_TEXT, LV_PART_MAIN);
        if (g_lbl_live_badge)
        {
            lv_label_set_text(g_lbl_live_badge, "[ \xe2\x97\x8f ACTIVE (P410) ]");
            lv_obj_set_style_text_color(g_lbl_live_badge, CLR_GREEN, LV_PART_MAIN);
        }

        /* Dim Box 2 (Slate) */
        lv_obj_set_style_bg_color(g_box_data, CLR_PANEL, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_box_data, CLR_CARD_BORDER, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_box_data, 1, LV_PART_MAIN);
        if (g_lbl_data_title) lv_obj_set_style_text_color(g_lbl_data_title, CLR_TEXT_DIM, LV_PART_MAIN);
        if (g_lbl_data_badge)
        {
            lv_label_set_text(g_lbl_data_badge, "STANDBY (Press S2)");
            lv_obj_set_style_text_color(g_lbl_data_badge, CLR_TEXT_DIM, LV_PART_MAIN);
        }

        display_ui_set_mode_badge("LIVE DHT11");
        if (g_lbl_src_mode) lv_label_set_text(g_lbl_src_mode, "Src: LIVE DHT11");
    }
    else /* Data-Driven Mode */
    {
        /* Highlight Box 2 (Blue) */
        lv_obj_set_style_bg_color(g_box_data, lv_color_hex(0x0C4A6E), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(g_box_data, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_box_data, CLR_BLUE, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_box_data, 2, LV_PART_MAIN);
        if (g_lbl_data_title) lv_obj_set_style_text_color(g_lbl_data_title, CLR_TEXT, LV_PART_MAIN);
        if (g_lbl_data_badge)
        {
            lv_label_set_text(g_lbl_data_badge, "[ \xe2\x97\x8f ACTIVE (USB) ]");
            lv_obj_set_style_text_color(g_lbl_data_badge, CLR_BLUE, LV_PART_MAIN);
        }

        /* Dim Box 1 (Slate) */
        lv_obj_set_style_bg_color(g_box_live, CLR_PANEL, LV_PART_MAIN);
        lv_obj_set_style_border_color(g_box_live, CLR_CARD_BORDER, LV_PART_MAIN);
        lv_obj_set_style_border_width(g_box_live, 1, LV_PART_MAIN);
        if (g_lbl_live_title) lv_obj_set_style_text_color(g_lbl_live_title, CLR_TEXT_DIM, LV_PART_MAIN);
        if (g_lbl_live_badge)
        {
            lv_label_set_text(g_lbl_live_badge, "STANDBY (Press S1)");
            lv_obj_set_style_text_color(g_lbl_live_badge, CLR_TEXT_DIM, LV_PART_MAIN);
        }

        display_ui_set_mode_badge("DATA-DRIVEN");
        if (g_lbl_src_mode) lv_label_set_text(g_lbl_src_mode, "Src: DATA-DRIVEN");
    }
}

/* ==========================================================================
 * display_ui_update — Refresh dashboard metrics
 * ========================================================================== */
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
                       const char        *mode_name)
{
    char buf[128];
    char hms[24];
    char dat[14];

    fmt_hms(ts,  hms, sizeof(hms));
    fmt_date(ts, dat, sizeof(dat));

    /* ── Packet Counter & Mode Badge ────────────────────────────────── */
    snprintf(buf, sizeof(buf), "Pkt #%04lu", (unsigned long)pkt_id);
    lv_label_set_text(g_lbl_pkt, buf);

    if (mode_name != NULL)
    {
        lv_label_set_text(g_lbl_src_mode, mode_name);
    }

    /* ── Session Min / Max ──────────────────────────────────────────── */
    if (s_prev_temp < -900.0f)
    {
        s_sess_min = temp_c;
        s_sess_max = temp_c;
    }
    if (temp_c < s_sess_min) s_sess_min = temp_c;
    if (temp_c > s_sess_max) s_sess_max = temp_c;

    /* ── Temperature & Trend Badge ──────────────────────────────────── */
    snprintf(buf, sizeof(buf), "%.2f", (double)temp_c);
    lv_label_set_text(g_lbl_temp, buf);

    if (s_prev_temp > -900.0f)
    {
        float d = temp_c - s_prev_temp;
        if (d > TEMP_CHANGE_THRESHOLD_C)
        {
            lv_label_set_text(g_lbl_trend, "RISING");
            lv_obj_set_style_text_color(g_lbl_trend, CLR_RED, LV_PART_MAIN);
        }
        else if (d < -TEMP_CHANGE_THRESHOLD_C)
        {
            lv_label_set_text(g_lbl_trend, "FALLING");
            lv_obj_set_style_text_color(g_lbl_trend, CLR_BLUE, LV_PART_MAIN);
        }
        else
        {
            lv_label_set_text(g_lbl_trend, "STABLE");
            lv_obj_set_style_text_color(g_lbl_trend, CLR_GREEN, LV_PART_MAIN);
        }
    }

    /* ── Humidity Readout ───────────────────────────────────────────── */
    snprintf(buf, sizeof(buf), "%.1f %%RH", (double)humidity);
    lv_label_set_text(g_lbl_humidity, buf);

    /* ── Time & Date ────────────────────────────────────────────────── */
    snprintf(buf, sizeof(buf), "Time: %s", hms);
    lv_label_set_text(g_lbl_time, buf);
    snprintf(buf, sizeof(buf), "Date: %s", dat);
    lv_label_set_text(g_lbl_date, buf);

    /* ── Rate of Change ─────────────────────────────────────────────── */
    if (s_rate_prev_temp > -900.0f && uptime_sec > s_rate_prev_sec)
    {
        float dt_sec = (float)(uptime_sec - s_rate_prev_sec);
        float rate   = ((temp_c - s_rate_prev_temp) / dt_sec) * 60.0f;

        snprintf(buf, sizeof(buf), "Thermal Drift: %+.3f \xc2\xb0""C/min", (double)rate);
        lv_obj_set_style_text_color(g_lbl_rate,
            (rate >  0.05f) ? CLR_RED  :
            (rate < -0.05f) ? CLR_BLUE : CLR_GREEN,
            LV_PART_MAIN);
        lv_label_set_text(g_lbl_rate, buf);
    }
    s_rate_prev_temp = temp_c;
    s_rate_prev_sec  = uptime_sec;

    /* ── Session Extremes ───────────────────────────────────────────── */
    snprintf(buf, sizeof(buf), "Session Min: %.2f \xc2\xb0""C", (double)s_sess_min);
    lv_label_set_text(g_lbl_sess_min, buf);
    snprintf(buf, sizeof(buf), "Session Max: %.2f \xc2\xb0""C", (double)s_sess_max);
    lv_label_set_text(g_lbl_sess_max, buf);

    /* ── Temperature Change Detection ───────────────────────────────── */
    if (s_prev_temp > -900.0f &&
        fabsf(temp_c - s_prev_temp) >= TEMP_CHANGE_THRESHOLD_C)
    {
        s_chg_from = s_prev_temp;
        s_chg_to   = temp_c;
        s_chg_ts   = *ts;
        s_has_chg  = true;
    }
    s_prev_temp = temp_c;

    /* ── Card 2: Last Change Event ──────────────────────────────────── */
    if (s_has_chg)
    {
        float delta = s_chg_to - s_chg_from;
        snprintf(buf, sizeof(buf),
                 "%.2f\xc2\xb0""C  \xe2\x86\x92  %.2f\xc2\xb0""C   (%+.2f\xc2\xb0""C)",
                 (double)s_chg_from, (double)s_chg_to, (double)delta);
        lv_label_set_text(g_lbl_chg_vals, buf);
        lv_obj_set_style_text_color(g_lbl_chg_vals, (delta > 0.0f) ? CLR_RED : CLR_BLUE, LV_PART_MAIN);

        char chg_hms[24], chg_dat[14];
        fmt_hms(&s_chg_ts, chg_hms, sizeof(chg_hms));
        fmt_date(&s_chg_ts, chg_dat, sizeof(chg_dat));
        snprintf(buf, sizeof(buf), "%s  (%s)", chg_hms, chg_dat);
        lv_label_set_text(g_lbl_chg_at, buf);

        uint32_t el = elapsed_sec(&s_chg_ts, ts);
        fmt_elapsed(el, buf, sizeof(buf));
        lv_label_set_text(g_lbl_chg_elaps, buf);
    }
    else
    {
        lv_label_set_text(g_lbl_chg_vals, "Baseline Initialized (Awaiting change)");
        lv_obj_set_style_text_color(g_lbl_chg_vals, CLR_TEXT_DIM, LV_PART_MAIN);
        lv_label_set_text(g_lbl_chg_at,    "System Startup");
        lv_label_set_text(g_lbl_chg_elaps, "0 seconds");
    }

    /* ── Card 3: AI Inference ───────────────────────────────────────── */
    if (result->signal_class_name && strlen(result->signal_class_name) > 0)
    {
        lv_label_set_text(g_lbl_pattern, result->signal_class_name);
    }
    else
    {
        lv_label_set_text(g_lbl_pattern, "SMOOTH (Normal Profile)");
    }

    if (result->is_anomaly)
    {
        lv_obj_set_style_text_color(g_lbl_anomaly, CLR_RED, LV_PART_MAIN);
        lv_label_set_text(g_lbl_anomaly, "CRITICAL ANOMALY \xe2\x80\x94 Thermal Runaway Spike!");
    }
    else
    {
        lv_obj_set_style_text_color(g_lbl_anomaly, CLR_GREEN, LV_PART_MAIN);
        lv_label_set_text(g_lbl_anomaly, "NORMAL \xe2\x80\x94 Clean Profile (No Anomaly)");
    }

    if (result->do_compress)
    {
        snprintf(buf, sizeof(buf), "Adaptive Residual Codec (Active  CR=%.2fx)", (double)cr);
        lv_obj_set_style_text_color(g_lbl_compress, CLR_GREEN, LV_PART_MAIN);
    }
    else if (result->is_anomaly)
    {
        snprintf(buf, sizeof(buf), "Skipped \xe2\x80\x94 Anomalous data sent uncompressed");
        lv_obj_set_style_text_color(g_lbl_compress, CLR_YELLOW, LV_PART_MAIN);
    }
    else
    {
        snprintf(buf, sizeof(buf), "Raw Transmission (Signal policy)");
        lv_obj_set_style_text_color(g_lbl_compress, CLR_TEXT_DIM, LV_PART_MAIN);
    }
    lv_label_set_text(g_lbl_compress, buf);

    snprintf(buf, sizeof(buf), "%lu ms (ARM Cortex-M85 @ 480 MHz)", (unsigned long)infer_ms);
    lv_label_set_text(g_lbl_infer_time, buf);

    /* ── Card 4: Compression Benchmarks ─────────────────────────────── */
    snprintf(buf, sizeof(buf),
             "Payload: %lu Bytes (Raw) -> %lu Bytes (Comp)",
             (unsigned long)orig_bytes, (unsigned long)comp_bytes);
    lv_label_set_text(g_lbl_payload, buf);

    float saved_pct = (orig_bytes > comp_bytes) ?
                      ((float)(orig_bytes - comp_bytes) / (float)orig_bytes) * 100.0f : 0.0f;
    snprintf(buf, sizeof(buf),
             "Compression Ratio: %.2fx  |  Bandwidth Saved: %.1f%%",
             (double)cr, (double)saved_pct);
    lv_label_set_text(g_lbl_cr, buf);

    lv_label_set_text(g_lbl_network,
                      "Stream Link: Active (SEGGER RTT / TCP 192.168.10.100)");
}
