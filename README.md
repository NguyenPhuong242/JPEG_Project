# JPEG Compressor — User Guide

This project is an educational implementation of a simplified JPEG compression/decompression chain. It covers the main steps: 8x8 block splitting, DCT, quantization, RLE, and Huffman coding.

### Main Modules

-   `src/dct/`: Implementation of the Discrete Cosine Transform (DCT) and its inverse (IDCT).
-   `src/quantification/`: Quantization tables and related functions.
-   `src/core/`:
    -   `cCompression`: The main class for the grayscale pipeline (RLE, Huffman, EQM, Compression Ratio).
    -   `cCompressionCouleur`: The main class for the color pipeline (PPM → YCbCr → component-wise compression).
-   `include/`: Contains the public header files.
-   `tests/`: Unit tests for DCT, quantization, RLE, and Huffman.

---

### 1. Prerequisites

-   **CMake** (version >= 3.10)
-   **A C++ Compiler** with C++17 support (e.g., g++, clang++)
-   **(Optional)** `doxygen` and `graphviz` for source code documentation generation.

```bash
sudo apt update
sudo apt install build-essential cmake g++ doxygen graphviz
```

---


### 2. Documentation
Generate or refresh the API reference with Doxygen:

```bash
doxygen docs/Doxyfile
xdg-open docs/docs/html/index.html   # open the HTML entry point
```

> Graph visualizations require `graphviz`. Disable them by flipping the relevant `HAVE_DOT` tags in `docs/Doxyfile` if you prefer faster runs.

The main page of the HTML output mirrors this README, and navigation exposes the annotated headers, implementation files, and directory diagrams.

### 3. Build Instructions

Use CMake to build the project. The following commands should be executed from the project's root directory.

```bash
# 1. Create a build directory to store compilation files
cmake -S . -B build

# 2. Compile the project
# The -j flag automatically uses multiple CPU cores to speed up compilation
cmake --build build -j
```

After a successful build, the main executable will be located at `build/jpeg_cli`.

---

### 4. Usage

The `build/jpeg_cli` executable is the main command-line interface (CLI) for this program.

#### a. Grayscale Workflow (text .img → .pgm/.huff)

-   **Compress:** takes an ASCII grayscale `.img` (one pixel per line, 0-255). Outputs:
    -   `<name>.huff`: Huffman stream with width/height trailer.
    -   `<name>.rle`: RLE intermediate (optional to keep).
    -   `recon_<name>.pgm`: Reconstructed image for quick visual check.

    ```bash
    # Syntax: ./build/jpeg_cli <image.img> [quality]
    # Example with the provided sample (128x128):
    ./build/jpeg_cli lenna.img 50
    # Produces lenna.huff, lenna.rle, recon_lenna.pgm in the current dir
    ```

-   **Decompress:** reads a `.huff` produced by this tool and writes `decomp_<name>.pgm`.
    ```bash
    # Syntax: ./build/jpeg_cli --decompress <file.huff>
    ./build/jpeg_cli --decompress lenna.huff
    # Produces decomp_lenna.pgm (dimensions taken from trailer). Legacy files without
    # trailer fall back to size inference and may be wrong; prefer re-compressing.
    ```

-   **Round-trip sanity check:**
    ```bash
    ./build/jpeg_cli lenna.img 50
    ./build/jpeg_cli --decompress lenna.huff
    # Compare recon_lenna.pgm and decomp_lenna.pgm (they should match)
    ```

#### b. Color Workflow (PPM P6 → Y/Cb/Cr bitstreams → PPM)

-   **Compress:** takes a binary PPM (P6) and splits Y, Cb, Cr, applying optional subsampling.
    Outputs `<base>_Y.huff`, `<base>_Cb.huff`, `<base>_Cr.huff`, and `<base>.meta` (stores width/height, quality, subsampling).

    ```bash
    # Syntax: ./build/jpeg_cli --color-compress <input.ppm> <base> [quality] [subsampling]
    # Subsampling: 444 (default), 422, or 420

    # Example 1: no subsampling
    ./build/jpeg_cli --color-compress lenna_color.ppm lenna_color 75 444

    # Example 2: 4:2:0 subsampling for higher compression
    ./build/jpeg_cli --color-compress lenna_color.ppm lenna_color 75 420
    # Produces lenna_color_Y.huff, lenna_color_Cb.huff, lenna_color_Cr.huff, lenna_color.meta
    ```

-   **Decompress:** reconstructs a PPM using the three component streams plus metadata.
    ```bash
    # Syntax: ./build/jpeg_cli --color-decompress <base> <output.ppm>
    ./build/jpeg_cli --color-decompress lenna_color recon_lenna_color.ppm
    # Reads lenna_color_*.huff and lenna_color.meta from the working directory
    ```

-   **Round-trip sanity check (color):**
    ```bash
    ./build/jpeg_cli --color-compress lenna_color.ppm demo 75 420
    ./build/jpeg_cli --color-decompress demo demo_out.ppm
    # Inspect demo_out.ppm; PSNR depends on quality/subsampling
    ```

-   **Need a sample PPM?** Generate a tiny one (8x8 gradient) and reuse it:
    ```bash
    python3 tools/make_sample_ppm.py tests/sample_color.ppm --width 8 --height 8
    ./build/jpeg_cli --color-compress tests/sample_color.ppm sample 75 444
    ./build/jpeg_cli --color-decompress sample sample_out.ppm
    ```

#### c. Other Utilities

-   **View Frequency Histogram:**
    Analyzes an RLE-encoded file to show the frequency of each symbol.

    ```bash
    # Syntax: ./build/jpeg_cli --histogram <file.rle>
    ./build/jpeg_cli --histogram build/lenna.rle
    ```

---

### 5. Running Tests

The project includes a test suite to verify the correctness of the core modules.

```bash
# Run all tests from the build directory
ctest --test-dir build -V
```

The `-V` flag enables verbose output to show detailed test results.

---

### Important Notes

-   **Educational Purpose**: The `.huff` file format is a custom format and is **not** a standard JPEG file. It was designed to make the encoding steps (RLE, Huffman) transparent and easy to debug.
-   **Quality**: This parameter controls the level of compression and information loss. A lower quality value results in higher compression but a lower-quality image.
