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
     * Constructor.
     * @param d symbol (one byte)
     * @param f frequency of the symbol
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
    /**
     * Default constructor.
     */
    cHuffman();

    /**
     * Parameterized constructor.
     * @param trame optional raw trame buffer
     * @param longueur length of the trame
     */
    cHuffman(char *trame, unsigned int longueur);

    /**
     * Destructor.
     */
    ~cHuffman();

    /**
     * Get the raw trame buffer.
     * @return mtrame
     */
    char *getTrame() const;

    /**
     * Get the length of the raw trame buffer.
     * @return length of mtrame
     */
    unsigned int getLongueur() const;

    /**
     * Get the root of the Huffman tree.
     * @return root node
     */
    sNoeud *getRacine() const;

    /**
     * Set the raw trame buffer and its length.
     * @param trame new trame buffer
     * @param longueur length of the trame
     */
    void setTrame(char *trame, unsigned int longueur);

    /**
     * Set the root of the Huffman tree.
     * @param racine new root node
     */
    void setRacine(sNoeud *racine);

    /**
     * Build Huffman codes from an array of symbols and their frequencies.
     * This builds the tree and stores the root in mRacine.
     *
     * @param Donnee    array of symbols
     * @param Frequence matching frequency array
     * @param Taille    number of symbols
     */
    void HuffmanCodes(char *Donnee, double *Frequence, unsigned int Taille);

    /**
     * Print the Huffman tree codes by traversing from the given root.
     * Each line: '<symbol>' : <binary-code>
     *
     * @param Racine node to start from (usually the tree root)
     */
    void AfficherHuffman(sNoeud *Racine);

    /**
     * Build a table mapping symbols to their Huffman binary codes.
     * @param table output map (symbol -> binary code)
     */
    void BuildTableCodes(std::map<char, std::string> &table);
};


#endif //JPEG_COMPRESSOR_CHUFFMAN_H
