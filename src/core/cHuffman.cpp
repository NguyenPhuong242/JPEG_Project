// #include "core/cHuffman.h"

// cHuffman::cHuffman() {
//     mtrame = nullptr;
//     mLongueur = 0;
//     mRacine = nullptr;
// }

// cHuffman::cHuffman(char *trame, unsigned int longueur){
//     mLongueur = longueur;
//     mtrame = new char[longueur];
//     std::memcpy(mtrame, trame, longueur);
//     mRacine = nullptr;
// }

// cHuffman::~cHuffman() {
//     delete[] mtrame;
//     /**
//      * Recursively delete tree
//      */
//     std::function<void(sNoeud*)> del = [&](sNoeud* n) {
//         if (!n) return;
//         del(n->mgauche);
//         del(n->mdroit);
//         delete n;
//     };
//     del(mRacine);
// }

// char * cHuffman::getTrame() const {
//     return mtrame;
// }

// unsigned int cHuffman::getLongueur() const {
//     return mLongueur;
// }

// sNoeud * cHuffman::getRacine() const {
//     return mRacine;
// }

// void cHuffman::setTrame(char *trame, unsigned int longueur) {
//     delete[] mtrame;
//     mLongueur = longueur;
//     mtrame = new char[longueur];
//     std::wmemcpy(mtrame, trame, longueur);
// }

// void cHuffman::setRacine(sNoeud *racine) {
//     mRacine = racine;
// }

// void cHuffman::HuffmanCodes(char *Donnee, double *Frequence, unsigned int Taille) {
//     /**
//      * Build the Huffman tree using a min-priority queue of nodes.
//      */
//     // Declare priority queue with custom comparator
//     std::priority_queue<sNoeud*, std::vector<sNoeud*>, compare> queue;
//     // Insert all symbols as leaf nodes
//     for (unsigned int i = 0; i < Taille; ++i)
//         queue.push(new sNoeud(Donnee[i], Frequence[i]));
//     // Build the tree
//     while (queue.size() > 1) {
//         sNoeud *gauche = queue.top(); queue.pop();
//         sNoeud *droit = queue.top(); queue.pop();
//         sNoeud *parent = new sNoeud('\0', gauche->mfreq + droit->mfreq);
//         parent->mgauche = gauche;
//         parent->mdroit = droit;
//         queue.push(parent);
//     }
//     mRacine = queue.top();
// }

// void cHuffman::AfficherHuffman(sNoeud *Racine) {
//     /** 
//      * Recursive function to print codes 
//     */
//     // Base case for recursion
//     if (!Racine) return;
//     // If leaf node, print the symbol and its code
//     if (Racine->mdonnee != '\0') {
//         std::cout << Racine->mdonnee << ": " << std::endl;
//     }
//     // Recursive for left and right subtrees
//     AfficherHuffman(Racine->mgauche);
//     AfficherHuffman(Racine->mdroit);
// }
