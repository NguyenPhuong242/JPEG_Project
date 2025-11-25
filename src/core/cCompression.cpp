#include "core/cCompression.h"
#include <vector>
#include <cstring>
#include "dct/dct.h"
#include "quantification/quantification.h"

/**
 * @file cCompression.cpp
 * @brief Implementation of the cCompression class.
 */

/**
 * Default constructor. Initializes members to default values.
 * Sets width and height to 0, quality to 50, and buffer to nullptr.
 */
cCompression::cCompression() {
    this->mLargeur = 0;
    this->mHauteur = 0;
    this->mQualite = 50;
    this->mBuffer = nullptr;
}

/**
 * Construct a compressor tied to an image size and optional quality.
 * @param largeur image width
 * @param hauteur image height
 * @param qualite initial quality (0-100, JPEG-style)
 * @param buffer optional image buffer (row pointers). If nullptr, no image is owned.
 */
cCompression::cCompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite, unsigned char **buffer){
    this->mLargeur = largeur;
    this->mHauteur = hauteur;
    this->mQualite = qualite;
    this->mBuffer = buffer;
}
/**
 * Destructor.
 */
cCompression::~cCompression() {
}

/**
 * Setters for image dimensions and quality.
 * @param largeur image width
 * @param hauteur image height
 * @param qualite quality (0-100)
 */
void cCompression::setLargeur(unsigned int largeur) {
    this->mLargeur = largeur;
}

void cCompression::setHauteur(unsigned int hauteur) {
    this->mHauteur = hauteur;
}

void cCompression::setQualite(unsigned int qualite) {
    this->mQualite = qualite;
}

void cCompression::setBuffer(unsigned char **buffer) {
    this->mBuffer = buffer;
}

/**
 * Getters for image dimensions and quality.
 * @return corresponding member value
 */
unsigned int cCompression::getLargeur() const {
    return this->mLargeur;
}

unsigned int cCompression::getHauteur() const {
    return this->mHauteur;
}

unsigned int cCompression::getQualite() const {
    return this->mQualite;
}

unsigned char** cCompression::getBuffer() const {
    return this->mBuffer;
}

/**
 * Getters and setters for the global quality parameter used by quantification helpers.
 */
static unsigned int gQualiteGlobale = 50;

/**
 * Get the global quality parameter.
 * @return current global quality (1..100)
 */
unsigned int cCompression::getQualiteGlobale() {
    return gQualiteGlobale;
}

/**
 * Set the global quality parameter.
 * @param qualite new global quality (clamped to 1..100)
 */
void cCompression::setQualiteGlobale(unsigned int qualite) {
    // clamp to 1..100
    if (qualite < 1) qualite = 1;
    if (qualite > 100) qualite = 100;
    gQualiteGlobale = qualite;
}

/**
 * Encode a single 8x8 quantized block with run-length encoding (RLE).
 * This version does NOT rely on a static zigzag table — it computes
 * zigzag traversal dynamically.
 */
void cCompression::RLE_Block(int **Img_Quant, int DC_precedent, char *Trame) {
    if (!Img_Quant || !Trame) return;

    constexpr int N = 8;
    constexpr int OUT_CAP = 128;
    int pos = 0;

    // DC coefficient: differential coding
    int DC = Img_Quant[0][0] - DC_precedent;
    Trame[pos++] = static_cast<char>(DC);

    int zero_run = 0;

    // Generate zigzag order dynamically (instead of static table)
    int i = 0, j = 0;
    bool up = true;

    for (int k = 1; k < N * N && pos + 2 < OUT_CAP; ++k) {
        // Move to next zigzag position
        if (up) {
            if (j == N - 1) { i++; up = false; }
            else if (i == 0) { j++; up = false; }
            else { i--; j++; }
        } else {
            if (i == N - 1) { j++; up = true; }
            else if (j == 0) { i++; up = true; }
            else { i++; j--; }
        }

        int v = Img_Quant[i][j];
        if (v == 0) { 
            ++zero_run;
            continue;
        }

        // Limit run to 255 (1 byte)
        if (zero_run > 255) zero_run = 255;
        Trame[pos++] = static_cast<char>(zero_run);
        Trame[pos++] = static_cast<char>(v);
        zero_run = 0;
    }

    // End-of-block marker
    if (pos < OUT_CAP) Trame[pos++] = static_cast<char>(0);

    // Fill the rest of buffer with zeros
    std::memset(Trame + pos, 0, OUT_CAP - pos);
}


/**
 * Concatenate block trames of the whole image into an integer trame.
 * The pedagogic convention used here stores the concatenated byte count
 * in Trame[0] and the bytes in Trame[1..N]. This is a convenience
 * wrapper matching the exercise's expected signature.
 * @param Trame integer buffer provided by caller
 */
void cCompression::RLE(int *Trame) {
    if (!Trame || !mBuffer || mLargeur == 0 || mHauteur == 0) return;
    if ((mLargeur % 8) || (mHauteur % 8)) return;

    std::vector<unsigned char> out;
    out.reserve((mLargeur / 8) * (mHauteur / 8) * 20);

    int block[8][8];
    double dct[8][8];
    int quant[8][8];

    int *block_rows[8], *quant_rows[8];
    double *dct_rows[8];
    for (int i = 0; i < 8; ++i) {
        block_rows[i] = block[i];
        dct_rows[i] = dct[i];
        quant_rows[i] = quant[i];
    }

    int previous_DC = 0;

    for (unsigned int by = 0; by < mHauteur; by += 8) {
        for (unsigned int bx = 0; bx < mLargeur; bx += 8) {
            // Copy pixels (level shift)
            for (int r = 0; r < 8; ++r)
                for (int c = 0; c < 8; ++c)
                    block[r][c] = static_cast<int>(mBuffer[by + r][bx + c]) - 128;

            // DCT + Quantization
            Calcul_DCT_Block(block_rows, dct_rows);
            quant_JPEG(dct_rows, quant_rows);

            // RLE encode block
            char blockTrame[128] = {0};
            RLE_Block(quant_rows, previous_DC, blockTrame);

            previous_DC = quant[0][0]; // save DC for next block

            // Append until EOB
            for (size_t i = 0; i < sizeof(blockTrame); ++i) {
                unsigned char b = static_cast<unsigned char>(blockTrame[i]);
                out.push_back(b);
                if (b == 0) break;
            }
        }
    }

    // Write output
    Trame[0] = static_cast<int>(out.size());
    for (int i = 0; i < Trame[0]; ++i)
        Trame[i + 1] = static_cast<int>(out[i]);
    
}


