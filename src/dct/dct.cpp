/**
 * @file dct.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements the 2D Discrete Cosine Transform (DCT) functions.
 */

#include "dct/dct.h"
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void Calcul_DCT_Block(int **Bloc8x8, double **DCT_Img) {
    const int N = 8;
    // For each frequency coefficient (u,v) in the output DCT matrix...
    for (int u = 0; u < N; ++u) {
        for (int v = 0; v < N; ++v) {
            double sum = 0.0;
            // ...sum over all spatial coordinates (x,y) in the input block.
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < N; ++y) {
                    sum += Bloc8x8[x][y] *
                           cos((2 * x + 1) * u * M_PI / (2.0 * N)) *
                           cos((2 * y + 1) * v * M_PI / (2.0 * N));
                }
            }

            // Define the normalization coefficients Cu and Cv.
            double Cu = (u == 0) ? (1.0 / sqrt(2.0)) : 1.0;
            double Cv = (v == 0) ? (1.0 / sqrt(2.0)) : 1.0;
            
            // Apply the normalization factors to get the final DCT coefficient.
            DCT_Img[u][v] = 0.25 * Cu * Cv * sum;
        }
    }
}

void Calcul_IDCT_Block(double **DCT_Img, int **Bloc8x8) {
    const int N = 8;
    // For each spatial coordinate (x,y) in the output block...
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            double sum = 0.0;
            // ...sum over all frequency coefficients (u,v) in the input DCT block.
            for (int u = 0; u < N; ++u) {
                for (int v = 0; v < N; ++v) {
                    // Define the normalization coefficients Cu and Cv.
                    double Cu = (u == 0) ? (1.0 / sqrt(2.0)) : 1.0;
                    double Cv = (v == 0) ? (1.0 / sqrt(2.0)) : 1.0;

                    sum += Cu * Cv * DCT_Img[u][v] *
                           cos((2 * x + 1) * u * M_PI / (2.0 * N)) *
                           cos((2 * y + 1) * v * M_PI / (2.0 * N));
                }
            }
            // Apply the normalization factor, round the result, and cast to an integer type.
            Bloc8x8[x][y] = static_cast<int>(round(0.25 * sum));
        }
    }
}

void Show_DCT_Block(double **DCT_Img) {
    const int N = 8;
    std::cout << "--- DCT Block Coefficients ---" << std::endl;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << DCT_Img[i][j] << "\t";
        }
        std::cout << std::endl;
    }
}

