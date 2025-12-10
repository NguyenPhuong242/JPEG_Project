/**
 * @file cCompressionCouleur.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements the cCompressionCouleur class for color image compression.
 */

#include "core/cCompressionCouleur.h"
#include "core/cCompression.h"

#include <vector>
#include <fstream>
#include <cmath>
#include <cstring>
#include <string>

// --- Static Helper Functions for Color Image Processing ---

/**
 * @brief Reads a binary (P6) PPM file.
 * @param[in] path Path to the PPM file.
 * @param[out] w Width of the image.
 * @param[out] h Height of the image.
 * @param[out] rgb A vector to be filled with the raw RGB pixel data.
 * @return True on success, false on failure.
 */
static bool readPPM(const char *path, unsigned int &w, unsigned int &h, std::vector<unsigned char> &rgb)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::string magic; in >> magic; if (magic != "P6") return false;
    while (in.peek() == '#') { std::string line; std::getline(in, line); }
    in >> w >> h;
    int maxv; in >> maxv;
    in.get(); // Consume whitespace
    size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
    rgb.resize(n);
    in.read(reinterpret_cast<char*>(rgb.data()), n);
    return static_cast<size_t>(in.gcount()) == n;
}

/**
 * @brief Writes a binary (P6) PPM file.
 * @param[in] path Path for the output PPM file.
 * @param[in] w Width of the image.
 * @param[in] h Height of the image.
 * @param[in] rgb A vector containing the raw RGB pixel data.
 * @return True on success, false on failure.
 */
static bool writePPM(const char *path, unsigned int w, unsigned int h, const std::vector<unsigned char> &rgb)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out << "P6\n" << w << " " << h << "\n255\n";
    out.write(reinterpret_cast<const char*>(rgb.data()), rgb.size());
    return true;
}

/**
 * @brief Converts a pixel from RGB to YCbCr color space.
 * @param[in] R Red component (0-255).
 * @param[in] G Green component (0-255).
 * @param[in] B Blue component (0-255).
 * @param[out] Y Luma component (0-255).
 * @param[out] Cb Blue-difference chroma (0-255).
 * @param[out] Cr Red-difference chroma (0-255).
 */
static inline void rgb_to_ycbcr(unsigned char R, unsigned char G, unsigned char B, unsigned char &Y, unsigned char &Cb, unsigned char &Cr)
{
    double r = static_cast<double>(R), g = static_cast<double>(G), b = static_cast<double>(B);
    double y  =  0.299 * r + 0.587 * g + 0.114 * b;
    double cb = -0.168736 * r - 0.331264 * g + 0.5 * b + 128.0;
    double cr =  0.5 * r - 0.418688 * g - 0.081312 * b + 128.0;
    Y = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(y)))));
    Cb = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(cb)))));
    Cr = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(cr)))));
}

/**
 * @brief Converts a pixel from YCbCr to RGB color space.
 * @param[in] Y Luma component (0-255).
 * @param[in] Cb Blue-difference chroma (0-255).
 * @param[in] Cr Red-difference chroma (0-255).
 * @param[out] R Red component (0-255).
 * @param[out] G Green component (0-255).
 * @param[out] B Blue component (0-255).
 */
static inline void ycbcr_to_rgb(unsigned char Y, unsigned char Cb, unsigned char Cr, unsigned char &R, unsigned char &G, unsigned char &B)
{
    double y = static_cast<double>(Y), cb = static_cast<double>(Cb) - 128.0, cr = static_cast<double>(Cr) - 128.0;
    double r = y + 1.402 * cr;
    double g = y - 0.344136 * cb - 0.714136 * cr;
    double b = y + 1.772 * cb;
    R = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(r)))));
    G = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(g)))));
    B = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(b)))));
}

/**
 * @brief Subsamples a chroma plane using 4:2:0 scheme (averages a 2x2 block).
 * @param[in] src The source chroma plane.
 * @param[in] w Width of the source plane.
 * @param[in] h Height of the source plane.
 * @param[out] dst The destination vector for the subsampled data.
 * @param[out] cw The new width of the subsampled plane.
 * @param[out] ch The new height of the subsampled plane.
 */
static void subsample420(const std::vector<unsigned char> &src, unsigned int w, unsigned int h, std::vector<unsigned char> &dst, unsigned int &cw, unsigned int &ch)
{
    cw = (w + 1) / 2; ch = (h + 1) / 2;
    dst.assign(static_cast<size_t>(cw) * ch, 0);
    for (unsigned int y = 0; y < ch; ++y) {
        for (unsigned int x = 0; x < cw; ++x) {
            int sum = 0, count = 0;
            for (int yy = 0; yy < 2; ++yy) for (int xx = 0; xx < 2; ++xx) {
                if (x*2 + xx < w && y*2 + yy < h) {
                    sum += src[(y*2 + yy) * w + (x*2 + xx)];
                    ++count;
                }
            }
            dst[y * cw + x] = static_cast<unsigned char>((count > 0) ? (sum / count) : 0);
        }
    }
}

/**
 * @brief Subsamples a chroma plane using 4:2:2 scheme (averages 2 horizontal pixels).
 */
static void subsample422(const std::vector<unsigned char> &src, unsigned int w, unsigned int h, std::vector<unsigned char> &dst, unsigned int &cw, unsigned int &ch)
{
    cw = (w + 1) / 2; ch = h;
    dst.assign(static_cast<size_t>(cw) * ch, 0);
    for (unsigned int y = 0; y < ch; ++y) {
        for (unsigned int x = 0; x < cw; ++x) {
            int sum = 0, count = 0;
            for (int xx = 0; xx < 2; ++xx) {
                if (x*2 + xx < w) {
                    sum += src[y * w + (x*2 + xx)];
                    ++count;
                }
            }
            dst[y * cw + x] = static_cast<unsigned char>((count > 0) ? (sum / count) : 0);
        }
    }
}

/**
 * @brief Performs bilinear upsampling on a single channel.
 * @param[in] src The source (smaller) data plane.
 * @param[in] cw Width of the source plane.
 * @param[in] ch Height of the source plane.
 * @param[out] dst The destination vector for the upsampled data.
 * @param[in] w The target width.
 * @param[in] h The target height.
 */
static void upsample_bilinear(const std::vector<unsigned char> &src, unsigned int cw, unsigned int ch, std::vector<unsigned char> &dst, unsigned int w, unsigned int h)
{
    dst.assign(static_cast<size_t>(w) * h, 0);
    if (cw == 0 || ch == 0) return;
    double sx_ratio = (cw > 1) ? (double)(cw - 1) / (w - 1) : 0;
    double sy_ratio = (ch > 1) ? (double)(ch - 1) / (h - 1) : 0;

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

            double p00 = src[y0 * cw + x0];
            double p01 = src[y0 * cw + x1];
            double p10 = src[y1 * cw + x0];
            double p11 = src[y1 * cw + x1];
            
            double val = p00 * (1-u)*(1-v) + p01 * u*(1-v) + p10 * (1-u)*v + p11 * u*v;
            dst[j * w + i] = static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(std::round(val)))));
        }
    }
}

/**
 * @brief Pads a single-channel image to be a multiple of 8x8 blocks by replicating border pixels.
 * @param[in] src Source image data.
 * @param[in] w Width of source.
 * @param[in] h Height of source.
 * @param[out] dst Destination vector for padded data.
 * @param[out] pw New padded width.
 * @param[out] ph New padded height.
 */
static void padToMultipleOf8(const std::vector<unsigned char> &src, unsigned int w, unsigned int h, std::vector<unsigned char> &dst, unsigned int &pw, unsigned int &ph)
{
    pw = ((w + 7) / 8) * 8;
    ph = ((h + 7) / 8) * 8;
    if (pw == w && ph == h) {
        dst = src;
        return;
    }
    dst.assign(static_cast<size_t>(pw) * ph, 0);
    for (unsigned int y = 0; y < ph; ++y) {
        unsigned int sy = std::min(y, h - 1);
        for (unsigned int x = 0; x < pw; ++x) {
            unsigned int sx = std::min(x, w - 1);
            dst[y * pw + x] = src[sy * w + sx];
        }
    }
}

/**
 * @brief Creates a 2D array of row pointers from a flat image buffer.
 * @param[in] buf The flat vector containing the image data.
 * @param[in] w Width of the image.
 * @param[in] h Height of the image.
 * @return A newly allocated array of row pointers (unsigned char**).
 * @note The caller is responsible for deleting the returned array of pointers.
 */
static unsigned char **makeRowPointers(std::vector<unsigned char> &buf, unsigned int w, unsigned int h)
{
    unsigned char **rows = new unsigned char*[h];
    for (unsigned int y = 0; y < h; ++y) rows[y] = buf.data() + static_cast<size_t>(y) * w;
    return rows;
}


// --- cCompressionCouleur Method Implementations ---

cCompressionCouleur::cCompressionCouleur() : cCompression(), mSubsamplingH(1), mSubsamplingV(1) {}

cCompressionCouleur::cCompressionCouleur(unsigned int largeur, unsigned int hauteur, unsigned int qualite, unsigned int subsamplingH, unsigned int subsamplingV, unsigned char **buffer)
    : cCompression(largeur, hauteur, qualite, buffer), mSubsamplingH(subsamplingH), mSubsamplingV(subsamplingV) {}

cCompressionCouleur::~cCompressionCouleur() {}

void cCompressionCouleur::setSubsamplingH(unsigned int subsamplingH) { mSubsamplingH = subsamplingH; }
void cCompressionCouleur::setSubsamplingV(unsigned int subsamplingV) { mSubsamplingV = subsamplingV; }
unsigned int cCompressionCouleur::getSubsamplingH() const { return mSubsamplingH; }
unsigned int cCompressionCouleur::getSubsamplingV() const { return mSubsamplingV; }

bool cCompressionCouleur::CompressPPM(const char *ppmPath, const char *basename, unsigned int qual, unsigned int subsamplingMode)
{
    // 1. Read PPM file
    unsigned int w=0, h=0;
    std::vector<unsigned char> rgb;
    if (!readPPM(ppmPath, w, h, rgb)) return false;

    // 2. Convert RGB to YCbCr color space
    std::vector<unsigned char> Y(w*h), Cb_full(w*h), Cr_full(w*h);
    for (unsigned int i = 0; i < h; ++i) {
        for (unsigned int j = 0; j < w; ++j) {
            size_t idx = (static_cast<size_t>(i) * w + j) * 3;
            rgb_to_ycbcr(rgb[idx], rgb[idx+1], rgb[idx+2], Y[i*w+j], Cb_full[i*w+j], Cr_full[i*w+j]);
        }
    }

    // 3. Perform chroma subsampling
    std::vector<unsigned char> Cb_sub, Cr_sub;
    unsigned int cw=w, ch=h;
    if (subsamplingMode == 420) {
        subsample420(Cb_full, w, h, Cb_sub, cw, ch);
        subsample420(Cr_full, w, h, Cr_sub, cw, ch);
    } else if (subsamplingMode == 422) {
        subsample422(Cb_full, w, h, Cb_sub, cw, ch);
        subsample422(Cr_full, w, h, Cr_sub, cw, ch);
    } else { // 444
        Cb_sub = Cb_full; Cr_sub = Cr_full;
    }

    // 4. Pad each color plane to be a multiple of 8x8
    std::vector<unsigned char> Y_pad, Cb_pad, Cr_pad;
    unsigned int Ypw, Yph, Cbpw, Cbph, Crpw, Crph;
    padToMultipleOf8(Y, w, h, Y_pad, Ypw, Yph);
    padToMultipleOf8(Cb_sub, cw, ch, Cb_pad, Cbpw, Cbph);
    padToMultipleOf8(Cr_sub, cw, ch, Cr_pad, Crpw, Crph);

    // 5. Compress each plane individually
    cCompression::setQualiteGlobale(qual);
    auto compress_plane = [&](std::vector<unsigned char>& data, unsigned int pw, unsigned int ph, const char* suffix) {
        unsigned char **rows = makeRowPointers(data, pw, ph);
        cCompression comp(pw, ph, qual, rows);
        std::vector<int> trame(1 + (pw/8)*(ph/8)*128);
        comp.RLE(trame.data());
        std::string filename = std::string(basename) + suffix;
        comp.Compression_JPEG(trame.data(), filename.c_str());
        delete[] rows;
    };
    compress_plane(Y_pad, Ypw, Yph, "_Y.huff");
    compress_plane(Cb_pad, Cbpw, Cbph, "_Cb.huff");
    compress_plane(Cr_pad, Crpw, Crph, "_Cr.huff");

    // 6. Write metadata file
    std::string meta_path = std::string(basename) + ".meta";
    std::ofstream m(meta_path, std::ios::binary);
    if (m) {
        uint32_t val;
        val = w; m.write(reinterpret_cast<const char*>(&val), sizeof(val));
        val = h; m.write(reinterpret_cast<const char*>(&val), sizeof(val));
        val = cw; m.write(reinterpret_cast<const char*>(&val), sizeof(val));
        val = ch; m.write(reinterpret_cast<const char*>(&val), sizeof(val));
        val = subsamplingMode; m.write(reinterpret_cast<const char*>(&val), sizeof(val));
        val = qual; m.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }
    return true;
}

bool cCompressionCouleur::DecompressToPPM(const char *basename, const char *outppm)
{
    // 1. Read metadata
    std::string meta_path = std::string(basename) + ".meta";
    std::ifstream m(meta_path, std::ios::binary);
    if (!m) return false;
    uint32_t w, h, cw, ch, sm, q;
    m.read(reinterpret_cast<char*>(&w), sizeof(w)); m.read(reinterpret_cast<char*>(&h), sizeof(h));
    m.read(reinterpret_cast<char*>(&cw), sizeof(cw)); m.read(reinterpret_cast<char*>(&ch), sizeof(ch));
    m.read(reinterpret_cast<char*>(&sm), sizeof(sm)); m.read(reinterpret_cast<char*>(&q), sizeof(q));
    m.close();

    cCompression::setQualiteGlobale(q);

    // 2. Decompress each color plane
    auto decompress_plane = [&](const char* suffix, unsigned int& pw, unsigned int& ph) -> std::vector<unsigned char> {
        std::string filename = std::string(basename) + suffix;
        cCompression comp;
        unsigned char** rows = comp.Decompression_JPEG(filename.c_str());
        if (!rows) return {};
        pw = comp.getLargeur();
        ph = comp.getHauteur();
        std::vector<unsigned char> data(pw * ph);
        for(unsigned int y=0; y<ph; ++y) std::memcpy(data.data() + y*pw, rows[y], pw);
        delete[] rows[0]; // Free buffer
        delete[] rows;    // Free row pointers
        return data;
    };
    unsigned int Ypw, Yph, Cbpw, Cbph, Crpw, Crph;
    std::vector<unsigned char> Y_pad = decompress_plane("_Y.huff", Ypw, Yph);
    std::vector<unsigned char> Cb_pad = decompress_plane("_Cb.huff", Cbpw, Cbph);
    std::vector<unsigned char> Cr_pad = decompress_plane("_Cr.huff", Crpw, Crph);
    if (Y_pad.empty()) return false;
    // If chroma planes are empty (very small files), fallback to neutral chroma (128)
    if (Cb_pad.empty()) {
        std::cerr << "[DecompressToPPM] Cb plane empty, using neutral 128 values\n";
        Cbpw = cw; Cbph = ch;
        Cb_pad.assign(static_cast<size_t>(Cbpw) * Cbph, 128);
    }
    if (Cr_pad.empty()) {
        std::cerr << "[DecompressToPPM] Cr plane empty, using neutral 128 values\n";
        Crpw = cw; Crph = ch;
        Cr_pad.assign(static_cast<size_t>(Crpw) * Crph, 128);
    }

    // 3. Upsample chroma planes if they were subsampled
    std::vector<unsigned char> Cb_full, Cr_full;
    if (cw != w || ch != h) {
        upsample_bilinear(Cb_pad, Cbpw, Cbph, Cb_full, w, h);
        upsample_bilinear(Cr_pad, Crpw, Crph, Cr_full, w, h);
    } else {
        Cb_full = Cb_pad;
        Cr_full = Cr_pad;
    }

    // 4. Convert YCbCr back to RGB, cropping any padding
    std::vector<unsigned char> rgb(static_cast<size_t>(w) * h * 3);
    for (unsigned int y=0; y<h; ++y) {
        for (unsigned int x=0; x<w; ++x) {
            unsigned char Yv = Y_pad[y*Ypw + x];
            unsigned char Cbv = Cb_full[y*w + x];
            unsigned char Crv = Cr_full[y*w + x];
            size_t idx = (static_cast<size_t>(y) * w + x) * 3;
            ycbcr_to_rgb(Yv, Cbv, Crv, rgb[idx], rgb[idx+1], rgb[idx+2]);
        }
    }

    // 5. Write the final PPM file
    return writePPM(outppm, w, h, rgb);
}
