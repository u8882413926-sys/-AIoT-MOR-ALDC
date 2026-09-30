/*
 * residual_compress.c
 * ===========================================================================
 * Residual + Run-Length Encoding compression — Implementation
 * ===========================================================================
 */

#include "residual_compress.h"
#include <string.h>
#include <stdint.h>

/* ── Internal: RLE encode an int16 array into byte output ──────────────
 * Format: [count(1 byte)][value_low(1 byte)][value_high(1 byte)] per run
 * Returns number of bytes written, or 0 if output buffer overflows.
 */
static uint32_t rle_encode_int16(const int16_t *data, uint32_t n,
                                  uint8_t *out,        uint32_t out_cap)
{
    uint32_t out_pos = 0;
    uint32_t i = 0;

    while (i < n)
    {
        /* Count consecutive equal values (up to 255) */
        uint32_t run = 1;
        while ((i + run < n) && (run < 255) && (data[i + run] == data[i]))
            run++;

        /* Need 3 bytes per run: count + low byte + high byte */
        if (out_pos + 3 > out_cap)  return 0;   /* Overflow — abort */

        out[out_pos++] = (uint8_t)run;
        out[out_pos++] = (uint8_t)(data[i] & 0xFF);
        out[out_pos++] = (uint8_t)((data[i] >> 8) & 0xFF);

        i += run;
    }
    return out_pos;
}

/* ==========================================================================
 * compress_signal
 * ========================================================================== */
float compress_signal(const float   *signal,    uint32_t  n,
                      uint8_t       *out_buf,   uint32_t *out_len,
                      uint32_t      *orig_bytes)
{
    if (!signal || !out_buf || !out_len || !orig_bytes || n < 2)
        return 0.0f;

    *orig_bytes = n * sizeof(float);   /* e.g. 100 × 4 = 400 bytes */

    /* ── Step 1: Compute residuals and quantise to int16 ─────────────── */
    static int16_t residuals[512];    /* Static: avoids stack blowup        */
    uint32_t       n_diff = n;        /* First value stored raw, rest diffs */

    /* Store the first sample's integer value as the "anchor" */
    residuals[0] = (int16_t)((int32_t)(signal[0] * 100.0f));

    for (uint32_t i = 1; i < n; i++)
    {
        float diff = signal[i] - signal[i - 1];
        int16_t q  = (int16_t)(diff * 100.0f);   /* Quantise: 0.01 resolution */
        residuals[i] = q;
    }

    /* ── Step 2: RLE encode the int16 residual array ─────────────────── */
    uint32_t rle_bytes = rle_encode_int16(residuals, n_diff,
                                           out_buf, COMPRESS_MAX_OUT_BYTES);
    if (rle_bytes == 0)
    {
        /* Compression failed (output too large) — copy raw floats instead */
        if (*orig_bytes <= COMPRESS_MAX_OUT_BYTES)
        {
            memcpy(out_buf, signal, *orig_bytes);
            *out_len = *orig_bytes;
        }
        else
        {
            *out_len = 0;
        }
        return 1.0f;   /* No compression achieved */
    }

    *out_len = rle_bytes;

    /* ── Compression ratio ───────────────────────────────────────────── */
    float cr = (float)(*orig_bytes) / (float)(rle_bytes);
    return cr;
}
