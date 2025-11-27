#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "core/cCompressionCouleur.h"
#include "core/cDecompressionCouleur.h"

namespace fs = std::filesystem;

int main()
{
    const unsigned int width = 16;
    const unsigned int height = 16;
    std::vector<unsigned char> rgb(width * height * 3U);

    for (unsigned int y = 0; y < height; ++y)
    {
        for (unsigned int x = 0; x < width; ++x)
        {
            const std::size_t idx = static_cast<std::size_t>(y) * width + x;
            rgb[idx * 3 + 0] = static_cast<unsigned char>((x * 16) % 256); // R gradient
            rgb[idx * 3 + 1] = static_cast<unsigned char>((y * 16) % 256); // G gradient
            rgb[idx * 3 + 2] = static_cast<unsigned char>(((x + y) * 8) % 256); // B gradient
        }
    }

    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    fs::path prefix = fs::temp_directory_path() / ("jpeg_color_test_" + std::to_string(now));

    cCompressionCouleur compressor(100, jpeg::ChromaSubsampling::Sampling444);
    if (!compressor.compressRGB(rgb, width, height, prefix.string()))
    {
        std::cerr << "Color compression failed\n";
        return 1;
    }

    cDecompressionCouleur decompressor(100);
    std::vector<unsigned char> recon;
    unsigned int outWidth = 0;
    unsigned int outHeight = 0;
    jpeg::ChromaSubsampling modeOut = jpeg::ChromaSubsampling::Sampling444;

    if (!decompressor.decompressRGB(prefix.string(), recon, outWidth, outHeight, modeOut))
    {
        std::cerr << "Color decompression failed\n";
        return 1;
    }

    if (outWidth != width || outHeight != height)
    {
        std::cerr << "Unexpected output dimensions\n";
        return 1;
    }

    if (recon.size() != rgb.size())
    {
        std::cerr << "Unexpected reconstructed buffer size\n";
        return 1;
    }

    if (modeOut != jpeg::ChromaSubsampling::Sampling444)
    {
        std::cerr << "Subsampling mode mismatch\n";
        return 1;
    }

    double mse = 0.0;
    for (std::size_t idx = 0; idx < rgb.size(); ++idx)
    {
        const int diff = static_cast<int>(rgb[idx]) - static_cast<int>(recon[idx]);
        mse += static_cast<double>(diff * diff);
    }
    mse /= static_cast<double>(rgb.size());

    const double psnr = (mse > 0.0) ? 10.0 * std::log10((255.0 * 255.0) / mse) : std::numeric_limits<double>::infinity();
    if (psnr < 35.0)
    {
        std::cerr << "PSNR too low: " << psnr << " dB (MSE=" << mse << ")\n";
        return 1;
    }

    fs::remove(prefix.string() + ".meta");
    fs::remove(prefix.string() + "_Y.huff");
    fs::remove(prefix.string() + "_Cb.huff");
    fs::remove(prefix.string() + "_Cr.huff");

    return 0;
}
