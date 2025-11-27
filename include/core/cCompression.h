//
// Created by nguyenphuong on 06/10/2025.
//

#ifndef JPEG_COMPRESSOR_CCOMPRESSION_H
#define JPEG_COMPRESSOR_CCOMPRESSION_H

#include "cHuffman.h"
/**
 * @brief High-level JPEG compression/decompression helper.
 *
 * This class implements the minimal functionality required by the pedagogic
 * sujet: RLE encoding of 8x8 quantized blocks, concatenation of block
 * trames, histogram computation, and wrapper functions to perform full
 * compression (RLE + Huffman + write file) and decompression (read file +
 * Huffman decode + RLE decode + dequant + IDCT). Members are intentionally
 * lightweight and follow the original function signatures from the lab.
 */
class cCompression {
    private:
        /** Image width in pixels. */
        unsigned int mLargeur;
        /** Image height in pixels. */
        unsigned int mHauteur;
        /** Raw image buffer (row pointers). Ownership: cCompression owns this when used as an image container. */
        unsigned char **mBuffer;
        /** Instance quality parameter (not the global quantizer). */
        unsigned int mQualite;
        /**
         * Static global quality parameter used by quantization helpers.
         * This is a process-wide setting used by the pedagogic functions.
         * Default is 50.
         */
            
    public:
        /** @brief Default ctor. Does not allocate image storage. */
        cCompression();
        /**
         * @brief Construct a compressor tied to an image size and optional quality.
         * @param largeur Image width in pixels.
         * @param hauteur Image height in pixels.
         * @param qualite Initial quality (0-100, JPEG-style).
         * @param buffer Optional caller-owned image buffer (row pointers). If nullptr, no image is owned.
         */
        cCompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned char **buffer=nullptr);
        /** @brief Release owned resources (notably the optional row buffer). */
        ~cCompression();

        /** @brief Set image width in pixels. */
        void setLargeur(unsigned int largeur);
        /** @brief Set image height in pixels. */
        void setHauteur(unsigned int hauteur);
        /** @brief Set per-instance quality factor (0-100). */
        void setQualite(unsigned int qualite);
        /** @brief Attach a caller-provided row buffer. The class does not copy the data. */
        void setBuffer(unsigned char **buffer);
        

        /** @brief Return current image width in pixels. */
        unsigned int getLargeur() const;
        /** @brief Return current image height in pixels. */
        unsigned int getHauteur() const;
        /** @brief Return per-instance quality factor. */
        unsigned int getQualite() const;
        /** @brief Return the attached row buffer pointer (may be nullptr). */
        unsigned char** getBuffer() const;

        /** @brief Read the global quality factor shared by quantification helpers. */
        static unsigned int getQualiteGlobale();
        /** @brief Set the global quality factor used by quantification helpers. */
        static void setQualiteGlobale(unsigned int qualite);
        /**
         * @brief Cache a Huffman table so decompression can reuse the latest encode table.
         * @param symbols Array of symbol values (length @p count).
         * @param frequencies Matching frequency values.
         * @param count Number of valid entries.
         */
        static void storeHuffmanTable(const char *symbols, const double *frequencies, unsigned int count);
        /**
         * @brief Retrieve the cached Huffman table, if available.
         * @param symbols Output buffer for symbols (must have capacity for the stored table).
         * @param frequencies Output buffer receiving frequencies.
         * @param count On success, receives the number of entries copied.
         * @return true when a cached table was copied to the outputs.
         */
        static bool loadHuffmanTable(char *symbols, double *frequencies, unsigned int &count);
        /** @brief Report whether a Huffman table is currently cached. */
        static bool hasStoredHuffmanTable();

        /**
         * @brief Encode a single quantized block to RLE bytes.
         * @param Img_Quant 8x8 quantized coefficients (row-major int**).
         * @param DC_precedent Previous block DC value for differential coding.
         * @param Trame Caller-provided byte buffer receiving the encoded stream.
         */
        void RLE_Block(int **Img_Quant, int DC_precedent, signed char *Trame);

        /**
         * @brief Concatenate all block trames into the pedagogic integer trame format.
         * @param Trame Integer buffer receiving byte count at index 0 and data afterwards.
         */
        void RLE(signed int *Trame);


        /**
         * @brief Build symbol histogram from an RLE byte stream.
         * @param Trame Input byte stream.
         * @param Longueur_Trame Number of valid bytes in the stream.
         * @param Donnee Output array receiving the distinct symbols.
         * @param Frequence Output array receiving frequencies matching @p Donnee.
         * @return Number of distinct symbols encountered.
         */
        unsigned int Histogramme(char *Trame, unsigned int Longueur_Trame, char *Donnee, double *Frequence);

        /**
         * @brief Run the complete JPEG-like encode pipeline and persist the result.
         * @param Trame_RLE Pointer to the integer trame produced by RLE().
         * @param Nom_Fichier Destination filename on disk.
         */
        void Compression_JPEG(int *Trame_RLE, const char *Nom_Fichier);

        /**
         * @brief Decode a previously compressed file and recreate the image rows.
         * @param Nom_Fichier_compresse Path to the compressed file.
         * @return Newly allocated 2D byte array (row pointers) on success, nullptr otherwise.
         */
        unsigned char **Decompression_JPEG(const char *Nom_Fichier_compresse);
};

#endif //JPEG_COMPRESSOR_CCOMPRESSION_H
