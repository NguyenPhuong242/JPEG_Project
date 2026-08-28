/**
 * @file cDecompression.h
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Defines the cDecompression class for grayscale JPEG-like decompression.
 */

#ifndef JPEG_COMPRESSOR_CDECOMPRESSION_H
#define JPEG_COMPRESSOR_CDECOMPRESSION_H

#include "core/cCompression.h"

/**
 * @class cDecompression
 * @brief Reconstructs grayscale images from the custom Huffman/RLE stream.
 *
 * This class owns the decompression side of the simplified JPEG-like pipeline:
 * Huffman decoding, RLE parsing, dequantization, inverse DCT and level shifting.
 */
class cDecompression : public cCompression {
public:
    /**
     * @brief Default constructor.
     */
    cDecompression();

    /**
     * @brief Constructs a decompressor with optional known image properties.
     * @param largeur The image width.
     * @param hauteur The image height.
     * @param qualite The quality factor used by dequantization.
     * @param buffer Optional image buffer.
     */
    cDecompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned char **buffer = nullptr);

    /**
     * @brief Destructor.
     */
    ~cDecompression() override;

    /**
     * @brief Decompresses an image from a file and reconstructs the pixel data.
     * @param[in] Nom_Fichier_compresse The path to the compressed file.
     * @return A newly allocated 2D array (unsigned char**) containing the image data.
     * @note The caller is responsible for freeing the allocated memory.
     */
    unsigned char **Decompression_JPEG(const char *Nom_Fichier_compresse);
};

#endif // JPEG_COMPRESSOR_CDECOMPRESSION_H
