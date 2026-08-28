/**
 * @file cDecompressionCouleur.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements color JPEG-like decompression.
 */

#include "core/cDecompressionCouleur.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static bool writePPM(const char *path, unsigned int w, unsigned int h, const std::vector<unsigned char> &rgb)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << w << " " << h << "\n255\n";
    out.write(reinterpret_cast<const char*>(rgb.data()), rgb.size());
    return true;
}

static inline void ycbcr_to_rgb(unsigned char Y, unsigned char Cb, unsigned char Cr, unsigned char &R, unsigned char &G, unsigned char &B)
{
    double y = static_cast<double>(Y);
    double cb = static_cast<double>(Cb) - 128.0;
    double cr = static_cast<double>(Cr) - 128.0;
    double r = y + 1.402 * cr;
    double g = y - 0.344136 * cb - 0.714136 * cr;
    double b = y + 1.772 * cb;
    R = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(r)))));
    G = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(g)))));
    B = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(b)))));
}

static bool crop_plane(const std::vector<unsigned char> &src,
                       unsigned int src_w,
                       unsigned int src_h,
                       unsigned int dst_w,
                       unsigned int dst_h,
                       std::vector<unsigned char> &dst)
{
    if (src_w < dst_w || src_h < dst_h) return false;
    if (src.size() < static_cast<size_t>(src_w) * src_h) return false;

    dst.assign(static_cast<size_t>(dst_w) * dst_h, 0);
    for (unsigned int y = 0; y < dst_h; ++y) {
        std::memcpy(dst.data() + static_cast<size_t>(y) * dst_w,
                    src.data() + static_cast<size_t>(y) * src_w,
                    dst_w);
    }
    return true;
}

static void upsample_bilinear(const std::vector<unsigned char> &src,
                              unsigned int cw,
                              unsigned int ch,
                              std::vector<unsigned char> &dst,
                              unsigned int w,
                              unsigned int h)
{
    dst.assign(static_cast<size_t>(w) * h, 0);
    if (cw == 0 || ch == 0 || w == 0 || h == 0) return;
    double sx_ratio = (cw > 1 && w > 1) ? static_cast<double>(cw - 1) / (w - 1) : 0.0;
    double sy_ratio = (ch > 1 && h > 1) ? static_cast<double>(ch - 1) / (h - 1) : 0.0;

    for (unsigned int j = 0; j < h; ++j) {
        double sy = sy_ratio * j;
        unsigned int y0 = static_cast<unsigned int>(sy);
        unsigned int y1 = std::min(y0 + 1, ch - 1);
        double v = sy - y0;

        for (unsigned int i = 0; i < w; ++i) {
            double sx = sx_ratio * i;
            unsigned int x0 = static_cast<unsigned int>(sx);
            unsigned int x1 = std::min(x0 + 1, cw - 1);
            double u = sx - x0;

            double p00 = src[static_cast<size_t>(y0) * cw + x0];
            double p01 = src[static_cast<size_t>(y0) * cw + x1];
            double p10 = src[static_cast<size_t>(y1) * cw + x0];
            double p11 = src[static_cast<size_t>(y1) * cw + x1];

            double val = p00 * (1 - u) * (1 - v) +
                         p01 * u * (1 - v) +
                         p10 * (1 - u) * v +
                         p11 * u * v;
            dst[static_cast<size_t>(j) * w + i] =
                static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(val)))));
        }
    }
}

cDecompressionCouleur::cDecompressionCouleur() : cDecompression()
{
}

cDecompressionCouleur::cDecompressionCouleur(unsigned int largeur, unsigned int hauteur, unsigned int qualite, unsigned char **buffer)
    : cDecompression(largeur, hauteur, qualite, buffer)
{
}

cDecompressionCouleur::~cDecompressionCouleur()
{
}

bool cDecompressionCouleur::DecompressToPPM(const char *basename, const char *outppm)
{
    if (!basename || !outppm) return false;

    std::string meta_path = std::string(basename) + ".meta";
    std::ifstream m(meta_path, std::ios::binary);
    if (!m) return false;

    uint32_t w = 0, h = 0, cw = 0, ch = 0, sm = 0, q = 0;
    m.read(reinterpret_cast<char*>(&w), sizeof(w));
    m.read(reinterpret_cast<char*>(&h), sizeof(h));
    m.read(reinterpret_cast<char*>(&cw), sizeof(cw));
    m.read(reinterpret_cast<char*>(&ch), sizeof(ch));
    m.read(reinterpret_cast<char*>(&sm), sizeof(sm));
    m.read(reinterpret_cast<char*>(&q), sizeof(q));
    if (!m || w == 0 || h == 0 || cw == 0 || ch == 0) return false;
    if (sm != 444 && sm != 422 && sm != 420) return false;

    cCompression::setQualiteGlobale(q);
    setQualite(q);

    auto decompress_plane = [&](const char* suffix, unsigned int& pw, unsigned int& ph) -> std::vector<unsigned char> {
        std::string filename = std::string(basename) + suffix;
        cDecompression decompressor;
        unsigned char** rows = decompressor.Decompression_JPEG(filename.c_str());
        if (!rows) return {};

        pw = decompressor.getLargeur();
        ph = decompressor.getHauteur();
        std::vector<unsigned char> data(static_cast<size_t>(pw) * ph);
        for (unsigned int y = 0; y < ph; ++y) {
            std::memcpy(data.data() + static_cast<size_t>(y) * pw, rows[y], pw);
        }
        delete[] rows[0];
        delete[] rows;
        return data;
    };

    unsigned int Ypw = 0, Yph = 0, Cbpw = 0, Cbph = 0, Crpw = 0, Crph = 0;
    std::vector<unsigned char> Y_pad = decompress_plane("_Y.huff", Ypw, Yph);
    std::vector<unsigned char> Cb_pad = decompress_plane("_Cb.huff", Cbpw, Cbph);
    std::vector<unsigned char> Cr_pad = decompress_plane("_Cr.huff", Crpw, Crph);
    if (Y_pad.empty() || Cb_pad.empty() || Cr_pad.empty()) return false;
    if (Ypw < w || Yph < h) return false;

    std::vector<unsigned char> Y_full, Cb_crop, Cr_crop;
    if (!crop_plane(Y_pad, Ypw, Yph, w, h, Y_full)) return false;
    if (!crop_plane(Cb_pad, Cbpw, Cbph, cw, ch, Cb_crop)) return false;
    if (!crop_plane(Cr_pad, Crpw, Crph, cw, ch, Cr_crop)) return false;

    std::vector<unsigned char> Cb_full, Cr_full;
    if (cw != w || ch != h) {
        upsample_bilinear(Cb_crop, cw, ch, Cb_full, w, h);
        upsample_bilinear(Cr_crop, cw, ch, Cr_full, w, h);
    } else {
        Cb_full = Cb_crop;
        Cr_full = Cr_crop;
    }

    std::vector<unsigned char> rgb(static_cast<size_t>(w) * h * 3);
    for (unsigned int y = 0; y < h; ++y) {
        for (unsigned int x = 0; x < w; ++x) {
            unsigned char Yv = Y_full[static_cast<size_t>(y) * w + x];
            unsigned char Cbv = Cb_full[static_cast<size_t>(y) * w + x];
            unsigned char Crv = Cr_full[static_cast<size_t>(y) * w + x];
            size_t idx = (static_cast<size_t>(y) * w + x) * 3;
            ycbcr_to_rgb(Yv, Cbv, Crv, rgb[idx], rgb[idx + 1], rgb[idx + 2]);
        }
    }

    return writePPM(outppm, w, h, rgb);
}
