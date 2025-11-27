//
// Created by nguyenphuong on 06/10/2025.
//

#ifndef JPEG_COMPRESSOR_DCT_H
#define JPEG_COMPRESSOR_DCT_H

/**
 * @file dct.h
 * @brief Discrete Cosine Transform helpers for 8x8 blocks.
 *
 * These functions compute the forward and inverse 2D DCT on 8x8 blocks.
 * The API uses double buffers for the transformed coefficients.
 */

/**
 * @brief Compute the 2D DCT of an 8x8 block.
 * @param Bloc8x8 8x8 input block (integers, typically level-shifted by -128).
 * @param DCT_Img 8x8 output buffer of DCT coefficients (double).
 */
void Calcul_DCT_Block(int **Bloc8x8, double **DCT_Img);

/**
 * @brief Compute the inverse 2D DCT of an 8x8 block.
 * @param DCT_Img 8x8 input buffer of DCT coefficients (double).
 * @param Bloc8x8 8x8 output block (integers, values will be rounded/clamped by the caller).
 */
void Calcul_IDCT_Block(double **DCT_Img, int **Bloc8x8);

/**
 * @brief Dump DCT coefficients to stdout for debugging.
 * @param DCT_Img 8x8 input buffer of DCT coefficients (double).
 */
void Show_DCT_Block(double **DCT_Img);

#endif //JPEG_COMPRESSOR_DCT_H