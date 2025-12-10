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

#### a. Grayscale Image Processing

This is the program's default mode.

-   **Grayscale Compression:**
    The program reads a text-based image file (e.g., `lenna.img`), calculates metrics, and generates 3 output files:
    -   `recon_lenna.pgm`: The reconstructed image for verification.
    -   `lenna.rle`: Data after RLE encoding.
    -   `lenna.huff`: The final data after Huffman encoding.

    ```bash
    # Syntax: ./build/jpeg_cli [image_file_path] [quality]
    # Example: Compress lenna.img with a quality of 50 (default)
    ./build/jpeg_cli lenna.img 50
    ```

-   **Grayscale Decompression:**
    Decompresses a `.huff` file created by this program.

    ```bash
    # Syntax: ./build/jpeg_cli --decompress <file.huff>
    # The output will be saved to decomp_lenna.pgm
    ./build/jpeg_cli --decompress build/lenna.huff
    ```

#### b. Color Image Processing (PPM Format)

-   **Color Compression:**
    Compresses a PPM (P6) image into three separate compressed components (Y, Cb, Cr).
    -   `<basename>_Y.huff`, `<basename>_Cb.huff`, `<basename>_Cr.huff`
    -   `<basename>.meta`: A file containing metadata (dimensions, quality, subsampling mode).

    ```bash
    # Syntax: ./build/jpeg_cli --color-compress <input.ppm> <basename> [quality] [subsampling]
    # Example: Compress color.ppm with quality 75 and 4:2:0 subsampling
    ./build/jpeg_cli --color-compress path/to/color.ppm my_image 75 420
    ```
    -   `quality`: 1-100 (affects the quantization matrix).
    -   `subsampling`: `444` (no subsampling), `422`, `420`.

-   **Color Decompression:**
    Reconstructs a PPM file from the component files (`.huff`) and the metadata file (`.meta`).

    ```bash
    # Syntax: ./build/jpeg_cli --color-decompress <basename> <output.ppm>
    # Example: Decompress from my_image_* files and save to final_image.ppm
    ./build/jpeg_cli --color-decompress my_image final_image.ppm
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
