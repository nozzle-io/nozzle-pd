#include "Base/GemPixObj.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "Gem/State.h"
#include "Gem/Cache.h"
#include "nozzle_pd_common.h"
#include "nozzle_pd_pixels.h"

#include <cstring>
#include <string>

namespace {

struct nozzle_frame_guard {
    NozzleFrame *frame{};
    explicit nozzle_frame_guard(NozzleFrame *f) : frame(f) {}
    ~nozzle_frame_guard() { if(frame) nozzle_frame_release(frame); }
    nozzle_frame_guard(const nozzle_frame_guard &) = delete;
    nozzle_frame_guard &operator=(const nozzle_frame_guard &) = delete;
};

} // namespace

class GEM_EXTERN pix_nozzle_send : public GemPixObj {
    CPPEXTERN_HEADER(pix_nozzle_send, GemPixObj);

public:
    pix_nozzle_send(int argc, t_atom *argv);
    virtual ~pix_nozzle_send();

    virtual void render(GemState *state);
    virtual void postrender(GemState *state);

protected:
    void nameMess(t_symbol *name);

private:
    NozzleSender *m_sender;
    t_symbol *m_sender_name;
    NozzleFrame *m_frame;
    bool m_frame_acquired;
    uint64_t m_frame_count;
};

CPPEXTERN_NEW_WITH_GIMME(pix_nozzle_send);

pix_nozzle_send :: pix_nozzle_send(int argc, t_atom *argv)
    : m_sender(nullptr)
    , m_sender_name(gensym("nozzle_sender"))
    , m_frame(nullptr)
    , m_frame_acquired(false)
    , m_frame_count(0)
{
    if(argc > 0 && atom_getsymbol(argv) != &s_) {
        nameMess(atom_getsymbol(argv));
    }
}

pix_nozzle_send :: ~pix_nozzle_send() {
    if(m_sender) {
        nozzle_sender_destroy(m_sender);
        m_sender = nullptr;
    }
}

void pix_nozzle_send :: nameMess(t_symbol *name) {
    if(!name || name == &s_) return;

    m_sender_name = name;

    if(m_sender) {
        nozzle_sender_destroy(m_sender);
        m_sender = nullptr;
    }
}

void pix_nozzle_send :: render(GemState *state) {
    pixBlock *pb = nullptr;
    state->get(GemState::_PIX, pb);
    if(!pb || !pb->image.data) return;

    imageStruct &img = pb->image;
    if(img.xsize <= 0 || img.ysize <= 0) return;

    if(!m_sender) {
        NozzleSenderDesc desc{};
        desc.name = m_sender_name->s_name;
        desc.application_name = "pix_nozzle_send";
        desc.ring_buffer_size = nozzle_pd::k_default_ring_buffer_size;
    desc.fallback_flags_valid = 1;
    desc.fallback_flags = NOZZLE_FALLBACK_SAFE_DEFAULTS;

        NozzleErrorCode err = nozzle_sender_create(&desc, &m_sender);
        if(err != NOZZLE_OK) {
            error("pix_nozzle_send: failed to create sender '%s' (error %d)",
                  m_sender_name->s_name, (int)err);
            return;
        }
    }

    NozzleTextureFormat fmt = nozzle_pd::gem_to_nozzle_format(img.csize, img.type);

    NozzleFrame *frame = nullptr;
    NozzleErrorCode err = nozzle_sender_acquire_writable_frame(
        m_sender, (uint32_t)img.xsize, (uint32_t)img.ysize, fmt, &frame);
    if(err != NOZZLE_OK || !frame) {
        return;
    }
    nozzle_frame_guard frame_guard{frame};

    NozzleMappedPixels mapped{};
    err = nozzle_frame_lock_writable_pixels_with_origin(
        frame, NOZZLE_ORIGIN_TOP_LEFT, &mapped);
    if(err != NOZZLE_OK) {
        (void)nozzle_sender_discard_frame(m_sender, frame);
        return;
    }

    nozzle_pd::copy_image_to_mapped(img, mapped);

    err = nozzle_frame_unlock_writable_pixels_checked(frame);
    if(err != NOZZLE_OK) {
        (void)nozzle_sender_discard_frame(m_sender, frame);
        return;
    }

    err = nozzle_sender_commit_frame(m_sender, frame);
    if(err == NOZZLE_OK) {
        m_frame_count++;
    }
}

void pix_nozzle_send :: postrender(GemState *state) {
}

void pix_nozzle_send :: obj_setupCallback(t_class *classPtr) {
    CPPEXTERN_MSG1(classPtr, "name", nameMess, t_symbol *);
}
