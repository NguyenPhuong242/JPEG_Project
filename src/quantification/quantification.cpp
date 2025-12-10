/**
 * @file quantification.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements functions for quantization and dequantization in the JPEG pipeline.
 */

#include <cmath>
#include <algorithm>
#include <iostream>
#include <quantification/quantification.h>
#include "core/cCompression.h"

/**
 * @brief The standard JPEG luminance quantization table from the ISO/ITU recommendation.
 *
 * This table provides baseline quantization values for an 8x8 block. It is
 * scaled by a quality factor to adjust the level of compression.
 */
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
 * @brief (Helper) Calculates a scaled 8x8 quantization table from a quality value.
 * @note "calculer" is French for "calculate".
 *
 * This function applies the standard JPEG formula to map a quality value (1-100)
 * to a scaling factor (`lambda`). This factor is then used to scale the baseline
 * `Q_LUMINANCE` table. The final table values are clamped to the range [1, 255].
 *
 * @param[out] Q_tab The 8x8 output table to be filled with scaled quantization values.
 * @param[in] qualite The quality setting, from 1 to 100.
 */
static void calculer_q_table(int Q_tab[8][8], unsigned int qualite) {
    double lambda;
    if (qualite < 50) {
        lambda = 5000.0 / qualite;
    } else {
        lambda = 200.0 - 2.0 * qualite;
    }

    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            double val = std::floor((Q_LUMINANCE[i][j] * lambda + 50.0) / 100.0);
            Q_tab[i][j] = static_cast<int>(std::max(1.0, std::min(255.0, val)));
        }
    }
}

void build_Q_table(int Qtab[8][8]) {
    // Fetches the global quality setting to build the table.
    unsigned int qualite = cCompression::getQualiteGlobale();
    calculer_q_table(Qtab, qualite);
}

void quant_JPEG(double** img_DCT, int** Img_Quant) {
    // 1. Build the appropriate quantization table for the current global quality.
    int Q_tab[8][8];
    build_Q_table(Q_tab);

    // 2. Perform element-wise division and round to the nearest integer.
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            Img_Quant[i][j] = static_cast<int>(std::round(img_DCT[i][j] / Q_tab[i][j]));
        }
    }
}

void dequant_JPEG(int** Img_Quant, double** img_DCT) {
    // 1. Build the same quantization table used for compression.
    int Q_tab[8][8];
    build_Q_table(Q_tab);

    // 2. Perform element-wise multiplication.
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            img_DCT[i][j] = Img_Quant[i][j] * Q_tab[i][j];
        }
    }
}

double EQM(int **Bloc8x8) {
    // Note: This implementation calculates the mean square value of the input block,
    // not the mean squared error between two blocks.
    double mse = 0.0;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            double v = static_cast<double>(Bloc8x8[i][j]);
            mse += v * v;
        }
    }
    return mse / 64.0;
}

double Taux_Compression(int **Bloc8x8) {
    // Note: This is a heuristic that calculates the ratio of zeroed coefficients
    // in a quantized block.
    int zeros = 0;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (Bloc8x8[i][j] == 0) ++zeros;
        }
    }
    return static_cast<double>(zeros) / 64.0;
}
