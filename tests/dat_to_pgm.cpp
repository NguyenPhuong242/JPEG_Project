#include <fstream>
#include <vector>
#include <cmath>
#include <iostream>

int main() {
    std::ifstream in("../../tests/lena.dat");
    if (!in) {
        std::cerr << "Cannot open lena.dat\n";
        return 1;
    }

    std::vector<int> vals;
    int v;
    while (in >> v) {
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        vals.push_back(v);
    }
    in.close();

    if (vals.empty()) {
        std::cerr << "No data in lena.dat\n";
        return 1;
    }

    std::size_t N = vals.size();
    int side = static_cast<int>(std::sqrt(static_cast<double>(N)) + 0.5);
    if (side * side != static_cast<int>(N)) {
        std::cerr << "Pixel count " << N << " is not a perfect square\n";
        return 1;
    }

    // Write ASCII PGM (P2)
    std::ofstream out("../../tests/lena.pgm");
    if (!out) {
        std::cerr << "Cannot open lena.pgm for writing\n";
        return 1;
    }

    out << "P2\n"            // magic word for ASCII PGM
        << side << " " << side << "\n"
        << 255 << "\n";

    for (std::size_t i = 0; i < N; ++i) {
        out << vals[i];
        if ((i + 1) % side == 0)
            out << '\n';
        else
            out << ' ';
    }

    std::cout << "Wrote lena.pgm (" << side << "x" << side << ")\n";
    return 0;
}
