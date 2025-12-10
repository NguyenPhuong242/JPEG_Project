/**
 * @file cCompressionCouleur.h
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Defines the cCompressionCouleur class for color image compression.
 */

#ifndef JPEG_COMPRESSOR_CCOMPRESSIONCOULEUR_H
#define JPEG_COMPRESSOR_CCOMPRESSIONCOULEUR_H

#include "core/cCompression.h"

/**
 * @class cCompressionCouleur
 * @brief Extends cCompression to handle color image compression (PPM format).
 *
 * This class manages the pipeline for compressing and decompressing P6 (binary)
 * PPM color images. It handles reading the PPM file, converting from RGB to
 * YCbCr color space, performing chroma subsampling (4:4:4, 4:2:2, 4:2:0), and
 * then uses the base cCompression functionality to compress each color component
 * (Y, Cb, Cr) into a separate file.
 */
class cCompressionCouleur : public cCompression {
private:
    /** @brief Chroma subsampling factor for the horizontal direction. (e.g., 2 for 4:2:2 and 4:2:0). */
    unsigned int mSubsamplingH;
    /** @brief Chroma subsampling factor for the vertical direction. (e.g., 2 for 4:2:0). */
    unsigned int mSubsamplingV;

public:
    /**
     * @brief Default constructor.
     */
    cCompressionCouleur();

    /**
     * @brief Constructs a cCompressionCouleur object with given parameters.
     * @param largeur The width of the image.
     * @param hauteur The height of the image.
     * @param qualite The quality setting for compression (0-100).
     * @param subsamplingH Horizontal subsampling factor.
     * @param subsamplingV Vertical subsampling factor.
     * @param buffer Optional pointer to an existing image buffer.
     */
    cCompressionCouleur(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned int subsamplingH = 1, unsigned int subsamplingV = 1, unsigned char **buffer=nullptr);

    /**
     * @brief Destructor.
     */
    ~cCompressionCouleur();

    /**
     * @brief Compresses a PPM (P6) image into three component Huffman files and a metadata file.
     *
     * The process involves reading the PPM, converting RGB to YCbCr, optionally
     * subsampling the Cb and Cr channels, and compressing each channel independently.
     *
     * @param[in] ppmPath Path to the input PPM (P6) file.
     * @param[in] basename The base name for the output files (e.g., "image").
     *                     This will produce "image_Y.huff", "image_Cb.huff", etc.
     * @param[in] qual The quality setting (1-100) for the JPEG quantization stage.
     * @param[in] subsamplingMode The chroma subsampling mode. Supported values: 444, 422, 420.
     * @return True on success, false on failure.
     */
    bool CompressPPM(const char *ppmPath, const char *basename, unsigned int qual, unsigned int subsamplingMode = 444);

    /**
     * @brief Decompresses three component Huffman files and reconstructs a PPM (P6) image.
     *
     * This method reads the metadata file to determine image dimensions and subsampling,
     * then decompresses each color channel, upsamples the chroma channels if necessary,
     * converts YCbCr back to RGB, and writes the output PPM file.
     *
     * @param[in] basename The base name used during compression (e.g., "image").
     *                     It expects to find "image.meta", "image_Y.huff", etc.
     * @param[in] outppm The path for the output PPM file to be created.
     * @return True on success, false on failure.
     */
    bool DecompressToPPM(const char *basename, const char *outppm);

    /**
     * @brief Sets the horizontal chroma subsampling factor.
     * @param subsamplingH The horizontal factor (e.g., 1, 2).
     */
    void setSubsamplingH(unsigned int subsamplingH);

    /**
     * @brief Sets the vertical chroma subsampling factor.
     * @param subsamplingV The vertical factor (e.g., 1, 2).
     */
    void setSubsamplingV(unsigned int subsamplingV);

    /**
     * @brief Gets the horizontal chroma subsampling factor.
     * @return The horizontal factor.
     */
    unsigned int getSubsamplingH() const;

    /**
     * @brief Gets the vertical chroma subsampling factor.
     * @return The vertical factor.
     */
    unsigned int getSubsamplingV() const;
};

#endif // JPEG_COMPRESSOR_CCOMPRESSIONCOULEUR_H
