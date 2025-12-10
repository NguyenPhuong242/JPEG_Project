/**
 * @file dct.h
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Declares functions for the 2D Discrete Cosine Transform (DCT) on 8x8 blocks.
 */

#ifndef JPEG_COMPRESSOR_DCT_H
#define JPEG_COMPRESSOR_DCT_H

/**
 * @brief Computes the forward 2D-DCT of an 8x8 block.
 *
 * This function transforms an 8x8 block from the spatial domain to the
 * frequency domain.
 * @note "Calcul" is French for "Calculation".
 *
 * @param[in] Bloc8x8 An 8x8 input block of pixel data (typically level-shifted by -128).
 * @param[out] DCT_Img An 8x8 output block to be filled with the resulting DCT coefficients.
 */
void Calcul_DCT_Block(int **Bloc8x8, double **DCT_Img);

/**
 * @brief Computes the inverse 2D-DCT of an 8x8 block.
 *
 * This function transforms an 8x8 block of DCT coefficients from the frequency
 * domain back to the spatial domain.
 * @note "Calcul" is French for "Calculation".
 *
 * @param[in] DCT_Img An 8x8 input block of DCT coefficients.
 * @param[out] Bloc8x8 An 8x8 output block to be filled with the reconstructed spatial data.
 */
void Calcul_IDCT_Block(double **DCT_Img, int **Bloc8x8);

/**
 * @brief A debugging utility to print the contents of an 8x8 DCT block to the console.
 *
 * @param[in] DCT_Img The 8x8 block of DCT coefficients to display.
 */
void Show_DCT_Block(double **DCT_Img);

#endif //JPEG_COMPRESSOR_DCT_H