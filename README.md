# nozzle-pd

PureData/GEM externals for [nozzle](https://github.com/nozzle-io/nozzle) — cross-platform GPU texture sharing between local processes.

## Objects

| Object | Description |
|--------|-------------|
| `pix_nozzle_send` | Sends 8-bit color pix data (CPU copy path; layout limits below) |
| `pix_nozzle_receive` | Receives 8-bit color frames as RGBA pix data (layout limits below) |
| `pix_nozzle_gl_send` | Sends OpenGL texture directly via nozzle (GPU path) |
| `pix_nozzle_gl_receive` | Receives nozzle frame and copies to OpenGL texture |

Help patches are shipped for all four objects:

- `pix_nozzle_send-help.pd`
- `pix_nozzle_receive-help.pd`
- `pix_nozzle_gl_send-help.pd`
- `pix_nozzle_gl_receive-help.pd`

## Build

```bash
git submodule update --init --recursive
make
```

Externals are output to `.build/`.

## Install from release zip

Download the platform zip from the `latest` development snapshot release or a
versioned `vX.Y.Z` release:

- `nozzle-pd-latest-<short_sha>-macos.zip`
- `nozzle-pd-latest-<short_sha>-linux.zip`
- `nozzle-pd-latest-<short_sha>-windows.zip`

Each zip contains the four externals, the four help patches, and this README in
a single top-level folder. Add that folder to Pd's search path.

## Install from local build

Copy the built externals (`.pd_darwin` on macOS, `.pd_linux` on Linux,
`.dll` on Windows) and the help patches to your Pd externals directory:

```bash
mkdir -p ~/.pd-externals/nozzle-pd/
cp .build/*.pd_darwin help/*.pd README.md ~/.pd-externals/nozzle-pd/  # macOS
# cp .build/*.pd_linux help/*.pd README.md ~/.pd-externals/nozzle-pd/  # Linux
# cp .build/*.dll help/*.pd README.md ~/.pd-externals/nozzle-pd/       # Windows
```

Then add `~/.pd-externals/nozzle-pd` to Pd's search path.

## Usage

### CPU pixel layout limits

**Non-byte CPU layouts are unsupported.** Do not send or receive 16-bit integer,
half-float, float, 32-bit integer or depth images with `pix_nozzle_send` or
`pix_nozzle_receive`. Known component-count and row-layout defects are tracked in
[nozzle-dev#219](https://github.com/nozzle-io/nozzle-dev/issues/219).

The tested CPU color-copy scope is:

- Send: `GL_UNSIGNED_BYTE` RGB/BGR with three components or RGBA/BGRA with four
  components; also four-component RGBA/BGRA packed `GL_UNSIGNED_INT_8_8_8_8`
  and `GL_UNSIGNED_INT_8_8_8_8_REV` (8 bits per component, not 32-bit channels).
  Three-component byte input expands alpha to 255.
- Receive: mapped RGBA8/BGRA8 color data becomes `GL_RGBA` / `GL_UNSIGNED_BYTE`
  with four components. The copy helpers preserve sRGB bytes; they do not perform
  a color-space conversion or establish Gem color-management behavior.

Other layouts, including single/two-channel, signed and unknown representations,
are outside this validated scope. The current runtime does not consistently
reject every unsupported layout: non-byte receive mappings use byte counts as
component counts, generic copies omit the component byte width from row sizing,
and float RGB input is not expanded to RGBA. Unknown type mappings can fall back
to RGBA8. A successful call is therefore not evidence of correct pixels for these
formats. Repeated copy failures can also emit a Pd console error every frame;
bounded reporting on error/layout transitions remains part of #219.

This support notice changes no runtime behavior. The existing
[CPU/native-peer harness](tests/README.md) checks asymmetric pixels, padding,
channel order and flips on the byte color path. It is not a real Pd/Gem host test;
loading, scheduling and display remain unverified. Restoring non-byte support
requires checking Gem's allocation/layout contract at an exact source revision,
independent typed pixel tests (including RGB alpha expansion and same-size format
changes), rejection tests and bounded error/recovery behavior in the host.

### pix_nozzle_send

```
[gemhead]
|
[pix_video]
|
[pix_nozzle_send my_sender_name]
```

Messages:
- `name <symbol>` — set sender name (default: "nozzle_sender")

### pix_nozzle_receive

```
[gemhead]
|
[pix_nozzle_receive my_sender_name]
|
[pix_texture]
```

Messages:
- `name <symbol>` — set sender name to connect to (default: "nozzle_sender")

### pix_nozzle_gl_send

```
[gemhead]
|
[pix_video]
|
[pix_nozzle_gl_send my_sender_name]
```

Messages:
- `name <symbol>` — set sender name (default: "nozzle_sender")

### pix_nozzle_gl_receive

```
[gemhead]
|
[pix_nozzle_gl_receive my_sender_name]
|
[pix_texture]
```

Messages:
- `name <symbol>` — set sender name to connect to (default: "nozzle_sender")

## Requirements

- C++17 compiler
- PureData with GEM
- macOS 12+ / Linux / Windows
- The published Linux x64 build targets **x86-64-v2** (no AVX, AVX2, FMA or F16C). CI sets this explicitly and verifies the binaries with `deps/nozzle/scripts/check_x86_64_isa.sh`.

## License

MIT
