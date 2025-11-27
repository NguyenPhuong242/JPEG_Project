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
        unsigned int mLargeur;        /**< Output image width in pixels. */
        unsigned int mHauteur;        /**< Output image height in pixels. */
        unsigned char **mBuffer;      /**< Row pointer buffer (may be caller-owned). */
        unsigned int mQualite;        /**< Quality factor used for dequantization. */
        bool mOwnBuffer;              /**< Whether the instance allocated @p mBuffer. */

        /**
         * @brief Expand the RLE byte stream back into quantized 8x8 blocks.
         * @param trame Concatenated RLE bytes.
         * @param blocks Output collection receiving quantized blocks.
         */
        void Inverse_RLE(const std::vector<char> &trame,
                          std::vector<std::array<std::array<int,8>,8>> &blocks);
        /** @brief Release the owned buffer when the instance manages memory. */
        void releaseBuffer();

    public:
        /** @brief Construct an empty decompressor. */
        cDecompression();
        /**
         * @brief Construct a decompressor targeting specific dimensions.
         * @param largeur Image width in pixels.
         * @param hauteur Image height in pixels.
         * @param qualite Initial quality factor (mirrors compression quality).
         * @param buffer Optional caller-provided row buffer; ownership stays with caller.
         */
        cDecompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned char **buffer = nullptr);
        /** @brief Destroy the instance and release owned memory. */
        ~cDecompression();

        /** @brief Set the output image width. */
        void setLargeur(unsigned int largeur);
        /** @brief Set the output image height. */
        void setHauteur(unsigned int hauteur);
        /** @brief Set the quality factor used during inverse quantization. */
        void setQualite(unsigned int qualite);
        /** @brief Attach a row buffer managed by the caller. */
        void setBuffer(unsigned char **buffer);

        /** @brief Return the current output width. */
        unsigned int getLargeur() const;
        /** @brief Return the current output height. */
        unsigned int getHauteur() const;
        /** @brief Return the quality factor. */
        unsigned int getQualite() const;
        /** @brief Return the bound row buffer (may be nullptr). */
        unsigned char **getBuffer() const;

        /**
         * @brief Perform the full JPEG-like decompression pipeline.
         * @param Nom_Fichier_compresse Path to the compressed file.
         * @return Newly allocated row buffer on success, nullptr on failure.
         */
        unsigned char **Decompression_JPEG(const char *Nom_Fichier_compresse);
};

#endif // JPEG_COMPRESSOR_CDECOMPRESSION_H
