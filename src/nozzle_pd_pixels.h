#pragma once

#include <nozzle/nozzle_c.h>
#include <cstring>

namespace nozzle_pd {

// Shared with the native-peer harness so it exercises the production copy path.
template<class Image>
inline void copy_image_to_mapped(const Image &img, const NozzleMappedPixels &mapped) {
    uint32_t w = (uint32_t)img.xsize;
    uint32_t h = (uint32_t)img.ysize;
    uint32_t src_row_bytes = w * (uint32_t)img.csize;
    uint32_t dst_row_bytes = (uint32_t)(mapped.row_stride_bytes < 0
        ? -mapped.row_stride_bytes : mapped.row_stride_bytes);
    uint32_t copy_bytes = (src_row_bytes < dst_row_bytes) ? src_row_bytes : dst_row_bytes;
    bool flip = img.upsidedown;
    bool need_rgb_to_rgba = (img.csize == 3 && mapped.format == NOZZLE_FORMAT_RGBA8_UNORM);
    for(uint32_t y = 0; y < h; y++) {
        uint32_t src_y = flip ? (h - 1 - y) : y;
        const unsigned char *src_row =
            (const unsigned char *)img.data + (int64_t)src_y * src_row_bytes;
        unsigned char *dst_row =
            (unsigned char *)mapped.data + (int64_t)y * mapped.row_stride_bytes;
        if(need_rgb_to_rgba) {
            for(uint32_t x = 0; x < w; x++) {
                dst_row[x * 4 + 0] = src_row[x * 3 + 0];
                dst_row[x * 4 + 1] = src_row[x * 3 + 1];
                dst_row[x * 4 + 2] = src_row[x * 3 + 2];
                dst_row[x * 4 + 3] = 255;
            }
        } else {
            memcpy(dst_row, src_row, copy_bytes);
        }
    }
}

template<class Image>
inline void copy_mapped_to_image(const NozzleMappedPixels &mapped, Image &img) {
    uint32_t w = (uint32_t)img.xsize;
    uint32_t h = (uint32_t)img.ysize;
    uint32_t src_row_bytes = (uint32_t)(mapped.row_stride_bytes < 0
        ? -mapped.row_stride_bytes : mapped.row_stride_bytes);
    uint32_t dst_row_bytes = w * (uint32_t)img.csize;
    uint32_t copy_bytes = (src_row_bytes < dst_row_bytes) ? src_row_bytes : dst_row_bytes;
    for(uint32_t y = 0; y < h; y++) {
        const unsigned char *src_row =
            (const unsigned char *)mapped.data + (int64_t)y * mapped.row_stride_bytes;
        unsigned char *dst_row =
            (unsigned char *)img.data + (int64_t)y * dst_row_bytes;
        memcpy(dst_row, src_row, copy_bytes);
    }
}

} // namespace nozzle_pd
