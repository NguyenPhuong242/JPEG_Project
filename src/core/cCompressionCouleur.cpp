/**
 * @file cCompressionCouleur.cpp
 * @author Khanh-Phuong NGUYEN
 * @date 2025-12-08
 * @brief Implements the cCompressionCouleur class for color image compression.
 */

#include "core/cCompressionCouleur.h"
#include "core/cCompression.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

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
    auto skip_ws_and_comments = [&]() {
        while (true) {
            in >> std::ws;
            if (in.peek() != '#') break;
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    };

    std::string magic;
    in >> magic;
    if (magic != "P6") return false;
    skip_ws_and_comments();
    in >> w;
    skip_ws_and_comments();
    in >> h;
    skip_ws_and_comments();
    int maxv;
    in >> maxv;
    if (!in || maxv != 255 || w == 0 || h == 0) return false;
    in.get(); // Consume whitespace
    size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
    rgb.resize(n);
    in.read(reinterpret_cast<char*>(rgb.data()), n);
    return static_cast<size_t>(in.gcount()) == n;
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
        std::vector<int> trame(1 + (pw/8)*(ph/8)*129);
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
