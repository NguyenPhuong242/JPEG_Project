#include <iostream>
#include <iomanip>
#include "core/cCompression.h"

int main() {
    // Example 8x8 block (same as other tests / Lena example)
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

    // Build an 8x8 buffer of unsigned char pointers (row pointers)
    unsigned char* bufferRows[8];
    for (int r = 0; r < 8; ++r) {
        bufferRows[r] = new unsigned char[8];
        for (int c = 0; c < 8; ++c) bufferRows[r][c] = static_cast<unsigned char>(blockVals[r][c]);
    }

    // Create compressor tied to this small image
    cCompression comp(8, 8, 50, bufferRows);
    cCompression::setQualiteGlobale(50);

    // Prepare output Trame (integer buffer). We reserve enough space.
    int Trame[1024];
    for (int i = 0; i < 1024; ++i) Trame[i] = 0;

    // Encode image using RLE (wrapper that writes Trame[0]=length, Trame[1..]=bytes)
    comp.RLE(Trame);

    int len = Trame[0];
    std::cout << "RLE trame length: " << len << "\n";
    std::cout << "Trame bytes (as signed ints):\n";
    for (int i = 0; i < len; ++i) {
        int val = Trame[i+1];
        std::cout << val << (i+1 < len ? ", " : "\n");
    }

    // Cleanup
    for (int r = 0; r < 8; ++r) delete[] bufferRows[r];

    std::cout << "test_rle: DONE\n";
    return 0;
}
