#include <iostream>
#include <iomanip>
#include "core/cHuffman.h"   // adjust path if needed

int main() {
    // Example data: 6 symbols with arbitrary frequencies
    // (you can replace these with the table from your assignment)
    char   Donnee[]    = { 'A', 'B', 'C', 'D', 'E', 'F' };
    double Frequence[] = {  5.0,  9.0, 12.0, 13.0, 16.0, 45.0 };
    const unsigned int Taille = sizeof(Donnee) / sizeof(Donnee[0]);

    std::cout << "Test Huffman\n";
    std::cout << "Symbols and frequencies:\n";
    for (unsigned int i = 0; i < Taille; ++i) {
        std::cout << "  '" << Donnee[i] << "' : " << Frequence[i] << "\n";
    }
    std::cout << "\nBuilding Huffman tree and printing codes:\n\n";

    // Build Huffman tree
    cHuffman h;
    h.HuffmanCodes(Donnee, Frequence, Taille);

    // Print codes starting from the root
    h.AfficherHuffman(h.getRacine());

    std::cout << "\nDone.\n";
    return 0;
}
