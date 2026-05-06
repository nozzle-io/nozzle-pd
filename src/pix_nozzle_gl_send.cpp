#include "Base/GemBase.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "Gem/State.h"
#include "Gem/Cache.h"
#include "nozzle_pd_common.h"

#include <string>

class GEM_EXTERN pix_nozzle_gl_send : public GemBase {
    CPPEXTERN_HEADER(pix_nozzle_gl_send, GemBase);

public:
    pix_nozzle_gl_send(int argc, t_atom *argv);
    virtual ~pix_nozzle_gl_send();

    virtual void render(GemState *state);

protected:
    void nameMess(t_symbol *name);

private:
    NozzleSender *m_sender;
    t_symbol *m_sender_name;
    uint64_t m_frame_count;
};

CPPEXTERN_NEW_WITH_GIMME(pix_nozzle_gl_send);

pix_nozzle_gl_send :: pix_nozzle_gl_send(int argc, t_atom *argv)
    : m_sender(nullptr)
    , m_sender_name(gensym("nozzle_sender"))
    , m_frame_count(0)
{
}

pix_nozzle_gl_send :: ~pix_nozzle_gl_send() {
    if(m_sender) {
        nozzle_sender_destroy(m_sender);
        m_sender = nullptr;
    }
}

void pix_nozzle_gl_send :: nameMess(t_symbol *name) {
    if(!name || name == &s_) return;

    m_sender_name = name;

    if(m_sender) {
        nozzle_sender_destroy(m_sender);
        m_sender = nullptr;
    }
}

void pix_nozzle_gl_send :: render(GemState *state) {
    if(!m_sender) {
        NozzleSenderDesc desc{};
        desc.name = m_sender_name->s_name;
        desc.application_name = "pix_nozzle_gl_send";
        desc.ring_buffer_size = nozzle_pd::k_default_ring_buffer_size;

        NozzleErrorCode err = nozzle_sender_create(&desc, &m_sender);
        if(err != NOZZLE_OK) {
            error("pix_nozzle_gl_send: failed to create sender '%s' (error %d)",
                  m_sender_name->s_name, (int)err);
            return;
        }
    }

    int tex_id = 0;
    int tex_target = GL_TEXTURE_2D;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex_id);

    if(tex_id <= 0) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, tex_id);

    int width = 0, height = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    int internal_format = GL_RGBA8;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);

    glBindTexture(GL_TEXTURE_2D, 0);

    if(width <= 0 || height <= 0) {
        return;
    }

    NozzleTextureFormat nozzle_fmt = NOZZLE_FORMAT_RGBA8_UNORM;
    switch(internal_format) {
        case GL_RGBA8:           nozzle_fmt = NOZZLE_FORMAT_RGBA8_UNORM; break;
        case GL_RGBA8_SNORM:     nozzle_fmt = NOZZLE_FORMAT_RGBA8_UNORM; break;
        case GL_SRGB8_ALPHA8:    nozzle_fmt = NOZZLE_FORMAT_RGBA8_SRGB; break;
        case GL_BGRA:            nozzle_fmt = NOZZLE_FORMAT_BGRA8_UNORM; break;
        case GL_R8:              nozzle_fmt = NOZZLE_FORMAT_R8_UNORM; break;
        case GL_RG8:             nozzle_fmt = NOZZLE_FORMAT_RG8_UNORM; break;
        case GL_RGBA16:          nozzle_fmt = NOZZLE_FORMAT_RGBA16_UNORM; break;
        case GL_RGBA16F:         nozzle_fmt = NOZZLE_FORMAT_RGBA16_FLOAT; break;
        case GL_RGBA32F:         nozzle_fmt = NOZZLE_FORMAT_RGBA32_FLOAT; break;
        case GL_R32F:            nozzle_fmt = NOZZLE_FORMAT_R32_FLOAT; break;
        case GL_RG32F:           nozzle_fmt = NOZZLE_FORMAT_RG32_FLOAT; break;
        case GL_R16F:            nozzle_fmt = NOZZLE_FORMAT_R16_FLOAT; break;
        case GL_RG16F:           nozzle_fmt = NOZZLE_FORMAT_RG16_FLOAT; break;
        case GL_DEPTH_COMPONENT32F:
        case GL_DEPTH32F_STENCIL8:
                                nozzle_fmt = NOZZLE_FORMAT_DEPTH32_FLOAT; break;
        default:                nozzle_fmt = NOZZLE_FORMAT_RGBA8_UNORM; break;
    }

    NozzleErrorCode err = nozzle_sender_publish_gl_texture(
        m_sender, (uint32_t)tex_id, (uint32_t)tex_target,
        (uint32_t)width, (uint32_t)height, nozzle_fmt);

    if(err != NOZZLE_OK) {
        error("pix_nozzle_gl_send: publish failed (error %d)", (int)err);
        return;
    }

    m_frame_count++;
}

void pix_nozzle_gl_send :: obj_setupCallback(t_class *classPtr) {
    CPPEXTERN_MSG1(classPtr, "name", nameMess, t_symbol *);
}
