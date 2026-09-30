/*
 * ai_pipeline.h
 * ===========================================================================
 * AIoT-MOR-ALDC Inference Pipeline for RK-RA8P1
 *
 * This is the C equivalent of the Python ai_moraldc_intel_real.py logic.
 * Steps performed on each 100-reading window:
 *   1. Feature extraction  (mean, std, min, max, pct25, pct75, diff_mean, diff_std)
 *   2. MinMax feature scaling
 *   3. Signal type classification  (Random Forest — classifier_model.h)
 *   4. Anomaly detection           (Isolation Forest — anomaly_model.h)
 *   5. Compression-mode decision   (Decision Tree   — compression_model.h)
 * ===========================================================================
 */

#ifndef AI_PIPELINE_H
#define AI_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>

/* ── Number of time-series readings to buffer before one inference ──────── */
#define AI_WINDOW_SIZE    (100U)

/* ── Number of extracted features ─────────────────────────────────────── */
#define AI_NUM_FEATURES   (8U)

/* ── Signal type labels (index maps to LABEL_NAMES[] in scaler_params.h) */
#define AI_CLASS_BURST    (0)
#define AI_CLASS_DRIFT    (1)
#define AI_CLASS_MIXED    (2)
#define AI_CLASS_NOISY    (3)
#define AI_CLASS_PERIODIC (4)
#define AI_CLASS_SMOOTH   (5)

/* ── AI pipeline result ─────────────────────────────────────────────────── */
typedef struct {
    int     signal_class;           /* Classifier output (0..5)             */
    const char *signal_class_name;  /* Human-readable string                */
    bool    is_anomaly;             /* true = anomaly detected              */
    bool    do_compress;            /* true = compression recommended       */
    float   features[AI_NUM_FEATURES]; /* Extracted & scaled features       */
} ai_result_t;

/* ── Public API ─────────────────────────────────────────────────────────── */

/**
 * @brief  Initialise the AI pipeline (validates model headers are loaded).
 *         Call once at startup before any ai_pipeline_run() calls.
 */
void ai_pipeline_init(void);

/**
 * @brief  Run the full AIoT-MOR-ALDC inference pipeline on one window of data.
 *
 * @param  window_data   Pointer to AI_WINDOW_SIZE float temperature readings.
 *                       Values should be raw Celsius (the function normalises them).
 * @param  result        Output — filled with classification, anomaly and compression decision.
 */
void ai_pipeline_run(const float *window_data, ai_result_t *result);

#endif /* AI_PIPELINE_H */
