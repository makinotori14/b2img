# b2img

`b2img` is a Linux-only utility for exploring binary files as images.

- `gui/` contains the GTK4 frontend.
- `core/` contains the decoding and analysis logic.
- `io/` handles file loading and image export.
- `cli/` parses commands and flags.
- `app/` contains shared application services for both GUI and CLI.

The main application is now a GTK4 frontend. A helper CLI remains available as
`b2img-cli`.

## Features

- Inspect a binary slice before decoding it.
- Render bit-packed indexed images.
- Render packed pixel data in all channel permutations of:
  - `RGB24`
  - `RGBA32`
- Decode simple 4-plane indexed layouts:
  - `linear`
  - `planar-image`
  - `planar-row`
  - `planar-pixel`
- Use built-in palettes:
  - `grayscale`
  - `vga16`
  - `cga0`
  - `cga1`
  - `cga2`

## Build

```bash
sudo apt install build-essential cmake pkg-config libgtk-4-dev
```

```bash
cmake -S . -B build
cmake --build build
```

## Usage

Run the GUI:

```bash
./build/b2img
```

The frontend lets you:

- open a binary file through the explorer
- drag & drop file into the window
- choose width, height, bit offset, layout, palette and encoding
- adjust width and height sliders
- rotate the preview by 90 degrees
- zoom the preview
- save the result as `.ppm`

Show CLI help:

```bash
./build/b2img-cli --help
```
