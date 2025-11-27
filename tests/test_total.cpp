#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <limits>
#include <algorithm>

#include "core/cCompression.h"
#include "core/cDecompression.h"
#include "dct/dct.h"
#include "quantification/quantification.h"

namespace {

bool loadLena(const char *filename,
              std::vector<unsigned char> &image,
              unsigned int &width,
              unsigned int &height)
{
    std::ifstream in(filename);
    if (!in)
    {
        std::cerr << "Cannot open " << filename << "\n";
        return false;
    }

    std::vector<int> vals;
    int v = 0;
    while (in >> v)
    {
        if (v < 0)
            v = 0;
        if (v > 255)
            v = 255;
        vals.push_back(v);
    }
    in.close();

    if (vals.empty())
    {
        std::cerr << "File " << filename << " is empty or invalid.\n";
        return false;
    }

    std::size_t N = vals.size();
    unsigned int side = static_cast<unsigned int>(std::sqrt(static_cast<double>(N)) + 0.5);
    if (side * side != N)
    {
        std::cerr << "Number of pixels (" << N
                  << ") is not a perfect square. Cannot infer width/height.\n";
        return false;
    }

    width = side;
    height = side;

    image.resize(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        image[i] = static_cast<unsigned char>(vals[i]);
    }
    return true;
}

void dumpBlock(const int block[8][8], const char *label)
{
    std::cout << label << "\n";
    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            std::cout << std::setw(5) << block[r][c] << ' ';
        }
        std::cout << '\n';
    }
}

void dumpBlock(const double block[8][8], const char *label)
{
    std::cout << label << "\n";
    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            std::cout << std::setw(9) << std::fixed << std::setprecision(2) << block[r][c] << ' ';
        }
        std::cout << '\n';
    }
}

bool writePGM(const char *filename,
              const unsigned char *data,
              unsigned int width,
              unsigned int height)
{
    std::ofstream out(filename, std::ios::binary);
    if (!out)
    {
        std::cerr << "Cannot write " << filename << "\n";
        return false;
    }

    out << "P5\n" << width << " " << height << "\n255\n";
    out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(width * height));
    return static_cast<bool>(out);
}

} // namespace

int main()
{
    const char *inputFile = "../../tests/lena.dat";
    const char *compressedFile = "../../tests/lena_total.huff";

    std::vector<unsigned char> img;
    unsigned int W = 0;
    unsigned int H = 0;

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

    // Prepare helpers for a forward DCT/quant/dequant round-trip on the first block
    int block[8][8];
    double dct[8][8];
    int quant[8][8];
    double dctRecovered[8][8];
    int spatialRecovered[8][8];

    int *blockRows[8];
    double *dctRows[8];
    int *quantRows[8];
    double *dctRecRows[8];
    int *spatialRows[8];

    for (int i = 0; i < 8; ++i)
    {
        blockRows[i] = block[i];
        dctRows[i] = dct[i];
        quantRows[i] = quant[i];
        dctRecRows[i] = dctRecovered[i];
        spatialRows[i] = spatialRecovered[i];
    }

    // Extract top-left 8x8 block with level shift
    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            block[r][c] = static_cast<int>(img[r * W + c]) - 128;
        }
    }

    dumpBlock(block, "Level-shifted block (top-left 8x8):");

    cCompression::setQualiteGlobale(50);

    Calcul_DCT_Block(blockRows, dctRows);
    dumpBlock(dct, "DCT coefficients:");

    quant_JPEG(dctRows, quantRows);
    dumpBlock(quant, "Quantized coefficients:");

    dequant_JPEG(quantRows, dctRecRows);
    dumpBlock(dctRecovered, "Dequantized DCT coefficients:");

    Calcul_IDCT_Block(dctRecRows, spatialRows);

    int zeroCount = 0;
    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            if (quant[r][c] == 0)
                ++zeroCount;
        }
    }

    double blockMSE = 0.0;
    for (int r = 0; r < 8; ++r)
    {
        for (int c = 0; c < 8; ++c)
        {
            int original = block[r][c];
            int recon = spatialRecovered[r][c];
            blockMSE += static_cast<double>((original - recon) * (original - recon));
        }
    }
    blockMSE /= 64.0;

    std::cout << "Zero fraction in quantized block: " << static_cast<double>(zeroCount) / 64.0 << '\n';
    std::cout << "Block MSE after quant+IDCT: " << blockMSE << '\n';

    // Full pipeline: RLE + Huffman + Disk + Decompression
    std::vector<unsigned char *> rows(H);
    for (unsigned int y = 0; y < H; ++y)
    {
        rows[y] = &img[y * W];
    }

    cCompression comp(W, H, 50, rows.data());
    cCompression::setQualiteGlobale(50);

    unsigned int nbBlocks = (W / 8) * (H / 8);
    std::vector<int> trame(1 + nbBlocks * 128, 0);
    comp.RLE(trame.data());

    unsigned int lenRLE = static_cast<unsigned int>(trame[0]);
    std::cout << "RLE length (bytes): " << lenRLE << '\n';

    comp.Compression_JPEG(trame.data(), compressedFile);

    std::ifstream fin(compressedFile, std::ios::binary | std::ios::ate);
    if (!fin)
    {
        std::cerr << "Failed to reopen compressed file " << compressedFile << '\n';
        return 1;
    }
    std::streamsize compressedSize = fin.tellg();
    fin.close();
    std::cout << "Compressed file size: " << compressedSize << " bytes\n";
    std::cout << "Compression ratio (RLE/Huffman): "
              << static_cast<double>(lenRLE) / static_cast<double>(compressedSize) << '\n';

    cDecompression dec;
    dec.setQualite(50);
    unsigned char **decodedFromFile = dec.Decompression_JPEG(compressedFile);
    std::cout << "Decompression from file done.\n";
    if (!decodedFromFile)
    {
        std::cerr << "Decompression failed.\n";
        return 1;
    }

    // Compare original vs decoded image
    double imageMSE = 0.0;
    unsigned int totalPixels = W * H;
    std::vector<unsigned char> reconBuffer(totalPixels);
    for (unsigned int y = 0; y < H; ++y)
    {
        for (unsigned int x = 0; x < W; ++x)
        {
            int orig = static_cast<int>(img[y * W + x]);
            int recon = static_cast<int>(decodedFromFile[y][x]);
            int diff = orig - recon;
            reconBuffer[y * W + x] = static_cast<unsigned char>(recon);
            imageMSE += static_cast<double>(diff * diff);
        }
    }
    imageMSE /= static_cast<double>(totalPixels);

    double psnr = 0.0;
    if (imageMSE > 0.0)
    {
        psnr = 10.0 * std::log10((255.0 * 255.0) / imageMSE);
    }
    else
    {
        psnr = std::numeric_limits<double>::infinity();
    }

    std::cout << "Image MSE (original vs decoded): " << imageMSE << '\n';
    std::cout << "PSNR: " << psnr << " dB\n";

    // Export original, reconstructed, and amplified difference images to PGM for visual inspection
    const char *origPGM = "../../tests/lena_total_original.pgm";
    const char *reconPGM = "../../tests/lena_total_recon.pgm";
    const char *diffPGM = "../../tests/lena_total_diff.pgm";

    if (writePGM(origPGM, img.data(), W, H))
        std::cout << "Wrote " << origPGM << '\n';
    if (writePGM(reconPGM, reconBuffer.data(), W, H))
        std::cout << "Wrote " << reconPGM << '\n';

    std::vector<unsigned char> diffBuffer(totalPixels);
    for (unsigned int i = 0; i < totalPixels; ++i)
    {
        int diff = std::abs(static_cast<int>(img[i]) - static_cast<int>(reconBuffer[i]));
        diff = std::min(255, diff * 8); // amplify differences for visibility
        diffBuffer[i] = static_cast<unsigned char>(diff);
    }
    if (writePGM(diffPGM, diffBuffer.data(), W, H))
        std::cout << "Wrote " << diffPGM << " (differences x8)" << '\n';

    std::cout << "test_total: DONE\n";
    return 0;
}
