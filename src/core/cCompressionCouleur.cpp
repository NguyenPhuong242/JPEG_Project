#include "core/cCompressionCouleur.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace
{
 using jpeg::ChromaSubsampling;

 /** @brief Lightweight record describing one encoded chroma channel. */
 struct ChannelMetadata
 {
 	std::uint8_t id;                  /**< Channel identifier (0=Y,1=Cb,2=Cr). */
 	unsigned int width;               /**< Channel width in pixels. */
 	unsigned int height;              /**< Channel height in pixels. */
 	std::string filename;             /**< Huffman bitstream filename. */
 	std::vector<char> symbols;        /**< Cached Huffman symbols. */
 	std::vector<double> frequencies;  /**< Cached Huffman frequencies. */
 };

 /** @brief Horizontal/vertical decimation factors for chroma planes. */
 struct SubsampleFactors
 {
 	unsigned int horizontal;
 	unsigned int vertical;
 };

 /** @brief Translate subsampling mode to per-axis decimation factors. */
 SubsampleFactors factorsFor(ChromaSubsampling mode)
 {
		switch (mode)
		{
		case ChromaSubsampling::Sampling444:
			return {1U, 1U};
		case ChromaSubsampling::Sampling422:
			return {2U, 1U};
		case ChromaSubsampling::Sampling420:
			return {2U, 2U};
		case ChromaSubsampling::Sampling411:
			return {4U, 1U};
		}
		return {1U, 1U};
	}

	 /**
	  * @brief Ensure image dimensions align with 8x8 blocks after subsampling.
	  * @return true when width/height are compatible with the requested factors.
	  */
	 bool ensureCompatibility(unsigned int width,
	 			unsigned int height,
	 			const SubsampleFactors &fac)
	{
		if ((width % (8U * fac.horizontal)) != 0U)
			return false;
		if ((height % (8U * fac.vertical)) != 0U)
			return false;
		return true;
	}

	 /**
	  * @brief Convert interleaved RGB pixels to planar YCbCr.
	  * @param rgb Source RGB buffer (size width * height * 3).
	  * @param width Image width.
	  * @param height Image height.
	  * @param Y Output luma plane.
	  * @param Cb Output blue-difference chroma plane.
	  * @param Cr Output red-difference chroma plane.
	  */
	 void rgbToYCbCr(const std::vector<unsigned char> &rgb,
	 		unsigned int width,
	 		unsigned int height,
	 		std::vector<unsigned char> &Y,
	 		std::vector<unsigned char> &Cb,
	 		std::vector<unsigned char> &Cr)
	{
		const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
		Y.resize(pixelCount);
		Cb.resize(pixelCount);
		Cr.resize(pixelCount);

		for (std::size_t idx = 0; idx < pixelCount; ++idx)
		{
			const double r = static_cast<double>(rgb[idx * 3 + 0]);
			const double g = static_cast<double>(rgb[idx * 3 + 1]);
			const double b = static_cast<double>(rgb[idx * 3 + 2]);

			const double y = 0.299 * r + 0.587 * g + 0.114 * b;
			const double cb = -0.168736 * r - 0.331264 * g + 0.5 * b + 128.0;
			const double cr = 0.5 * r - 0.418688 * g - 0.081312 * b + 128.0;

			Y[idx] = static_cast<unsigned char>(std::clamp(std::lround(y), 0L, 255L));
			Cb[idx] = static_cast<unsigned char>(std::clamp(std::lround(cb), 0L, 255L));
			Cr[idx] = static_cast<unsigned char>(std::clamp(std::lround(cr), 0L, 255L));
		}
	}

	 /**
	  * @brief Average down a chroma plane according to the sampling factors.
	  * @param src Source plane at full resolution.
	  * @param width Source width.
	  * @param height Source height.
	  * @param fac Horizontal/vertical subsampling factors.
	  * @param outWidth Receives resulting width.
	  * @param outHeight Receives resulting height.
	  * @return New subsampled plane stored in a vector.
	  */
	 std::vector<unsigned char> subsamplePlane(const std::vector<unsigned char> &src,
	 				 unsigned int width,
	 				 unsigned int height,
	 				 const SubsampleFactors &fac,
	 				 unsigned int &outWidth,
	 				 unsigned int &outHeight)
	{
		outWidth = width / fac.horizontal;
		outHeight = height / fac.vertical;
		std::vector<unsigned char> dst(static_cast<std::size_t>(outWidth) * static_cast<std::size_t>(outHeight));

		for (unsigned int y = 0; y < outHeight; ++y)
		{
			for (unsigned int x = 0; x < outWidth; ++x)
			{
				double acc = 0.0;
				unsigned int samples = 0;
				for (unsigned int v = 0; v < fac.vertical; ++v)
				{
					for (unsigned int h = 0; h < fac.horizontal; ++h)
					{
						const unsigned int srcX = x * fac.horizontal + h;
						const unsigned int srcY = y * fac.vertical + v;
						const std::size_t srcIdx = static_cast<std::size_t>(srcY) * width + srcX;
						acc += static_cast<double>(src[srcIdx]);
						++samples;
					}
				}
				const double avg = acc / static_cast<double>(samples);
				dst[static_cast<std::size_t>(y) * outWidth + x] = static_cast<unsigned char>(std::clamp(std::lround(avg), 0L, 255L));
			}
		}

		return dst;
	}

	 /** @brief Copy cached Huffman data into the channel metadata structure. */
	 void collectHuffmanMetadata(ChannelMetadata &meta)
	{
		char symbols[256];
		double freqs[256];
		unsigned int count = 0;
		if (!cCompression::loadHuffmanTable(symbols, freqs, count))
		{
			meta.symbols.clear();
			meta.frequencies.clear();
			return;
		}
		meta.symbols.assign(symbols, symbols + count);
		meta.frequencies.assign(freqs, freqs + count);
	}

	 /**
	  * @brief Serialize color metadata describing the encoded channels.
	  * @throws std::runtime_error when the file cannot be written.
	  */
	 void writeMetadata(const std::string &path,
	 		  unsigned int width,
	 		  unsigned int height,
	 		  unsigned int quality,
	 		  ChromaSubsampling mode,
	 		  const std::vector<ChannelMetadata> &channels)
	{
		std::ofstream out(path, std::ios::binary);
		if (!out)
		{
			throw std::runtime_error("Cannot write color metadata: " + path);
		}

		const std::uint32_t magic = jpeg::kColorMetaMagic;
		const std::uint32_t version = jpeg::kColorMetaVersion;
		const std::uint32_t channelCount = static_cast<std::uint32_t>(channels.size());
		const std::uint32_t subsampling = static_cast<std::uint32_t>(mode);

		out.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
		out.write(reinterpret_cast<const char *>(&version), sizeof(version));
		out.write(reinterpret_cast<const char *>(&width), sizeof(width));
		out.write(reinterpret_cast<const char *>(&height), sizeof(height));
		out.write(reinterpret_cast<const char *>(&quality), sizeof(quality));
		out.write(reinterpret_cast<const char *>(&subsampling), sizeof(subsampling));
		out.write(reinterpret_cast<const char *>(&channelCount), sizeof(channelCount));

		for (const ChannelMetadata &meta : channels)
		{
			out.write(reinterpret_cast<const char *>(&meta.id), sizeof(meta.id));
			out.write(reinterpret_cast<const char *>(&meta.width), sizeof(meta.width));
			out.write(reinterpret_cast<const char *>(&meta.height), sizeof(meta.height));

			const std::uint32_t nameLen = static_cast<std::uint32_t>(meta.filename.size());
			out.write(reinterpret_cast<const char *>(&nameLen), sizeof(nameLen));
			out.write(meta.filename.data(), nameLen);

			const std::uint32_t symbolCount = static_cast<std::uint32_t>(meta.symbols.size());
			out.write(reinterpret_cast<const char *>(&symbolCount), sizeof(symbolCount));
			for (std::uint32_t i = 0; i < symbolCount; ++i)
			{
				const std::int8_t sym = static_cast<std::int8_t>(meta.symbols[i]);
				out.write(reinterpret_cast<const char *>(&sym), sizeof(sym));
				const double freq = meta.frequencies[i];
				out.write(reinterpret_cast<const char *>(&freq), sizeof(freq));
			}
		}
	}

	 /**
	  * @brief Compress a single planar channel and gather Huffman metadata.
	  * @param plane Input plane data (row-major).
	  * @param width Plane width.
	  * @param height Plane height.
	  * @param quality Compression quality factor.
	  * @param filename Output Huffman filename.
	  * @param channelId Channel identifier stored in metadata.
	  * @return Filled metadata record for the channel.
	  */
	 ChannelMetadata compressChannel(const std::vector<unsigned char> &plane,
	 		     unsigned int width,
	 		     unsigned int height,
	 		     unsigned int quality,
	 		     const std::string &filename,
	 		     std::uint8_t channelId)
	{
		std::vector<unsigned char *> rows(height);
		for (unsigned int y = 0; y < height; ++y)
		{
			rows[y] = const_cast<unsigned char *>(&plane[static_cast<std::size_t>(y) * width]);
		}

		cCompression comp(width, height, quality, rows.data());
		cCompression::setQualiteGlobale(quality);

		const unsigned int blockCount = (width / 8U) * (height / 8U);
		std::vector<int> trame(1 + blockCount * 128U, 0);
		comp.RLE(trame.data());
		comp.Compression_JPEG(trame.data(), filename.c_str());

		ChannelMetadata meta;
		meta.id = channelId;
		meta.width = width;
		meta.height = height;
		meta.filename = filename;
		collectHuffmanMetadata(meta);
		return meta;
	}
}

/**
 * @brief Construct a color compressor with quality and subsampling settings.
 */
cCompressionCouleur::cCompressionCouleur(unsigned int qualite,
		   jpeg::ChromaSubsampling mode)
	: cCompression(0, 0, qualite, nullptr), mSubsampling(mode)
{
}

/** @brief Modify the chroma subsampling mode. */
void cCompressionCouleur::setSubsampling(jpeg::ChromaSubsampling mode)
{
	mSubsampling = mode;
}

/** @brief Inspect the current chroma subsampling mode. */
jpeg::ChromaSubsampling cCompressionCouleur::getSubsampling() const
{
	return mSubsampling;
}

/**
 * @brief Compress an RGB image into per-channel Huffman bitstreams plus metadata.
 */
bool cCompressionCouleur::compressRGB(const std::vector<unsigned char> &rgb,
		 unsigned int width,
		 unsigned int height,
		 const std::string &outputPrefix) const
{
	if (rgb.size() != static_cast<std::size_t>(width) * height * 3ULL)
	{
		return false;
	}

	const SubsampleFactors fac = factorsFor(mSubsampling);
	if (!ensureCompatibility(width, height, fac))
	{
		return false;
	}

	std::vector<unsigned char> Y;
	std::vector<unsigned char> Cb;
	std::vector<unsigned char> Cr;
	rgbToYCbCr(rgb, width, height, Y, Cb, Cr);

	unsigned int chromaWidth = width;
	unsigned int chromaHeight = height;
	std::vector<unsigned char> CbSub;
	std::vector<unsigned char> CrSub;

	if (fac.horizontal == 1U && fac.vertical == 1U)
	{
		CbSub = Cb;
		CrSub = Cr;
	}
	else
	{
		CbSub = subsamplePlane(Cb, width, height, fac, chromaWidth, chromaHeight);
		CrSub = subsamplePlane(Cr, width, height, fac, chromaWidth, chromaHeight);
	}

	std::vector<ChannelMetadata> channels;
	channels.reserve(3);

	const std::string yFile = outputPrefix + "_Y.huff";
	const ChannelMetadata yMeta = compressChannel(Y, width, height, getQualite(), yFile, 0U);
	channels.push_back(yMeta);

	const std::string cbFile = outputPrefix + "_Cb.huff";
	const ChannelMetadata cbMeta = compressChannel(CbSub, chromaWidth, chromaHeight, getQualite(), cbFile, 1U);
	channels.push_back(cbMeta);

	const std::string crFile = outputPrefix + "_Cr.huff";
	const ChannelMetadata crMeta = compressChannel(CrSub, chromaWidth, chromaHeight, getQualite(), crFile, 2U);
	channels.push_back(crMeta);

	try
	{
		writeMetadata(outputPrefix + ".meta", width, height, getQualite(), mSubsampling, channels);
	}
	catch (const std::exception &)
	{
		return false;
	}

	return true;
}
