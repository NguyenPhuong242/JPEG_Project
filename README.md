# JPEG Compressor — User Guide

**An educational implementation of a simplified JPEG-like compression pipeline (grayscale and color).**

- **Repository:** [https://github.com/NguyenPhuong242/JPEG_Project](https://github.com/NguyenPhuong242/JPEG_Project)
- **Author:** Khanh-Phuong NGUYEN (NguyenPhuong242)
- **License:** MIT (see `LICENSE`)

---

## 1. Overview

This project provides a modular and pedagogical implementation of the essential steps of a JPEG-style compression/decompression chain. It is designed to demonstrate:

- 8×8 block splitting
- DCT (Discrete Cosine Transform) / IDCT
- Quantization
- RLE (Run-Length Encoding)
- Huffman entropy coding
- Optional YCbCr color pipeline with subsampling

**Note:** The produced `*.huff` files are a custom, transparent format for educational purposes and are not standard JPEG files.

---

## 2. Features

- **Grayscale pipeline:** Compression of ASCII pixel maps to custom bitstreams.
- **Color pipeline:** Full support for PPM (P6) images converted to Y/Cb/Cr streams.
- **Chroma Subsampling:** Supports 4:4:4 (no subsampling), 4:2:2, and 4:2:0.
- **Modular Design:** Distinct modules for DCT, Quantization, RLE, and Huffman.
- **Analysis Tools:** Frequency histogram generation and unit tests.
- **Documentation:** Doxygen-ready comments.

---

## 3. Repository Structure

| Directory | Description |
| :--- | :--- |
| `src/` | Source code (DCT, Quantization, RLE, Huffman, Core logic) |
| `include/` | Public header files |
| `tests/` | Unit tests |
| `tools/` | Helper scripts (e.g., sample PPM generator) |
| `docs/` | Doxygen configuration and generated documentation |
| `build/` | Out-of-source build directory (ignored by git) |

---

## 4. Prerequisites

- **CMake** ≥ 3.10
- **C++17 compiler** (g++ or clang++)
- **Optional:** `doxygen` and `graphviz` (for generating documentation)

**Installation (Debian/Ubuntu):**

```bash
sudo apt update
sudo apt install build-essential cmake g++ doxygen graphviz
```

---

## 5. Build Instructions

Perform an out-of-source build from the project root. The `-j` flag utilizes multiple CPU cores for faster compilation.

```bash
# 1. Clean previous builds (optional)
rm -rf build

# 2. Configure project
mkdir build && cd build
cmake ..

# 3. Compile
cmake --build . -- -j

# 4. Test (optional)
ctest --test-dir build -V
```

After a successful build, the main executable is located at `build/jpeg_cli`.

---

## 6. Usage

The `jpeg_cli` tool handles both grayscale and color workflows.

### A. Grayscale Workflow

Input is an ASCII grayscale `.img` file (one pixel value 0–255 per line) or PGM.

#### 1. Compress
```bash
# Syntax: ./build/jpeg_cli <input.img> [quality]
./build/jpeg_cli lenna.img 50
```
**Outputs:**
- `lenna.huff`: The compressed Huffman stream.
- `lenna.rle`: Intermediate RLE data (optional).
- `recon_lenna.pgm`: Reconstructed image for immediate verification.

#### 2. Decompress
```bash
# Syntax: ./build/jpeg_cli --decompress <file.huff>
./build/jpeg_cli --decompress lenna.huff
```
**Outputs:**
- `decomp_lenna.pgm`

#### 3. Round-trip Check
```bash
./build/jpeg_cli lenna.img 50
./build/jpeg_cli --decompress lenna.huff
# You can now compare recon_lenna.pgm and decomp_lenna.pgm
```

### B. Color Workflow (YCbCr)

Input is a standard PPM (P6 format) image.

#### 1. Compress
```bash
# Syntax: ./build/jpeg_cli --color-compress <input.ppm> <base_name> [quality] [subsampling]
# Subsampling options: 444 (default), 422, 420

# Example: 4:2:0 subsampling (high compression)
./build/jpeg_cli --color-compress lenna_color.ppm lenna_output 75 420
```
**Outputs:**
- `lenna_output_Y.huff`, `lenna_output_Cb.huff`, `lenna_output_Cr.huff`
- `lenna_output.meta` (Contains dimensions and sampling factors)

#### 2. Decompress
```bash
# Syntax: ./build/jpeg_cli --color-decompress <base_name> <output.ppm>
./build/jpeg_cli --color-decompress lenna_output recon_final.ppm
```

#### 3. Test with a generated sample
```bash
# Generate a tiny 8x8 PPM
python3 tools/make_sample_ppm.py tests/sample_color.ppm --width 8 --height 8

# Compress and Decompress
./build/jpeg_cli --color-compress tests/sample_color.ppm sample 75 444
./build/jpeg_cli --color-decompress sample sample_out.ppm
```

### C. Utilities

#### Histogram Analysis
Analyze the frequency distribution of the RLE stream.
```bash
./build/jpeg_cli --histogram <file.rle>
```

#### Verbose Testing
Enable verbose output to view detailed test logs (if running test suites).
```bash
./build/tests/test<name> --verbose
```

### D. Help Command
For a summary of commands and options:
```bash
./build/jpeg_cli --help
```

---

## 7. Important Notes

1.  **File Format:** The `.huff` format used here is **custom**. It is designed to make the RLE and Huffman encoding steps transparent for debugging and learning. It is **not** compatible with standard `.jpg` viewers.
2.  **Quality Factor:** The quality argument (1–100) controls the quantization matrix.
    -   **Low value:** High compression, lower quality (more data loss).
    -   **High value:** Low compression, higher quality.

## 8. Generating Documentation
If `doxygen` and `graphviz` are installed, generate the documentation as follows:

```bash
cd docs
doxygen Doxyfile
```
The generated documentation will be located in `docs/html/index.html`. Open this file in a web browser to explore the code structure and comments.

````bash
xdg-open docs/html/index.html
````

The small report is in the docs folder as `project_report.pdf`.

## 9. Contributing
Contributions are welcome! Please fork the repository and submit pull requests for any enhancements or bug fixes. Make sure to follow the existing coding style and include tests for new features.