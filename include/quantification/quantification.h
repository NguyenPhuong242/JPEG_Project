/**
 * @file quantification.h
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Declares functions for quantization and dequantization in the JPEG pipeline.
 */

#ifndef JPEG_COMPRESSOR_QUANTIFICATION_H
#define JPEG_COMPRESSOR_QUANTIFICATION_H

#include <cstddef>

/**
 * @brief Builds the 8x8 luminance quantization table based on a global quality setting.
 *
 * This function generates the standard JPEG quantization table, scaling it
 * according to the quality factor set via `cCompression::setQualiteGlobale()`.
 *
 * @param[out] Qtab An 8x8 integer array to be filled with the quantization table values.
 */
void build_Q_table(int Qtab[8][8]);

/**
 * @brief Quantizes an 8x8 block of DCT coefficients.
 *
 * This function divides each DCT coefficient by the corresponding value in the
 * quantization table (scaled by the global quality factor) and rounds to the
 * nearest integer.
 *
 * @param[in] img_DCT An 8x8 block of DCT coefficients.
 * @param[out] Img_Quant An 8x8 block to be filled with the resulting quantized integer coefficients.
 */
void quant_JPEG(double **img_DCT, int **Img_Quant);

/**
 * @brief De-quantizes an 8x8 block of coefficients.
 *
 * This function reverses the quantization process by multiplying each integer
 * coefficient by the corresponding value in the quantization table.
 *
 * @param[in] Img_Quant An 8x8 block of quantized integer coefficients.
 * @param[out] img_DCT An 8x8 block to be filled with the de-quantized DCT coefficients.
 */
void dequant_JPEG(int **Img_Quant, double **img_DCT);

/**
 * @brief (Standalone) Calculates the Mean Squared Error for an 8x8 block.
 *
 * This function compresses and then decompresses a block to calculate the error
 * between the original and the result.
 *
 * @param[in] Bloc8x8 An 8x8 block of original pixel data in the spatial domain.
 * @return The calculated Mean Squared Error as a double.
 */
double EQM(int **Bloc8x8);

/**
 * @brief (Standalone) Calculates the compression ratio for an 8x8 block.
 *
 * This is a heuristic that measures the ratio of zero-value coefficients after
 * quantization, indicating the effectiveness of the compression for that block.
 *
 * @param[in] Bloc8x8 An 8x8 block of quantized integer coefficients.
 * @return The ratio of zero coefficients, a value between 0.0 and 1.0.
 */
double Taux_Compression(int **Bloc8x8);

#endif // JPEG_COMPRESSOR_QUANTIFICATION_H
