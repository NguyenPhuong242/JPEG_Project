#include "core/cDecompression.h"

#include <array>
#include <vector>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <functional>
#include <cmath>
#include <iostream>

#include "core/cCompression.h"
#include "core/cHuffman.h"
#include "dct/dct.h"
#include "quantification/quantification.h"

/** @brief Create an empty decompressor with default quality. */
cDecompression::cDecompression()
{
    mLargeur = 0;
    mHauteur = 0;
    mQualite = 50;
    mBuffer = nullptr;
    mOwnBuffer = false;
}

/**
 * @brief Initialise a decompressor with explicit dimensions and buffer.
 * @param largeur Image width in pixels.
 * @param hauteur Image height in pixels.
 * @param qualite Quality factor for inverse quantization.
 * @param buffer Optional externally managed buffer.
 */
cDecompression::cDecompression(unsigned int largeur, unsigned int hauteur,
                               unsigned int qualite, unsigned char **buffer)
{
    mLargeur = largeur;
    mHauteur = hauteur;
    mQualite = qualite;
    mBuffer = buffer;
    mOwnBuffer = false;
}

/** @brief Destroy the decompressor and release owned buffers. */
cDecompression::~cDecompression()
{
    releaseBuffer();
}

/** @brief Release the currently owned buffer, if any. */
void cDecompression::releaseBuffer()
{
    if (!mBuffer || !mOwnBuffer)
        return;

    for (unsigned int y = 0; y < mHauteur; ++y)
    {
        delete[] mBuffer[y];
    }
    delete[] mBuffer;
    mBuffer = nullptr;
    mOwnBuffer = false;
}

/** @brief Update the reconstructed image width. */
void cDecompression::setLargeur(unsigned int largeur)
{
    mLargeur = largeur;
}

/** @brief Update the reconstructed image height. */
void cDecompression::setHauteur(unsigned int hauteur)
{
    mHauteur = hauteur;
}

/** @brief Update the quality factor guiding inverse quantization. */
void cDecompression::setQualite(unsigned int qualite)
{
    mQualite = qualite;
}

/** @brief Attach an externally managed buffer and drop any owned buffer. */
void cDecompression::setBuffer(unsigned char **buffer)
{
    releaseBuffer();
    mBuffer = buffer;
    mOwnBuffer = false;
}

/** @brief Return the current output width. */
unsigned int cDecompression::getLargeur() const
{
    return mLargeur;
}

/** @brief Return the current output height. */
unsigned int cDecompression::getHauteur() const
{
    return mHauteur;
}

/** @brief Return the quality factor used for dequantization. */
unsigned int cDecompression::getQualite() const
{
    return mQualite;
}

/** @brief Access the row buffer pointer (may be caller-owned). */
unsigned char **cDecompression::getBuffer() const
{
    return mBuffer;
}

// ===== Helpers for Huffman decoding (local to this translation unit) ====

/** @brief Build symbol ↔ code lookup tables from the Huffman tree. */
static void buildCodeTables(cHuffman &h,
                            std::map<char, std::string> &codeTable,
                            std::map<std::string, char> &revTable)
{
    codeTable.clear();
    revTable.clear();

    std::function<void(sNoeud *, std::string)> rec =
        [&](sNoeud *n, std::string code)
    {
        if (!n)
            return;
        if (!n->mgauche && !n->mdroit)
        {
            codeTable[n->mdonnee] = code;
            revTable[code] = n->mdonnee;
            return;
        }
        rec(n->mgauche, code + "0");
        rec(n->mdroit, code + "1");
    };

    rec(h.getRacine(), "");
}

/**
 * @brief Decode a Huffman-compressed file back to an RLE byte stream.
 * @param filename Path to the compressed file.
 * @param h Huffman instance used for decoding.
 * @return Recovered RLE bytes.
 */
static std::vector<char> HuffmanDecodeFile(const char *filename,
                                           cHuffman &h)
{
    std::vector<char> trame;

    std::ifstream in(filename, std::ios::binary);
    if (!in)
    {
        std::cerr << "Cannot open compressed file " << filename << "\n";
        return trame;
    }

    std::vector<unsigned char> data(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());
    in.close();

    std::map<char, std::string> codeTable;
    std::map<std::string, char> revTable;
    buildCodeTables(h, codeTable, revTable);

    sNoeud *root = h.getRacine();
    sNoeud *node = root;

    for (unsigned char byte : data)
    {
        for (int bitPos = 7; bitPos >= 0; --bitPos)
        {
            int bit = (byte >> bitPos) & 1;
            node = (bit == 0) ? node->mgauche : node->mdroit;

            if (!node)
            {
                node = root;
                continue;
            }

            if (!node->mgauche && !node->mdroit)
            {
                trame.push_back(node->mdonnee);
                node = root;
            }
        }
    }

    return trame;
}

/**
 * @brief Expand an RLE byte stream back into quantized 8x8 blocks.
 * @param trame Input RLE byte stream.
 * @param blocks Output container of quantized blocks.
 */
void cDecompression::Inverse_RLE(const std::vector<char> &trame,
                                 std::vector<std::array<std::array<int, 8>, 8>> &blocks)
{
    blocks.clear();
    if (trame.empty())
        return;

    constexpr int N = 8;

    int idx = 0;
    int previous_DC = 0;

    while (idx < static_cast<int>(trame.size()))
    {
        std::array<std::array<int, 8>, 8> block{};
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                block[r][c] = 0;

        int i = 0, j = 0;
        bool up = true;
        int k = 0;

        int DCdiff = static_cast<signed char>(trame[idx++]);
        int DC = previous_DC + DCdiff;
        previous_DC = DC;
        block[0][0] = DC;
        k = 1;

        while (idx + 1 <= static_cast<int>(trame.size()) && k < N * N)
        {
            signed char run = trame[idx++];
            signed char coeff = trame[idx++];

            if (run == 0 && coeff == 0)
            {
                break;
            }

            for (int step = 0; step < run && k < N * N; ++step)
            {
                if (up)
                {
                    if (j == N - 1)
                    {
                        ++i;
                        up = false;
                    }
                    else if (i == 0)
                    {
                        ++j;
                        up = false;
                    }
                    else
                    {
                        --i;
                        ++j;
                    }
                }
                else
                {
                    if (i == N - 1)
                    {
                        ++j;
                        up = true;
                    }
                    else if (j == 0)
                    {
                        ++i;
                        up = true;
                    }
                    else
                    {
                        ++i;
                        --j;
                    }
                }
                ++k;
            }

            if (k < N * N)
            {
                if (up)
                {
                    if (j == N - 1)
                    {
                        ++i;
                        up = false;
                    }
                    else if (i == 0)
                    {
                        ++j;
                        up = false;
                    }
                    else
                    {
                        --i;
                        ++j;
                    }
                }
                else
                {
                    if (i == N - 1)
                    {
                        ++j;
                        up = true;
                    }
                    else if (j == 0)
                    {
                        ++i;
                        up = true;
                    }
                    else
                    {
                        ++i;
                        --j;
                    }
                }
                ++k;

                if (i >= 0 && i < N && j >= 0 && j < N)
                {
                    block[i][j] = coeff;
                }
            }
        }

        blocks.push_back(block);
    }
}

/**
 * @brief Perform full decompression of a Huffman file into spatial pixels.
 * @param Nom_Fichier_compresse Path to the compressed file (bitstream).
 * @return Pointer to reconstructed image rows; nullptr on failure.
 */
unsigned char **cDecompression::Decompression_JPEG(const char *Nom_Fichier_compresse)
{
    if (!Nom_Fichier_compresse)
        return nullptr;

    char symbols[256];
    double freqs[256];
    unsigned int count = 0;

    if (!cCompression::loadHuffmanTable(symbols, freqs, count))
    {
        std::cerr << "No Huffman table available. Call Compression_JPEG first.\n";
        return nullptr;
    }

    cHuffman h;
    h.HuffmanCodes(symbols, freqs, count);

    std::vector<char> rleBytes = HuffmanDecodeFile(Nom_Fichier_compresse, h);
    if (rleBytes.empty())
        return nullptr;

    std::vector<std::array<std::array<int, 8>, 8>> qBlocks;
    Inverse_RLE(rleBytes, qBlocks);

    const unsigned int nbBlocks = static_cast<unsigned int>(qBlocks.size());
    if (nbBlocks == 0)
        return nullptr;

    unsigned int blocksPerSide = static_cast<unsigned int>(std::sqrt(static_cast<double>(nbBlocks)) + 0.5);

    mLargeur = blocksPerSide * 8;
    mHauteur = blocksPerSide * 8;

    releaseBuffer();
    mBuffer = new unsigned char *[mHauteur];
    mOwnBuffer = true;
    for (unsigned int y = 0; y < mHauteur; ++y)
        mBuffer[y] = new unsigned char[mLargeur];

    cCompression::setQualiteGlobale(mQualite);

    int quant[8][8];
    double dct[8][8];
    int spat[8][8];

    int *quant_rows[8];
    double *dct_rows[8];
    int *spat_rows[8];

    for (int i = 0; i < 8; ++i)
    {
        quant_rows[i] = quant[i];
        dct_rows[i] = dct[i];
        spat_rows[i] = spat[i];
    }

    unsigned int b = 0;
    for (unsigned int by = 0; by < mHauteur; by += 8)
    {
        for (unsigned int bx = 0; bx < mLargeur; bx += 8)
        {
            for (int r = 0; r < 8; ++r)
                for (int c = 0; c < 8; ++c)
                    quant[r][c] = qBlocks[b][r][c];

            dequant_JPEG(quant_rows, dct_rows);
            Calcul_IDCT_Block(dct_rows, spat_rows);

            for (int r = 0; r < 8; ++r)
            {
                for (int c = 0; c < 8; ++c)
                {
                    int val = spat[r][c] + 128;
                    if (val < 0)
                        val = 0;
                    if (val > 255)
                        val = 255;
                    mBuffer[by + r][bx + c] = static_cast<unsigned char>(val);
                }
            }

            ++b;
        }
    }

    return mBuffer;
}
