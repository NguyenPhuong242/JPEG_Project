#include <iostream>
#include <iomanip>
#include <cmath>
#include "dct/dct.h"
#include "quantification/quantification.h"
#include "core/cCompression.h"

int main() {
    // Use the same example block as in test_dct
    const int blockVals[8][8] = {
        {139, 144, 149, 153, 155, 155, 155, 155},
        {144, 151, 153, 156, 159, 156, 156, 156},
        {150, 155, 160, 163, 158, 156, 156, 156},
        {159, 161, 162, 160, 160, 159, 159, 159},
        {159, 160, 161, 162, 162, 155, 155, 155},
        {161, 161, 161, 161, 160, 157, 157, 157},
        {162, 162, 161, 163, 162, 157, 157, 157},
        {162, 162, 161, 161, 163, 158, 158, 158}
    };
    // Print original block
    std::cout << "Original block:\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << blockVals[i][j] << "\t";
        std::cout << "\n";
    }

    // allocate arrays
    int** shiftedIn = new int*[8];
    int** Img_Quant = new int*[8];
    double** dctBlock = new double*[8];
    double** dequant = new double*[8];
    for (int i = 0; i < 8; ++i) {
        shiftedIn[i] = new int[8];
        Img_Quant[i] = new int[8];
        dctBlock[i] = new double[8];
        dequant[i] = new double[8];
    }

    // level-shift
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            shiftedIn[i][j] = blockVals[i][j] - 128;

    // print level-shifted block
    std::cout << "Level-shifted block (p):\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << shiftedIn[i][j] << "\t";
        std::cout << "\n";
    }
    // forward DCT
    Calcul_DCT_Block(shiftedIn, dctBlock);
    // Print DCT coefficients
    std::cout << "DCT coefficients:\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << std::fixed << std::setprecision(2) << dctBlock[i][j] << "\t";
        std::cout << "\n";
    }

    // set global quality and quantize
    cCompression::setQualiteGlobale(50);
    quant_JPEG(dctBlock, Img_Quant);
    // Print quantized coefficients
    std::cout << "Quantized coefficients:\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << Img_Quant[i][j] << "\t";
        std::cout << "\n";
    }

    // dequantize back (P')
    dequant_JPEG(Img_Quant, dequant);

    // Print dequantized DCT coefficients P' (integer multiples of Qtab)
    std::cout << "Dequantized DCT coefficients (P'):\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << static_cast<int>(std::round(dequant[i][j])) << "\t";
        std::cout << "\n";
    }

    // Now perform IDCT on the dequantized coefficients to get reconstructed spatial block
    int** reconShifted = new int*[8];
    for (int i = 0; i < 8; ++i) {
        reconShifted[i] = new int[8];
    }
    Calcul_IDCT_Block(dequant, reconShifted);

    // Print reconstructed block (shift back +128)
    std::cout << "Reconstructed block after dequant+IDCT:\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << (reconShifted[i][j] + 128) << "\t";
        std::cout << "\n";
    }

    // Compute mean squared error between original and reconstructed
    double mse = 0.0;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            double diff = static_cast<double>( (reconShifted[i][j] + 128) - blockVals[i][j] );
            mse += diff * diff;
        }
    }
    mse /= 64.0;
    std::cout << "MSE between original and reconstructed: " << mse << "\n";

    // Compression rate (fraction of zeros in Img_Quant)
    double taux = Taux_Compression(Img_Quant);
    std::cout << "Compression rate (fraction of zeros): " << taux << "\n";

    // Compare dequantized coefficients to original DCT (within half-quant step)
    // For each coefficient, allowed error is <= Q_tab[u][v]/2 (since quantization used rounding)
    // We'll compute the Q table here by calling build_Q_table
    int Qtab[8][8];
    build_Q_table(Qtab);

    // Print quantization table for the current global quality
    std::cout << "Quantization table (quality=" << cCompression::getQualiteGlobale() << "):\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << Qtab[i][j] << "\t";
        std::cout << "\n";
    }

    // Print quantized integer coefficients
    std::cout << "Quantized coefficients Img_Quant:\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << Img_Quant[i][j] << "\t";
        std::cout << "\n";
    }

    bool ok = true;
    for (int u = 0; u < 8 && ok; ++u) {
        for (int v = 0; v < 8; ++v) {
            double diff = std::fabs(dequant[u][v] - dctBlock[u][v]);
            double allowed = static_cast<double>(Qtab[u][v]) / 2.0 + 1e-9;
            if (diff > allowed) {
                std::cerr << "Quantization mismatch at (" << u << "," << v << "): got " << dequant[u][v]
                          << " expected " << dctBlock[u][v] << " diff=" << diff << " allowed=" << allowed << "\n";
                ok = false;
                break;
            }
        }
    }

    // cleanup
    for (int i = 0; i < 8; ++i) {
        delete[] shiftedIn[i];
        delete[] Img_Quant[i];
        delete[] dctBlock[i];
        delete[] dequant[i];
    }
    delete[] shiftedIn;
    delete[] Img_Quant;
    delete[] dctBlock;
    delete[] dequant;

    if (ok) {
        std::cout << "test_quantification: PASS\n";
        return 0;
    } else {
        std::cerr << "test_quantification: FAIL\n";
        return 1;
    }
}
