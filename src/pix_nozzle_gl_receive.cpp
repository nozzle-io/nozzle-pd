#include "Base/GemBase.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "Gem/State.h"
#include "Gem/Image.h"
#include "Gem/Cache.h"

#include <string>

class GEM_EXTERN pix_nozzle_gl_receive : public GemBase {
    CPPEXTERN_HEADER(pix_nozzle_gl_receive, GemBase);

public:
    pix_nozzle_gl_receive(int argc, t_atom *argv);
    virtual ~pix_nozzle_gl_receive();

    virtual void render(GemState *state);
    virtual void postrender(GemState *state);

protected:
    void nameMess(t_symbol *name);

private:
    NozzleReceiver *m_receiver;
    t_symbol *m_sender_name;
    GLuint m_gl_texture;
    uint32_t m_tex_width;
    uint32_t m_tex_height;
    bool m_tex_initialized;
    pixBlock m_pixBlock;
    bool m_pix_allocated;
    uint32_t m_last_width;
    uint32_t m_last_height;
    bool m_connected;
};

CPPEXTERN_NEW_WITH_GIMME(pix_nozzle_gl_receive);

pix_nozzle_gl_receive :: pix_nozzle_gl_receive(int argc, t_atom *argv)
    : m_receiver(nullptr)
    , m_sender_name(gensym("nozzle_sender"))
    , m_gl_texture(0)
    , m_tex_width(0)
    , m_tex_height(0)
    , m_tex_initialized(false)
    , m_pix_allocated(false)
    , m_last_width(0)
    , m_last_height(0)
    , m_connected(false)
{
    m_pixBlock.newimage = 0;
}

pix_nozzle_gl_receive :: ~pix_nozzle_gl_receive() {
    if(m_receiver) {
        nozzle_receiver_destroy(m_receiver);
        m_receiver = nullptr;
    }
    if(m_gl_texture) {
        glDeleteTextures(1, &m_gl_texture);
        m_gl_texture = 0;
    }
    if(m_pix_allocated) {
        m_pixBlock.image.clear();
        m_pix_allocated = false;
    }
}

void pix_nozzle_gl_receive :: nameMess(t_symbol *name) {
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
        case NOZZLE_FORMAT_RGBA8_UNORM:
        case NOZZLE_FORMAT_BGRA8_UNORM:
        case NOZZLE_FORMAT_RGBA8_SRGB:
        case NOZZLE_FORMAT_BGRA8_SRGB:
        case NOZZLE_FORMAT_R16_UNORM:
        case NOZZLE_FORMAT_RG16_UNORM:
        case NOZZLE_FORMAT_RGBA16_UNORM:
        case NOZZLE_FORMAT_R32_FLOAT:
        case NOZZLE_FORMAT_RG32_FLOAT:
        case NOZZLE_FORMAT_RGBA32_FLOAT:
        case NOZZLE_FORMAT_R32_UINT:
        case NOZZLE_FORMAT_RGBA32_UINT:
        case NOZZLE_FORMAT_DEPTH32_FLOAT:
        case NOZZLE_FORMAT_R16_FLOAT:
        case NOZZLE_FORMAT_RG16_FLOAT:
        case NOZZLE_FORMAT_RGBA16_FLOAT:
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

static GLenum nozzle_to_gl_internal_format(NozzleTextureFormat fmt) {
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

static GLenum nozzle_to_gl_format(NozzleTextureFormat fmt) {
    switch(fmt) {
        case NOZZLE_FORMAT_R8_UNORM:         return GL_RED;
        case NOZZLE_FORMAT_RG8_UNORM:        return GL_RG;
        case NOZZLE_FORMAT_RGBA8_UNORM:      return GL_RGBA;
        case NOZZLE_FORMAT_BGRA8_UNORM:      return GL_BGRA;
        case NOZZLE_FORMAT_RGBA8_SRGB:       return GL_RGBA;
        case NOZZLE_FORMAT_BGRA8_SRGB:       return GL_BGRA;
        default:                             return GL_RGBA;
    }
}

void pix_nozzle_gl_receive :: render(GemState *state) {
    if(!m_receiver) {
        NozzleReceiverDesc desc{};
        desc.name = m_sender_name->s_name;
        desc.application_name = "pix_nozzle_gl_receive";
        desc.receive_mode = NOZZLE_RECEIVE_LATEST_ONLY;

        NozzleErrorCode err = nozzle_receiver_create(&desc, &m_receiver);
        if(err != NOZZLE_OK) {
            if(!m_connected) {
                post("pix_nozzle_gl_receive: waiting for sender '%s'",
                     m_sender_name->s_name);
                m_connected = true;
            }
            return;
        }
        post("pix_nozzle_gl_receive: connected to '%s'", m_sender_name->s_name);
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

    if(!m_tex_initialized || w != m_tex_width || h != m_tex_height) {
        if(m_gl_texture) {
            glDeleteTextures(1, &m_gl_texture);
        }

        glGenTextures(1, &m_gl_texture);
        glBindTexture(GL_TEXTURE_2D, m_gl_texture);

        GLenum internal_fmt = nozzle_to_gl_internal_format(finfo.format);
        GLenum gl_fmt = nozzle_to_gl_format(finfo.format);
        GLenum gl_type = GL_UNSIGNED_BYTE;
        if(finfo.format == NOZZLE_FORMAT_RGBA16_FLOAT ||
           finfo.format == NOZZLE_FORMAT_R16_FLOAT ||
           finfo.format == NOZZLE_FORMAT_RG16_FLOAT) {
            gl_type = GL_HALF_FLOAT;
        } else if(finfo.format == NOZZLE_FORMAT_RGBA32_FLOAT ||
                  finfo.format == NOZZLE_FORMAT_R32_FLOAT ||
                  finfo.format == NOZZLE_FORMAT_RG32_FLOAT) {
            gl_type = GL_FLOAT;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internal_fmt, (GLsizei)w, (GLsizei)h,
                     0, gl_fmt, gl_type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_tex_width = w;
        m_tex_height = h;
        m_tex_initialized = true;
    }

    err = nozzle_frame_copy_to_gl_texture(
        frame, m_gl_texture, GL_TEXTURE_2D, w, h, finfo.format);
    if(err != NOZZLE_OK) {
        error("pix_nozzle_gl_receive: copy to GL texture failed (error %d)",
              (int)err);
        nozzle_frame_release(frame);
        return;
    }

    nozzle_frame_release(frame);

    int csize = nozzle_to_gem_csize(finfo.format);
    int format = nozzle_to_gem_format(finfo.format);
    int type = nozzle_to_gem_type(finfo.format);

    if(!m_pix_allocated || w != m_last_width || h != m_last_height) {
        if(m_pix_allocated) {
            m_pixBlock.image.clear();
        }
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

    m_pixBlock.newimage = 1;
    state->set(GemState::_PIX, &m_pixBlock);
}

void pix_nozzle_gl_receive :: postrender(GemState *state) {
    m_pixBlock.newimage = 0;
}

void pix_nozzle_gl_receive :: obj_setupCallback(t_class *classPtr) {
    CPPEXTERN_MSG1(classPtr, "name", nameMess, t_symbol *);
}
