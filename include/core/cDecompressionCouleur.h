#ifndef JPEG_COMPRESSOR_CDECOMPRESSIONCOULEUR_H
#define JPEG_COMPRESSOR_CDECOMPRESSIONCOULEUR_H

#include <string>
#include <vector>

#include "core/ColorCodecCommon.h"
#include "core/cDecompression.h"

/**
 * @brief JPEG-like decompression front-end for color assets.
 *
 * The class reads the metadata produced by cCompressionCouleur, restores each
 * channel to the spatial domain, performs chroma upsampling, and converts the
 * YCbCr planes back to interleaved RGB pixels.
 */
class cDecompressionCouleur : public cDecompression
{
public:
	/**
	 * @brief Construct a color decompressor with an optional quality factor.
	 * @param qualite Quality value used during dequantization.
	 */
	explicit cDecompressionCouleur(unsigned int qualite = 50);

	/**
	 * @brief Reconstruct an RGB buffer from color codec artifacts.
	 * @param inputPrefix Prefix identifying the metadata and per-channel files.
	 * @param rgb Output vector receiving interleaved RGB pixels.
	 * @param width Output image width in pixels.
	 * @param height Output image height in pixels.
	 * @param modeOut Receives the subsampling mode stored in the metadata.
	 * @return true on success, false when any reconstruction step fails.
	 */
	bool decompressRGB(const std::string &inputPrefix,
					   std::vector<unsigned char> &rgb,
					   unsigned int &width,
					   unsigned int &height,
					   jpeg::ChromaSubsampling &modeOut) const;
};

#endif // JPEG_COMPRESSOR_CDECOMPRESSIONCOULEUR_H
