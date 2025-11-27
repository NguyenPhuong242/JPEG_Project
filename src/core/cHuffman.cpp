#include "core/cHuffman.h"
#include <iostream>
#include <queue>
#include <vector>
#include <string>

namespace {

/**
 * @brief Recursively populate the Huffman lookup table.
 * @param node Current node.
 * @param prefix Accumulated code bits.
 * @param table Output map from symbol to code string.
 */
void buildTableRec(sNoeud *node,
                   const std::string &prefix,
                   std::map<char, std::string> &table)
{
    if (!node) return;

    if (!node->mgauche && !node->mdroit) {
        table[node->mdonnee] = prefix;    // leaf => final code
        return;
    }

    buildTableRec(node->mgauche, prefix + "0", table);
    buildTableRec(node->mdroit,  prefix + "1", table);
}
}

/** @brief Recursively free all nodes in a Huffman tree. */
void deleteTree(sNoeud *node)
{
    if (!node) return;
    deleteTree(node->mgauche);
    deleteTree(node->mdroit);
    delete node;
}

/** @brief Recursively print codes for each leaf node. */
void printCodesRec(sNoeud *node, const std::string &prefix)
{
    if (!node) return;

    // Leaf node: has a symbol and no children
    if (!node->mgauche && !node->mdroit) {
        std::cout << "'" << node->mdonnee << "' : " << prefix << '\n';
        return;
    }

    // Traverse left with '0'
    printCodesRec(node->mgauche, prefix + "0");
    // Traverse right with '1'
    printCodesRec(node->mdroit, prefix + "1");
}


/* ======================================================================= */
/*  Constructors / Destructor                                              */
/* ======================================================================= */

/** @brief Default constructor, initialises members to null. */
cHuffman::cHuffman()
    : mtrame(nullptr),
      mLongueur(0),
      mRacine(nullptr)
{
}

/**
 * @brief Construct the helper with an optional raw trame buffer.
 * @param trame Pointer to raw data (not owned).
 * @param longueur Number of bytes referenced by @p trame.
 */
cHuffman::cHuffman(char *trame, unsigned int longueur)
    : mtrame(trame),
      mLongueur(longueur),
      mRacine(nullptr)
{
}

/** @brief Destructor releases the current Huffman tree (but not mtrame). */
cHuffman::~cHuffman()
{
    // The trame pointer is not owned by this class (pedagogic choice),
    // so we do NOT delete mtrame here.
    deleteTree(mRacine);
    mRacine = nullptr;
}

/* ======================================================================= */
/*  Getters / Setters                                                      */
/* ======================================================================= */

/** @brief Return the raw trame pointer. */
char *cHuffman::getTrame() const
{
    return mtrame;
}

/** @brief Return the trame length. */
unsigned int cHuffman::getLongueur() const
{
    return mLongueur;
}

/** @brief Return the root node of the Huffman tree. */
sNoeud *cHuffman::getRacine() const
{
    return mRacine;
}

/** @brief Update the raw trame pointer and its length. */
void cHuffman::setTrame(char *trame, unsigned int longueur)
{
    mtrame    = trame;
    mLongueur = longueur;
}

/** @brief Assign a new Huffman tree root, deleting any existing tree. */
void cHuffman::setRacine(sNoeud *racine)
{
    // Free the previous tree, if any
    deleteTree(mRacine);
    mRacine = racine;
}

/* ======================================================================= */
/*  Huffman building                                                       */
/* ======================================================================= */

/**
 * @brief Build the Huffman tree from symbol statistics.
 * @param Donnee Symbol array.
 * @param Frequence Matching frequencies.
 * @param Taille Number of symbols.
 */
void cHuffman::HuffmanCodes(char *Donnee, double *Frequence, unsigned int Taille)
{
    if (!Donnee || !Frequence || Taille == 0) {
        // nothing to build
        deleteTree(mRacine);
        mRacine = nullptr;
        return;
    }

    std::priority_queue<
        sNoeud*,
        std::vector<sNoeud*>,
        compare
    > minHeap;

    // 1) create a leaf node for each symbol and push it in the queue
    for (unsigned int i = 0; i < Taille; ++i) {
        sNoeud *n = new sNoeud(Donnee[i], Frequence[i]);
        minHeap.push(n);
    }

    // 2) build the tree
    while (minHeap.size() > 1) {
        // Take two nodes with smallest frequency
        sNoeud *gauche = minHeap.top(); minHeap.pop();
        sNoeud *droit  = minHeap.top(); minHeap.pop();

        // Internal node has no symbol, freq = sum
        sNoeud *parent = new sNoeud('\0', gauche->mfreq + droit->mfreq);
        parent->mgauche = gauche;
        parent->mdroit  = droit;

        minHeap.push(parent);
    }

    // 3) final remaining node is the root
    deleteTree(mRacine);   // free old tree if any
    mRacine = minHeap.top();
}

/** @brief Generate the code table from the built Huffman tree. */
void cHuffman::BuildTableCodes(std::map<char, std::string> &table)
{
    table.clear();
    buildTableRec(mRacine, "", table);
}

/* ======================================================================= */
/*  Printing the codes                                                     */
/* ======================================================================= */

/** @brief Print all Huffman codes starting from the provided root. */
void cHuffman::AfficherHuffman(sNoeud *Racine)
{
    if (!Racine) {
        std::cout << "(arbre Huffman vide)\n";
        return;
    }

    printCodesRec(Racine, "");
}

