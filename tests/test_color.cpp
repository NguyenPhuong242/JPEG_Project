// Simple functional test for color compression pipeline
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <cstdio>
#include "core/cCompressionCouleur.h"

static bool file_exists(const std::string &path) {
    struct stat buf;
    return (stat(path.c_str(), &buf) == 0);
}

int main() {
    const char *input_ppm = "lenna_color.ppm";
    const std::string basename = "tmp_test_color";
    const std::string outppm = "tmp_decomp_color.ppm";

    if (!file_exists(input_ppm)) {
        std::cerr << "Input PPM not found: " << input_ppm << std::endl;
        return 1;
    }

    cCompressionCouleur cc;
    unsigned int quality = 50;
    unsigned int subsampling = 444; // 4:4:4

    bool ok = cc.CompressPPM(input_ppm, basename.c_str(), quality, subsampling);
    if (!ok) {
        std::cerr << "CompressPPM failed" << std::endl;
        return 1;
    }

    const std::string yfile = basename + "_Y.huff";
    const std::string cbfile = basename + "_Cb.huff";
    const std::string crfile = basename + "_Cr.huff";
    const std::string metafile = basename + ".meta";

    if (!file_exists(yfile) || !file_exists(cbfile) || !file_exists(crfile) || !file_exists(metafile)) {
        std::cerr << "Missing output files after compression" << std::endl;
        return 1;
    }

    bool ok2 = cc.DecompressToPPM(basename.c_str(), outppm.c_str());
    if (!ok2) {
        std::cerr << "DecompressToPPM failed" << std::endl;
        // try to cleanup what we can
        std::remove(yfile.c_str()); std::remove(cbfile.c_str()); std::remove(crfile.c_str()); std::remove(metafile.c_str());
        return 1;
    }

    if (!file_exists(outppm)) {
        std::cerr << "Decompressed PPM not produced" << std::endl;
        std::remove(yfile.c_str()); std::remove(cbfile.c_str()); std::remove(crfile.c_str()); std::remove(metafile.c_str());
        return 1;
    }

    // Quick sanity check: first line should contain P6
    std::ifstream fin(outppm, std::ios::binary);
    if (!fin) {
        std::cerr << "Cannot open decompressed PPM" << std::endl;
        return 1;
    }
    std::string header;
    if (!std::getline(fin, header)) {
        std::cerr << "Empty decompressed PPM" << std::endl;
        fin.close();
        return 1;
    }
    fin.close();

    if (header.find("P6") == std::string::npos) {
        std::cerr << "Decompressed file does not look like P6 PPM" << std::endl;
        return 1;
    }

    // Cleanup generated files
    std::remove(yfile.c_str());
    std::remove(cbfile.c_str());
    std::remove(crfile.c_str());
    std::remove(metafile.c_str());
    std::remove(outppm.c_str());

    std::cout << "testcolor: OK" << std::endl;
    return 0;
}
