/**
 * @file cHuffman.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements the cHuffman class for building a Huffman tree and codes.
 */

#include "core/cHuffman.h"
#include <iostream>
#include <queue>
#include <vector>
#include <string>

// --- Anonymous Namespace for Local Helper Functions ---

namespace {

/**
 * @brief Recursively traverses the Huffman tree to build a code table.
 * @param[in] node The current node in the tree to process.
 * @param[in] prefix The binary string representing the code to this point.
 * @param[out] table The map to populate with symbol-to-code mappings.
 */
void buildTableRec(sNoeud *node,
                   const std::string &prefix,
                   std::map<char, std::string> &table)
{
    if (!node) return;

    // If it's a leaf node, we have found a code for a symbol.
    if (!node->mgauche && !node->mdroit) {
        table[node->mdonnee] = prefix;
        return;
    }

    // Recurse for left (0) and right (1) children.
    buildTableRec(node->mgauche, prefix + "0", table);
    buildTableRec(node->mdroit,  prefix + "1", table);
}

/**
 * @brief Recursively deletes all nodes in the Huffman tree.
 * @param[in] node The root node of the tree (or subtree) to delete.
 */
void deleteTree(sNoeud *node)
{
    if (!node) return;
    deleteTree(node->mgauche);
    deleteTree(node->mdroit);
    delete node;
}

/**
 * @brief Recursively traverses the tree to print the Huffman codes.
 * @param[in] node The current node to process.
 * @param[in] prefix The binary string code accumulated so far.
 */
void printCodesRec(sNoeud *node, const std::string &prefix)
{
    if (!node) return;

    // A leaf node has a symbol to print.
    if (!node->mgauche && !node->mdroit) {
        std::cout << "'" << node->mdonnee << "' : " << prefix << '\n';
        return;
    }

    printCodesRec(node->mgauche, prefix + "0");
    printCodesRec(node->mdroit, prefix + "1");
}

} // namespace


// --- cHuffman Method Implementations ---

cHuffman::cHuffman()
    : mtrame(nullptr),
      mLongueur(0),
      mRacine(nullptr)
{
}

cHuffman::cHuffman(char *trame, unsigned int longueur)
    : mtrame(trame),
      mLongueur(longueur),
      mRacine(nullptr)
{
}

cHuffman::~cHuffman()
{
    // The trame pointer is not owned by this class, so it is not deleted here.
    // The tree, however, is owned and must be freed.
    deleteTree(mRacine);
    mRacine = nullptr;
}


// --- Getters / Setters ---

char *cHuffman::getTrame() const
{
    return mtrame;
}

unsigned int cHuffman::getLongueur() const
{
    return mLongueur;
}

sNoeud *cHuffman::getRacine() const
{
    return mRacine;
}

void cHuffman::setTrame(char *trame, unsigned int longueur)
{
    mtrame    = trame;
    mLongueur = longueur;
}

void cHuffman::setRacine(sNoeud *racine)
{
    if (mRacine != racine) {
        deleteTree(mRacine); // Free the previous tree if it's different.
    }
    mRacine = racine;
}


// --- Core Huffman Logic ---

void cHuffman::HuffmanCodes(char *Donnee, double *Frequence, unsigned int Taille)
{
    if (!Donnee || !Frequence || Taille == 0) {
        deleteTree(mRacine);
        mRacine = nullptr;
        return;
    }

    // A min-heap to store tree nodes, ordered by frequency.
    std::priority_queue<sNoeud*, std::vector<sNoeud*>, compare> minHeap;

    // 1. Create a leaf node for each symbol and add it to the min-heap.
    for (unsigned int i = 0; i < Taille; ++i) {
        minHeap.push(new sNoeud(Donnee[i], Frequence[i]));
    }

    // 2. Build the tree by merging nodes.
    // The loop continues until only one node remains in the heap (the root).
    while (minHeap.size() > 1) {
        // Extract the two nodes with the lowest frequencies.
        sNoeud *gauche = minHeap.top(); minHeap.pop();
        sNoeud *droit  = minHeap.top(); minHeap.pop();

        // Create a new internal node with these two nodes as children.
        // The new node's frequency is the sum of its children's frequencies.
        sNoeud *parent = new sNoeud('\0', gauche->mfreq + droit->mfreq);
        parent->mgauche = gauche;
        parent->mdroit  = droit;

        minHeap.push(parent);
    }

    // 3. The last remaining node is the root of the Huffman tree.
    deleteTree(mRacine); // Free any pre-existing tree.
    mRacine = minHeap.top();
}

void cHuffman::BuildTableCodes(std::map<char, std::string> &table)
{
    table.clear();
    buildTableRec(mRacine, "", table);
}

void cHuffman::AfficherHuffman(sNoeud *Racine)
{
    if (!Racine) {
        std::cout << "(Empty Huffman Tree)\n";
        return;
    }
    printCodesRec(Racine, "");
}

