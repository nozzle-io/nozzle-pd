#include "Base/GemBase.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "Gem/State.h"
#include "Gem/Image.h"
#include "Gem/Cache.h"
#include "nozzle_pd_common.h"

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

        GLenum internal_fmt = nozzle_pd::nozzle_to_gl_internal_format(finfo.format);
        GLenum gl_fmt = nozzle_pd::nozzle_to_gl_format(finfo.format);
        GLenum gl_type = nozzle_pd::nozzle_to_gl_type(finfo.format);

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

    int csize = nozzle_pd::nozzle_to_gem_csize(finfo.format);
    int format = nozzle_pd::nozzle_to_gem_format(finfo.format);
    int type = nozzle_pd::nozzle_to_gem_type(finfo.format);

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
