#pragma once

#include <stdint.h>

/**
 * RLE sprite data format.
 *
 * Each frame is a sequence of (value, run_count) uint16_t pairs packed
 * contiguously into rle_data[]. frame_offsets[i] gives the uint16_t word
 * offset into rle_data where frame i begins; frame_offsets[frame_count]
 * is a sentinel pointing past the last frame's data.
 */
typedef struct {
    const uint16_t *rle_data;
    const uint32_t *frame_offsets; /* frame_count + 1 entries */
    int frame_count;
    int width;
    int height;
} rle_sprite_t;

/**
 * Decode RLE data to raw RGB565 pixels.
 *
 * @param rle       Pointer to (value, count) pairs
 * @param out       Output buffer (must hold pixel_count uint16_t values)
 * @param pixel_count  Number of pixels to decode (width * height)
 */
static inline void rle_decode_rgb565(const uint16_t *rle, uint16_t *out, int pixel_count)
{
    int pos = 0;
    while (pos < pixel_count) {
        uint16_t value = *rle++;
        uint16_t count = *rle++;
        for (uint16_t i = 0; i < count && pos < pixel_count; i++) {
            out[pos++] = value;
        }
    }
}

/**
 * Decode RLE data directly to ARGB8888 (LVGL format: B, G, R, A bytes).
 *
 * Pixels matching transparent_key get alpha = 0; all others get alpha = 0xFF.
 * RGB565 channels are expanded to 8-bit.
 *
 * @param rle              Pointer to (value, count) pairs
 * @param out              Output buffer (must hold pixel_count * 4 bytes)
 * @param pixel_count      Number of pixels (width * height)
 * @param transparent_key  RGB565 value treated as fully transparent
 */
static inline void rle_decode_argb8888(const uint16_t *rle, uint8_t *out,
                                       int pixel_count, uint16_t transparent_key)
{
    int pos = 0;
    while (pos < pixel_count) {
        uint16_t value = *rle++;
        uint16_t count = *rle++;

        /* Pre-compute the BGRA bytes for this run */
        uint8_t r5 = (value >> 11) & 0x1F;
        uint8_t g6 = (value >> 5)  & 0x3F;
        uint8_t b5 =  value        & 0x1F;
        uint8_t b = (uint8_t)((b5 << 3) | (b5 >> 2));
        uint8_t g = (uint8_t)((g6 << 2) | (g6 >> 4));
        uint8_t r = (uint8_t)((r5 << 3) | (r5 >> 2));
        uint8_t a = (value == transparent_key) ? 0x00 : 0xFF;

        for (uint16_t i = 0; i < count && pos < pixel_count; i++, pos++) {
            out[pos * 4 + 0] = b;
            out[pos * 4 + 1] = g;
            out[pos * 4 + 2] = r;
            out[pos * 4 + 3] = a;
        }
    }
}
