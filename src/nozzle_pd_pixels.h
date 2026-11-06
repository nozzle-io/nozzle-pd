#pragma once

#include "nozzle_pd_common.h"
#include <cstring>

namespace nozzle_pd {

inline bool is_color8(NozzleTextureFormat format) {
    return format == NOZZLE_FORMAT_RGBA8_UNORM || format == NOZZLE_FORMAT_BGRA8_UNORM ||
           format == NOZZLE_FORMAT_RGBA8_SRGB || format == NOZZLE_FORMAT_BGRA8_SRGB;
}

inline bool is_bgra8(NozzleTextureFormat format) {
    return format == NOZZLE_FORMAT_BGRA8_UNORM || format == NOZZLE_FORMAT_BGRA8_SRGB;
}

// Convert a logical RGBA channel to its source byte, including GL packed words.
template<class Image>
inline bool color8_source_channels(const Image &img, uint8_t rgba[4]) {
    const bool bgr = img.format == GL_BGR || img.format == GL_BGRA;
    if(!((img.csize == 3 && (img.format == GL_RGB || img.format == GL_BGR)) ||
         (img.csize == 4 && (img.format == GL_RGBA || img.format == GL_BGRA)))) return false;
    rgba[0] = bgr ? 2 : 0;
    rgba[1] = 1;
    rgba[2] = bgr ? 0 : 2;
    rgba[3] = 3;
    if(img.type == GL_UNSIGNED_BYTE) return true;
    if(img.csize != 4 || (img.type != GL_UNSIGNED_INT_8_8_8_8 &&
                         img.type != GL_UNSIGNED_INT_8_8_8_8_REV)) return false;
    const uint32_t one = 1;
    const bool little_endian = *reinterpret_cast<const unsigned char *>(&one) == 1;
    if(little_endian != (img.type == GL_UNSIGNED_INT_8_8_8_8_REV)) {
        for(int c = 0; c < 4; ++c) rgba[c] = 3 - rgba[c];
    }
    return true;
}

template<class Image>
inline bool valid_color8_mapping(const Image &img, const NozzleMappedPixels &mapped) {
    if(!img.data || !mapped.data || img.xsize <= 0 || img.ysize <= 0 ||
       mapped.width != static_cast<uint32_t>(img.xsize) ||
       mapped.height != static_cast<uint32_t>(img.ysize)) return false;
    const int64_t row_bytes = static_cast<int64_t>(img.xsize) * 4;
    return mapped.row_stride_bytes >= row_bytes || mapped.row_stride_bytes <= -row_bytes;
}

// Shared with the native-peer harness so it exercises the production copy path.
template<class Image>
inline bool copy_image_to_mapped(const Image &img, const NozzleMappedPixels &mapped) {
    if(is_color8(mapped.format)) {
        uint8_t rgba[4];
        if(!valid_color8_mapping(img, mapped) || !color8_source_channels(img, rgba)) return false;
        const uint8_t order[4] = {rgba[is_bgra8(mapped.format) ? 2 : 0], rgba[1],
                                  rgba[is_bgra8(mapped.format) ? 0 : 2], rgba[3]};
        const int64_t src_stride = static_cast<int64_t>(img.xsize) * img.csize;
        for(int y = 0; y < img.ysize; ++y) {
            const int src_y = img.upsidedown ? img.ysize - 1 - y : y;
            const auto *src = img.data + src_y * src_stride;
            auto *dst = static_cast<unsigned char *>(mapped.data) + y * mapped.row_stride_bytes;
            if(img.csize == 3) {
                for(int x = 0; x < img.xsize; ++x) {
                    for(int c = 0; c < 3; ++c) dst[4*x+c] = src[3*x+order[c]];
                    dst[4*x+3] = 255;
                }
            } else {
                // Row-wise calls keep signed mapped strides and Gem's flip out of
                // the core swizzler, whose stride contract is non-negative.
                if(nozzle_swizzle_channels(src, dst, mapped.width, 1, src_stride,
                    static_cast<int64_t>(mapped.width) * 4, NOZZLE_FORMAT_RGBA8_UNORM,
                    order) != NOZZLE_OK) return false;
            }
        }
        return true;
    }
    // Non-byte layout repair is tracked separately in nozzle-dev#219.
    uint32_t w = (uint32_t)img.xsize;
    uint32_t h = (uint32_t)img.ysize;
    uint32_t src_row_bytes = w * (uint32_t)img.csize;
    uint32_t dst_row_bytes = (uint32_t)(mapped.row_stride_bytes < 0
        ? -mapped.row_stride_bytes : mapped.row_stride_bytes);
    uint32_t copy_bytes = (src_row_bytes < dst_row_bytes) ? src_row_bytes : dst_row_bytes;
    bool flip = img.upsidedown;
    for(uint32_t y = 0; y < h; y++) {
        uint32_t src_y = flip ? (h - 1 - y) : y;
        const unsigned char *src_row =
            (const unsigned char *)img.data + (int64_t)src_y * src_row_bytes;
        unsigned char *dst_row =
            (unsigned char *)mapped.data + (int64_t)y * mapped.row_stride_bytes;
        memcpy(dst_row, src_row, copy_bytes);
    }
    return true;
}

template<class Image>
inline bool copy_mapped_to_image(const NozzleMappedPixels &mapped, Image &img) {
    if(is_color8(mapped.format)) {
        if(!valid_color8_mapping(img, mapped) || img.csize != 4 ||
           img.type != GL_UNSIGNED_BYTE || img.format != GL_RGBA) return false;
        const uint8_t order[4] = {static_cast<uint8_t>(is_bgra8(mapped.format) ? 2 : 0),
                                  1, static_cast<uint8_t>(is_bgra8(mapped.format) ? 0 : 2), 3};
        const int64_t dst_stride = static_cast<int64_t>(img.xsize) * 4;
        for(int y = 0; y < img.ysize; ++y) {
            const auto *src = static_cast<const unsigned char *>(mapped.data) + y * mapped.row_stride_bytes;
            auto *dst = img.data + y * dst_stride;
            if(nozzle_swizzle_channels(src, dst, mapped.width, 1, dst_stride, dst_stride,
                NOZZLE_FORMAT_RGBA8_UNORM, order) != NOZZLE_OK) return false;
        }
        return true;
    }
    // Preserve the existing non-byte path until nozzle-dev#219 is addressed.
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
    return true;
}

} // namespace nozzle_pd
