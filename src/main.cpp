#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "core/cCompression.h"
#include "core/cDecompression.h"

namespace
{

/**
 * @brief Resolve a file path by searching current and parent directories.
 * @param input User-provided path (relative or absolute).
 * @param resolved Receives the best matching absolute path.
 * @return true if a regular file was found.
 */
bool resolveExistingFile(const std::string &input,
						 std::string &resolved)
{
	namespace fs = std::filesystem;
	fs::path original(input);

	std::vector<fs::path> candidates;
	candidates.push_back(original);

	if (!original.is_absolute())
	{
		fs::path current = fs::current_path();
		fs::path parent = current;
		for (int i = 0; i < 3 && !parent.empty(); ++i)
		{
			candidates.push_back(parent / original);
			parent = parent.parent_path();
		}
	}

	fs::path best;
	bool bestSet = false;
	bool bestInBuild = false;

	for (const auto &cand : candidates)
	{
		if (cand.empty())
			continue;

		std::error_code ec;
		if (fs::exists(cand, ec) && fs::is_regular_file(cand, ec))
		{
			fs::path normalized = fs::weakly_canonical(cand, ec);
			if (ec)
			{
				normalized = fs::absolute(cand, ec);
				if (ec)
					normalized = cand.lexically_normal();
			}
			bool inBuild = std::any_of(normalized.begin(), normalized.end(), [](const fs::path &part) { return part == "build"; });
			if (!bestSet || (bestInBuild && !inBuild))
			{
				best = normalized;
				bestInBuild = inBuild;
				bestSet = true;
				if (!inBuild)
					break;
			}
		}
	}

	if (bestSet)
	{
		resolved = best.string();
		return true;
	}

	return false;
}

/**
 * @brief Resolve a directory path by searching current and parent directories.
 * @param input User-provided path (relative or absolute).
 * @param resolved Receives the best matching directory path.
 * @return true if a directory was found.
 */
bool resolveExistingDirectory(const std::string &input,
					std::string &resolved)
{
	namespace fs = std::filesystem;
	fs::path original(input);

	std::vector<fs::path> candidates;
	candidates.push_back(original);

	if (!original.is_absolute())
	{
		fs::path current = fs::current_path();
		fs::path parent = current;
		for (int i = 0; i < 3 && !parent.empty(); ++i)
		{
			candidates.push_back(parent / original);
			parent = parent.parent_path();
		}
	}

	fs::path best;
	bool bestSet = false;
	bool bestInBuild = false;

	for (const auto &cand : candidates)
	{
		if (cand.empty())
			continue;

		std::error_code ec;
		if (fs::exists(cand, ec) && fs::is_directory(cand, ec))
		{
			fs::path normalized = fs::weakly_canonical(cand, ec);
			if (ec)
			{
				normalized = fs::absolute(cand, ec);
				if (ec)
					normalized = cand.lexically_normal();
			}
			bool inBuild = std::any_of(normalized.begin(), normalized.end(), [](const fs::path &part) { return part == "build"; });
			if (!bestSet || (bestInBuild && !inBuild))
			{
				best = normalized;
				bestInBuild = inBuild;
				bestSet = true;
				if (!inBuild)
					break;
			}
		}
	}

	if (bestSet)
	{
		resolved = best.string();
		return true;
	}

	return false;
}

/**
 * @brief Resolve an output path, optionally anchored to known existing paths.
 * @param input Requested output path (relative or absolute).
 * @param anchors Paths used as hints for resolving relative inputs.
 * @param resolved Receives the normalized output path.
 * @return true if a plausible destination path is produced.
 */
bool resolveOutputPath(const std::string &input,
					 std::initializer_list<std::string> anchors,
					 std::string &resolved)
{
	namespace fs = std::filesystem;
	fs::path requested(input);

	if (requested.empty())
	{
		return false;
	}

	if (requested.is_absolute())
	{
		resolved = requested.lexically_normal().string();
		return true;
	}

	for (const auto &anchor : anchors)
	{
		if (anchor.empty())
			continue;

		fs::path anchorPath(anchor);
		std::error_code ec;
		fs::path base = anchorPath;
		if (fs::is_regular_file(anchorPath, ec))
		{
			base = anchorPath.parent_path();
		}
		else if (!fs::is_directory(anchorPath, ec))
		{
			continue;
		}

		if (base.empty())
			continue;

		if (!requested.has_parent_path())
		{
			fs::path candidate = base / requested;
			resolved = candidate.lexically_normal().string();
			return true;
		}

		auto reqIt = requested.begin();
		if (reqIt == requested.end())
			continue;

		fs::path first = *reqIt;
		fs::path remainder;
		++reqIt;
		for (; reqIt != requested.end(); ++reqIt)
		{
			remainder /= *reqIt;
		}

		if (!first.empty() && first == base.filename())
		{
			fs::path candidate = base / remainder;
			resolved = candidate.lexically_normal().string();
			return true;
		}
	}

	fs::path parent = requested.parent_path();
	if (!parent.empty())
	{
		std::string resolvedParent;
		if (resolveExistingDirectory(parent.string(), resolvedParent))
		{
			fs::path combined = fs::path(resolvedParent) / requested.filename();
			resolved = combined.lexically_normal().string();
			return true;
		}
	}

	fs::path current = fs::current_path();
	fs::path cursor = current;
	for (int i = 0; i < 4 && !cursor.empty(); ++i)
	{
		fs::path candidate = cursor / requested;
		fs::path parentDir = candidate.parent_path();
		std::error_code ec;
		if (fs::exists(parentDir, ec) && fs::is_directory(parentDir, ec))
		{
			fs::path normalized = fs::weakly_canonical(candidate, ec);
			if (ec)
			{
				normalized = fs::absolute(candidate, ec);
				if (ec)
				{
					normalized = candidate.lexically_normal();
				}
			}
			resolved = normalized.string();
			return true;
		}
		cursor = cursor.parent_path();
	}

	fs::path fallback = current / requested;
	std::error_code ec;
	fs::path normalized = fs::weakly_canonical(fallback, ec);
	if (ec)
	{
		normalized = fs::absolute(fallback, ec);
		if (ec)
			normalized = fallback.lexically_normal();
	}
	resolved = normalized.string();
	return true;
}

/**
 * @brief Resolve an output path without additional anchors.
 */
bool resolveOutputPath(const std::string &input,
					 std::string &resolved)
{
	return resolveOutputPath(input, {}, resolved);
}

/**
 * @brief Load a whitespace-separated ASCII image file (square, grayscale).
 * @param filename Source file path.
 * @param image Output buffer containing pixels.
 * @param width Receives inferred width (multiple of 8).
 * @param height Receives inferred height (multiple of 8).
 * @return true on success.
 */
bool loadAsciiImage(const std::string &filename,
					std::vector<unsigned char> &image,
					unsigned int &width,
					unsigned int &height)
{
	std::string resolved;
	if (!resolveExistingFile(filename, resolved))
	{
		std::cerr << "Cannot locate " << filename << " (tried relative to current and parent directories).\n";
		return false;
	}

	std::ifstream in(resolved);
	if (!in)
	{
		std::cerr << "Cannot open " << resolved << "\n";
		return false;
	}

	std::vector<int> values;
	int v = 0;
	while (in >> v)
	{
		values.push_back(std::clamp(v, 0, 255));
	}
	in.close();

	if (values.empty())
	{
		std::cerr << "Image file " << filename << " is empty or invalid.\n";
		return false;
	}

	std::size_t N = values.size();
	unsigned int side = static_cast<unsigned int>(std::sqrt(static_cast<double>(N)) + 0.5);
	if (side * side != N)
	{
		std::cerr << "Pixel count (" << N
				  << ") is not a perfect square; cannot infer width/height.\n";
		return false;
	}

	width = side;
	height = side;
	if ((width % 8) || (height % 8))
	{
		std::cerr << "Width and height must be multiples of 8.\n";
		return false;
	}

	image.resize(N);
	for (std::size_t i = 0; i < N; ++i)
	{
		image[i] = static_cast<unsigned char>(values[i]);
	}
	return true;
}

/**
 * @brief Write an 8-bit grayscale buffer to a binary PGM file.
 * @param filename Destination filename.
 * @param image Interleaved grayscale pixels.
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @return true when the file was written successfully.
 */
bool writePGM(const std::string &filename,
			  const std::vector<unsigned char> &image,
			  unsigned int width,
			  unsigned int height)
{
	namespace fs = std::filesystem;
	fs::path target(filename);
	std::error_code ec;
	if (!target.parent_path().empty())
	{
		fs::create_directories(target.parent_path(), ec);
	}

	std::ofstream out(filename, std::ios::binary);
	if (!out)
	{
		std::cerr << "Cannot write " << filename << "\n";
		return false;
	}

	out << "P5\n" << width << " " << height << "\n255\n";
	out.write(reinterpret_cast<const char *>(image.data()), static_cast<std::streamsize>(image.size()));
	if (!out)
	{
		std::cerr << "Failed while writing " << filename << "\n";
		return false;
	}
	return true;
}

/**
 * @brief Prompt the user for a JPEG quality value (1..100).
 * @return Selected quality; defaults to 50 on empty input.
 */
unsigned int requestQuality()
{
	unsigned int quality = 50;
	while (true)
	{
		std::cout << "Enter JPEG quality (1-100, default 50): ";
		std::string line;
		if (!std::getline(std::cin, line))
		{
			return 50;
		}
		if (line.empty())
		{
			break;
		}
		try
		{
			int q = std::stoi(line);
			if (q >= 1 && q <= 100)
			{
				quality = static_cast<unsigned int>(q);
				break;
			}
		}
		catch (...)
		{
		}
		std::cout << "Invalid quality; please try again.\n";
	}
	return quality;
}

struct CompressionSummary
{
	unsigned int rleLength = 0;
	std::streamsize bitstreamSize = 0;
};

/** @brief Create parent directories for @p path if needed. */
bool ensureParentDirectory(const std::string &path)
{
	namespace fs = std::filesystem;
	fs::path target(path);
	fs::path parent = target.parent_path();
	if (parent.empty())
	{
		return true;
	}

	std::error_code ec;
	fs::create_directories(parent, ec);
	return !ec;
}

/**
 * @brief Compress an image buffer to a Huffman bitstream on disk.
 * @param image Mutable image data (will not be modified).
 * @param width Image width (multiple of 8).
 * @param height Image height (multiple of 8).
 * @param quality JPEG quality factor.
 * @param outputPath Destination filename.
 * @param summary Receives compression statistics.
 * @return true on success.
 */
bool compressToBitstream(std::vector<unsigned char> &image,
						 unsigned int width,
						 unsigned int height,
						 unsigned int quality,
						 const std::string &outputPath,
						 CompressionSummary &summary)
{
	if ((width % 8U) || (height % 8U))
	{
		std::cerr << "Image dimensions must be multiples of 8.\n";
		return false;
	}

	std::vector<unsigned char *> rows(height);
	for (unsigned int y = 0; y < height; ++y)
	{
		rows[y] = image.data() + static_cast<std::size_t>(y) * width;
	}

	cCompression::setQualiteGlobale(quality);
	cCompression comp(width, height, quality, rows.data());

	const unsigned int blockCount = (width / 8U) * (height / 8U);
	std::vector<int> trame(1 + blockCount * 128U, 0);
	comp.RLE(trame.data());

	summary.rleLength = static_cast<unsigned int>(trame[0]);

	if (!ensureParentDirectory(outputPath))
	{
		std::cerr << "Unable to prepare output directory for " << outputPath << "\n";
		return false;
	}

	comp.Compression_JPEG(trame.data(), outputPath.c_str());

	std::ifstream fin(outputPath, std::ios::binary | std::ios::ate);
	if (!fin)
	{
		std::cerr << "Failed to verify output file " << outputPath << "\n";
		return false;
	}
	summary.bitstreamSize = fin.tellg();
	return true;
}

/**
 * @brief Decompress a Huffman bitstream into a grayscale buffer.
 * @param inputPath Source bitstream path.
 * @param quality JPEG quality factor.
 * @param buffer Output grayscale pixels.
 * @param width Receives image width.
 * @param height Receives image height.
 * @return true on success.
 */
bool decompressToBuffer(const std::string &inputPath,
						unsigned int quality,
						std::vector<unsigned char> &buffer,
						unsigned int &width,
						unsigned int &height)
{
	cDecompression dec;
	dec.setQualite(quality);
	unsigned char **decoded = dec.Decompression_JPEG(inputPath.c_str());
	if (!decoded)
	{
		return false;
	}

	width = dec.getLargeur();
	height = dec.getHauteur();
	buffer.resize(static_cast<std::size_t>(width) * height);
	for (unsigned int y = 0; y < height; ++y)
	{
		std::copy(decoded[y], decoded[y] + width, buffer.begin() + static_cast<std::size_t>(y) * width);
	}
	return true;
}

/**
 * @brief Interactive workflow: load ASCII image and emit Huffman stream.
 */
void compressOnly()
{
	namespace fs = std::filesystem;
	std::cout << "Path to ASCII grayscale image (e.g. tests/lena.dat): ";
	std::string inputPath;
	if (!std::getline(std::cin, inputPath) || inputPath.empty())
	{
		std::cerr << "No input path provided.\n";
		return;
	}

	std::string resolvedInput;
	if (!resolveExistingFile(inputPath, resolvedInput))
	{
		std::cerr << "Cannot locate " << inputPath << " (try using a path relative to the project root).\n";
		return;
	}

	std::cout << "Output path for Huffman stream (e.g. tests/lena.huff): ";
	std::string outputPath;
	if (!std::getline(std::cin, outputPath) || outputPath.empty())
	{
		std::cerr << "No output path provided.\n";
		return;
	}

	std::string resolvedOutput;
	if (!resolveOutputPath(outputPath, {resolvedInput}, resolvedOutput))
	{
		std::cerr << "Could not resolve output path " << outputPath << "\n";
		return;
	}

	unsigned int quality = requestQuality();

	std::vector<unsigned char> image;
	unsigned int width = 0;
	unsigned int height = 0;
	if (!loadAsciiImage(resolvedInput, image, width, height))
		return;

	CompressionSummary summary;
	if (!compressToBitstream(image, width, height, quality, resolvedOutput, summary))
	{
		return;
	}

	std::cout << "RLE length (bytes): " << summary.rleLength << '\n';
	std::cout << "Compressed file size: " << summary.bitstreamSize << " bytes\n";
	if (summary.bitstreamSize > 0)
	{
		std::cout << "Compression ratio (RLE/Huffman): "
			  << static_cast<double>(summary.rleLength) / static_cast<double>(summary.bitstreamSize) << '\n';
	}
	std::cout << "Huffman stream saved to " << resolvedOutput << '\n';
}

/**
 * @brief Interactive workflow: read Huffman stream and produce a PGM image.
 */
void decompressOnly()
{
	namespace fs = std::filesystem;
	std::cout << "Path to Huffman stream produced earlier in this session: ";
	std::string inputPath;
	if (!std::getline(std::cin, inputPath) || inputPath.empty())
	{
		std::cerr << "No input path provided.\n";
		return;
	}

	std::string resolvedInput;
	if (!resolveExistingFile(inputPath, resolvedInput))
	{
		std::cerr << "Cannot locate " << inputPath << " (ensure you compressed during this run).\n";
		return;
	}

	std::cout << "Output PGM path (e.g. tests/lena_recon.pgm): ";
	std::string outputPath;
	if (!std::getline(std::cin, outputPath) || outputPath.empty())
	{
		std::cerr << "No output path provided.\n";
		return;
	}

	std::string resolvedOutput;
	if (!resolveOutputPath(outputPath, {resolvedInput}, resolvedOutput))
	{
		std::cerr << "Could not resolve output path " << outputPath << "\n";
		return;
	}

	unsigned int quality = requestQuality();

	std::vector<unsigned char> buffer;
	unsigned int width = 0;
	unsigned int height = 0;
	if (!decompressToBuffer(resolvedInput, quality, buffer, width, height))
	{
		std::cerr << "Decompression failed. Ensure you compressed in this session so Huffman table is available.\n";
		return;
	}

	if (writePGM(resolvedOutput, buffer, width, height))
	{
		std::cout << "Wrote reconstructed image to " << resolvedOutput << '\n';
	}
}

/**
 * @brief Interactive workflow: end-to-end compression, reconstruction, and reports.
 */
void fullPipeline()
{
	namespace fs = std::filesystem;
	std::cout << "Path to ASCII grayscale image (e.g. tests/lena.dat): ";
	std::string inputPath;
	if (!std::getline(std::cin, inputPath) || inputPath.empty())
	{
		std::cerr << "No input path provided.\n";
		return;
	}

	std::string resolvedInput;
	if (!resolveExistingFile(inputPath, resolvedInput))
	{
		std::cerr << "Cannot locate " << inputPath << " (try using a path relative to the project root).\n";
		return;
	}

	std::cout << "Output base path (e.g. tests/lena_total or tests/run1/output): ";
	std::string basePathInput;
	if (!std::getline(std::cin, basePathInput) || basePathInput.empty())
	{
		std::cerr << "No output base provided.\n";
		return;
	}

	std::string resolvedBase;
	if (!resolveOutputPath(basePathInput, {resolvedInput}, resolvedBase))
	{
		std::cerr << "Could not resolve output base " << basePathInput << "\n";
		return;
	}

	namespace fsLocal = std::filesystem;
	fsLocal::path basePath(resolvedBase);
	std::error_code ec;
	fsLocal::path outDir;
	std::string baseStem;
	if (fsLocal::is_directory(basePath, ec))
	{
		outDir = basePath;
		baseStem = basePath.filename().string();
		if (baseStem.empty())
		{
			baseStem = "output";
		}
	}
	else
	{
		outDir = basePath.parent_path();
		baseStem = basePath.stem().string();
		if (baseStem.empty())
		{
			baseStem = basePath.filename().string();
			if (baseStem.empty())
			{
				baseStem = "output";
			}
		}
	}

	fsLocal::path resolvedOriginal = outDir / (baseStem + "_01_original.pgm");
	fsLocal::path resolvedPreview = outDir / (baseStem + "_02_compressed_preview.pgm");
	fsLocal::path resolvedHuff = outDir / (baseStem + "_03_compressed.huff");
	fsLocal::path resolvedRecon = outDir / (baseStem + "_04_decompressed.pgm");
	fsLocal::path resolvedDiff = outDir / (baseStem + "_05_diff.pgm");

	unsigned int quality = requestQuality();

	std::vector<unsigned char> image;
	unsigned int width = 0;
	unsigned int height = 0;
	if (!loadAsciiImage(resolvedInput, image, width, height))
		return;

	const std::string originalPathStr = resolvedOriginal.string();
	if (!writePGM(originalPathStr, image, width, height))
	{
		return;
	}
	std::cout << "Wrote original image to " << resolvedOriginal << '\n';

	std::vector<unsigned char> preview(image.size(), 0);
	for (unsigned int by = 0; by < height; by += 8)
	{
		for (unsigned int bx = 0; bx < width; bx += 8)
		{
			unsigned int blockSum = 0;
			for (unsigned int y = 0; y < 8; ++y)
			{
				for (unsigned int x = 0; x < 8; ++x)
				{
					blockSum += image[(by + y) * width + (bx + x)];
				}
			}
			double mean = static_cast<double>(blockSum) / 64.0;
			unsigned char value = static_cast<unsigned char>(std::clamp(static_cast<int>(std::lround(mean)), 0, 255));
			for (unsigned int y = 0; y < 8; ++y)
			{
				for (unsigned int x = 0; x < 8; ++x)
				{
					preview[(by + y) * width + (bx + x)] = value;
				}
			}
		}
	}

	const std::string previewPathStr = resolvedPreview.string();
	if (writePGM(previewPathStr, preview, width, height))
	{
		std::cout << "Wrote compression preview to " << resolvedPreview << '\n';
	}

	const std::string huffPathStr = resolvedHuff.string();
	CompressionSummary summary;
	if (!compressToBitstream(image, width, height, quality, huffPathStr, summary))
	{
		return;
	}
	std::cout << "RLE length (bytes): " << summary.rleLength << '\n';
	std::cout << "Compressed file size: " << summary.bitstreamSize << " bytes\n";

	std::vector<unsigned char> recon;
	unsigned int reconWidth = 0;
	unsigned int reconHeight = 0;
	if (!decompressToBuffer(huffPathStr, quality, recon, reconWidth, reconHeight))
	{
		std::cerr << "Decompression failed right after compression (unexpected).\n";
		return;
	}

	if (recon.size() != image.size())
	{
		std::cerr << "Decompressed image size mismatch.\n";
		return;
	}

	const std::string reconPathStr = resolvedRecon.string();
	if (writePGM(reconPathStr, recon, reconWidth, reconHeight))
	{
		std::cout << "Wrote reconstructed image to " << resolvedRecon << '\n';
	}

	double mse = 0.0;
	for (std::size_t idx = 0; idx < recon.size(); ++idx)
	{
		int diff = static_cast<int>(image[idx]) - static_cast<int>(recon[idx]);
		mse += static_cast<double>(diff * diff);
	}
	mse /= static_cast<double>(recon.size());
	double psnr = (mse > 0.0)
				  ? 10.0 * std::log10((255.0 * 255.0) / mse)
				  : std::numeric_limits<double>::infinity();
	std::cout << "Image MSE: " << mse << '\n';
	std::cout << "PSNR: " << psnr << " dB\n";

	std::vector<unsigned char> diffImg(recon.size());
	for (std::size_t i = 0; i < recon.size(); ++i)
	{
		int diff = std::abs(static_cast<int>(image[i]) - static_cast<int>(recon[i]));
		diff = std::min(255, diff * 8);
		diffImg[i] = static_cast<unsigned char>(diff);
	}
	const std::string diffPathStr = resolvedDiff.string();
	if (writePGM(diffPathStr, diffImg, reconWidth, reconHeight))
	{
		std::cout << "Wrote amplified difference image to " << resolvedDiff << '\n';
	}

	std::cout << "\nOutputs (ordered):\n";
	std::cout << "  1. Original image        : " << resolvedOriginal << '\n';
	std::cout << "  2. Compression preview   : " << resolvedPreview << '\n';
	std::cout << "  3. Compressed bitstream  : " << resolvedHuff << '\n';
	std::cout << "  4. Decompressed image    : " << resolvedRecon << '\n';
	std::cout << "  5. Difference image      : " << resolvedDiff << '\n';
}

/** @brief Print a short usage string for command-line invocations. */
void printUsage(std::ostream &out)
{
	out << "Usage:\n"
	    << "  jpeg_cli           Launch interactive menu\n"
	    << "  jpeg_cli --help    Display this help message\n\n"
	    << "The interactive menu lets you compress, decompress, or run the full pipeline "
		   "with guided prompts." << '\n';
}

} // namespace

/**
 * @brief Entry point for the JPEG demo CLI orchestrating interactive workflows.
 */
int main(int argc, char **argv)
{
	namespace fs = std::filesystem;
	if (argc > 1)
	{
		std::string arg = argv[1];
		if (arg == "--help" || arg == "-h")
		{
			printUsage(std::cout);
			return 0;
		}

		std::cerr << "Unknown argument '" << arg << "'. Use --help to see available options.\n";
		return 1;
	}

	const char *TEST_DIR = "tests";
	std::vector<std::string> cleaned;
	fs::path current = fs::current_path();
	for (int i = 0; i < 4 && !current.empty(); ++i)
	{
		fs::path candidate = current / TEST_DIR;
		current = current.parent_path();

		std::error_code ec;
		if (!fs::exists(candidate, ec) || !fs::is_directory(candidate, ec))
		{
			continue;
		}

		fs::path normalized = candidate.lexically_normal();
		std::string normalizedStr = normalized.string();
		if (std::find(cleaned.begin(), cleaned.end(), normalizedStr) != cleaned.end())
		{
			continue;
		}
		cleaned.push_back(normalizedStr);

		for (const auto &entry : fs::directory_iterator(normalized, ec))
		{
			if (entry.is_regular_file(ec))
			{
				auto ext = entry.path().extension().string();
				if (ext == ".pgm" || ext == ".huff")
				{
					fs::remove(entry.path(), ec);
				}
			}
		}
	}

	while (true)
	{
		std::cout << "\n=== JPEG Demo CLI ===\n";
		std::cout << "1) Compress ASCII image to Huffman stream\n";
		std::cout << "2) Decompress Huffman stream to PGM (same session)\n";
		std::cout << "3) Compress + decompress + stats + PGM outputs\n";
		std::cout << "0) Exit\n";
		std::cout << "Select option: ";

		std::string line;
		if (!std::getline(std::cin, line))
			break;

		if (line == "0")
		{
			break;
		}
		else if (line == "1")
		{
			compressOnly();
		}
		else if (line == "2")
		{
			decompressOnly();
		}
		else if (line == "3")
		{
			fullPipeline();
		}
		else
		{
			std::cout << "Unknown option.\n";
		}
	}

	return 0;
}
