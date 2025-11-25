//
// Created by nguyenphuong on 06/10/2025.
//

#ifndef JPEG_COMPRESSOR_CCOMPRESSION_H
#define JPEG_COMPRESSOR_CCOMPRESSION_H

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
    public:
        /** Default ctor. Does not allocate image storage. */
        cCompression();
        /**
         * Construct a compressor tied to an image size and optional quality.
         * @param largeur image width
         * @param hauteur image height
         * @param qualite initial quality (0-100, JPEG-style)
         * @param buffer optional image buffer (row pointers). If nullptr, no image is owned.
         *
         */
        cCompression(unsigned int largeur, unsigned int hauteur, unsigned int qualite = 50, unsigned char **buffer=nullptr);
        ~cCompression();

        /**
         * Setters for image dimensions and quality.
         * @param largeur image width
         * @param hauteur image height
         * @param qualite quality (0-100)
         */
        void setLargeur(unsigned int largeur);
        void setHauteur(unsigned int hauteur);
        void setQualite(unsigned int qualite);
        void setBuffer(unsigned char **buffer);
        

        /**
         * Getters for image dimensions and quality.
         * @return corresponding member value 
         */
        unsigned int getLargeur() const;
        unsigned int getHauteur() const;
        unsigned int getQualite() const;
        unsigned char** getBuffer() const;

        /**
         * Global quality used by the quantification helpers. This is a process-
         * wide parameter used by the pedagogic functions in this project and can
         * be set/read from anywhere.
         */
        static unsigned int getQualiteGlobale();
        static void setQualiteGlobale(unsigned int qualite);

        /**
         * Encode a single 8x8 quantized block with run-length encoding (RLE).
         * The function writes bytes into the caller-provided buffer `Trame` and
         * terminates the block with the EOB marker (255,0). The caller must
         * ensure `Trame` has enough capacity (we reserve 128 bytes per block in
         * the driver).
         * @param Img_Quant 8x8 quantized coefficients (row-major int**)
         * @param DC_precedent previous DC coefficient (for differential coding)
         * @param Trame caller-provided byte buffer
         */
        void RLE_Block(int **Img_Quant, int DC_precedent, char *Trame);

        /**
         * Concatenate block trames of the whole image into an integer trame.
         * The pedagogic convention used here stores the concatenated byte count
         * in Trame[0] and the bytes in Trame[1..N]. This is a convenience
         * wrapper matching the exercise's expected signature.
         * @param Trame integer buffer provided by caller
         */
        void RLE(int *Trame);


        /**
         * Compute histogram of bytes after RLE. Used to build Huffman codes.
         * @param Trame input byte buffer
         * @param Longueur_Trame number of bytes in Trame
         * @param Donnee output array of distinct symbols found (filled by function)
         * @param Frequence output frequency array matching Donnee
         * @return number of distinct symbols
         */
        unsigned int Histogramme(char *Trame, unsigned int Longueur_Trame, char *Donnee, double *Frequence);

        /**
         * Perform full JPEG-like compression and write to disk. The function
         * accepts a raw byte trame of length Longueur_Trame (the RLE concatenation
         * of all blocks) and writes a compressed file named Nom_Fichier.
         * @param Trame pointer to bytes (may contain zeros)
         * @param Longueur_Trame length in bytes
         * @param Nom_Fichier filename to create on disk
         */
        void Compression_JPEG(char *Trame, unsigned int Longueur_Trame, char *Nom_Fichier);

        /**
         * Read and decompress a compressed file previously produced by
         * Compression_JPEG. Returns a newly allocated 2D byte array (row
         * pointers) with dimensions set by the compressor. The caller is
         * responsible for freeing the returned memory with delete[].
         * @param Nom_Fichier_compresse compressed filename to read
         * @return char** image rows (or nullptr on failure)
         */
        char** Decompression_JPEG(char* Nom_Fichier_compresse);
        /**
         * Decompress directly from an RLE trame (concatenation of block trames)
         * produced by RLE_Block. This function assumes the trame contains raw
         * run-length encoded blocks (no Huffman header). The original image is
         * assumed square with side length `imgSize` (pixels) and composed of
         * 8x8 blocks. The quality parameter is used to rebuild the quantization
         * table during dequantization.
         *
         * @param Trame pointer to the concatenated RLE bytes
         * @param Longueur_Trame number of bytes in Trame
         * @param imgSize side length (pixels) of the square image (must be multiple of 8)
         * @param qualite quality factor used for dequantization (1..100)
         * @return newly allocated image as char** (rows) or nullptr on error
         */
        char** Decompression_JPEG_from_trame(char* Trame, unsigned int Longueur_Trame, unsigned int imgSize, unsigned int qualite);
};

#endif //JPEG_COMPRESSOR_CCOMPRESSION_H
