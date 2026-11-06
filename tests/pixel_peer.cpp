#include "Gem/GemGL.h"
#include "nozzle_pd_common.h"
#include "nozzle_pd_pixels.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

namespace {
constexpr int width = 65;
constexpr int height = 3;

// Only the fields consumed by the exact production helper; no mock core API.
struct image_view {
    int xsize = width;
    int ysize = height;
    int csize = 4;
    unsigned int format = GL_RGBA;
    unsigned int type = GL_UNSIGNED_BYTE;
    bool upsidedown = false;
    unsigned char *data = nullptr;
};

struct exchange {
    NozzleSender *sender = nullptr;
    NozzleReceiver *receiver = nullptr;
    NozzleFrame *write = nullptr;
    NozzleFrame *read = nullptr;
    bool committed = false;
    ~exchange() {
        if(read) {
            nozzle_frame_unlock_pixels(read);
            nozzle_frame_release(read);
        }
        if(write) {
            nozzle_frame_unlock_writable_pixels(write);
            if(!committed) (void)nozzle_sender_discard_frame(sender, write);
            nozzle_frame_release(write);
        }
        if(receiver) nozzle_receiver_destroy(receiver);
        if(sender) nozzle_sender_destroy(sender);
    }
};

bool ok(NozzleErrorCode code, const char *operation) {
    if(code == NOZZLE_OK) return true;
    std::fprintf(stderr, "%s error=%d\n", operation, (int)code);
    return false;
}

bool run_case(const char *label, bool pd_sends, int channels,
              unsigned int format, unsigned int type, bool flip) {
    exchange peer;
    std::string name = "pd218-" + std::to_string(getpid()) + "-" + label;
    NozzleSenderDesc desc{};
    desc.name = name.c_str();
    desc.application_name = "pd pixel peer";
    desc.ring_buffer_size = 3;
    desc.fallback_flags_valid = 1;
    desc.fallback_flags = NOZZLE_FALLBACK_SAFE_DEFAULTS;
    if(!ok(nozzle_sender_create(&desc, &peer.sender), "create sender")) return false;
    NozzleSenderInfoSnapshot sender_info{};
    if(!ok(nozzle_sender_get_info_snapshot(peer.sender, &sender_info), "sender info") ||
       sender_info.backend != NOZZLE_BACKEND_METAL) {
        std::fprintf(stderr, "native Metal backend is required, no SKIP\n");
        return false;
    }
    if(!ok(nozzle_sender_acquire_writable_frame(peer.sender, width, height,
        NOZZLE_FORMAT_RGBA8_UNORM, &peer.write), "acquire writable")) return false;
    NozzleMappedPixels mapped{};
    if(!ok(nozzle_frame_lock_writable_pixels_with_origin(peer.write,
        NOZZLE_ORIGIN_TOP_LEFT, &mapped), "map writable")) return false;
    if(mapped.format != NOZZLE_FORMAT_BGRA8_UNORM) {
        std::fprintf(stderr, "expected actual Metal BGRA8 storage, got %d\n", mapped.format);
        return false;
    }
    std::printf("%s storage=BGRA8 row_stride=%lld\n", label, (long long)mapped.row_stride_bytes);
    if(pd_sends) {
        std::vector<unsigned char> source(width * height * channels);
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                auto *pixel = source.data() + ((flip ? height - 1 - y : y) * width + x) * channels;
                const unsigned char r = (unsigned char)(190 + y);
                const unsigned char g = (unsigned char)(20 + x);
                const unsigned char b = (unsigned char)(40 + y);
                if(type == GL_UNSIGNED_INT_8_8_8_8 || type == GL_UNSIGNED_INT_8_8_8_8_REV) {
                    // Construct the specified GL numeric word, not the helper's byte permutation.
                    const uint32_t first = format == GL_BGRA ? b : r;
                    const uint32_t third = format == GL_BGRA ? r : b;
                    const uint32_t word = type == GL_UNSIGNED_INT_8_8_8_8
                        ? (first << 24) | (uint32_t(g) << 16) | (third << 8) | 149u
                        : first | (uint32_t(g) << 8) | (third << 16) | (149u << 24);
                    std::memcpy(pixel, &word, sizeof(word));
                } else {
                    const bool bgr = format == GL_BGR || format == GL_BGRA;
                    pixel[0] = bgr ? b : r;
                    pixel[1] = g;
                    pixel[2] = bgr ? r : b;
                    if(channels == 4) pixel[3] = 149;
                }
            }
        }
        image_view input{width, height, channels, format, type, flip, source.data()};
        if(!nozzle_pd::copy_image_to_mapped(input, mapped)) return false;
    } else {
        // Independent native producer: write BGRA bytes directly, never Pd's helper.
        for(int y = 0; y < height; ++y) {
            auto *row = static_cast<unsigned char *>(mapped.data) + y * mapped.row_stride_bytes;
            for(int x = 0; x < width; ++x) {
                row[4*x] = (unsigned char)(40+y);
                row[4*x+1] = (unsigned char)(20+x);
                row[4*x+2] = (unsigned char)(190+y);
                row[4*x+3] = 149;
            }
        }
    }
    if(!ok(nozzle_frame_unlock_writable_pixels_checked(peer.write), "unlock writable") ||
       !ok(nozzle_sender_commit_frame(peer.sender, peer.write), "commit")) return false;
    peer.committed = true;
    NozzleReceiverDesc receiver_desc{};
    receiver_desc.name = name.c_str();
    receiver_desc.receive_mode = NOZZLE_RECEIVE_LATEST_ONLY;
    if(!ok(nozzle_receiver_create(&receiver_desc, &peer.receiver), "create receiver")) return false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    NozzleAcquireDesc acq{};
    acq.timeout_ms = 20;
    while(std::chrono::steady_clock::now() < deadline) {
        if(nozzle_receiver_acquire_frame(peer.receiver, &acq, &peer.read) == NOZZLE_OK) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if(!peer.read) {
        std::fprintf(stderr, "native peer timed out\n");
        return false;
    }
    if(!ok(nozzle_frame_lock_pixels_with_origin(peer.read, NOZZLE_ORIGIN_TOP_LEFT,
        &mapped), "map read")) return false;
    if(mapped.width != width || mapped.height != height || mapped.format != NOZZLE_FORMAT_BGRA8_UNORM) return false;

    std::vector<unsigned char> output(width * height * 4, 0xcd);
    if(!pd_sends) {
        NozzleFrameInfo info{};
        if(!ok(nozzle_frame_get_info(peer.read, &info), "frame info")) return false;
        image_view image{width, height, nozzle_pd::nozzle_to_gem_csize(mapped.format),
            (unsigned)nozzle_pd::nozzle_to_gem_format(mapped.format),
            (unsigned)nozzle_pd::nozzle_to_gem_type(mapped.format), false, output.data()};
        if(image.csize != 4 || image.type != GL_UNSIGNED_BYTE || image.format != GL_RGBA) {
            std::fprintf(stderr, "unexpected GEM output layout\n");
            return false;
        }
        if(!nozzle_pd::copy_mapped_to_image(mapped, image)) return false;
    }
    // The native receiver oracle never calls Pd's receive conversion, and vice versa.
    for(int y = 0; y < height; ++y) {
        const auto *row = pd_sends ? static_cast<const unsigned char *>(mapped.data) + y * mapped.row_stride_bytes
                                   : output.data() + y * width * 4;
        for(int x = 0; x < width; ++x) {
            const unsigned char expected[4] = {
                (unsigned char)((pd_sends ? 40 : 190) + y), (unsigned char)(20+x),
                (unsigned char)((pd_sends ? 190 : 40) + y), (unsigned char)(channels == 3 ? 255 : 149)};
            for(int c = 0; c < 4; ++c) {
                if(row[4*x+c] != expected[c]) {
                    std::fprintf(stderr, "%s mismatch x=%d y=%d channel=%d actual=%u expected=%u\n",
                        label, x, y, c, row[4*x+c], expected[c]);
                    return false;
                }
            }
        }
    }
    return true;
}

// Supplement the real Metal peer: it normally gives positive BGRA row strides.
bool check_signed_rows(NozzleTextureFormat format) {
    unsigned char input[] = {191,23,41, 192,24,42, 193,25,43, 194,26,44};
    unsigned char storage[32];
    std::memset(storage, 0xcd, sizeof(storage));
    NozzleMappedPixels mapped{};
    mapped.data = storage + 16;
    mapped.row_stride_bytes = -16;
    mapped.width = 2;
    mapped.height = 2;
    mapped.format = format;
    mapped.origin = NOZZLE_ORIGIN_TOP_LEFT;
    image_view source{2, 2, 3, GL_RGB, GL_UNSIGNED_BYTE, true, input};
    if(!nozzle_pd::copy_image_to_mapped(source, mapped)) return false;
    const bool bgra = format == NOZZLE_FORMAT_BGRA8_SRGB;
    for(int y = 0; y < 2; ++y) {
        const auto *row = storage + 16 - y * 16;
        for(int x = 0; x < 2; ++x) {
            const auto *rgb = input + ((1-y)*2+x)*3;
            if(row[4*x] != rgb[bgra ? 2 : 0] || row[4*x+1] != rgb[1] ||
               row[4*x+2] != rgb[bgra ? 0 : 2] || row[4*x+3] != 255) return false;
        }
        for(int i = 8; i < 16; ++i) if(row[i] != 0xcd) return false;
    }
    // Exercise the four-channel core-swizzle branch with a signed destination too.
    unsigned char input4[16];
    for(int x = 0; x < 4; ++x) {
        std::memcpy(input4 + 4*x, input + 3*x, 3);
        input4[4*x+3] = 149;
    }
    image_view source4{2, 2, 4, GL_RGBA, GL_UNSIGNED_BYTE, true, input4};
    std::memset(storage, 0xcd, sizeof(storage));
    if(!nozzle_pd::copy_image_to_mapped(source4, mapped)) return false;
    for(int y = 0; y < 2; ++y) {
        const auto *row = storage + 16 - y * 16;
        for(int x = 0; x < 2; ++x) {
            const auto *rgb = input + ((1-y)*2+x)*3;
            if(row[4*x] != rgb[bgra ? 2 : 0] || row[4*x+1] != rgb[1] ||
               row[4*x+2] != rgb[bgra ? 0 : 2] || row[4*x+3] != 149) return false;
        }
        for(int i = 8; i < 16; ++i) if(row[i] != 0xcd) return false;
    }
    // Independent input for receive: don't feed it the send helper's result.
    std::memset(storage, 0xcd, sizeof(storage));
    for(int y = 0; y < 2; ++y) {
        auto *row = storage + 16 - y * 16;
        for(int x = 0; x < 2; ++x) {
            row[4*x] = (bgra ? 41 : 191) + y;
            row[4*x+1] = 23 + x;
            row[4*x+2] = (bgra ? 191 : 41) + y;
            row[4*x+3] = 149 + x;
        }
    }
    unsigned char output[16]{};
    image_view target{2, 2, 4, GL_RGBA, GL_UNSIGNED_BYTE, false, output};
    if(!nozzle_pd::copy_mapped_to_image(mapped, target)) return false;
    for(int x = 0; x < 4; ++x) {
        if(output[4*x] != 191 + x/2 || output[4*x+1] != 23 + x%2 ||
           output[4*x+2] != 41 + x/2 || output[4*x+3] != 149 + x%2) return false;
    }
    // Reject an undersized row before writing anything.
    std::memset(storage, 0xcd, sizeof(storage));
    mapped.row_stride_bytes = 7;
    if(nozzle_pd::copy_image_to_mapped(source, mapped) ||
       nozzle_pd::copy_mapped_to_image(mapped, target)) return false;
    for(auto byte : storage) if(byte != 0xcd) return false;
    return true;
}
} // namespace

int main() {
    struct test_case { const char *name; bool sends; int channels; unsigned format; unsigned type; bool flip; };
    const test_case cases[] = {
        {"rgba", true, 4, GL_RGBA, GL_UNSIGNED_BYTE, false},
        {"rgb", true, 3, GL_RGB, GL_UNSIGNED_BYTE, false},
        {"bgra", true, 4, GL_BGRA, GL_UNSIGNED_BYTE, false},
        {"bgr", true, 3, GL_BGR, GL_UNSIGNED_BYTE, false},
        {"gem-packed", true, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8, false},
        {"bgra-packed-rev", true, 4, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, false},
        {"rgba-packed", true, 4, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, false},
        {"rgba-packed-rev", true, 4, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, false},
        {"rgba-flipped", true, 4, GL_RGBA, GL_UNSIGNED_BYTE, true},
        {"native-to-pd", false, 4, GL_RGBA, GL_UNSIGNED_BYTE, false},
    };
    int failed = 0;
    for(const auto &test : cases) {
        const bool pass = run_case(test.name, test.sends, test.channels, test.format, test.type, test.flip);
        std::printf("%s %s\n", test.name, pass ? "PASS" : "FAIL");
        if(!pass) ++failed;
    }
    for(auto format : {NOZZLE_FORMAT_RGBA8_UNORM, NOZZLE_FORMAT_BGRA8_SRGB}) {
        const bool pass = check_signed_rows(format);
        std::printf("signed-rows format=%d %s\n", (int)format, pass ? "PASS" : "FAIL");
        if(!pass) ++failed;
    }
    std::printf("pixel_peer failed=%d total=12\n", failed);
    return failed == 0 ? 0 : 1;
}
