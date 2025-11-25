#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>        // std::sqrt
#include <cstdio>       // std::remove (optional)
#include "core/cCompression.h"

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
    // 1) load lena.dat like you already do in test_compression
    
    std::vector<unsigned char> img;
    unsigned int W = 0, H = 0;

    if (!loadLena(inputFile, img, W, H))
    {
        return 1;
    }

    std::cout << "Loaded image " << W << "x" << H
              << " (" << img.size() << " pixels)\n";

    if (W % 8 || H % 8)
    {
        std::cerr << "Width/height must be multiples of 8 for JPEG blocks.\n";
        return 1;
    }
    // ... load into std::vector<unsigned char> img, W, H ...

    std::vector<unsigned char *> rows(H);
    for (unsigned int y = 0; y < H; ++y)
        rows[y] = &img[y * W];

    cCompression comp(W, H, 50, rows.data());
    cCompression::setQualiteGlobale(50);

    // 2) RLE
    std::vector<int> Trame(1 + (W / 8) * (H / 8) * 128, 0);
    comp.RLE(Trame.data());
    unsigned int lenRLE = Trame[0];
    std::cout << "RLE length = " << lenRLE << " bytes\n";

    // 3) Huffman compression
    comp.Compression_JPEG(Trame.data(), "lena.huff");

    // 4) Decompression in the SAME process
    unsigned char **decoded = comp.Decompression_JPEG("lena.huff");
    if (!decoded)
    {
        std::cerr << "Decompression failed.\n";
        return 1;
    }

    // 5) here you can compute PSNR, or write decoded image to a PGM/BMP, etc.
    std::cout << "Decompression OK in same program.\n";

    return 0;
}
