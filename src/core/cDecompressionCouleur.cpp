#include "core/cDecompressionCouleur.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <utility>

#include "core/cCompression.h"

namespace
{
 using jpeg::ChromaSubsampling;

 /** @brief Metadata per encoded channel restored from the sidecar file. */
 struct ChannelMetadata
 {
 	std::uint8_t id;                  /**< Channel identifier (0=Y,1=Cb,2=Cr). */
 	unsigned int width;               /**< Channel width in pixels. */
 	unsigned int height;              /**< Channel height in pixels. */
 	std::string filename;             /**< Huffman bitstream filename. */
 	std::vector<char> symbols;        /**< Huffman symbols for the channel. */
 	std::vector<double> frequencies;  /**< Huffman frequencies. */
 };

 /** @brief Header information parsed from the metadata sidecar. */
 struct MetaHeader
 {
 	unsigned int width;
 	unsigned int height;
 	unsigned int quality;
 	ChromaSubsampling subsampling;
 	std::vector<ChannelMetadata> channels;
 };

 /**
  * @brief Parse the color metadata sidecar file.
  * @throws std::runtime_error if the file is missing or invalid.
  */
 MetaHeader readMetadata(const std::string &path)
 {
		std::ifstream in(path, std::ios::binary);
		if (!in)
		{
			throw std::runtime_error("Cannot open color metadata: " + path);
		}

		std::uint32_t magic = 0;
		std::uint32_t version = 0;
		in.read(reinterpret_cast<char *>(&magic), sizeof(magic));
		in.read(reinterpret_cast<char *>(&version), sizeof(version));
		if (magic != jpeg::kColorMetaMagic || version != jpeg::kColorMetaVersion)
		{
			throw std::runtime_error("Unsupported color metadata format: " + path);
		}

		MetaHeader header{};
		std::uint32_t width = 0;
		std::uint32_t height = 0;
		std::uint32_t quality = 0;
		std::uint32_t subsampling = 0;
		std::uint32_t channelCount = 0;

		in.read(reinterpret_cast<char *>(&width), sizeof(width));
		in.read(reinterpret_cast<char *>(&height), sizeof(height));
		in.read(reinterpret_cast<char *>(&quality), sizeof(quality));
		in.read(reinterpret_cast<char *>(&subsampling), sizeof(subsampling));
		in.read(reinterpret_cast<char *>(&channelCount), sizeof(channelCount));

		header.width = width;
		header.height = height;
		header.quality = quality;
		header.subsampling = static_cast<ChromaSubsampling>(subsampling);

		header.channels.reserve(channelCount);
		for (std::uint32_t idx = 0; idx < channelCount; ++idx)
		{
			ChannelMetadata meta{};
			in.read(reinterpret_cast<char *>(&meta.id), sizeof(meta.id));
			in.read(reinterpret_cast<char *>(&meta.width), sizeof(meta.width));
			in.read(reinterpret_cast<char *>(&meta.height), sizeof(meta.height));

			std::uint32_t nameLen = 0;
			in.read(reinterpret_cast<char *>(&nameLen), sizeof(nameLen));
			meta.filename.resize(nameLen);
			in.read(meta.filename.data(), nameLen);

			std::uint32_t symbolCount = 0;
			in.read(reinterpret_cast<char *>(&symbolCount), sizeof(symbolCount));
			meta.symbols.resize(symbolCount);
			meta.frequencies.resize(symbolCount);
			for (std::uint32_t i = 0; i < symbolCount; ++i)
			{
				std::int8_t sym = 0;
				double freq = 0.0;
				in.read(reinterpret_cast<char *>(&sym), sizeof(sym));
				in.read(reinterpret_cast<char *>(&freq), sizeof(freq));
				meta.symbols[i] = static_cast<char>(sym);
				meta.frequencies[i] = freq;
			}

			header.channels.push_back(std::move(meta));
		}

		return header;
	}

	 /**
	  * @brief Upsample a plane using nearest-neighbour reconstruction.
	  * @param src Source plane.
	  * @param srcWidth Source width.
	  * @param srcHeight Source height.
	  * @param dstWidth Destination width.
	  * @param dstHeight Destination height.
	  * @return Upsampled plane.
	  */
	 std::vector<unsigned char> upsamplePlane(const std::vector<unsigned char> &src,
	 			   unsigned int srcWidth,
	 			   unsigned int srcHeight,
	 			   unsigned int dstWidth,
	 			   unsigned int dstHeight)
	{
		std::vector<unsigned char> dst(static_cast<std::size_t>(dstWidth) * dstHeight);
		const double scaleX = static_cast<double>(srcWidth) / static_cast<double>(dstWidth);
		const double scaleY = static_cast<double>(srcHeight) / static_cast<double>(dstHeight);

		for (unsigned int y = 0; y < dstHeight; ++y)
		{
			const unsigned int srcY = std::min(static_cast<unsigned int>(std::floor(y * scaleY)), srcHeight - 1U);
			for (unsigned int x = 0; x < dstWidth; ++x)
			{
				const unsigned int srcX = std::min(static_cast<unsigned int>(std::floor(x * scaleX)), srcWidth - 1U);
				dst[static_cast<std::size_t>(y) * dstWidth + x] = src[static_cast<std::size_t>(srcY) * srcWidth + srcX];
			}
		}

		return dst;
	}

	 /**
	  * @brief Copy decompressed rows into a flat vector buffer.
	  * @param plane Pointer-to-rows buffer.
	  * @param width Row width.
	  * @param height Number of rows.
	  * @return Copy of the plane data.
	  */
	 std::vector<unsigned char> copyPlane(unsigned char **plane,
	 		unsigned int width,
	 		unsigned int height)
	{
		std::vector<unsigned char> buffer(static_cast<std::size_t>(width) * height);
		for (unsigned int y = 0; y < height; ++y)
		{
			std::copy_n(plane[y], width, buffer.begin() + static_cast<std::size_t>(y) * width);
		}
		return buffer;
	}

	 /**
	  * @brief Convert YCbCr planes back to interleaved RGB pixels.
	  * @param Y Luma plane.
	  * @param Cb Blue-difference plane.
	  * @param Cr Red-difference plane.
	  * @param width Image width.
	  * @param height Image height.
	  * @return Interleaved RGB buffer.
	  */
	 std::vector<unsigned char> convertToRGB(const std::vector<unsigned char> &Y,
	 				  const std::vector<unsigned char> &Cb,
	 				  const std::vector<unsigned char> &Cr,
	 				  unsigned int width,
	 				  unsigned int height)
	{
		std::vector<unsigned char> rgb(static_cast<std::size_t>(width) * height * 3U);
		for (unsigned int y = 0; y < height; ++y)
		{
			for (unsigned int x = 0; x < width; ++x)
			{
				const std::size_t idx = static_cast<std::size_t>(y) * width + x;
				const double yy = static_cast<double>(Y[idx]);
				const double cb = static_cast<double>(Cb[idx]) - 128.0;
				const double cr = static_cast<double>(Cr[idx]) - 128.0;

				double r = yy + 1.402 * cr;
				double g = yy - 0.344136 * cb - 0.714136 * cr;
				double b = yy + 1.772 * cb;

				r = std::clamp(r, 0.0, 255.0);
				g = std::clamp(g, 0.0, 255.0);
				b = std::clamp(b, 0.0, 255.0);

				rgb[idx * 3 + 0] = static_cast<unsigned char>(std::lround(r));
				rgb[idx * 3 + 1] = static_cast<unsigned char>(std::lround(g));
				rgb[idx * 3 + 2] = static_cast<unsigned char>(std::lround(b));
			}
		}
		return rgb;
	}
}

/** @brief Construct a color decompressor with a desired quality factor. */
cDecompressionCouleur::cDecompressionCouleur(unsigned int qualite)
	: cDecompression(0, 0, qualite, nullptr)
{
}

/**
 * @brief Rebuild an RGB image from color compression artifacts.
 * @param inputPrefix Prefix of the color metadata and channel files.
 * @param rgb Output RGB buffer.
 * @param width Output width in pixels.
 * @param height Output height in pixels.
 * @param modeOut Receives the stored subsampling mode.
 * @return true on success, false on any error.
 */
bool cDecompressionCouleur::decompressRGB(const std::string &inputPrefix,
		 std::vector<unsigned char> &rgb,
		 unsigned int &width,
		 unsigned int &height,
		 jpeg::ChromaSubsampling &modeOut) const
{
	MetaHeader header;
	try
	{
		header = readMetadata(inputPrefix + ".meta");
	}
	catch (const std::exception &)
	{
		return false;
	}

	if (header.channels.size() != jpeg::kExpectedChannelCount)
	{
		return false;
	}

	width = header.width;
	height = header.height;
	modeOut = header.subsampling;

	std::vector<unsigned char> Y;
	std::vector<unsigned char> Cb;
	std::vector<unsigned char> Cr;
	unsigned int cbWidth = 0;
	unsigned int cbHeight = 0;
	unsigned int crWidth = 0;
	unsigned int crHeight = 0;

	for (const ChannelMetadata &meta : header.channels)
	{
		if (meta.symbols.empty() || meta.frequencies.empty())
		{
			return false;
		}

		cCompression::storeHuffmanTable(meta.symbols.data(), meta.frequencies.data(), static_cast<unsigned int>(meta.symbols.size()));

		cDecompression dec(0, 0, header.quality, nullptr);
		unsigned char **plane = dec.Decompression_JPEG(meta.filename.c_str());
		if (!plane)
		{
			return false;
		}

		const std::vector<unsigned char> planeData = copyPlane(plane, meta.width, meta.height);

		if (meta.id == 0U)
		{
			Y = planeData;
		}
		else if (meta.id == 1U)
		{
			Cb = planeData;
			cbWidth = meta.width;
			cbHeight = meta.height;
		}
		else if (meta.id == 2U)
		{
			Cr = planeData;
			crWidth = meta.width;
			crHeight = meta.height;
		}
	}

	if (Y.empty() || Cb.empty() || Cr.empty())
	{
		return false;
	}
	if (cbWidth == 0U || cbHeight == 0U || crWidth == 0U || crHeight == 0U)
	{
		return false;
	}

	const std::vector<unsigned char> CbUp = upsamplePlane(Cb, cbWidth, cbHeight, width, height);
	const std::vector<unsigned char> CrUp = upsamplePlane(Cr, crWidth, crHeight, width, height);

	rgb = convertToRGB(Y, CbUp, CrUp, width, height);
	return true;
}
