#include "Base/GemPixObj.h"

extern "C" {
#include <nozzle/nozzle_c.h>
}

#include "Gem/State.h"
#include "Gem/Cache.h"
#include "nozzle_pd_common.h"

#include <cstring>
#include <string>

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

    NozzleMappedPixels mapped{};
    err = nozzle_frame_lock_writable_pixels_with_origin(
        frame, NOZZLE_ORIGIN_TOP_LEFT, &mapped);
    if(err != NOZZLE_OK) {
        nozzle_frame_release(frame);
        return;
    }

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

    err = nozzle_frame_unlock_writable_pixels_checked(frame);
    if(err != NOZZLE_OK) {
        /* Commit rejects failed-unlock frames and releases the sender slot. */
        (void)nozzle_sender_commit_frame(m_sender, frame);
        nozzle_frame_release(frame);
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
