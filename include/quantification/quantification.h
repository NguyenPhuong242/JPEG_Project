#ifndef JPEG_COMPRESSOR_QUANTIFICATION_H
#define JPEG_COMPRESSOR_QUANTIFICATION_H

#include <cstddef>


/**
 * @file quantification.h
 * @brief Quantization and helper metrics used in the JPEG pipeline.
 */

/**
 * @brief Build the 8x8 luminance quantization table for the current global quality.
 * @param Qtab Output 8x8 integer quantization table.
 */
void build_Q_table(int Qtab[8][8]);

/**
 * @brief Quantize one 8x8 block in the DCT domain.
 * @param img_DCT Input 8x8 DCT coefficients.
 * @param Img_Quant Output 8x8 quantized integer coefficients.
 */
void quant_JPEG(double **img_DCT, int **Img_Quant);

/**
 * @brief Dequantize one 8x8 block back to (approximate) DCT domain.
 * @param Img_Quant Input 8x8 quantized integers.
 * @param img_DCT Output 8x8 dequantized DCT coefficients.
 */
void dequant_JPEG(int **Img_Quant, double **img_DCT);

/**
 * @brief Mean squared error on an 8x8 spatial block.
 * @param Bloc8x8 Input 8x8 block in spatial domain.
 * @return Mean squared error.
 */
double EQM(int **Bloc8x8);

/**
 * @brief Compression rate heuristic: fraction of zeros in an 8x8 quantized block.
 * @param Bloc8x8 Quantized block.
 * @return Fraction in [0,1].
 */
double Taux_Compression(int **Bloc8x8);

#endif // JPEG_COMPRESSOR_QUANTIFICATION_H
