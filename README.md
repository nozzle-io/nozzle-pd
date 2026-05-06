# nozzle-pd

PureData/GEM externals for [nozzle](https://github.com/nozzle-io/nozzle) — cross-platform GPU texture sharing between local processes.

## Objects

| Object | Description |
|--------|-------------|
| `pix_nozzle_send` | Sends GEM pix image data via nozzle (CPU copy path) |
| `pix_nozzle_receive` | Receives nozzle frames and injects into GEM render chain |
| `pix_nozzle_gl_send` | Sends OpenGL texture directly via nozzle (GPU path) |
| `pix_nozzle_gl_receive` | Receives nozzle frame and copies to OpenGL texture |

## Build

```bash
git submodule update --init --recursive
make
```

Externals are output to `.build/`.

## Install

Copy the built externals (`.pd_darwin` on macOS, `.pd_linux` on Linux) to your Pd externals directory:

```bash
cp .build/pix_nozzle_* ~/.pd-externals/nozzle-pd/
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
- macOS 12+ / Linux

## License

MIT
