#include "core/cCompression.h"
#include <vector>
#include <cstring>
#include <fstream>
#include <array>
#include <map>
#include <string>
#include <functional>
#include <cmath>

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
cCompression::cCompression()
{
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
cCompression::cCompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite, unsigned char **buffer)
{
    this->mLargeur = largeur;
    this->mHauteur = hauteur;
    this->mQualite = qualite;
    this->mBuffer = buffer;
}
/**
 * Destructor.
 */
cCompression::~cCompression()
{
}

/**
 * Setters for image dimensions and quality.
 * @param largeur image width
 * @param hauteur image height
 * @param qualite quality (0-100)
 */
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

/**
 * Getters for image dimensions and quality.
 * @return corresponding member value
 */
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

/**
 * Getters and setters for the global quality parameter used by quantification helpers.
 */
static unsigned int gQualiteGlobale = 50;
static char   gHuffSymbols[256];
static double gHuffFreqs[256];
static unsigned int gHuffCount = 0;

/**
 * Get the global quality parameter.
 * @return current global quality (1..100)
 */
unsigned int cCompression::getQualiteGlobale()
{
    return gQualiteGlobale;
}

/**
 * Set the global quality parameter.
 * @param qualite new global quality (clamped to 1..100)
 */
void cCompression::setQualiteGlobale(unsigned int qualite)
{
    // clamp to 1..100
    if (qualite < 1)
        qualite = 1;
    if (qualite > 100)
        qualite = 100;
    gQualiteGlobale = qualite;
}

/**
 * Encode a single 8x8 quantized block with run-length encoding (RLE).
 * This version does NOT rely on a static zigzag table — it computes
 * zigzag traversal dynamically.
 */
void cCompression::RLE_Block(int **Img_Quant, int DC_precedent, signed char *Trame)
{
    if (!Img_Quant || !Trame)
        return;

    constexpr int N = 8;
    constexpr int OUT_CAP = 128;

    int pos = 0;

    // DC coefficient: differential coding
    int DC = Img_Quant[0][0] - DC_precedent;
    Trame[pos++] = static_cast<signed char>(DC);

    int zero_run = 0;

    // Zigzag traversal, generated dynamically
    int i = 0;
    int j = 0;
    bool up = true;

    for (int k = 1; k < N * N && pos + 2 < OUT_CAP; ++k)
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

        int v = Img_Quant[i][j];

        if (v == 0)
        {
            ++zero_run;
            continue;
        }

        if (zero_run > 255)
            zero_run = 255;

        Trame[pos++] = static_cast<signed char>(zero_run); // run
        Trame[pos++] = static_cast<signed char>(v);        // coeff

        zero_run = 0;
    }

    // End-of-block marker: the special pair (0, 0)
    if (pos + 2 <= OUT_CAP)
    {
        Trame[pos++] = static_cast<signed char>(0); // run = 0
        Trame[pos++] = static_cast<signed char>(0); // coeff = 0 (EOB)
    }

    // (Optional) pad the rest of the buffer; not logically used
    if (pos < OUT_CAP)
    {
        std::memset(Trame + pos, 0, OUT_CAP - pos);
    }
}

/**
 * Concatenate block trames of the whole image into an integer trame.
 * The pedagogic convention used here stores the concatenated byte count
 * in Trame[0] and the bytes in Trame[1..N]. This is a convenience
 * wrapper matching the exercise's expected signature.
 * @param Trame integer buffer provided by caller
 */
void cCompression::RLE(signed int *Trame)
{
    if (!Trame || !mBuffer || mLargeur == 0 || mHauteur == 0)
        return;
    if ((mLargeur % 8) || (mHauteur % 8))
        return;

    std::vector<signed char> out;
    out.reserve((mLargeur / 8) * (mHauteur / 8) * 20);

    int block[8][8];
    double dct[8][8];
    int quant[8][8];

    int *block_rows[8];
    double *dct_rows[8];
    int *quant_rows[8];

    for (int i = 0; i < 8; ++i)
    {
        block_rows[i] = block[i];
        dct_rows[i] = dct[i];
        quant_rows[i] = quant[i];
    }

    int previous_DC = 0;

    for (unsigned int by = 0; by < mHauteur; by += 8)
    {
        for (unsigned int bx = 0; bx < mLargeur; bx += 8)
        {

            // Copy 8x8 block with level shift
            for (int r = 0; r < 8; ++r)
                for (int c = 0; c < 8; ++c)
                    block[r][c] = static_cast<int>(mBuffer[by + r][bx + c]) - 128;

            // DCT + quantization
            Calcul_DCT_Block(block_rows, dct_rows);
            quant_JPEG(dct_rows, quant_rows);

            // RLE for this block
            signed char blockTrame[128] = {0};
            RLE_Block(quant_rows, previous_DC, blockTrame);

            previous_DC = quant[0][0]; // save DC for next block

            // Append DC
            out.push_back(blockTrame[0]);

            // Append (run, coeff) pairs until we see (0,0), inclusive
            int pos = 1;
            while (pos + 1 < 128)
            {
                signed char run = blockTrame[pos++];
                signed char val = blockTrame[pos++];

                out.push_back(run);
                out.push_back(val);

                if (run == 0 && val == 0)
                {
                    break; // end-of-block
                }
            }
        }
    }

    // Export to integer trame, preserving sign
    Trame[0] = static_cast<int>(out.size());
    for (int i = 0; i < Trame[0]; ++i)
    {
        Trame[i + 1] = static_cast<int>(out[i]);
    }
}

unsigned int cCompression::Histogramme(char *Trame, unsigned int Longueur_Trame, char *Donnee, double *Frequence)
{
    if (!Trame || !Donnee || !Frequence)
        return 0;

    // Count occurrences for each possible byte value
    unsigned int counts[256] = {0};

    for (unsigned int i = 0; i < Longueur_Trame; ++i)
    {
        unsigned char sym = static_cast<unsigned char>(Trame[i]);
        counts[sym]++;
    }

    // Build Donnee / Frequence arrays only for used symbols
    unsigned int nbSymboles = 0;
    for (int s = 0; s < 256; ++s)
    {
        if (counts[s] != 0)
        {
            Donnee[nbSymboles] = static_cast<char>(s);
            Frequence[nbSymboles] = static_cast<double>(counts[s]); // or /Longueur_Trame
            ++nbSymboles;
        }
    }

    return nbSymboles;
}

void cCompression::Compression_JPEG(int *Trame_RLE, const char *Nom_Fichier)
{
    if (!Trame_RLE || !Nom_Fichier)
        return;

    // 1) length is stored in Trame_RLE[0]
    unsigned int len = static_cast<unsigned int>(Trame_RLE[0]);

    // 2) convert to char trame (the actual bytes)
    std::vector<char> trame(len);
    for (unsigned int i = 0; i < len; ++i)
    {
        trame[i] = static_cast<char>(Trame_RLE[i + 1]);
    }

    // 3) build histogram
    char Donnee[256];
    double Frequence[256];
    unsigned int nbSym =
        Histogramme(trame.data(), len, Donnee, Frequence);

    // keep this table globally for decompression
    gHuffCount = nbSym;
    for (unsigned int i = 0; i < nbSym; ++i)
    {
        gHuffSymbols[i] = Donnee[i];
        gHuffFreqs[i] = Frequence[i];
    }

    // 4) build Huffman tree
    cHuffman h(trame.data(), len);
    h.HuffmanCodes(Donnee, Frequence, nbSym);

    // 5) build code table
    std::map<char, std::string> codeTable;
    h.BuildTableCodes(codeTable); // or your own equivalent

    // 6) open output file and write Huffman bitstream
    std::ofstream out(Nom_Fichier, std::ios::binary);
    if (!out)
    {
        std::cerr << "Cannot open " << Nom_Fichier << " for writing\n";
        return;
    }

    unsigned char currentByte = 0;
    int bitPos = 7;

    for (unsigned int i = 0; i < len; ++i)
    {
        char sym = trame[i];
        const std::string &code = codeTable[sym];

        for (char b : code)
        {
            int bit = (b == '1') ? 1 : 0;
            if (bit)
            {
                currentByte |= (1u << bitPos);
            }
            --bitPos;

            if (bitPos < 0)
            {
                out.put(static_cast<char>(currentByte));
                currentByte = 0;
                bitPos = 7;
            }
        }
    }

    if (bitPos != 7)
    {
        out.put(static_cast<char>(currentByte));
    }

    out.close();
}

// ===== Helpers for Huffman decoding (free functions, not class members) ====

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
            { // defensive
                node = root;
                continue;
            }

            if (!node->mgauche && !node->mdroit)
            {
                trame.push_back(node->mdonnee); // decoded symbol
                node = root;
            }
        }
    }

    return trame;
}

void cCompression::Inverse_RLE(const std::vector<char> &trame,
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
        // new 8x8 block, all zeros
        std::array<std::array<int, 8>, 8> block{};
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                block[r][c] = 0;

        // zigzag state
        int i = 0, j = 0;
        bool up = true;
        int k = 0; // coefficient index in zigzag order (0..63)

        // DC
        int DCdiff = static_cast<signed char>(trame[idx++]);
        int DC = previous_DC + DCdiff;
        previous_DC = DC;
        block[0][0] = DC;
        k = 1;

        // decode AC coefficients until (0,0)
        while (idx + 1 <= static_cast<int>(trame.size()) && k < N * N)
        {
            signed char run = trame[idx++];
            signed char coeff = trame[idx++];

            if (run == 0 && coeff == 0)
            {
                // EOB -> remaining coefficients are zeros
                break;
            }

            // advance 'run' zero positions in zigzag
            for (int step = 0; step < run && k < N * N; ++step)
            {
                // move to next zigzag position
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

            // now place the non-zero coeff at current (i,j)
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


unsigned char **cCompression::Decompression_JPEG(const char *Nom_Fichier_compresse)
{
    if (!Nom_Fichier_compresse) return nullptr;

    // We must have compressed something before (Huffman table known)
    if (gHuffCount == 0) {
        std::cerr << "No Huffman table available. Call Compression_JPEG first.\n";
        return nullptr;
    }

    // 1) rebuild the same Huffman tree as in Compression_JPEG
    cHuffman h;
    h.HuffmanCodes(gHuffSymbols, gHuffFreqs, gHuffCount);

    // 2) Huffman decode file -> RLE bytes
    std::vector<char> rleBytes = HuffmanDecodeFile(Nom_Fichier_compresse, h);
    if (rleBytes.empty()) return nullptr;

    // 3) inverse RLE -> quantized 8x8 blocks
    std::vector<std::array<std::array<int,8>,8>> qBlocks;
    Inverse_RLE(rleBytes, qBlocks);

    const unsigned int nbBlocks = static_cast<unsigned int>(qBlocks.size());
    if (nbBlocks == 0) return nullptr;

    // 4) image is square, each block = 8x8
    unsigned int blocksPerSide =
        static_cast<unsigned int>(std::sqrt(static_cast<double>(nbBlocks)) + 0.5);

    mLargeur = blocksPerSide * 8;
    mHauteur = blocksPerSide * 8;

    // allocate image buffer
    mBuffer = new unsigned char*[mHauteur];
    for (unsigned int y = 0; y < mHauteur; ++y)
        mBuffer[y] = new unsigned char[mLargeur];

    // temp arrays for dequant + IDCT
    int    quant[8][8];
    double dct[8][8];
    int    spat[8][8];

    int    *quant_rows[8];
    double *dct_rows[8];
    int    *spat_rows[8];

    for (int i = 0; i < 8; ++i) {
        quant_rows[i] = quant[i];
        dct_rows[i]   = dct[i];
        spat_rows[i]  = spat[i];
    }

    unsigned int b = 0;
    for (unsigned int by = 0; by < mHauteur; by += 8) {
        for (unsigned int bx = 0; bx < mLargeur; bx += 8) {

            // copy quantized block
            for (int r = 0; r < 8; ++r)
                for (int c = 0; c < 8; ++c)
                    quant[r][c] = qBlocks[b][r][c];

            // inverse quantization + inverse DCT
            dequant_JPEG(quant_rows, dct_rows);
            Calcul_IDCT_Block(dct_rows, spat_rows);

            // level shift + clamp
            for (int r = 0; r < 8; ++r) {
                for (int c = 0; c < 8; ++c) {
                    int val = spat[r][c] + 128;
                    if (val < 0)   val = 0;
                    if (val > 255) val = 255;
                    mBuffer[by + r][bx + c] =
                        static_cast<unsigned char>(val);
                }
            }

            ++b;
        }
    }

    return mBuffer;
}

