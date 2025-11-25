#ifndef JPEG_COMPRESSOR_QUANTIFICATION_H
#define JPEG_COMPRESSOR_QUANTIFICATION_H

#include <cstddef>


/**
 * @file quantification.h
 * @brief Quantization and helper metrics used in the JPEG pipeline.
 */

/** Build the 8x8 luminance quantization table for the current global quality.
 *  @param Qtab output 8x8 integer quantization table
 */
void build_Q_table(int Qtab[8][8]);

/** Quantize one 8x8 block in the DCT domain.
 *  @param img_DCT input 8x8 DCT coefficients
 *  @param Img_Quant output 8x8 quantized integer coefficients
 */
void quant_JPEG(double **img_DCT, int **Img_Quant);

/** Dequantize one 8x8 block back to (approximate) DCT domain.
 *  @param Img_Quant input 8x8 quantized integers
 *  @param img_DCT output 8x8 dequantized DCT coefficients
 */
void dequant_JPEG(int **Img_Quant, double **img_DCT);

/** Mean Squared Error on an 8x8 spatial block. Useful for per-block EQM tests.
 *  @param Bloc8x8 input 8x8 block in spatial domain
 *  @return mean squared error
 */
double EQM(int **Bloc8x8);

/** Compression rate heuristic: fraction of zeros in an 8x8 quantized block.
 *  @param Bloc8x8 quantized block
 *  @return fraction in [0,1]
 */
double Taux_Compression(int **Bloc8x8);

#endif // JPEG_COMPRESSOR_QUANTIFICATION_H
