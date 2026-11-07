# nozzle-pd

PureData/GEM externals for [nozzle](https://github.com/nozzle-io/nozzle) — cross-platform GPU texture sharing between local processes.

## Objects

| Object | Description |
|--------|-------------|
| `pix_nozzle_send` | Sends GEM pix image data via nozzle (CPU copy path) |
| `pix_nozzle_receive` | Receives nozzle frames and injects into GEM render chain |
| `pix_nozzle_gl_send` | **Unsupported**: known GL format-mapping and state-handling defects; see below |
| `pix_nozzle_gl_receive` | **Unsupported**: received texture is not delivered to Gem; see below |

Help patches are shipped for all four objects:

The GL help patches are legacy examples, not working or validated usage recipes.

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

### GL objects — unsupported

Do not use `pix_nozzle_gl_send` or `pix_nozzle_gl_receive` for frame delivery.
They remain in development packages, but have known correctness defects tracked
in [nozzle-dev#220](https://github.com/nozzle-io/nozzle-dev/issues/220):

- The receiver copies into a private GL texture, then publishes a newly allocated
  Gem pix block without populating its pixels or exposing the texture to Gem.
  Connecting its output to `pix_texture` does not make that block contain the
  received frame.
- Receive texture allocation tracks dimensions only, so a same-size format
  change can retain the wrong representation. GL binding/state restoration and
  texture lifetime across context teardown also need a defined host contract.
- The sender conflates `GL_RG16` with single-channel `R16`, treats some signed
  normalized formats as unsigned, and guesses RGBA8 for unknown internal formats.
  These paths can misrepresent data instead of rejecting it.

The CPU objects above are the alternative within their supported pixel layouts.
Building all four externals, or passing the CPU channel-copy/native-peer test,
does **not** validate the GL objects in a running Pd/Gem host. Restoring GL support
requires independent send/receive asymmetric-pattern tests in that host, including
same-size format transitions and context teardown. This notice records the
current limitation; it does not disable or repair the GL runtime code.

## Requirements

- C++17 compiler
- PureData with GEM
- macOS 12+ / Linux / Windows
- The published Linux x64 build targets **x86-64-v2** (no AVX, AVX2, FMA or F16C). CI sets this explicitly and verifies the binaries with `deps/nozzle/scripts/check_x86_64_isa.sh`.

## License

MIT
