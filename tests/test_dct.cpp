#include <iostream>
#include <iomanip>
#include <cmath>
#include "dct/dct.h"

int main() {
    // Original 8x8 block values (Lena example)
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
    int** shiftedOut = new int*[8];
    double** dctBlock = new double*[8];
    for (int i = 0; i < 8; ++i) {
        shiftedIn[i] = new int[8];
        shiftedOut[i] = new int[8];
        dctBlock[i] = new double[8];
    }

    // level-shift (bring values into range [-128,127])
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            shiftedIn[i][j] = blockVals[i][j] - 128;

    // Print level-shifted block 'p' (as in course example)
    std::cout << "Level-shifted block (p):\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << shiftedIn[i][j] << "\t";
        std::cout << "\n";
    }

    // Forward DCT
    Calcul_DCT_Block(shiftedIn, dctBlock);

    // Show DCT coefficients (formatted)
    std::cout << "DCT Block:\n" << std::fixed << std::setprecision(5);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            std::cout << dctBlock[i][j] << '\t';
        }
        std::cout << '\n';
    }

    // Compare entire DCT matrix against example/reference values (from course example)
    const double expected[8][8] = {
        {235.62500, -1.03333, -12.08090, -5.20290, 2.12500, -1.67243, -2.70797, 1.32384},
        {-22.59044, -17.48418, -6.24048, -3.15738, -2.85567, -0.06946, 0.43417, -1.18558},
        {-10.94926, -9.26240, -1.57583, 1.53009, 0.20295, -0.94186, -0.56694, -0.06292},
        {-7.08156, -1.90718, 0.22479, 1.45389, 0.89625, -0.07987, -0.04229, 0.33154},
        {-0.62500, -0.83811, 1.46988, 1.55628, -0.12500, -0.66099, 0.60885, 1.27521},
        {1.75408, -0.20286, 1.62049, -0.34244, -0.77554, 1.47594, 1.04100, -0.99296},
        {-1.28252, -0.35995, -0.31694, -1.46010, -0.48996, 1.73484, 1.07583, -0.76135},
        {-2.59990, 1.55185, -3.76278, -1.84476, 1.87162, 1.21395, -0.56788, -0.44564}
    };
    const double coeffTol = 1e-2; // allow small numerical differences
    bool ok = true;
    for (int u = 0; u < 8 && ok; ++u) {
        for (int v = 0; v < 8; ++v) {
            if (std::fabs(dctBlock[u][v] - expected[u][v]) > coeffTol) {
                std::cerr << "DCT mismatch at (" << u << "," << v << "): got " << dctBlock[u][v]
                          << " expected " << expected[u][v] << "\n";
                ok = false;
            }
        }
    }
    if (!ok) {
        std::cerr << "Full DCT comparison failed. Dumping matrices:\n";
        std::cerr << "Computed:\n";
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) std::cerr << dctBlock[i][j] << "\t";
            std::cerr << "\n";
        }
        std::cerr << "Expected:\n";
        for (int i = 0; i < 8; ++i) {
            for (int j = 0; j < 8; ++j) std::cerr << expected[i][j] << "\t";
            std::cerr << "\n";
        }
        // cleanup and fail
        for (int i = 0; i < 8; ++i) {
            delete[] shiftedIn[i];
            delete[] shiftedOut[i];
            delete[] dctBlock[i];
        }
        delete[] shiftedIn;
        delete[] shiftedOut;
        delete[] dctBlock;
        return 1;
    }

    // Inverse DCT
    Calcul_IDCT_Block(dctBlock, shiftedOut);

    // Print reconstructed block (shift back)
    std::cout << "Reconstructed block:\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << (shiftedOut[i][j] + 128) << "\t";
        std::cout << "\n";
    }

    // Verify reconstruction
    const int tolerance = 1;
    for (int i = 0; i < 8 && ok; ++i) {
        for (int j = 0; j < 8; ++j) {
            int recovered = shiftedOut[i][j] + 128;
            int diff = std::abs(recovered - blockVals[i][j]);
            if (diff > tolerance) {
                std::cerr << "Mismatch at (" << i << "," << j << "): got " << recovered
                          << " expected " << blockVals[i][j] << " (diff=" << diff << ")\n";
                ok = false;
                break;
            }
        }
    }

    // cleanup
    for (int i = 0; i < 8; ++i) {
        delete[] shiftedIn[i];
        delete[] shiftedOut[i];
        delete[] dctBlock[i];
    }
    delete[] shiftedIn;
    delete[] shiftedOut;
    delete[] dctBlock;

    if (ok) {
        std::cout << "test_dct: PASS\n";
        return 0;
    } else {
        std::cerr << "test_dct: FAIL\n";
        return 1;
    }
}

