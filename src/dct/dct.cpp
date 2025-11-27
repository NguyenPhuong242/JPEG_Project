//
// Created by nguyenphuong on 06/10/2025.
//

#include "dct/dct.h"
#include <cmath>
#include <iostream>

/**
 * @brief Compute the 2D DCT of an 8x8 block.
 * @param Bloc8x8 8x8 input block (integers, typically level-shifted by -128).
 * @param DCT_Img 8x8 output buffer of DCT coefficients (double).
 */
void Calcul_DCT_Block(int **Bloc8x8, double **DCT_Img) {
    const int N = 8;
    for (int u = 0; u < N; ++u) {
        for (int v = 0; v < N; ++v) {
            double sum = 0.0;
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < N; ++y) {
                    sum += Bloc8x8[x][y] *
                           cos((2 * x + 1) * u * M_PI / (2 * N)) *
                           cos((2 * y + 1) * v * M_PI / (2 * N));
                }
            }
            double Cu;
            double Cv;
            if (u == 0) {
                Cu = 1.0 / sqrt(2.0);
            } else {
                Cu = 1.0;
            }

            if (v == 0) {
                Cv = 1.0 / sqrt(2.0);
            } else {
                Cv = 1.0;
            }
            DCT_Img[u][v] = 0.25 * Cu * Cv * sum;
        }
    }
}

/**
 * @brief Compute the inverse 2D DCT of an 8x8 block.
 * @param DCT_Img 8x8 input buffer of DCT coefficients (double).
 * @param Bloc8x8 8x8 output block (integers, values rounded by the caller).
 */
void Calcul_IDCT_Block(double **DCT_Img, int **Bloc8x8) {
    const int N = 8;
    for (int x = 0; x < N; ++x) {
        for (int y = 0; y < N; ++y) {
            double sum = 0.0;
            for (int u = 0; u < N; ++u) {
                for (int v = 0; v < N; ++v) {
                    double Cu;
                    double Cv;
                    if (u == 0) {
                        Cu = 1.0 / sqrt(2.0);
                    } else {
                        Cu = 1.0;
                    }

                    if (v == 0) {
                        Cv = 1.0 / sqrt(2.0);
                    } else {
                        Cv = 1.0;
                    }

                    sum += Cu * Cv * DCT_Img[u][v] *
                           cos((2 * x + 1) * u * M_PI / (2 * N)) *
                           cos((2 * y + 1) * v * M_PI / (2 * N));
                }
            }
            Bloc8x8[x][y] = static_cast<char>(round(0.25 * sum));
        }
    }
}

/**
 * @brief Dump a DCT block to stdout (debug helper).
 * @param DCT_Img 8x8 input buffer of DCT coefficients (double).
 */
void Show_DCT_Block(double **DCT_Img) {
    const int N = 8;
    std::cout << "DCT Block:\n";
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            std::cout << DCT_Img[i][j] << "\t";
        }
        std::cout << "\n";
    }
}

