# nozzle-pd

PureData/GEM externals for [nozzle](https://github.com/nozzle-io/nozzle) — cross-platform GPU texture sharing between local processes.

## Objects

| Object | Description |
|--------|-------------|
| `pix_nozzle_send` | Sends GEM pix image data via nozzle (CPU copy path) |
| `pix_nozzle_receive` | Receives nozzle frames and injects into GEM render chain |
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

## License

MIT
