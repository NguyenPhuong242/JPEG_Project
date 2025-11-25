#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>        // std::sqrt
#include <cstdio>       // std::remove (optional)
#include "core/cCompression.h"

// ---------------------------------------------------------
//  Load lena.dat  (ASCII: one gray value per whitespace)
//  Returns a square image: width = height = sqrt(N)
// ---------------------------------------------------------
bool loadLena(const char *filename,
              std::vector<unsigned char> &image,
              unsigned int &width,
              unsigned int &height)
{
    std::ifstream in(filename);
    if (!in) {
        std::cerr << "Cannot open " << filename << "\n";
        return false;
    }

    std::vector<int> vals;
    int v;
    while (in >> v) {
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        vals.push_back(v);
    }
    in.close();

    if (vals.empty()) {
        std::cerr << "File " << filename << " is empty or invalid.\n";
        return false;
    }

    // We expect a square image (e.g. 128x128, 256x256, ...)
    std::size_t N = vals.size();
    unsigned int side = static_cast<unsigned int>(std::sqrt(static_cast<double>(N)) + 0.5);
    if (side * side != N) {
        std::cerr << "Number of pixels (" << N
                  << ") is not a perfect square. Cannot infer width/height.\n";
        return false;
    }

    width  = side;
    height = side;

    image.resize(N);
    for (std::size_t i = 0; i < N; ++i) {
        image[i] = static_cast<unsigned char>(vals[i]);
    }

    return true;
}

int main()
{
    const char *inputFile  = "../../tests/lena.dat";      // put lena.dat next to the executable
    const char *outputFile = "../../tests/lena.huff";    // compressed output

    // 1) Load lena.dat as a grayscale image
    std::vector<unsigned char> img;
    unsigned int W = 0, H = 0;

    if (!loadLena(inputFile, img, W, H)) {
        return 1;
    }

    std::cout << "Loaded image " << W << "x" << H
              << " (" << img.size() << " pixels)\n";

    if (W % 8 || H % 8) {
        std::cerr << "Width/height must be multiples of 8 for JPEG blocks.\n";
        return 1;
    }

    // 2) Build buffer of row pointers for cCompression
    std::vector<unsigned char*> rows(H);
    for (unsigned int y = 0; y < H; ++y) {
        rows[y] = &img[y * W];
    }

    // 3) Create compressor and run RLE
    cCompression comp(W, H, 50, rows.data());
    cCompression::setQualiteGlobale(50);

    // Worst-case RLE length: at most 128 bytes per 8x8 block + 1 for length
    unsigned int nbBlocks = (W / 8) * (H / 8);
    unsigned int maxRLEInts = 1 + nbBlocks * 128;

    std::vector<int> Trame_RLE(maxRLEInts, 0);
    comp.RLE(Trame_RLE.data());

    unsigned int lenRLE = static_cast<unsigned int>(Trame_RLE[0]);
    std::cout << "RLE trame length (bytes): " << lenRLE << "\n";

    // 4) Huffman compress the RLE trame and write to disk
    comp.Compression_JPEG(Trame_RLE.data(), outputFile);

    // 5) Check output file size
    std::ifstream fin(outputFile, std::ios::binary | std::ios::ate);
    if (!fin) {
        std::cerr << "Failed to open output file " << outputFile << " for size check.\n";
        return 1;
    }
    std::streamsize outSize = fin.tellg();
    fin.close();

    std::cout << "Huffman-compressed file size: " << outSize << " bytes\n";
    std::cout << "Compression ratio (RLE bytes / Huffman bytes): "
              << static_cast<double>(lenRLE) / static_cast<double>(outSize) << "\n";

    std::cout << "test_compression: DONE\n";
    return 0;
}
