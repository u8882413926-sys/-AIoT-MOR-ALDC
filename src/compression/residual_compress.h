/*
 * residual_compress.h
 * ===========================================================================
 * Lightweight residual + RLE compression for sensor data.
 * Embedded equivalent of the Python zlib.compress() + residual encoding
 * used in ai_moraldc_intel_real.py.
 *
 * Why not use zlib on the board?
 *   zlib requires heap allocation and a ~32 KB window buffer which is wasteful.
 *   This implementation uses only stack/static memory.
 *
 * Algorithm:
 *   1. Compute residuals: diff[i] = signal[i+1] - signal[i]
 *   2. Quantise residuals to int16 (×100 scale)
 *   3. Run-Length Encode the int16 stream
 *   Compression ratio: 1.5× to 3× depending on signal type.
 * ===========================================================================
 */

#ifndef RESIDUAL_COMPRESS_H
#define RESIDUAL_COMPRESS_H

#include <stdint.h>
#include <stddef.h>

/* Maximum possible output size (conservative bound: 2× input) */
#define COMPRESS_MAX_OUT_BYTES   (512U)

/**
 * @brief  Compress a float signal using residual + RLE encoding.
 *
 * @param  signal       Input float array of 'n' samples.
 * @param  n            Number of samples.
 * @param  out_buf      Output byte buffer. Must be at least COMPRESS_MAX_OUT_BYTES.
 * @param  out_len      Filled with the number of bytes written to out_buf.
 * @param  orig_bytes   Filled with the original size in bytes (n × 4).
 *
 * @return Compression ratio (original_bytes / compressed_bytes), or 0.0f on error.
 */
float compress_signal(const float *signal, uint32_t n,
                      uint8_t *out_buf, uint32_t *out_len,
                      uint32_t *orig_bytes);

#endif /* RESIDUAL_COMPRESS_H */
