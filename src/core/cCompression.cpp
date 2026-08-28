/**
 * @file cCompression.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements the cCompression class for the grayscale JPEG pipeline.
 */

#include "core/cCompression.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <string>

#include "dct/dct.h"
#include "quantification/quantification.h"
#include <iostream>


// --- Static Member Definitions ---

/** @brief Global quality parameter backing the static quantization helpers. */
static unsigned int gQualiteGlobale = 50;
/** @brief Cached Huffman symbols from the most recent encoding operation. */
static char   gHuffSymbols[256];
/** @brief Cached Huffman frequencies corresponding to gHuffSymbols. */
static double gHuffFreqs[256];
/** @brief Number of valid entries in the cached Huffman table. */
static unsigned int gHuffCount = 0;


// --- cCompression Method Implementations ---

cCompression::cCompression()
{
    this->mLargeur = 0;
    this->mHauteur = 0;
    this->mQualite = 50;
    this->mBuffer = nullptr;
}

cCompression::cCompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite, unsigned char **buffer)
{
    this->mLargeur = largeur;
    this->mHauteur = hauteur;
    this->mQualite = qualite;
    this->mBuffer = buffer;
}

cCompression::~cCompression()
{
    // Destructor is empty as the class does not assume ownership of the mBuffer data.
}

void cCompression::setLargeur(unsigned int largeur)
{
    this->mLargeur = largeur;
}

void cCompression::setHauteur(unsigned int hauteur)
{
    this->mHauteur = hauteur;
}

void cCompression::setQualite(unsigned int qualite)
{
    this->mQualite = qualite;
}

void cCompression::setBuffer(unsigned char **buffer)
{
    this->mBuffer = buffer;
}

unsigned int cCompression::getLargeur() const
{
    return this->mLargeur;
}

unsigned int cCompression::getHauteur() const
{
    return this->mHauteur;
}

unsigned int cCompression::getQualite() const
{
    return this->mQualite;
}

unsigned char **cCompression::getBuffer() const
{
    return this->mBuffer;
}

unsigned int cCompression::getQualiteGlobale()
{
    return gQualiteGlobale;
}

void cCompression::setQualiteGlobale(unsigned int qualite)
{
    gQualiteGlobale = (qualite < 1) ? 1 : (qualite > 100) ? 100 : qualite;
}

void cCompression::storeHuffmanTable(const char *symbols, const double *frequencies, unsigned int count)
{
    if (!symbols || !frequencies || count == 0) {
        gHuffCount = 0;
        return;
    }
    gHuffCount = (count > 256) ? 256 : count;
    std::memcpy(gHuffSymbols, symbols, gHuffCount);
    for (unsigned int i = 0; i < gHuffCount; ++i) {
        gHuffFreqs[i] = frequencies[i];
    }
}

bool cCompression::loadHuffmanTable(char *symbols, double *frequencies, unsigned int &count)
{
    if (!symbols || !frequencies || gHuffCount == 0) {
        count = 0;
        return false;
    }
    std::memcpy(symbols, gHuffSymbols, gHuffCount);
    for (unsigned int i = 0; i < gHuffCount; ++i) {
        frequencies[i] = gHuffFreqs[i];
    }
    count = gHuffCount;
    return true;
}

bool cCompression::hasStoredHuffmanTable()
{
    return gHuffCount != 0;
}

double cCompression::EQM(int **Bloc8x8)
{
    if (!Bloc8x8) return 0.0;

    // Buffers for the pipeline
    int shifted[8][8];      int* shifted_ptrs[8];
    double dct[8][8];       double* dct_ptrs[8];
    int quant[8][8];        int* quant_ptrs[8];
    double dequant[8][8];   double* dequant_ptrs[8];
    int recon[8][8];        int* recon_ptrs[8];
    for (int i=0; i<8; ++i) {
        shifted_ptrs[i] = shifted[i]; dct_ptrs[i] = dct[i]; quant_ptrs[i] = quant[i];
        dequant_ptrs[i] = dequant[i]; recon_ptrs[i] = recon[i];
    }

    // Pipeline: Shift -> DCT -> Quant -> Dequant -> IDCT
    for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) shifted[i][j] = Bloc8x8[i][j] - 128;
    Calcul_DCT_Block(shifted_ptrs, dct_ptrs);
    quant_JPEG(dct_ptrs, quant_ptrs);
    dequant_JPEG(quant_ptrs, dequant_ptrs);
    Calcul_IDCT_Block(dequant_ptrs, recon_ptrs);

    // Calculate sum of squared differences
    double sum_sq_err = 0.0;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            int reconstructed_val = recon[i][j] + 128;
            reconstructed_val = (reconstructed_val < 0) ? 0 : (reconstructed_val > 255) ? 255 : reconstructed_val;
            double diff = static_cast<double>(Bloc8x8[i][j] - reconstructed_val);
            sum_sq_err += diff * diff;
        }
    }
    return sum_sq_err / 64.0;
}

double cCompression::Taux_Compression(int **Bloc8x8)
{
    if (!Bloc8x8) return 0.0;

    // Buffers
    int shifted[8][8];      int* shifted_ptrs[8];
    double dct[8][8];       double* dct_ptrs[8];
    int quant[8][8];        int* quant_ptrs[8];
    for (int i=0; i<8; ++i) {
        shifted_ptrs[i] = shifted[i]; dct_ptrs[i] = dct[i]; quant_ptrs[i] = quant[i];
    }

    // Pipeline: Shift -> DCT -> Quant
    for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) shifted[i][j] = Bloc8x8[i][j] - 128;
    Calcul_DCT_Block(shifted_ptrs, dct_ptrs);
    quant_JPEG(dct_ptrs, quant_ptrs);

    // Count zero coefficients
    int zero_count = 0;
    for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) if (quant[i][j] == 0) ++zero_count;

    return static_cast<double>(zero_count) / 64.0;
}

void cCompression::RLE_Block(int **Img_Quant, int DC_precedent, signed char *Trame)
{
    if (!Img_Quant || !Trame) return;

    // Zigzag scan order
    static const int zigzag[64] = {
         0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
    };

    int linear_quant[64];
    for(int i=0; i<64; ++i) {
        linear_quant[i] = Img_Quant[zigzag[i]/8][zigzag[i]%8];
    }

    int pos = 0;
    // DC coefficient is differentially coded
    int dc_diff = linear_quant[0] - DC_precedent;
    Trame[pos++] = static_cast<signed char>(dc_diff);

    // AC coefficients are run-length encoded
    int zero_run = 0;
    for (int k = 1; k < 64; ++k) {
        if (linear_quant[k] == 0) {
            zero_run++;
        } else {
            while (zero_run > 15) { // Max run length is 15
                Trame[pos++] = 0x0F; // (15, 0)
                Trame[pos++] = 0x00;
                zero_run -= 16;
            }
            Trame[pos++] = static_cast<signed char>(zero_run);
            Trame[pos++] = static_cast<signed char>(linear_quant[k]);
            zero_run = 0;
        }
    }

    // End-of-Block marker. A block can take up to 1 + 63*2 + 2 bytes.
    if (pos + 1 < 129) {
        Trame[pos++] = 0x00; // (0, 0)
        Trame[pos++] = 0x00;
    }
}

void cCompression::RLE(signed int *Trame)
{
    if (!Trame || !mBuffer || mLargeur == 0 || mHauteur == 0) return;
    if ((mLargeur % 8) || (mHauteur % 8)) return;

    std::vector<signed char> out_stream;
    int previous_DC = 0;

    // Buffers for block processing
    int block[8][8];        int* block_ptrs[8];
    double dct[8][8];       double* dct_ptrs[8];
    int quant[8][8];        int* quant_ptrs[8];
    for (int i=0; i<8; ++i) {
        block_ptrs[i] = block[i]; dct_ptrs[i] = dct[i]; quant_ptrs[i] = quant[i];
    }

    for (unsigned int by = 0; by < mHauteur; by += 8) {
        for (unsigned int bx = 0; bx < mLargeur; bx += 8) {
            // Level-shift and copy block
            for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
                block[r][c] = static_cast<int>(mBuffer[by + r][bx + c]) - 128;
            }

            // DCT and Quantization
            Calcul_DCT_Block(block_ptrs, dct_ptrs);
            quant_JPEG(dct_ptrs, quant_ptrs);

            // RLE encoding for the block
            signed char block_trame[129] = {0};
            RLE_Block(quant_ptrs, previous_DC, block_trame);
            previous_DC = quant[0][0]; // Update previous DC for next block

            // Append DC first, then AC pairs until EOB is found.
            out_stream.push_back(block_trame[0]);
            for(int i=1; i + 1 < 129; i+=2) {
                out_stream.push_back(block_trame[i]);
                out_stream.push_back(block_trame[i+1]);
                if (block_trame[i] == 0 && block_trame[i+1] == 0) {
                    break;
                }
            }
        }
    }

    // Copy to the output integer array format
    Trame[0] = static_cast<int>(out_stream.size());
    for (size_t i = 0; i < out_stream.size(); ++i) {
        Trame[i + 1] = static_cast<int>(out_stream[i]);
    }
}

unsigned int cCompression::Histogramme(char *Trame, unsigned int Longueur_Trame, char *Donnee, double *Frequence)
{
    if (!Trame || !Donnee || !Frequence) return 0;

    unsigned int counts[256] = {0};
    for (unsigned int i = 0; i < Longueur_Trame; ++i) {
        counts[static_cast<unsigned char>(Trame[i])]++;
    }

    unsigned int nbSymboles = 0;
    for (int s = 0; s < 256; ++s) {
        if (counts[s] > 0) {
            Donnee[nbSymboles] = static_cast<char>(s);
            Frequence[nbSymboles] = static_cast<double>(counts[s]);
            nbSymboles++;
        }
    }
    return nbSymboles;
}

void cCompression::Compression_JPEG(int *Trame_RLE, const char *Nom_Fichier)
{
    if (!Trame_RLE || !Nom_Fichier) return;

    // 1. Convert integer RLE trame to a byte stream.
    unsigned int len = static_cast<unsigned int>(Trame_RLE[0]);
    std::vector<char> trame(len);
    for (unsigned int i = 0; i < len; ++i) {
        trame[i] = static_cast<char>(Trame_RLE[i + 1]);
    }

    // 2. Build frequency histogram and cache the Huffman table.
    char Donnee[256];
    double Frequence[256];
    unsigned int nbSym = Histogramme(trame.data(), len, Donnee, Frequence);
    storeHuffmanTable(Donnee, Frequence, nbSym);

    // 3. Generate Huffman codes.
    cHuffman h(trame.data(), len);
    h.HuffmanCodes(Donnee, Frequence, nbSym);
    std::map<char, std::string> codeTable;
    h.BuildTableCodes(codeTable);

    // 4. Encode the byte stream into a bitstream.
    std::vector<unsigned char> bitBytes;
    uint32_t payload_bits = 0;
    unsigned char current_byte = 0;
    int bit_pos = 7;

    for (char sym : trame) {
        const std::string& code = codeTable[sym];
        for (char bit : code) {
            if (bit == '1') {
                current_byte |= (1u << bit_pos);
            }
            bit_pos--;
            payload_bits++;
            if (bit_pos < 0) {
                bitBytes.push_back(current_byte);
                current_byte = 0;
                bit_pos = 7;
            }
        }
    }
    if (bit_pos != 7) { // Push the last partially filled byte
        bitBytes.push_back(current_byte);
    }

    // 5. Write the custom 'HUF1' file format.
    std::ofstream out(Nom_Fichier, std::ios::binary);
    if (!out) return;

    out.write("HUF1", 4); // Magic number

    uint16_t nbSym16 = static_cast<uint16_t>(nbSym); // Symbol count
    out.write(reinterpret_cast<const char*>(&nbSym16), sizeof(nbSym16));

    for (unsigned int i = 0; i < nbSym; ++i) { // Symbol table
        char sym = Donnee[i];
        uint32_t cnt = static_cast<uint32_t>(Frequence[i]);
        out.put(sym);
        out.write(reinterpret_cast<const char*>(&cnt), sizeof(cnt));
    }

    uint32_t payload_bytes = static_cast<uint32_t>(bitBytes.size()); // Payload info
    out.write(reinterpret_cast<const char*>(&payload_bytes), sizeof(payload_bytes));
    out.write(reinterpret_cast<const char*>(&payload_bits), sizeof(payload_bits));

    if (!bitBytes.empty()) { // Payload data
        out.write(reinterpret_cast<const char*>(bitBytes.data()), bitBytes.size());
    }

    // Optional width/height/quality trailer to avoid guessing during decompression.
    // Old files do not include this, so the reader treats it as optional.
    if (mLargeur != 0 && mHauteur != 0) {
        uint32_t w = mLargeur;
        uint32_t h = mHauteur;
        uint32_t q = cCompression::getQualiteGlobale();
        out.write(reinterpret_cast<const char*>(&w), sizeof(w));
        out.write(reinterpret_cast<const char*>(&h), sizeof(h));
        out.write(reinterpret_cast<const char*>(&q), sizeof(q));
    }

    out.close();
}
