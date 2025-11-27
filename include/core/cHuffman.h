#ifndef JPEG_COMPRESSOR_CHUFFMAN_H
#define JPEG_COMPRESSOR_CHUFFMAN_H

#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <functional>
#include <map>


/**
 * @brief Node used by the pedagogic Huffman tree implementation.
 *
 * Contains a symbol (one byte) and its frequency. Left/right pointers form
 * the binary tree used to derive codes. This is a simple, non-owning
 * structure; cHuffman is responsible for deleting allocated nodes.
 */
struct sNoeud {
    char mdonnee;    /**< symbol (one byte) */
    double mfreq;    /**< frequency of the symbol */
    sNoeud *mgauche; /**< left child */
    sNoeud *mdroit;  /**< right child */

    /**
     * @brief Construct a leaf with symbol and frequency.
     * @param d Huffman symbol.
     * @param f Corresponding frequency.
     */
    sNoeud(char d, double f)
        : mdonnee(d), mfreq(f), mgauche(nullptr), mdroit(nullptr) {}
};

/**
 * @brief Comparison functor for priority queue of sNoeud pointers.
 *
 * Used to build the Huffman tree by ordering nodes by frequency.
 * Lower frequency nodes have higher priority.
 */
struct compare {
    bool operator()(sNoeud *gauche, sNoeud *droit) const {
        return gauche->mfreq > droit->mfreq;
    }
};

/**
 * @brief Simple Huffman coder/decoder used for exercises.
 *
 * The class builds a Huffman binary tree from a list of symbols and
 * frequencies and can print or expose the root for traversal. The
 * implementation is intentionally straightforward for teaching purposes.
 */
class cHuffman {
private:
    char *mtrame;           /**< raw trame buffer (if used) */
    unsigned int mLongueur; /**< length of mtrame */
    sNoeud *mRacine;        /**< root of the Huffman tree */

    static void buildHuffmanCodeTable(cHuffman &h,
                                  std::map<char, std::string> &codeTable,
                                  std::map<std::string, char> &revTable);

    static std::vector<char> HuffmanDecodeFile(const char *filename,
                                           cHuffman &h);
public:
    /** @brief Default constructor. */
    cHuffman();

    /**
     * @brief Construct with an optional raw trame buffer.
     * @param trame Optional raw trame buffer.
     * @param longueur Number of bytes inside @p trame.
     */
    cHuffman(char *trame, unsigned int longueur);

    /** @brief Destructor releasing the Huffman tree. */
    ~cHuffman();

    /** @brief Return the raw trame buffer. */
    char *getTrame() const;

    /** @brief Return the length of the raw trame buffer. */
    unsigned int getLongueur() const;

    /** @brief Return the root of the Huffman tree. */
    sNoeud *getRacine() const;

    /**
     * @brief Replace the raw trame buffer and its length.
     * @param trame New trame buffer.
     * @param longueur Number of bytes pointed to by @p trame.
     */
    void setTrame(char *trame, unsigned int longueur);

    /**
     * @brief Set the root node of the Huffman tree.
     * @param racine Newly built tree root.
     */
    void setRacine(sNoeud *racine);

    /**
     * @brief Build the Huffman tree from symbol statistics.
     * @param Donnee Array of symbols.
     * @param Frequence Matching array of frequencies.
     * @param Taille Number of entries in the arrays.
     */
    void HuffmanCodes(char *Donnee, double *Frequence, unsigned int Taille);

    /**
     * @brief Print the binary code for each symbol starting from @p Racine.
     * @param Racine Node to traverse (defaults to the tree root).
     */
    void AfficherHuffman(sNoeud *Racine);

    /**
     * @brief Populate a lookup table mapping symbols to Huffman codes.
     * @param table Output map (symbol -> binary code).
     */
    void BuildTableCodes(std::map<char, std::string> &table);
};


#endif //JPEG_COMPRESSOR_CHUFFMAN_H
