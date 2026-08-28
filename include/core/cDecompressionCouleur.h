/**
 * @file cDecompressionCouleur.h
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Defines the cDecompressionCouleur class for color image decompression.
 */

#ifndef JPEG_COMPRESSOR_CDECOMPRESSIONCOULEUR_H
#define JPEG_COMPRESSOR_CDECOMPRESSIONCOULEUR_H

#include "core/cDecompression.h"

/**
 * @class cDecompressionCouleur
 * @brief Reconstructs PPM color images from Y, Cb and Cr compressed streams.
 */
class cDecompressionCouleur : public cDecompression {
public:
    /**
     * @brief Default constructor.
     */
    cDecompressionCouleur();

    /**
     * @brief Constructs a color decompressor with optional known image properties.
     */
    cDecompressionCouleur(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned char **buffer = nullptr);

    /**
     * @brief Destructor.
     */
    ~cDecompressionCouleur() override;

    /**
     * @brief Decompresses three component Huffman files and reconstructs a PPM image.
     * @param[in] basename The base name used during compression.
     * @param[in] outppm The path for the output PPM file.
     * @return True on success, false on failure.
     */
    bool DecompressToPPM(const char *basename, const char *outppm);
};

#endif // JPEG_COMPRESSOR_CDECOMPRESSIONCOULEUR_H
