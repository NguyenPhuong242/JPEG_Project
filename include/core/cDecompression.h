//
// Created by GitHub Copilot on 27/11/2025.
//

#ifndef JPEG_COMPRESSOR_CDECOMPRESSION_H
#define JPEG_COMPRESSOR_CDECOMPRESSION_H

#include <vector>
#include <array>

/**
 * @brief High-level JPEG decompression helper.
 *
 * The class mirrors the compression interface but focuses solely on
 * reconstructing images from the Huffman-compressed bitstream produced by
 * cCompression. It performs Huffman decoding, inverse RLE, dequantization,
 * and IDCT to rebuild the spatial domain image.
 */
class cDecompression {
    private:
        unsigned int mLargeur;
        unsigned int mHauteur;
        unsigned char **mBuffer;
        unsigned int mQualite;
        bool mOwnBuffer;

        void Inverse_RLE(const std::vector<char> &trame,
                          std::vector<std::array<std::array<int,8>,8>> &blocks);
        void releaseBuffer();

    public:
        cDecompression();
        cDecompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned char **buffer = nullptr);
        ~cDecompression();

        void setLargeur(unsigned int largeur);
        void setHauteur(unsigned int hauteur);
        void setQualite(unsigned int qualite);
        void setBuffer(unsigned char **buffer);

        unsigned int getLargeur() const;
        unsigned int getHauteur() const;
        unsigned int getQualite() const;
        unsigned char **getBuffer() const;

        unsigned char **Decompression_JPEG(const char *Nom_Fichier_compresse);
};

#endif // JPEG_COMPRESSOR_CDECOMPRESSION_H
