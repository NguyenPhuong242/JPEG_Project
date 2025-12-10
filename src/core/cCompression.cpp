/**
 * @file cCompression.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements the cCompression class for the grayscale JPEG pipeline.
 */

#include "core/cCompression.h"
#include <vector>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <cmath>

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

    // End-of-Block marker
    if (pos < 128) {
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
            signed char block_trame[128] = {0};
            RLE_Block(quant_ptrs, previous_DC, block_trame);
            previous_DC = quant[0][0]; // Update previous DC for next block

            // Append RLE data to the main stream until EOB is found
            for(int i=0; i<128; i+=2) {
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
    out.close();
}

unsigned char **cCompression::Decompression_JPEG(const char *Nom_Fichier_compresse)
{
    if (!Nom_Fichier_compresse) return nullptr;

    // 1. Read the entire compressed file into a byte vector.
    std::ifstream in(Nom_Fichier_compresse, std::ios::binary);
    if (!in) return nullptr;
    std::vector<unsigned char> filedata((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    // 2. Prepare the Huffman table.
    // This involves either parsing our custom 'HUF1' header or using a previously cached table.
    char Donnee[256];
    double Frequence[256];
    unsigned int nbSym = 0;
    const unsigned char *payload = nullptr;
    size_t payload_size = 0;
    uint32_t payload_bits = 0;

    if (filedata.size() >= 4 && filedata[0]=='H' && filedata[1]=='U' && filedata[2]=='F' && filedata[3]=='1') {
        // Custom 'HUF1' header found. Parse it to extract the Huffman table and payload info.
        size_t pos = 4;
        if (pos + sizeof(uint16_t) > filedata.size()) return nullptr;
        uint16_t nb = 0; std::memcpy(&nb, filedata.data()+pos, sizeof(nb)); pos += sizeof(nb);
        nbSym = nb;
        if (nbSym > 256) return nullptr;

        for (unsigned int i = 0; i < nbSym; ++i) {
            if (pos + 1 + sizeof(uint32_t) > filedata.size()) return nullptr;
            Donnee[i] = static_cast<char>(filedata[pos++]);
            uint32_t cnt = 0; std::memcpy(&cnt, filedata.data()+pos, sizeof(cnt)); pos += sizeof(cnt);
            Frequence[i] = static_cast<double>(cnt);
        }

        if (pos + sizeof(uint32_t) * 2 > filedata.size()) return nullptr;
        uint32_t payload_bytes = 0; std::memcpy(&payload_bytes, filedata.data()+pos, sizeof(payload_bytes)); pos += sizeof(payload_bytes);
        std::memcpy(&payload_bits, filedata.data()+pos, sizeof(payload_bits)); pos += sizeof(payload_bits);

        if (pos + payload_bytes > filedata.size()) return nullptr;
        payload = filedata.data() + pos;
        payload_size = payload_bytes;
        std::cerr << "[Decompression_JPEG] Parsed HUF1 header: nbSym=" << nbSym << " payload_bytes=" << payload_bytes << " payload_bits=" << payload_bits << "\n";
    } else {
        // No header. Fall back to a cached Huffman table if available.
        if (!cCompression::loadHuffmanTable(Donnee, Frequence, nbSym)) {
            return nullptr; // No table available. Cannot decompress.
        }
        payload = filedata.data();
        payload_size = filedata.size();
    }

    // 3. Build the Huffman decoding tree.
    cHuffman h;
    if (nbSym == 0) return nullptr;
    h.HuffmanCodes(Donnee, Frequence, nbSym);
    sNoeud *root = h.getRacine();
    if (!root) return nullptr;
    std::cerr << "[Decompression_JPEG] Built Huffman tree, root=" << root << "\n";

    // 4. Decode the bitstream payload into an RLE byte stream.
    std::vector<char> trameDec;
    uint64_t valid_bits = (payload_bits > 0) ? payload_bits : static_cast<uint64_t>(payload_size) * 8ULL;

    sNoeud *cursor = root;
    for (uint64_t bitIndex = 0; bitIndex < valid_bits; ++bitIndex) {
        int bit = 7 - static_cast<int>(bitIndex % 8ULL);
        unsigned char byte = payload[bitIndex / 8ULL];
        int val = ((byte >> bit) & 1);

        cursor = (val == 0) ? cursor->mgauche : cursor->mdroit;
        if (!cursor) return nullptr; // Invalid bitstream

        if (!cursor->mgauche && !cursor->mdroit) { // Leaf node found
            trameDec.push_back(cursor->mdonnee);
            cursor = root; // Reset for next symbol
        }
    }
    if (trameDec.empty()) return nullptr;
    std::cerr << "[Decompression_JPEG] Decoded " << trameDec.size() << " symbols into trameDec\n";

    // 5. Parse the RLE stream into 8x8 quantized blocks.
    static const int zigzag[64] = { // Zigzag scan order
         0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
    };

    std::vector<std::array<int,64>> quantBlocks;
    int previous_DC = 0;
    size_t p = 0;
    while (p < trameDec.size()) {
        std::array<int,64> q{};
        q.fill(0);

        signed char dc_diff = static_cast<signed char>(trameDec[p++]);
        int DC = static_cast<int>(dc_diff) + previous_DC;
        q[0] = DC;
        previous_DC = DC;

        int idx = 1;
        // Need two bytes (run,val) to decode an AC pair. Ensure bounds strictly.
        while ((p + 1) < trameDec.size() && idx < 64) {
            unsigned char run_u = static_cast<unsigned char>(trameDec[p++]);
            signed char val_s = static_cast<signed char>(trameDec[p++]);
            if (run_u == 0 && static_cast<unsigned char>(val_s) == 0) break; // EOB
            idx += static_cast<int>(run_u);
            if (idx >= 64) break;
            int zz = zigzag[idx];
            if (zz < 0 || zz >= 64) {
                std::cerr << "[Decompression_JPEG] Zigzag index out of range: idx=" << idx << " zz=" << zz << "\n";
                break;
            }
            q[zz] = static_cast<int>(val_s);
            idx++;
        }
        quantBlocks.push_back(q);
    }
    if (quantBlocks.empty()) return nullptr;
    std::cerr << "[Decompression_JPEG] Parsed " << quantBlocks.size() << " quant blocks\n";

    // 6. Reconstruct the image from the quantized blocks.
    size_t nblocks = quantBlocks.size();
    // Try to infer a rectangular block grid. Prefer a layout close to square.
    size_t blocks_w = static_cast<size_t>(std::floor(std::sqrt(static_cast<double>(nblocks))));
    if (blocks_w == 0) blocks_w = 1;
    while (blocks_w > 1 && (nblocks % blocks_w) != 0) {
        --blocks_w;
    }
    size_t blocks_h = (nblocks + blocks_w - 1) / blocks_w; // ceil division
    if (blocks_w * blocks_h < nblocks) blocks_h = (nblocks + blocks_w - 1) / blocks_w;
    std::cerr << "[Decompression_JPEG] Inferred block grid: blocks_w=" << blocks_w << " blocks_h=" << blocks_h << " (nblocks=" << nblocks << ")\n";

    size_t width = blocks_w * 8;
    size_t height = blocks_h * 8;
    this->mLargeur = static_cast<unsigned int>(width);
    this->mHauteur = static_cast<unsigned int>(height);

    unsigned char *buf = new unsigned char[width * height];
    unsigned char **rows = new unsigned char*[height];
    for (size_t r = 0; r < height; ++r) rows[r] = buf + r * width;

    // Buffers for block processing
    double dequantized_block[8][8]; double* dequantized_ptrs[8];
    int reconstructed_block[8][8]; int* reconstructed_ptrs[8];
    int quant_matrix[8][8]; int* quant_ptrs[8];
    for (int i=0; i<8; ++i) {
        dequantized_ptrs[i] = dequantized_block[i];
        reconstructed_ptrs[i] = reconstructed_block[i];
        quant_ptrs[i] = quant_matrix[i];
    }

    for (size_t i = 0; i < nblocks; ++i) {
        for (int k = 0; k < 64; ++k) {
            quant_matrix[k/8][k%8] = quantBlocks[i][k];
        }

        dequant_JPEG(quant_ptrs, dequantized_ptrs);
        Calcul_IDCT_Block(dequantized_ptrs, reconstructed_ptrs);

        size_t block_row = i / blocks_w;
        size_t block_col = i % blocks_w;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int val = reconstructed_block[r][c] + 128;
                val = (val < 0) ? 0 : (val > 255) ? 255 : val;
                rows[block_row * 8 + r][block_col * 8 + c] = static_cast<unsigned char>(val);
            }
        }
    }

    return rows;
}
