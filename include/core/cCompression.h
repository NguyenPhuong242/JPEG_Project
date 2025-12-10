/**
 * @file cCompression.h
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Defines the cCompression class for the core grayscale JPEG compression pipeline.
 */

#ifndef JPEG_COMPRESSOR_CCOMPRESSION_H
#define JPEG_COMPRESSOR_CCOMPRESSION_H

#include "cHuffman.h"

/**
 * @class cCompression
 * @brief Manages the core pipeline for a simplified grayscale JPEG-like compression.
 *
 * This class encapsulates the main stages of the compression and decompression
 * process for grayscale images. It handles Run-Length Encoding (RLE), Huffman
 * coding integration, quality metrics (EQM, compression ratio), and file I/O for
 * the custom compressed format.
 */
class cCompression {
private:
    /** @brief The width of the image in pixels. */
    unsigned int mLargeur;
    /** @brief The height of the image in pixels. */
    unsigned int mHauteur;
    /** @brief A buffer of row pointers for the raw image data.
     *  @note The class does not own the data buffer itself, only the pointers, unless allocated by it.
     */
    unsigned char **mBuffer;
    /** @brief The per-instance quality setting (0-100), not directly used by static helpers. */
    unsigned int mQualite;

public:
    /**
     * @brief Default constructor. Initializes members to zero/nullptr.
     */
    cCompression();

    /**
     * @brief Constructs a cCompression object with given parameters.
     * @param largeur The width of the image.
     * @param hauteur The height of the image.
     * @param qualite The quality setting for this instance (0-100).
     * @param buffer Optional pointer to an existing image buffer.
     */
    cCompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned char **buffer=nullptr);

    /**
     * @brief Destructor.
     * @note Does not free the image buffer itself, as ownership is not assumed.
     */
    ~cCompression();

    /**
     * @brief Sets the image width.
     * @param largeur The width in pixels.
     */
    void setLargeur(unsigned int largeur);

    /**
     * @brief Sets the image height.
     * @param hauteur The height in pixels.
     */
    void setHauteur(unsigned int hauteur);

    /**
     * @brief Sets the per-instance quality factor.
     * @param qualite The quality factor (0-100).
     */
    void setQualite(unsigned int qualite);

    /**
     * @brief Attaches a caller-provided image buffer.
     * @param buffer A 2D array (unsigned char**) representing the image.
     * @note The class does not take ownership or copy the data.
     */
    void setBuffer(unsigned char **buffer);

    /**
     * @brief Gets the current image width.
     * @return The width in pixels.
     */
    unsigned int getLargeur() const;

    /**
     * @brief Gets the current image height.
     * @return The height in pixels.
     */
    unsigned int getHauteur() const;

    /**
     * @brief Gets the per-instance quality factor.
     * @return The quality factor (0-100).
     */
    unsigned int getQualite() const;

    /**
     * @brief Gets the attached image buffer.
     * @return A pointer to the 2D image buffer, or nullptr if not set.
     */
    unsigned char** getBuffer() const;

    /**
     * @brief Gets the global quality factor used by static quantization helpers.
     * @return The global quality setting (0-100).
     */
    static unsigned int getQualiteGlobale();

    /**
     * @brief Sets the global quality factor used by static quantization helpers.
     * @param qualite The new global quality setting (0-100).
     */
    static void setQualiteGlobale(unsigned int qualite);

    /**
     * @brief Caches a Huffman table for later use (e.g., by the decompressor).
     * @param[in] symbols An array of symbol values.
     * @param[in] frequencies An array of corresponding frequencies.
     * @param[in] count The number of entries in the arrays.
     */
    static void storeHuffmanTable(const char *symbols, const double *frequencies, unsigned int count);

    /**
     * @brief Retrieves the cached Huffman table.
     * @param[out] symbols A buffer to store the symbol values.
     * @param[out] frequencies A buffer to store the frequency values.
     * @param[out] count The number of entries copied.
     * @return True if a cached table was successfully loaded, false otherwise.
     */
    static bool loadHuffmanTable(char *symbols, double *frequencies, unsigned int &count);

    /**
     * @brief Checks if a Huffman table is currently cached.
     * @return True if a table is cached, false otherwise.
     */
    static bool hasStoredHuffmanTable();

    /**
     * @brief Calculates the Mean Squared Error (MSE) between an original 8x8 block and its compressed-decompressed version.
     * @param[in] Bloc8x8 An 8x8 integer block of original pixel data (0-255).
     * @return The calculated Mean Squared Error as a double.
     */
    double EQM(int **Bloc8x8);

    /**
     * @brief Calculates the compression ratio for a single 8x8 block.
     * @param[in] Bloc8x8 An 8x8 integer block of original pixel data.
     * @return The compression ratio.
     * @note This ratio is based on the number of zero vs. non-zero coefficients after quantization.
     */
    double Taux_Compression(int **Bloc8x8);

    /**
     * @brief Performs Run-Length Encoding on a single quantized 8x8 block.
     * @param[in] Img_Quant An 8x8 block of quantized DCT coefficients.
     * @param[in] DC_precedent The DC coefficient from the previous block for differential coding.
     * @param[out] Trame A buffer to receive the RLE-encoded byte stream.
     */
    void RLE_Block(int **Img_Quant, int DC_precedent, signed char *Trame);

    /**
     * @brief Performs RLE on all 8x8 blocks of the attached image buffer.
     * @param[out] Trame An integer array where Trame[0] will be the total length, followed by the RLE data.
     * @note The image buffer must be set via setBuffer() before calling.
     */
    void RLE(signed int *Trame);

    /**
     * @brief Builds a frequency histogram from an RLE byte stream.
     * @param[in] Trame The input RLE byte stream.
     * @param[in] Longueur_Trame The number of bytes in the stream.
     * @param[out] Donnee An array to be filled with the unique symbols found.
     * @param[out] Frequence An array to be filled with the corresponding frequencies.
     * @return The number of unique symbols found.
     */
    unsigned int Histogramme(char *Trame, unsigned int Longueur_Trame, char *Donnee, double *Frequence);

    /**
     * @brief Compresses an RLE stream using Huffman coding and writes it to a file.
     * @param[in] Trame_RLE The integer RLE stream from the RLE() method.
     * @param[in] Nom_Fichier The name of the output file.
     */
    void Compression_JPEG(int *Trame_RLE, const char *Nom_Fichier);

    /**
     * @brief Decompresses an image from a file and reconstructs the pixel data.
     * @param[in] Nom_Fichier_compresse The path to the compressed file.
     * @return A newly allocated 2D array (unsigned char**) containing the image data.
     * @note The caller is responsible for freeing the allocated memory.
     */
    unsigned char **Decompression_JPEG(const char *Nom_Fichier_compresse);
};

#endif //JPEG_COMPRESSOR_CCOMPRESSION_H
