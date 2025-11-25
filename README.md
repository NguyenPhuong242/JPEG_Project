# JPEG Compressor

Small educational JPEG compression project (DCT + quantification) used for a programming exercise.

Contents
- `include/` : public headers
- `src/` : implementation (DCT, quantification, core)
- `tests/` : small manual unit tests for DCT and quantification
- `docs/` : Doxygen configuration and generated HTML in `docs/html`

Quick build (Linux)

Install required packages (Debian/Ubuntu):
```bash
sudo apt update
sudo apt install build-essential cmake g++ doxygen graphviz
```

Configure and build:
```bash
cmake -S . -B build
cmake --build build -j
```

Run unit tests (via CTest):
```bash
ctest --test-dir build -V
```

Run the demo executable (prints an example DCT block):
```bash
./build/jpeg_compressor
```

Generate documentation

The repo contains a `docs/Doxyfile`. To regenerate the Doxygen HTML documentation:

```bash
# from project root
doxygen docs/Doxyfile
# open generated docs
xdg-open docs/html/index.html
```

Notes
- Graphs in the Doxygen output require `graphviz` (dot). If you do not want graphs, you can disable them in `docs/Doxyfile`.
- The project is intentionally minimal and educational; it implements DCT and quantization, with helper tests referencing figures in the course "sujet" PDF.

Contributing / Next steps
- Implement zig-zag ordering and run-length encoding.
- Add Huffman entropy coding and write a minimal valid JPEG file.
- Add more unit tests and sample images for end-to-end validation.

Author: student project
