#include <cmath>
#include <algorithm>
#include <iostream>
#include <quantification/quantification.h>
#include "core/cCompression.h"

// Standard JPEG luminance quantization table (ISO/ITU recomm.)
static const int Q_LUMINANCE[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68,109,103, 77},
    {24, 35, 55, 64, 81,104,113, 92},
    {49, 64, 78, 87,103,121,120,101},
    {72, 92, 95, 98,112,100,103, 99}
};

/**
 * @brief Compute the scaled JPEG quantization table for a given quality.
 *
 * Uses the standard JPEG mapping from quality (1..100) to a scaling factor
 * and clamps the resulting integer entries to [1,255].
 * @param Q_tab Output quantization table (8x8).
 * @param qualite Quality in [1,100].
 */
static void calculer_q_table(int Q_tab[8][8], unsigned int qualite) {
    double lambda;
    if (qualite < 50) lambda = 5000.0 / qualite;
    else lambda = 200.0 - 2.0 * qualite;

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            double val = std::floor((Q_LUMINANCE[i][j] * lambda + 50.0) / 100.0);
            Q_tab[i][j] = static_cast<int>(std::max(1.0, std::min(255.0, val)));
        }
    }
}

/**
 * @brief Build the 8x8 luminance quantization table for the current global quality.
 * @param Qtab Output 8x8 integer quantization table.
 */
void build_Q_table(int Qtab[8][8]) {
    unsigned int qualite = cCompression::getQualiteGlobale();
    calculer_q_table(Qtab, qualite);
}

/**
 * @brief Quantize an 8x8 DCT block using the global quality.
 * @param img_DCT Input DCT coefficients.
 * @param Img_Quant Output quantized integers.
 */
void quant_JPEG(double** img_DCT, int** Img_Quant) {
    unsigned int qualite = cCompression::getQualiteGlobale();
    int Q_tab[8][8];
    calculer_q_table(Q_tab, qualite);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            Img_Quant[i][j] = static_cast<int>(std::round(img_DCT[i][j] / Q_tab[i][j]));
        }
    }
}

/**
 * @brief Dequantize an 8x8 quantized block back to the DCT domain.
 * @param Img_Quant Input quantized integers.
 * @param img_DCT Output dequantized DCT coefficients.
 */
void dequant_JPEG(int** Img_Quant, double** img_DCT) {
    unsigned int qualite = cCompression::getQualiteGlobale();
    int Q_tab[8][8];
    calculer_q_table(Q_tab, qualite);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            img_DCT[i][j] = Img_Quant[i][j] * Q_tab[i][j];
        }
    }
}

/**
 * @brief Mean squared error on an 8x8 spatial block.
 * @param Bloc8x8 Input 8x8 block in spatial domain.
 * @return Mean squared error.
 */
double EQM(int **Bloc8x8) {
    double mse = 0.0;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            double v = static_cast<double>(Bloc8x8[i][j]);
            mse += v * v;
        }
    }
    return mse / 64.0;
}

/**
 * @brief Compression-rate heuristic: fraction of zero coefficients.
 * @param Bloc8x8 Quantized block.
 * @return Fraction in [0,1].
 */
double Taux_Compression(int **Bloc8x8) {
    int zeros = 0;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (Bloc8x8[i][j] == 0) ++zeros;
        }
    }
    return static_cast<double>(zeros) / 64.0;
}
