# JPEG Compressor

Educational grayscale JPEG pipeline that showcases every major step of the codec: block extraction, 8x8 DCT, quantization, zig-zag + run-length encoding, Huffman entropy coding, and a CLI that can compress and decompress sample images.

## Features
- Baseline JPEG-style pipeline implemented in modern C++.
- Modular headers under `include/` for reuse in other exercises.
- CLI (`jpeg_cli`) able to compress, decompress, or run a full round trip.
- Doxygen documentation with class and file overviews.
- Lightweight test binaries covering DCT, quantization, Huffman, and RLE pieces.

## Project Layout
```
include/          Public headers for the core, DCT, and quantization modules
src/              Implementations plus the CLI entry point
tests/            Simple regression tests and fixtures (run via CTest)
docs/             Doxygen config (`Doxyfile`) and generated output under docs/docs/
build/            Out-of-tree CMake build products
```

## Prerequisites (Debian/Ubuntu)
```bash
sudo apt update
sudo apt install build-essential cmake g++ doxygen graphviz
```

## Build and Test
```bash
# Configure & build
cmake -S . -B build
cmake --build build -j

# Run unit tests
ctest --test-dir build -V
```

## CLI Usage
The build outputs `build/jpeg_cli`. Run it without arguments to open the interactive menu that guides you through compression, decompression, or the full pipeline.

```bash
# Show a short usage summary
./build/jpeg_cli --help

# Start the interactive workflow
./build/jpeg_cli
```

## Documentation
Generate or refresh the API reference with Doxygen:

```bash
doxygen docs/Doxyfile
xdg-open docs/docs/html/index.html   # open the HTML entry point
```

> Graph visualizations require `graphviz`. Disable them by flipping the relevant `HAVE_DOT` tags in `docs/Doxyfile` if you prefer faster runs.

The main page of the HTML output mirrors this README, and navigation exposes the annotated headers, implementation files, and directory diagrams.

## Roadmap / Contributions
1. Expand coverage for color images and chroma subsampling modes.
2. Produce minimal valid `.jpg` files instead of the custom `.huff` bitstream.
3. Flesh out automated unit tests and CI.
4. Document remaining internals that still trigger Doxygen `@param` duplication warnings.

Feel free to open issues or submit pull requests if you extend or fix the pipeline.
