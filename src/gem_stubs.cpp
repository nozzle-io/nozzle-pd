// gem_stubs.cpp - Minimal GEM symbol implementations for Windows PE linking.
//
// On Windows, PE format requires all symbols resolved at link time.
// The pre-built Gem.dll exports most symbols but NOT all the ones our
// externals need (e.g. gem::CPPExtern_proxy, GemBase internals).
// This file, together with a few GEM source files compiled directly,
// provides every symbol the nozzle-pd externals reference.
//
// GEM source files compiled directly (clean deps, no OpenGL/windowing):
//   src/Gem/Exception.cpp
//   src/RTE/Atom.cpp
//   src/Base/CPPExtern.cpp
//
// Everything else is stubbed here with minimal but correct implementations.

#define GEM_INTERNAL

#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <m_pd.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <map>
#include <string>
#include <new>
#include <algorithm>

#include "Base/CPPExtern.h"
#include "Base/GemBase.h"
#include "Base/GemPixObj.h"
#include "Gem/State.h"
#include "Gem/Image.h"
#include "Gem/Cache.h"
#include "Gem/Exception.h"
#include "Gem/Version.h"
#include "Gem/ExportDef.h"
#include "Gem/ContextData.h"
#include "Utils/SIMD.h"
#include "RTE/RTE.h"

// ============================================================================
// gem_register_class_setup  (from Gem/Setup.cpp - heavy deps, stubbed)
// ============================================================================

typedef void (*t_class_setup)(void);

struct class_setup_node {
    class_setup_node *next;
    const char *name;
    t_class_setup setup;
};

static class_setup_node *s_class_setup_list = nullptr;

extern "C" {

void gem_register_class_setup(const char *name, t_class_setup setup)
{
    auto *node = new class_setup_node;
    node->next = s_class_setup_list;
    node->setup = setup;
    node->name = name;
    s_class_setup_list = node;
}

} // extern "C"

// ============================================================================
// gem::RTE::RTE stubs  (CPPExtern.cpp calls getRuntimeEnvironment)
// ============================================================================

namespace gem {
namespace RTE {

RTE *RTE::getRuntimeEnvironment()
{
    return nullptr;
}

} // namespace RTE
} // namespace gem

// ============================================================================
// GemVersion stubs  (CPPExtern.cpp calls checkGemVersion)
// ============================================================================

namespace gem {

const char *Version::versionString()
{
    return "0.94";
}

bool Version::versionCheck(int major, int minor)
{
    (void)major;
    (void)minor;
    return true;
}

} // namespace gem

// ============================================================================
// GemSIMD stubs  (GemPixObj.cpp calls getCPU/requestCPU)
// ============================================================================

int GemSIMD::realcpuid = GEM_SIMD_NONE;
int GemSIMD::cpuid = GEM_SIMD_NONE;

GemSIMD::GemSIMD() {}

GemSIMD::~GemSIMD() {}

int GemSIMD::getCPU()
{
    return cpuid;
}

int GemSIMD::requestCPU(int n)
{
    if (n <= realcpuid) {
        cpuid = n;
    }
    return cpuid;
}

int GemSIMD::simd_runtime_check()
{
    return realcpuid;
}

// ============================================================================
// gem::ContextDataBase stubs  (GemBase PIMPL uses ContextData<T>)
// ============================================================================

namespace gem {

const unsigned int ContextDataBase::INVALID_CONTEXT = 0xFFFFFFFF;

unsigned int ContextDataBase::getCurContext() const
{
    return 0;
}

ContextDataBase::~ContextDataBase() {}

} // namespace gem

// ============================================================================
// imageStruct  (from Gem/Image.cpp - 1639 lines, too heavy, stubbed)
// ============================================================================

imageStruct::imageStruct()
    : data(nullptr)
    , pdata(nullptr)
    , datasize(0)
    , xsize(0)
    , ysize(0)
    , csize(4)
    , type(GL_UNSIGNED_BYTE)
    , format(GL_RGBA)
    , not_owned(0)
    , upsidedown(false)
{}

imageStruct::imageStruct(const imageStruct &org)
    : data(nullptr)
    , pdata(nullptr)
    , datasize(0)
    , xsize(org.xsize)
    , ysize(org.ysize)
    , csize(org.csize)
    , type(org.type)
    , format(org.format)
    , not_owned(org.not_owned)
    , upsidedown(org.upsidedown)
{
    if (org.data && !org.not_owned && org.datasize > 0) {
        pdata = new unsigned char[org.datasize];
        data = pdata;
        datasize = org.datasize;
        memcpy(data, org.data, datasize);
    } else {
        data = org.data;
    }
}

imageStruct::~imageStruct()
{
    clear();
}

imageStruct &imageStruct::operator=(const imageStruct &org)
{
    if (&org == this) {
        return *this;
    }
    clear();
    xsize = org.xsize;
    ysize = org.ysize;
    csize = org.csize;
    type = org.type;
    format = org.format;
    not_owned = org.not_owned;
    upsidedown = org.upsidedown;
    if (org.data && !org.not_owned && org.datasize > 0) {
        pdata = new unsigned char[org.datasize];
        data = pdata;
        datasize = org.datasize;
        memcpy(data, org.data, datasize);
    } else {
        data = org.data;
    }
    return *this;
}

unsigned char *imageStruct::allocate(size_t size)
{
    clear();
    pdata = new unsigned char[size];
    data = pdata;
    datasize = size;
    not_owned = 0;
    return data;
}

unsigned char *imageStruct::allocate()
{
    return allocate(static_cast<size_t>(xsize) * static_cast<size_t>(ysize)
                   * static_cast<size_t>(csize));
}

unsigned char *imageStruct::reallocate(size_t size)
{
    if (size <= datasize) {
        return data;
    }
    return allocate(size);
}

unsigned char *imageStruct::reallocate()
{
    return reallocate(static_cast<size_t>(xsize) * static_cast<size_t>(ysize)
                      * static_cast<size_t>(csize));
}

void imageStruct::clear()
{
    if (pdata) {
        delete[] pdata;
        pdata = nullptr;
    }
    data = nullptr;
    datasize = 0;
}

int imageStruct::setFormat(int fmt)
{
    format = fmt;
    switch (fmt) {
    case GL_LUMINANCE:  csize = 1; break;
    case GL_YUV422_GEM: csize = 2; break;
    case GL_RGB:
    case GL_BGR_EXT:    csize = 3; break;
    case GL_RGBA:
    case GL_BGRA_EXT:   csize = 4; break;
    default:            csize = 4; break;
    }
    return csize;
}

int imageStruct::setFormat()
{
    return setFormat(format);
}

void imageStruct::copy2ImageStruct(imageStruct *to) const
{
    if (!to) {
        return;
    }
    to->xsize = xsize;
    to->ysize = ysize;
    to->csize = csize;
    to->type = type;
    to->format = format;
    to->upsidedown = upsidedown;
    to->not_owned = 1;
    to->data = data;
}

// Virtual method stubs (needed for vtable completeness)

void imageStruct::info()
{}

bool imageStruct::getRGB(int, int, unsigned char *, unsigned char *,
                         unsigned char *, unsigned char *) const
{
    return false;
}

bool imageStruct::getGrey(int, int, unsigned char *) const
{
    return false;
}

bool imageStruct::getYUV(int, int, unsigned char *, unsigned char *,
                         unsigned char *) const
{
    return false;
}

void imageStruct::setBlack()
{
    if (data && datasize > 0) {
        memset(data, 0, datasize);
    }
}

void imageStruct::setWhite()
{
    if (data && datasize > 0) {
        memset(data, 255, datasize);
    }
}

void imageStruct::copy2Image(imageStruct *) const
{}

void imageStruct::refreshImage(imageStruct *) const
{}

bool imageStruct::convertTo(imageStruct *, unsigned int) const
{
    return false;
}

bool imageStruct::convertFrom(const imageStruct *, unsigned int)
{
    return false;
}

bool imageStruct::fromRGB(const unsigned char *) { return false; }
bool imageStruct::fromRGBA(const unsigned char *) { return false; }
bool imageStruct::fromBGR(const unsigned char *) { return false; }
bool imageStruct::fromBGRA(const unsigned char *) { return false; }
bool imageStruct::fromRGB16(const unsigned char *) { return false; }
bool imageStruct::fromABGR(const unsigned char *) { return false; }
bool imageStruct::fromARGB(const unsigned char *) { return false; }
bool imageStruct::fromGray(const unsigned char *) { return false; }
bool imageStruct::fromGray(const short *) { return false; }
bool imageStruct::fromUYVY(const unsigned char *) { return false; }
bool imageStruct::fromYUY2(const unsigned char *) { return false; }
bool imageStruct::fromYVYU(const unsigned char *) { return false; }
bool imageStruct::fromYV12(const unsigned char *, const unsigned char *,
                           const unsigned char *) { return false; }
bool imageStruct::fromYV12(const unsigned char *) { return false; }
bool imageStruct::fromYU12(const unsigned char *) { return false; }
bool imageStruct::fromYV12(const short *, const short *,
                           const short *) { return false; }
bool imageStruct::fromYV12(const short *) { return false; }

void imageStruct::fixUpDown()
{}

// ============================================================================
// pixBlock  (from Gem/Image.cpp)
// ============================================================================

pixBlock::pixBlock()
    : newimage(0)
    , newfilm(0)
{}

// ============================================================================
// GemBase  (from Base/GemBase.cpp - depends on GemWindow, stubbed)
// ============================================================================

struct GemBase::PIMPL {
    GemBase *parent;
    gem::ContextData<bool> enabled;
    gem::ContextData<GemBase::RenderState> state;
    bool debugGL;

    PIMPL(GemBase *_parent)
        : parent(_parent)
        , enabled(true)
        , state(GemBase::INIT)
        , debugGL(false)
    {}

    PIMPL(PIMPL *p)
        : parent(p->parent)
        , enabled(p->enabled)
        , state(p->state)
        , debugGL(p->debugGL)
    {}
};

GemBase::GemBase()
    : gem_amRendering(false)
    , m_cache(nullptr)
    , m_modified(true)
    , m_out1(nullptr)
    , m_pimpl(new PIMPL(this))
{
    m_out1 = outlet_new(this->x_obj, 0);
    pd_bind(&this->x_obj->ob_pd, gensym("__gemBase"));
}

GemBase::~GemBase()
{
    if (gem_amRendering) {
        stopRendering();
        gem_amRendering = false;
    }
    if (m_out1) {
        outlet_free(m_out1);
    }
    pd_unbind(&this->x_obj->ob_pd, gensym("__gemBase"));
}

void GemBase::gem_startstopMess(int state)
{
    if (state && !gem_amRendering) {
        m_pimpl->enabled = isRunnable();
        if (m_pimpl->enabled) {
            startRendering();
            m_pimpl->state = RENDERING;
        }
    } else if (!state && gem_amRendering) {
        if (m_pimpl->enabled) {
            stopRendering();
            m_pimpl->state = ENABLED;
        }
    }
    gem_amRendering = (state != 0);

    t_atom ap[1];
    SETFLOAT(ap, state);
    outlet_anything(this->m_out1, gensym("gem_state"), 1, ap);
}

void GemBase::gem_renderMess(GemCache *cache, GemState *state)
{
    m_cache = cache;
    if (m_cache && m_cache->m_magic != GEMCACHE_MAGIC) {
        m_cache = nullptr;
    }
    if (m_pimpl->state == INIT) {
        if (isRunnable()) {
            m_pimpl->state = ENABLED;
        } else {
            m_pimpl->state = DISABLED;
        }
    }
    if (m_pimpl->state == MODIFIED) {
        stopRendering();
        m_pimpl->state = ENABLED;
    }
    if (m_pimpl->state == ENABLED) {
        startRendering();
        m_pimpl->state = RENDERING;
    }
    if (m_pimpl->state == RENDERING) {
        gem_amRendering = true;
        if (state) {
            render(state);
        }
        continueRender(state);
        if (state) {
            postrender(state);
        }
    }
    m_modified = false;
}

void GemBase::continueRender(GemState *state)
{
    t_atom ap[2];
    ap[0].a_type = A_POINTER;
    ap[0].a_w.w_gpointer = reinterpret_cast<t_gpointer *>(m_cache);
    ap[1].a_type = A_POINTER;
    ap[1].a_w.w_gpointer = reinterpret_cast<t_gpointer *>(state);
    outlet_anything(this->m_out1, gensym("gem_state"), 2, ap);
}

void GemBase::setModified()
{
    if (m_cache && m_cache->m_magic != GEMCACHE_MAGIC) {
        m_cache = nullptr;
    }
    if (m_cache) {
        m_cache->dirty = true;
    }
    m_modified = true;
    switch (m_pimpl->state) {
    case DISABLED:
    case INIT:
        break;
    default:
        m_pimpl->state = MODIFIED;
    }
}

void GemBase::realStopRendering()
{
    stopRendering();
    m_cache = nullptr;
    m_pimpl->state = ENABLED;
}

bool GemBase::isRunnable()
{
    return true;
}

GemBase::RenderState GemBase::getState()
{
    return m_pimpl->state;
}

void GemBase::beforeDeletion()
{
    // Simplified: skip GemWindow::stopInAllContexts() (not available without
    // full GEM windowing system). Our externals don't hold GL resources that
    // need explicit cleanup.
    CPPExtern::beforeDeletion();
}

void GemBase::obj_setupCallback(t_class *classPtr)
{
    // gem_state callback - handles rendering pipeline messages
    struct CB_gem_state {
        static void callback(void *data, t_symbol *, int argc, t_atom *argv)
        {
            auto *obj = static_cast<GemBase *>(
                static_cast<Obj_header *>(data)->data);
            if (argc == 2 && argv->a_type == A_POINTER
                && (argv + 1)->a_type == A_POINTER) {
                obj->gem_renderMess(
                    reinterpret_cast<GemCache *>(argv->a_w.w_gpointer),
                    reinterpret_cast<GemState *>(
                        (argv + 1)->a_w.w_gpointer));
            } else if (argc == 1 && argv->a_type == A_FLOAT) {
                obj->gem_startstopMess(atom_getint(argv));
            }
        }
        explicit CB_gem_state(t_class *c)
        {
            class_addmethod(c, reinterpret_cast<t_method>(callback),
                            gensym("gem_state"), A_GIMME, A_NULL);
        }
    };
    CB_gem_state cb_gs(classPtr);

    // __gem_context callback - handles context destruction
    struct CB_gem_context {
        static void callback(void *data, t_float v0)
        {
            auto *obj = static_cast<GemBase *>(
                static_cast<Obj_header *>(data)->data);
            bool state = static_cast<bool>(v0);
            if (!state && obj->gem_amRendering) {
                if (obj->m_pimpl->enabled) {
                    obj->stopRendering();
                    obj->m_pimpl->state = obj->ENABLED;
                }
            }
            obj->gem_amRendering = !state;
        }
        explicit CB_gem_context(t_class *c)
        {
            class_addmethod(c, reinterpret_cast<t_method>(callback),
                            gensym("__gem_context"), A_FLOAT, A_NULL);
        }
    };
    CB_gem_context cb_gc(classPtr);
}

// ============================================================================
// GemPixObj  (from Base/GemPixObj.cpp - depends on Utils/Functions.h, stubbed)
// ============================================================================

GemPixObj::GemPixObj()
    : cachedPixBlock(pixBlock())
    , orgPixBlock(nullptr)
    , m_processOnOff(1)
    , m_simd(GemSIMD::getCPU())
    , m_doROI(false)
{
    cachedPixBlock.newimage = 0;
    cachedPixBlock.newfilm = 0;
}

void GemPixObj::setPixModified()
{
    if (m_cache && m_cache->m_magic != GEMCACHE_MAGIC) {
        m_cache = nullptr;
    }
    if (m_cache) {
        m_cache->resendImage = 1;
    }
}

void GemPixObj::render(GemState *state)
{
    pixBlock *image = nullptr;
    if (!state || !state->get(GemState::_PIX, image)) {
        return;
    }
    if (!image || !&image->image) {
        return;
    }
    cachedPixBlock.newimage = image->newimage;
    if (!image->newimage) {
        image = &cachedPixBlock;
    } else {
        orgPixBlock = image;
        cachedPixBlock.newimage = image->newimage;
        cachedPixBlock.newfilm = image->newfilm;
        image->image.copy2ImageStruct(&cachedPixBlock.image);
        image = &cachedPixBlock;
        if (m_processOnOff) {
            switch (image->image.type) {
            case GL_FLOAT:
                processFloat32(image->image);
                break;
            case GL_DOUBLE:
                processFloat64(image->image);
                break;
            default:
                switch (image->image.format) {
                case GL_RGBA:
                case GL_BGRA_EXT:
                    processRGBAImage(image->image);
                    break;
                case GL_RGB:
                case GL_BGR_EXT:
                    processRGBImage(image->image);
                    break;
                case GL_LUMINANCE:
                    processGrayImage(image->image);
                    break;
                case GL_YUV422_GEM:
                    processYUVImage(image->image);
                    break;
                default:
                    processImage(image->image);
                }
            }
        }
    }
    state->set(GemState::_PIX, image);
}

void GemPixObj::postrender(GemState *state)
{
    state->set(GemState::_PIX, orgPixBlock);
}

void GemPixObj::processImage(imageStruct &image)
{
    switch (image.format) {
    case GL_RGBA:
    case GL_BGRA_EXT:
        error("cannot handle RGBA image");
        break;
    case GL_RGB:
    case GL_BGR_EXT:
        error("cannot handle RGB image");
        break;
    case GL_LUMINANCE:
        error("cannot handle Grey image");
        break;
    case GL_YUV422_GEM:
        error("cannot handle YUV image");
        break;
    default:
        error("cannot handle this format (%x) !", image.format);
    }
}

void GemPixObj::processFloat32(imageStruct &image)
{
    switch (image.format) {
    case GL_RGBA:
    case GL_BGRA_EXT:
        error("cannot handle RGBA/float image");
        break;
    default:
        error("cannot handle this format (0x%X/float) !", image.format);
    }
}

void GemPixObj::processFloat64(imageStruct &image)
{
    switch (image.format) {
    case GL_RGBA:
    case GL_BGRA_EXT:
        error("cannot handle RGBA/double image");
        break;
    default:
        error("cannot handle this format (0x%X/double) !", image.format);
    }
}

void GemPixObj::processRGBAImage(imageStruct &image) { processImage(image); }
void GemPixObj::processRGBImage(imageStruct &image) { processImage(image); }
void GemPixObj::processGrayImage(imageStruct &image) { processImage(image); }
void GemPixObj::processYUVImage(imageStruct &image) { processImage(image); }

void GemPixObj::processRGBAMMX(imageStruct &image) { processRGBAImage(image); }
void GemPixObj::processGrayMMX(imageStruct &image) { processGrayImage(image); }
void GemPixObj::processYUVMMX(imageStruct &image) { processYUVImage(image); }
void GemPixObj::processRGBASSE2(imageStruct &image) { processRGBAMMX(image); }
void GemPixObj::processGraySSE2(imageStruct &image) { processGrayMMX(image); }
void GemPixObj::processYUVSSE2(imageStruct &image) { processYUVMMX(image); }
void GemPixObj::processRGBAAltivec(imageStruct &image) { processRGBAImage(image); }
void GemPixObj::processGrayAltivec(imageStruct &image) { processGrayImage(image); }
void GemPixObj::processYUVAltivec(imageStruct &image) { processYUVImage(image); }

void GemPixObj::processOnOff(int on)
{
    m_processOnOff = on;
    setPixModified();
}

void GemPixObj::SIMD(int n)
{
    m_simd = GemSIMD::requestCPU(n);
}

void GemPixObj::obj_setupCallback(t_class *classPtr)
{
    CPPEXTERN_MSG1(classPtr, "float", processOnOff, int);
    CPPEXTERN_MSG1(classPtr, "simd", SIMD, int);
}
