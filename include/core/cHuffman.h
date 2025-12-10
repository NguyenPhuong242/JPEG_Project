/**
 * @file cHuffman.h
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Defines the structures and class for Huffman coding.
 */

#ifndef JPEG_COMPRESSOR_CHUFFMAN_H
#define JPEG_COMPRESSOR_CHUFFMAN_H

#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <functional>
#include <map>


/**
 * @struct sNoeud
 * @brief A node in the Huffman binary tree.
 *
 * This structure represents a single node, which can be a leaf (containing a
 * symbol) or an internal node. It stores the character, its frequency, and
 * pointers to its left and right children.
 */
struct sNoeud {
    /** @brief The character/symbol for this node (if it's a leaf). */
    char mdonnee;
    /** @brief The frequency of the symbol or the combined frequency of its children. */
    double mfreq;
    /** @brief Pointer to the left child node (for bits '0'). */
    sNoeud *mgauche;
    /** @brief Pointer to the right child node (for bits '1'). */
    sNoeud *mdroit;

    /**
     * @brief Constructs a leaf node with a symbol and its frequency.
     * @param d The character/symbol.
     * @param f The frequency of the character.
     */
    sNoeud(char d, double f)
        : mdonnee(d), mfreq(f), mgauche(nullptr), mdroit(nullptr) {}
};

/**
 * @struct compare
 * @brief Functor for comparing two sNoeud pointers based on frequency.
 *
 * This is used by the `std::priority_queue` to build the Huffman tree. It ensures
 * that nodes with lower frequencies have higher priority, which is essential for
* the Huffman algorithm.
 */
struct compare {
    /**
     * @brief Overloaded operator() to perform the comparison.
     * @param gauche The left node to compare.
     * @param droit The right node to compare.
     * @return True if the left node's frequency is greater than the right's.
     */
    bool operator()(sNoeud *gauche, sNoeud *droit) const {
        return gauche->mfreq > droit->mfreq;
    }
};

/**
 * @class cHuffman
 * @brief Manages the creation of a Huffman tree and the generation of codes.
 *
 * This class takes a set of symbols and their frequencies, builds the
 * corresponding Huffman tree, and provides methods to generate or display
 * the resulting Huffman codes for each symbol.
 */
class cHuffman {
private:
    /** @brief (Legacy) Pointer to a raw data stream. Not actively used in the main pipeline. */
    char *mtrame;
    /** @brief (Legacy) The length of the raw data stream. */
    unsigned int mLongueur;
    /** @brief The root node of the constructed Huffman tree. */
    sNoeud *mRacine;

    /**
     * @brief (Private Helper) Recursively builds the Huffman code table.
     * @param h The cHuffman instance.
     * @param codeTable The map to store symbol-to-code mappings.
     * @param revTable The map to store code-to-symbol mappings.
     */
    static void buildHuffmanCodeTable(cHuffman &h,
                                  std::map<char, std::string> &codeTable,
                                  std::map<std::string, char> &revTable);

    /**
     * @brief (Private Helper) Decodes a file using a given Huffman tree.
     * @param filename The path to the file to decode.
     * @param h The cHuffman instance containing the tree.
     * @return A vector of decoded characters.
     */
    static std::vector<char> HuffmanDecodeFile(const char *filename,
                                           cHuffman &h);
public:
    /**
     * @brief Default constructor. Initializes root to nullptr.
     */
    cHuffman();

    /**
     * @brief Legacy constructor with an optional raw data buffer.
     * @param trame Pointer to the raw data.
     * @param longueur The length of the data.
     */
    cHuffman(char *trame, unsigned int longueur);

    /**
     * @brief Destructor. Frees all nodes in the Huffman tree.
     */
    ~cHuffman();

    /** @brief Gets the pointer to the legacy raw data stream. */
    char *getTrame() const;
    /** @brief Gets the length of the legacy raw data stream. */
    unsigned int getLongueur() const;
    /** @brief Gets the root node of the Huffman tree. */
    sNoeud *getRacine() const;

    /**
     * @brief Sets the legacy raw data stream.
     * @param trame Pointer to the new data.
     * @param longueur The length of the new data.
     */
    void setTrame(char *trame, unsigned int longueur);

    /**
     * @brief Sets the root node of the Huffman tree.
     * @param racine The new root node.
     */
    void setRacine(sNoeud *racine);

    /**
     * @brief Builds the Huffman tree from arrays of symbols and frequencies.
     * @param[in] Donnee An array of characters/symbols.
     * @param[in] Frequence An array of corresponding frequencies.
     * @param[in] Taille The number of valid entries in the arrays.
     */
    void HuffmanCodes(char *Donnee, double *Frequence, unsigned int Taille);

    /**
     * @brief Displays the Huffman codes for all symbols by traversing the tree.
     * @param Racine The node to start traversal from (typically the root).
     * @note "Afficher" is French for "Display".
     */
    void AfficherHuffman(sNoeud *Racine);

    /**
     * @brief Populates a map with symbol-to-code mappings by traversing the tree.
     * @param[out] table A map where the key is the symbol and the value is the Huffman code string.
     */
    void BuildTableCodes(std::map<char, std::string> &table);
};


#endif //JPEG_COMPRESSOR_CHUFFMAN_H
