#ifndef JPEG_COMPRESSOR_CCOMPRESSIONCOULEUR_H
#define JPEG_COMPRESSOR_CCOMPRESSIONCOULEUR_H

#include <string>
#include <vector>

#include "core/ColorCodecCommon.h"
#include "core/cCompression.h"

/**
 * @brief JPEG-like compression front-end for RGB images.
 *
 * The class wraps the grayscale compressor to handle RGB input: it performs
 * RGB → YCbCr conversion, chroma subsampling, per-channel compression, and
 * persists metadata describing the resulting sidecar files.
 */
class cCompressionCouleur : public cCompression
{
public:
	/**
	 * @brief Construct a color compressor with a quality and subsampling mode.
	 * @param qualite Per-instance JPEG quality in [0,100].
	 * @param mode Chroma subsampling pattern to apply to Cb/Cr planes.
	 */
	explicit cCompressionCouleur(unsigned int qualite = 50,
								 jpeg::ChromaSubsampling mode = jpeg::ChromaSubsampling::Sampling420);

	/** @brief Change the chroma subsampling mode applied during compression. */
	void setSubsampling(jpeg::ChromaSubsampling mode);
	/** @brief Retrieve the currently configured subsampling mode. */
	jpeg::ChromaSubsampling getSubsampling() const;

	/**
	 * @brief Compress an RGB buffer and write per-channel artifacts to disk.
	 * @param rgb Interleaved RGB pixels (size width * height * 3).
	 * @param width Image width in pixels.
	 * @param height Image height in pixels.
	 * @param outputPrefix Prefix used for generated sidecar files (meta + channels).
	 * @return true on success, false if compression failed.
	 */
	bool compressRGB(const std::vector<unsigned char> &rgb,
					 unsigned int width,
					 unsigned int height,
					 const std::string &outputPrefix) const;

private:
	jpeg::ChromaSubsampling mSubsampling; /**< Active chroma subsampling mode. */
};

#endif // JPEG_COMPRESSOR_CCOMPRESSIONCOULEUR_H
