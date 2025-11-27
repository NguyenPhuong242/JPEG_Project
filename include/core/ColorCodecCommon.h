#ifndef JPEG_COMPRESSOR_COLORCODECCOMMON_H
#define JPEG_COMPRESSOR_COLORCODECCOMMON_H

#include <cstdint>

namespace jpeg
{
	/**
	 * @brief Chroma subsampling patterns supported by the color codec.
	 *
	 * Values match the classic JPEG sampling notation where the first digit
	 * is the luma sampling rate and the remaining digits describe the chroma
	 * decimation ratio.
	 */
	enum class ChromaSubsampling : std::uint32_t
	{
		Sampling444 = 0, /**< No subsampling (full resolution chroma). */
		Sampling422 = 1, /**< Horizontal chroma downsampling by 2. */
		Sampling420 = 2, /**< Horizontal and vertical downsampling by 2. */
		Sampling411 = 3  /**< Horizontal chroma downsampling by 4. */
	};

	/** @brief File signature for color metadata sidecar files ('YCC0'). */
	constexpr std::uint32_t kColorMetaMagic = 0x59434330;
	/** @brief Version number of the serialized metadata layout. */
	constexpr std::uint32_t kColorMetaVersion = 1;
	/** @brief Expected number of channels present in the metadata (Y, Cb, Cr). */
	constexpr std::uint32_t kExpectedChannelCount = 3;
}

#endif // JPEG_COMPRESSOR_COLORCODECCOMMON_H
