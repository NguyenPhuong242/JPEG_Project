/**
 * @file cDecompression.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements grayscale JPEG-like decompression.
 */

#include "core/cDecompression.h"

#include "dct/dct.h"
#include "quantification/quantification.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <new>
#include <vector>

cDecompression::cDecompression() : cCompression()
{
}

cDecompression::cDecompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite, unsigned char **buffer)
    : cCompression(largeur, hauteur, qualite, buffer)
{
}

cDecompression::~cDecompression()
{
}

unsigned char **cDecompression::Decompression_JPEG(const char *Nom_Fichier_compresse)
{
    if (!Nom_Fichier_compresse) return nullptr;

    std::ifstream in(Nom_Fichier_compresse, std::ios::binary);
    if (!in) return nullptr;
    std::vector<unsigned char> filedata((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    char Donnee[256];
    double Frequence[256];
    unsigned int nbSym = 0;
    const unsigned char *payload = nullptr;
    size_t payload_size = 0;
    uint32_t payload_bits = 0;

    if (filedata.size() >= 4 && filedata[0] == 'H' && filedata[1] == 'U' && filedata[2] == 'F' && filedata[3] == '1') {
        size_t pos = 4;
        if (pos + sizeof(uint16_t) > filedata.size()) return nullptr;
        uint16_t nb = 0;
        std::memcpy(&nb, filedata.data() + pos, sizeof(nb));
        pos += sizeof(nb);
        nbSym = nb;
        if (nbSym > 256) return nullptr;

        for (unsigned int i = 0; i < nbSym; ++i) {
            if (pos + 1 + sizeof(uint32_t) > filedata.size()) return nullptr;
            Donnee[i] = static_cast<char>(filedata[pos++]);
            uint32_t cnt = 0;
            std::memcpy(&cnt, filedata.data() + pos, sizeof(cnt));
            pos += sizeof(cnt);
            Frequence[i] = static_cast<double>(cnt);
        }

        if (pos + sizeof(uint32_t) * 2 > filedata.size()) return nullptr;
        uint32_t payload_bytes = 0;
        std::memcpy(&payload_bytes, filedata.data() + pos, sizeof(payload_bytes));
        pos += sizeof(payload_bytes);
        std::memcpy(&payload_bits, filedata.data() + pos, sizeof(payload_bits));
        pos += sizeof(payload_bits);
        if (static_cast<uint64_t>(payload_bits) > static_cast<uint64_t>(payload_bytes) * 8ULL) return nullptr;

        if (pos + payload_bytes > filedata.size()) return nullptr;
        payload = filedata.data() + pos;
        payload_size = payload_bytes;

        size_t trailer_pos = pos + payload_bytes;
        if (trailer_pos + sizeof(uint32_t) * 2 <= filedata.size()) {
            uint32_t w = 0, h = 0;
            std::memcpy(&w, filedata.data() + trailer_pos, sizeof(uint32_t));
            std::memcpy(&h, filedata.data() + trailer_pos + sizeof(uint32_t), sizeof(uint32_t));
            if (w != 0 && h != 0) {
                setLargeur(w);
                setHauteur(h);
            }
        }
        if (trailer_pos + sizeof(uint32_t) * 3 <= filedata.size()) {
            uint32_t q = 0;
            std::memcpy(&q, filedata.data() + trailer_pos + sizeof(uint32_t) * 2, sizeof(uint32_t));
            if (q >= 1 && q <= 100) {
                setQualite(q);
                cCompression::setQualiteGlobale(q);
            }
        }
    } else {
        if (!cCompression::loadHuffmanTable(Donnee, Frequence, nbSym)) {
            return nullptr;
        }
        payload = filedata.data();
        payload_size = filedata.size();
    }

    if (payload_bits == 0 && payload_size == 0) {
        return nullptr;
    }

    cHuffman h;
    if (nbSym == 0) return nullptr;
    h.HuffmanCodes(Donnee, Frequence, nbSym);
    sNoeud *root = h.getRacine();
    if (!root) return nullptr;

    std::vector<char> trameDec;
    uint64_t valid_bits = (payload_bits > 0) ? payload_bits : static_cast<uint64_t>(payload_size) * 8ULL;

    if (!root->mgauche && !root->mdroit) {
        trameDec.assign(static_cast<size_t>(valid_bits), root->mdonnee);
    } else {
        sNoeud *cursor = root;
        for (uint64_t bitIndex = 0; bitIndex < valid_bits; ++bitIndex) {
            int bit = 7 - static_cast<int>(bitIndex % 8ULL);
            unsigned char byte = payload[bitIndex / 8ULL];
            int val = ((byte >> bit) & 1);

            cursor = (val == 0) ? cursor->mgauche : cursor->mdroit;
            if (!cursor) return nullptr;

            if (!cursor->mgauche && !cursor->mdroit) {
                trameDec.push_back(cursor->mdonnee);
                cursor = root;
            }
        }
    }
    if (trameDec.empty()) return nullptr;

    static const int zigzag[64] = {
         0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
    };

    unsigned int stored_width = getLargeur();
    unsigned int stored_height = getHauteur();
    size_t expected_blocks = 0;
    if (stored_width != 0 && stored_height != 0 &&
        (stored_width % 8) == 0 && (stored_height % 8) == 0) {
        expected_blocks = static_cast<size_t>(stored_width / 8) * static_cast<size_t>(stored_height / 8);
    }

    std::vector<std::array<int, 64>> quantBlocks;
    int previous_DC = 0;
    size_t p = 0;
    while (p < trameDec.size() && (expected_blocks == 0 || quantBlocks.size() < expected_blocks)) {
        std::array<int, 64> q{};
        q.fill(0);

        signed char dc_diff = static_cast<signed char>(trameDec[p++]);
        int DC = static_cast<int>(dc_diff) + previous_DC;
        q[0] = DC;
        previous_DC = DC;

        int idx = 1;
        bool saw_eob = false;
        while ((p + 1) < trameDec.size()) {
            unsigned char run_u = static_cast<unsigned char>(trameDec[p++]);
            signed char val_s = static_cast<signed char>(trameDec[p++]);
            if (run_u == 0 && val_s == 0) {
                saw_eob = true;
                break;
            }
            if (run_u == 15 && val_s == 0) {
                idx += 16;
                if (idx > 64) return nullptr;
                continue;
            }
            idx += static_cast<int>(run_u);
            if (idx >= 64) return nullptr;
            int zz = zigzag[idx];
            if (zz < 0 || zz >= 64) {
                return nullptr;
            }
            q[zz] = static_cast<int>(val_s);
            idx++;
        }
        if (!saw_eob) return nullptr;
        quantBlocks.push_back(q);
    }
    if (quantBlocks.empty()) return nullptr;
    if (expected_blocks != 0 && quantBlocks.size() != expected_blocks) return nullptr;
    if (expected_blocks != 0 && p != trameDec.size()) return nullptr;

    size_t nblocks = quantBlocks.size();
    size_t blocks_w = 0, blocks_h = 0;
    if (getLargeur() != 0 && getHauteur() != 0) {
        blocks_w = getLargeur() / 8;
        blocks_h = getHauteur() / 8;
    } else {
        blocks_w = static_cast<size_t>(std::floor(std::sqrt(static_cast<double>(nblocks))));
        if (blocks_w == 0) blocks_w = 1;
        while (blocks_w > 1 && (nblocks % blocks_w) != 0) {
            --blocks_w;
        }
        blocks_h = (nblocks + blocks_w - 1) / blocks_w;
        if (blocks_w * blocks_h < nblocks) blocks_h = (nblocks + blocks_w - 1) / blocks_w;
        setLargeur(static_cast<unsigned int>(blocks_w * 8));
        setHauteur(static_cast<unsigned int>(blocks_h * 8));
    }

    if (blocks_w == 0 || blocks_h == 0) return nullptr;

    size_t width = static_cast<size_t>(getLargeur());
    size_t height = static_cast<size_t>(getHauteur());
    if (width == 0 || height == 0) return nullptr;
    if (height > std::numeric_limits<size_t>::max() / width) return nullptr;
    size_t pixel_count = width * height;

    unsigned char *buf = new (std::nothrow) unsigned char[pixel_count]();
    if (!buf) return nullptr;
    unsigned char **rows = new (std::nothrow) unsigned char*[height];
    if (!rows) {
        delete[] buf;
        return nullptr;
    }
    for (size_t r = 0; r < height; ++r) rows[r] = buf + r * width;

    double dequantized_block[8][8];
    double* dequantized_ptrs[8];
    int reconstructed_block[8][8];
    int* reconstructed_ptrs[8];
    int quant_matrix[8][8];
    int* quant_ptrs[8];
    for (int i = 0; i < 8; ++i) {
        dequantized_ptrs[i] = dequantized_block[i];
        reconstructed_ptrs[i] = reconstructed_block[i];
        quant_ptrs[i] = quant_matrix[i];
    }

    for (size_t i = 0; i < nblocks; ++i) {
        for (int k = 0; k < 64; ++k) {
            quant_matrix[k / 8][k % 8] = quantBlocks[i][k];
        }

        dequant_JPEG(quant_ptrs, dequantized_ptrs);
        Calcul_IDCT_Block(dequantized_ptrs, reconstructed_ptrs);

        size_t block_row = i / blocks_w;
        size_t block_col = i % blocks_w;
        if (block_row >= blocks_h || block_col >= blocks_w) continue;

        for (int r = 0; r < 8; ++r) {
            size_t ry = block_row * 8 + static_cast<size_t>(r);
            if (ry >= height) continue;
            for (int c = 0; c < 8; ++c) {
                size_t rx = block_col * 8 + static_cast<size_t>(c);
                if (rx >= width) continue;
                int val = reconstructed_block[r][c] + 128;
                val = (val < 0) ? 0 : (val > 255) ? 255 : val;
                rows[ry][rx] = static_cast<unsigned char>(val);
            }
        }
    }

    return rows;
}
