#pragma once

#include <nozzle/nozzle_c.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace nozzle_pd {

constexpr uint32_t k_default_ring_buffer_size = 3;

inline int nozzle_to_gem_csize(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:         return 1;
        case NOZZLE_FORMAT_RG8_UNORM:        return 2;
        case NOZZLE_FORMAT_RGBA8_UNORM:      return 4;
        case NOZZLE_FORMAT_BGRA8_UNORM:      return 4;
        case NOZZLE_FORMAT_RGBA8_SRGB:       return 4;
        case NOZZLE_FORMAT_BGRA8_SRGB:       return 4;
        case NOZZLE_FORMAT_R16_UNORM:        return 2;
        case NOZZLE_FORMAT_RG16_UNORM:       return 4;
        case NOZZLE_FORMAT_RGBA16_UNORM:     return 8;
        case NOZZLE_FORMAT_R16_FLOAT:        return 2;
        case NOZZLE_FORMAT_RG16_FLOAT:       return 4;
        case NOZZLE_FORMAT_RGBA16_FLOAT:     return 8;
        case NOZZLE_FORMAT_R32_FLOAT:        return 4;
        case NOZZLE_FORMAT_RG32_FLOAT:       return 8;
        case NOZZLE_FORMAT_RGBA32_FLOAT:     return 16;
        case NOZZLE_FORMAT_R32_UINT:         return 4;
        case NOZZLE_FORMAT_RGBA32_UINT:      return 16;
        case NOZZLE_FORMAT_DEPTH32_FLOAT:    return 4;
        default:                             return 4;
    }
}

inline int nozzle_to_gem_format(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:         return GL_LUMINANCE;
        case NOZZLE_FORMAT_RG8_UNORM:        return GL_LUMINANCE_ALPHA;
        case NOZZLE_FORMAT_RGBA8_UNORM:      return GL_RGBA;
        case NOZZLE_FORMAT_BGRA8_UNORM:      return GL_RGBA;
        case NOZZLE_FORMAT_RGBA8_SRGB:       return GL_RGBA;
        case NOZZLE_FORMAT_BGRA8_SRGB:       return GL_RGBA;
        case NOZZLE_FORMAT_R16_UNORM:        return GL_LUMINANCE;
        case NOZZLE_FORMAT_RG16_UNORM:       return GL_LUMINANCE_ALPHA;
        case NOZZLE_FORMAT_RGBA16_UNORM:     return GL_RGBA;
        case NOZZLE_FORMAT_R16_FLOAT:        return GL_LUMINANCE;
        case NOZZLE_FORMAT_RG16_FLOAT:       return GL_LUMINANCE_ALPHA;
        case NOZZLE_FORMAT_RGBA16_FLOAT:     return GL_RGBA;
        case NOZZLE_FORMAT_R32_FLOAT:        return GL_LUMINANCE;
        case NOZZLE_FORMAT_RG32_FLOAT:       return GL_LUMINANCE_ALPHA;
        case NOZZLE_FORMAT_RGBA32_FLOAT:     return GL_RGBA;
        case NOZZLE_FORMAT_R32_UINT:         return GL_LUMINANCE;
        case NOZZLE_FORMAT_RGBA32_UINT:      return GL_RGBA;
        case NOZZLE_FORMAT_DEPTH32_FLOAT:    return GL_LUMINANCE;
        default:                             return GL_RGBA;
    }
}

inline int nozzle_to_gem_type(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:
        case NOZZLE_FORMAT_RG8_UNORM:
        case NOZZLE_FORMAT_RGBA8_UNORM:
        case NOZZLE_FORMAT_BGRA8_UNORM:
        case NOZZLE_FORMAT_RGBA8_SRGB:
        case NOZZLE_FORMAT_BGRA8_SRGB:
            return GL_UNSIGNED_BYTE;
        case NOZZLE_FORMAT_R16_UNORM:
        case NOZZLE_FORMAT_RG16_UNORM:
        case NOZZLE_FORMAT_RGBA16_UNORM:
            return GL_UNSIGNED_SHORT;
        case NOZZLE_FORMAT_R16_FLOAT:
        case NOZZLE_FORMAT_RG16_FLOAT:
        case NOZZLE_FORMAT_RGBA16_FLOAT:
            return GL_HALF_FLOAT;
        case NOZZLE_FORMAT_R32_FLOAT:
        case NOZZLE_FORMAT_RG32_FLOAT:
        case NOZZLE_FORMAT_RGBA32_FLOAT:
        case NOZZLE_FORMAT_DEPTH32_FLOAT:
            return GL_FLOAT;
        case NOZZLE_FORMAT_R32_UINT:
        case NOZZLE_FORMAT_RGBA32_UINT:
            return GL_UNSIGNED_INT;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

inline GLenum nozzle_to_gl_internal_format(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:         return GL_R8;
        case NOZZLE_FORMAT_RG8_UNORM:        return GL_RG8;
        case NOZZLE_FORMAT_RGBA8_UNORM:      return GL_RGBA8;
        case NOZZLE_FORMAT_BGRA8_UNORM:      return GL_RGBA8;
        case NOZZLE_FORMAT_RGBA8_SRGB:       return GL_SRGB8_ALPHA8;
        case NOZZLE_FORMAT_BGRA8_SRGB:       return GL_SRGB8_ALPHA8;
        case NOZZLE_FORMAT_R16_UNORM:        return GL_R16;
        case NOZZLE_FORMAT_RG16_UNORM:       return GL_RG16;
        case NOZZLE_FORMAT_RGBA16_UNORM:     return GL_RGBA16;
        case NOZZLE_FORMAT_R16_FLOAT:        return GL_R16F;
        case NOZZLE_FORMAT_RG16_FLOAT:       return GL_RG16F;
        case NOZZLE_FORMAT_RGBA16_FLOAT:     return GL_RGBA16F;
        case NOZZLE_FORMAT_R32_FLOAT:        return GL_R32F;
        case NOZZLE_FORMAT_RG32_FLOAT:       return GL_RG32F;
        case NOZZLE_FORMAT_RGBA32_FLOAT:     return GL_RGBA32F;
        case NOZZLE_FORMAT_R32_UINT:         return GL_R32UI;
        case NOZZLE_FORMAT_RGBA32_UINT:      return GL_RGBA32UI;
        case NOZZLE_FORMAT_DEPTH32_FLOAT:    return GL_DEPTH_COMPONENT32F;
        default:                             return GL_RGBA8;
    }
}

inline GLenum nozzle_to_gl_format(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:
        case NOZZLE_FORMAT_R16_UNORM:
        case NOZZLE_FORMAT_R16_FLOAT:
        case NOZZLE_FORMAT_R32_FLOAT:
        case NOZZLE_FORMAT_R32_UINT:
        case NOZZLE_FORMAT_DEPTH32_FLOAT:
            return GL_RED;
        case NOZZLE_FORMAT_RG8_UNORM:
        case NOZZLE_FORMAT_RG16_UNORM:
        case NOZZLE_FORMAT_RG16_FLOAT:
        case NOZZLE_FORMAT_RG32_FLOAT:
            return GL_RG;
        case NOZZLE_FORMAT_BGRA8_UNORM:
        case NOZZLE_FORMAT_BGRA8_SRGB:
            return GL_BGRA;
        case NOZZLE_FORMAT_RGBA8_UNORM:
        case NOZZLE_FORMAT_RGBA8_SRGB:
        case NOZZLE_FORMAT_RGBA16_UNORM:
        case NOZZLE_FORMAT_RGBA16_FLOAT:
        case NOZZLE_FORMAT_RGBA32_FLOAT:
        case NOZZLE_FORMAT_RGBA32_UINT:
        default:
            return GL_RGBA;
    }
}

inline GLenum nozzle_to_gl_type(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:
        case NOZZLE_FORMAT_RG8_UNORM:
        case NOZZLE_FORMAT_RGBA8_UNORM:
        case NOZZLE_FORMAT_BGRA8_UNORM:
        case NOZZLE_FORMAT_RGBA8_SRGB:
        case NOZZLE_FORMAT_BGRA8_SRGB:
            return GL_UNSIGNED_BYTE;
        case NOZZLE_FORMAT_R16_UNORM:
        case NOZZLE_FORMAT_RG16_UNORM:
        case NOZZLE_FORMAT_RGBA16_UNORM:
            return GL_UNSIGNED_SHORT;
        case NOZZLE_FORMAT_R16_FLOAT:
        case NOZZLE_FORMAT_RG16_FLOAT:
        case NOZZLE_FORMAT_RGBA16_FLOAT:
            return GL_HALF_FLOAT;
        case NOZZLE_FORMAT_R32_FLOAT:
        case NOZZLE_FORMAT_RG32_FLOAT:
        case NOZZLE_FORMAT_RGBA32_FLOAT:
        case NOZZLE_FORMAT_DEPTH32_FLOAT:
            return GL_FLOAT;
        case NOZZLE_FORMAT_R32_UINT:
        case NOZZLE_FORMAT_RGBA32_UINT:
            return GL_UNSIGNED_INT;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

inline NozzleTextureFormat gem_to_nozzle_format(int csize, int type) {
    if(type == GL_UNSIGNED_BYTE) {
        switch(csize) {
            case 4: return NOZZLE_FORMAT_RGBA8_UNORM;
            case 3: return NOZZLE_FORMAT_RGBA8_UNORM;
            case 1: return NOZZLE_FORMAT_R8_UNORM;
        }
    } else if(type == GL_FLOAT) {
        switch(csize) {
            case 4: return NOZZLE_FORMAT_RGBA32_FLOAT;
            case 3: return NOZZLE_FORMAT_RGBA32_FLOAT;
            case 1: return NOZZLE_FORMAT_R32_FLOAT;
        }
    } else if(type == GL_HALF_FLOAT) {
        switch(csize) {
            case 4: return NOZZLE_FORMAT_RGBA16_FLOAT;
            case 3: return NOZZLE_FORMAT_RGBA16_FLOAT;
            case 1: return NOZZLE_FORMAT_R16_FLOAT;
        }
    }
    return NOZZLE_FORMAT_RGBA8_UNORM;
}

} // namespace nozzle_pd
