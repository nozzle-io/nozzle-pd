# CPU pixel peer

The macOS CI step runs `make GEM_DIR=deps/gem-headers PD_DIR=deps/pd-headers test-pixel-peer`
after building the externals. It requires a working Metal backend and fails,
rather than skips, if the expected BGRA8 mapping is unavailable.

`pixel_peer.cpp` calls the same copy helpers used by `pix_nozzle_send` and
`pix_nozzle_receive`. A real native core peer supplies or checks mapped bytes
without invoking the opposite Pd helper. This is a same-process native-peer
test, not a same-binding round trip and not a running Pd/Gem host test.

The real Metal cases cover byte RGB/RGBA/BGR/BGRA input, both GL packed 8-bit
word types with RGBA/BGRA component order, source flipping, alpha and independent
native-to-Pd receive. An asymmetric 65x3 image checks padded mapped rows. Two
additional CPU-layout cases exercise negative mapped strides, RGB expansion,
RGBA8/BGRA8-sRGB byte preservation, padding and short-row rejection. They do not
claim a native sRGB or negative-stride backend mapping was exercised.

The repaired color path interprets both Gem `format` and `type`, and emits
explicit `GL_RGBA` / `GL_UNSIGNED_BYTE` CPU output. Four-channel permutations
use core's swizzle API. Unsupported 8-bit source layouts fail without committing
a frame; copy failures are not published as new receive images.

Known boundaries:

- Real Pd/Gem loading, scheduling and display remain unverified by this harness.
- Non-byte component counts and row layouts remain a separate repair:
  https://github.com/nozzle-io/nozzle-dev/issues/219
- The GL externals do not enter this CPU helper. GL output delivery and format
  handling are tracked separately:
  https://github.com/nozzle-io/nozzle-dev/issues/220

The original six failing cases and exact CI evidence are recorded at
https://github.com/nozzle-io/nozzle-dev/issues/218#issuecomment-6675499905 .
