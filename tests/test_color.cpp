// Simple functional test for color compression pipeline
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include "core/cCompressionCouleur.h"
#include "core/cDecompressionCouleur.h"

static bool file_exists(const std::string &path) {
    struct stat buf;
    return (stat(path.c_str(), &buf) == 0);
}

static bool read_ppm(const std::string &path, unsigned int &w, unsigned int &h, std::vector<unsigned char> &rgb) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::string magic;
    int maxv = 0;
    in >> magic >> w >> h >> maxv;
    in.get();
    if (!in || magic != "P6" || maxv != 255 || w == 0 || h == 0) return false;
    rgb.resize(static_cast<size_t>(w) * h * 3);
    in.read(reinterpret_cast<char *>(rgb.data()), rgb.size());
    return static_cast<size_t>(in.gcount()) == rgb.size();
}

int main() {
    const char *input_ppm = "sample_color.ppm";
    const std::string basename = "tmp_test_color";
    const std::string outppm = "tmp_decomp_color.ppm";
    const std::string yfile = basename + "_Y.huff";
    const std::string cbfile = basename + "_Cb.huff";
    const std::string crfile = basename + "_Cr.huff";
    const std::string metafile = basename + ".meta";
    auto cleanup = [&]() {
        std::remove(yfile.c_str());
        std::remove(cbfile.c_str());
        std::remove(crfile.c_str());
        std::remove(metafile.c_str());
        std::remove(outppm.c_str());
    };

    if (!file_exists(input_ppm)) {
        std::cerr << "Input PPM not found: " << input_ppm << std::endl;
        return 1;
    }

    cCompressionCouleur cc;
    unsigned int quality = 90;
    unsigned int subsampling = 444; // 4:4:4

    bool ok = cc.CompressPPM(input_ppm, basename.c_str(), quality, subsampling);
    if (!ok) {
        std::cerr << "CompressPPM failed" << std::endl;
        cleanup();
        return 1;
    }

    if (!file_exists(yfile) || !file_exists(cbfile) || !file_exists(crfile) || !file_exists(metafile)) {
        std::cerr << "Missing output files after compression" << std::endl;
        cleanup();
        return 1;
    }

    const std::string bad_quality_base = "tmp_bad_quality_color";
    const std::string bad_quality_meta = bad_quality_base + ".meta";
    {
        std::ofstream bad_meta(bad_quality_meta, std::ios::binary);
        uint32_t w = 8, h = 8, cw = 8, ch = 8, sm = 444, q = 0;
        bad_meta.write(reinterpret_cast<const char*>(&w), sizeof(w));
        bad_meta.write(reinterpret_cast<const char*>(&h), sizeof(h));
        bad_meta.write(reinterpret_cast<const char*>(&cw), sizeof(cw));
        bad_meta.write(reinterpret_cast<const char*>(&ch), sizeof(ch));
        bad_meta.write(reinterpret_cast<const char*>(&sm), sizeof(sm));
        bad_meta.write(reinterpret_cast<const char*>(&q), sizeof(q));
    }
    cDecompressionCouleur bad_dc;
    if (bad_dc.DecompressToPPM(bad_quality_base.c_str(), "tmp_bad_quality_color.ppm")) {
        std::cerr << "Invalid color quality metadata was accepted" << std::endl;
        std::remove(bad_quality_meta.c_str());
        std::remove("tmp_bad_quality_color.ppm");
        cleanup();
        return 1;
    }
    std::remove(bad_quality_meta.c_str());
    std::remove("tmp_bad_quality_color.ppm");

    cDecompressionCouleur dc;
    bool ok2 = dc.DecompressToPPM(basename.c_str(), outppm.c_str());
    if (!ok2) {
        std::cerr << "DecompressToPPM failed" << std::endl;
        cleanup();
        return 1;
    }

    if (!file_exists(outppm)) {
        std::cerr << "Decompressed PPM not produced" << std::endl;
        cleanup();
        return 1;
    }

    unsigned int in_w = 0, in_h = 0, out_w = 0, out_h = 0;
    std::vector<unsigned char> input_rgb, output_rgb;
    if (!read_ppm(input_ppm, in_w, in_h, input_rgb) ||
        !read_ppm(outppm, out_w, out_h, output_rgb)) {
        std::cerr << "Failed to read input or decompressed PPM" << std::endl;
        cleanup();
        return 1;
    }

    if (in_w != out_w || in_h != out_h || input_rgb.size() != output_rgb.size()) {
        std::cerr << "Decompressed PPM dimensions/data size mismatch" << std::endl;
        cleanup();
        return 1;
    }

    bool has_color = false;
    for (size_t i = 0; i + 2 < output_rgb.size(); i += 3) {
        if (output_rgb[i] != output_rgb[i + 1] || output_rgb[i] != output_rgb[i + 2]) {
            has_color = true;
            break;
        }
    }
    if (!has_color) {
        std::cerr << "Decompressed image lost chroma information" << std::endl;
        cleanup();
        return 1;
    }

    cleanup();

    std::cout << "testcolor: OK" << std::endl;
    return 0;
}
