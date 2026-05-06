#include "Base/GemBase.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "Gem/State.h"
#include "Gem/Image.h"
#include "Gem/Cache.h"

#include <cstring>
#include <string>

class GEM_EXTERN pix_nozzle_receive : public GemBase {
    CPPEXTERN_HEADER(pix_nozzle_receive, GemBase);

public:
    pix_nozzle_receive(int argc, t_atom *argv);
    virtual ~pix_nozzle_receive();

    virtual void render(GemState *state);
    virtual void postrender(GemState *state);

protected:
    void nameMess(t_symbol *name);

private:
    NozzleReceiver *m_receiver;
    t_symbol *m_sender_name;
    pixBlock m_pixBlock;
    bool m_pix_allocated;
    uint32_t m_last_width;
    uint32_t m_last_height;
    bool m_connected;
};

CPPEXTERN_NEW_WITH_GIMME(pix_nozzle_receive);

pix_nozzle_receive :: pix_nozzle_receive(int argc, t_atom *argv)
    : m_receiver(nullptr)
    , m_sender_name(gensym("nozzle_sender"))
    , m_pix_allocated(false)
    , m_last_width(0)
    , m_last_height(0)
    , m_connected(false)
{
    m_pixBlock.newimage = 0;
}

pix_nozzle_receive :: ~pix_nozzle_receive() {
    if(m_receiver) {
        nozzle_receiver_destroy(m_receiver);
        m_receiver = nullptr;
    }
    if(m_pix_allocated) {
        m_pixBlock.image.clear();
        m_pix_allocated = false;
    }
}

void pix_nozzle_receive :: nameMess(t_symbol *name) {
    if(!name || name == &s_) return;

    m_sender_name = name;

    if(m_receiver) {
        nozzle_receiver_destroy(m_receiver);
        m_receiver = nullptr;
    }
    m_connected = false;
}

static int nozzle_to_gem_csize(NozzleTextureFormat fmt) {
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

static int nozzle_to_gem_format(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:         return GL_LUMINANCE;
        case NOZZLE_FORMAT_RG8_UNORM:        return GL_LUMINANCE_ALPHA;
        case NOZZLE_FORMAT_RGBA8_UNORM:      return GL_RGBA;
        case NOZZLE_FORMAT_BGRA8_UNORM:      return GL_RGBA;
        case NOZZLE_FORMAT_RGBA8_SRGB:       return GL_RGBA;
        case NOZZLE_FORMAT_BGRA8_SRGB:       return GL_RGBA;
        case NOZZLE_FORMAT_R16_UNORM:
        case NOZZLE_FORMAT_RG16_UNORM:
        case NOZZLE_FORMAT_RGBA16_UNORM:
        case NOZZLE_FORMAT_R32_FLOAT:
        case NOZZLE_FORMAT_RG32_FLOAT:
        case NOZZLE_FORMAT_RGBA32_FLOAT:
        case NOZZLE_FORMAT_R32_UINT:
        case NOZZLE_FORMAT_RGBA32_UINT:
        case NOZZLE_FORMAT_DEPTH32_FLOAT:    return GL_RGBA;
        case NOZZLE_FORMAT_R16_FLOAT:
        case NOZZLE_FORMAT_RG16_FLOAT:
        case NOZZLE_FORMAT_RGBA16_FLOAT:     return GL_RGBA;
        default:                             return GL_RGBA;
    }
}

static int nozzle_to_gem_type(NozzleTextureFormat fmt) {
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
        case NOZZLE_FORMAT_R32_UINT:
        case NOZZLE_FORMAT_RGBA32_UINT:
            return GL_UNSIGNED_SHORT;
        case NOZZLE_FORMAT_R16_FLOAT:
        case NOZZLE_FORMAT_RG16_FLOAT:
        case NOZZLE_FORMAT_RGBA16_FLOAT:
        case NOZZLE_FORMAT_R32_FLOAT:
        case NOZZLE_FORMAT_RG32_FLOAT:
        case NOZZLE_FORMAT_RGBA32_FLOAT:
        case NOZZLE_FORMAT_DEPTH32_FLOAT:
            return GL_FLOAT;
        default:
            return GL_UNSIGNED_BYTE;
    }
}

void pix_nozzle_receive :: render(GemState *state) {
    if(!m_receiver) {
        NozzleReceiverDesc desc{};
        desc.name = m_sender_name->s_name;
        desc.application_name = "pix_nozzle_receive";
        desc.receive_mode = NOZZLE_RECEIVE_LATEST_ONLY;

        NozzleErrorCode err = nozzle_receiver_create(&desc, &m_receiver);
        if(err != NOZZLE_OK) {
            if(!m_connected) {
                post("pix_nozzle_receive: waiting for sender '%s'",
                     m_sender_name->s_name);
                m_connected = true;
            }
            return;
        }
        post("pix_nozzle_receive: connected to '%s'", m_sender_name->s_name);
    }

    NozzleAcquireDesc acq{};
    acq.timeout_ms = 0;

    NozzleFrame *frame = nullptr;
    NozzleErrorCode err = nozzle_receiver_acquire_frame(m_receiver, &acq, &frame);
    if(err != NOZZLE_OK || !frame) {
        return;
    }

    NozzleFrameInfo finfo{};
    nozzle_frame_get_info(frame, &finfo);

    uint32_t w = finfo.width;
    uint32_t h = finfo.height;

    NozzleMappedPixels mapped{};
    err = nozzle_frame_lock_pixels_with_origin(frame, NOZZLE_ORIGIN_TOP_LEFT, &mapped);
    if(err != NOZZLE_OK) {
        nozzle_frame_release(frame);
        return;
    }

    if(!m_pix_allocated || w != m_last_width || h != m_last_height) {
        if(m_pix_allocated) {
            m_pixBlock.image.clear();
        }

        int csize = nozzle_to_gem_csize(finfo.format);
        int format = nozzle_to_gem_format(finfo.format);
        int type = nozzle_to_gem_type(finfo.format);

        m_pixBlock.image.xsize = (int)w;
        m_pixBlock.image.ysize = (int)h;
        m_pixBlock.image.csize = csize;
        m_pixBlock.image.format = format;
        m_pixBlock.image.type = type;
        m_pixBlock.image.upsidedown = false;
        m_pixBlock.image.allocate();

        m_pix_allocated = true;
        m_last_width = w;
        m_last_height = h;
    }

    uint32_t src_row_bytes = (uint32_t)(mapped.row_stride_bytes < 0
        ? -mapped.row_stride_bytes : mapped.row_stride_bytes);
    uint32_t dst_row_bytes = (uint32_t)(w * m_pixBlock.image.csize);
    uint32_t copy_bytes = (src_row_bytes < dst_row_bytes) ? src_row_bytes : dst_row_bytes;

    for(uint32_t y = 0; y < h; y++) {
        const unsigned char *src_row =
            (const unsigned char *)mapped.data + (int64_t)y * mapped.row_stride_bytes;
        unsigned char *dst_row =
            (unsigned char *)m_pixBlock.image.data + (int64_t)y * dst_row_bytes;
        memcpy(dst_row, src_row, copy_bytes);
    }

    nozzle_frame_unlock_pixels(frame);
    nozzle_frame_release(frame);

    m_pixBlock.newimage = 1;
    state->set(GemState::_PIX, &m_pixBlock);
}

void pix_nozzle_receive :: postrender(GemState *state) {
    m_pixBlock.newimage = 0;
}

void pix_nozzle_receive :: obj_setupCallback(t_class *classPtr) {
    CPPEXTERN_MSG1(classPtr, "name", nameMess, t_symbol *);
}
