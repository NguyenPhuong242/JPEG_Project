// main.cpp
// Pipeline: read ASCII image, level-shift, DCT, quant, dequant, IDCT,
// compute EQM (MSE) and Taux_Compression via cCompression, and produce RLE trame.

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cmath>
#include <algorithm>

#include "core/cCompression.h"
#include "dct/dct.h"
#include "quantification/quantification.h"
#include "core/cCompressionCouleur.h"

using namespace std;

// --- Helper functions for printing blocks ---
void print_int_block(const std::string& title, int* block[8]) {
    cout << "\n--- " << title << " ---\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            cout << setw(5) << block[i][j];
        }
        cout << "\n";
    }
}

void print_double_block(const std::string& title, double* block[8]) {
    cout << "\n--- " << title << " ---\n";
    cout << fixed << setprecision(2);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            cout << setw(9) << block[i][j];
        }
        cout << "\n";
    }
}
// ---

void print_help() {
    cout << "JPEG Compressor - Educational Tool\n\n";
    cout << "Usage: ./build/jpeg_cli [command] [options...]\n\n";
    cout << "Commands:\n";
    cout << "  (no command)              Compress a grayscale image.\n";
    cout << "                            Args: [infile] [quality]\n";
    cout << "                            Default: lenna.img 50\n\n";
    cout << "  --process [infile] [qual] Show step-by-step pipeline for the first 8x8 block.\n";
    cout << "                            Default: lenna.img 50\n\n";
    cout << "  --decompress <file.huff>  Decompress a .huff file into a .pgm image.\n\n";
    cout << "  --color-compress ...      Compress a color PPM image.\n";
    cout << "                            Args: <input.ppm> <basename> [quality] [subsampling]\n";
    cout << "                            Subsampling modes: 444, 422, 420\n\n";
    cout << "  --color-decompress ...    Decompress a color image.\n";
    cout << "                            Args: <basename> <output.ppm>\n\n";
    cout << "  --histogram <file.rle>    Show frequency histogram of an RLE file.\n\n";
    cout << "  -h, --help                Show this help message.\n";
}

int main(int argc, char** argv) {
    if (argc > 1 && (string(argv[1]) == "--help" || string(argv[1]) == "-h")) {
        print_help();
        return 0;
    }

	// quick histogram mode: ./jpeg_cli --histogram <rle-file>
	if (argc > 1 && std::string(argv[1]) == "--histogram") {
		const char *rlepath = (argc > 2) ? argv[2] : "lenna.rle";
		// Read file bytes
		std::ifstream rf(rlepath, std::ios::binary);
		if (!rf) { std::cerr << "Cannot open RLE file: " << rlepath << '\n'; return 1; }
		std::vector<char> trame((std::istreambuf_iterator<char>(rf)), std::istreambuf_iterator<char>());
		rf.close();

		cCompression compressor;
		char Donnee[256]; double Frequence[256];
		unsigned int nb = compressor.Histogramme(trame.data(), static_cast<unsigned int>(trame.size()), Donnee, Frequence);

		// collect into vector and sort by frequency desc
		std::vector<std::pair<int,double>> items; items.reserve(nb);
		for (unsigned int i = 0; i < nb; ++i) {
			int sym = static_cast<int>(static_cast<signed char>(Donnee[i]));
			items.emplace_back(sym, Frequence[i]);
		}
		std::sort(items.begin(), items.end(), [](auto &a, auto &b){ return a.second > b.second; });

		std::cout << "Histogram for " << rlepath << " (distinct=" << nb << ")\n";
		std::cout << "  sym  count\n";
		for (size_t i = 0; i < items.size(); ++i) {
			std::cout << std::setw(5) << items[i].first << " " << std::setw(7) << static_cast<long long>(items[i].second) << "\n";
			if (i >= 49) break; // show up to top 50
		}
		return 0;
	}

    if (argc > 1 && std::string(argv[1]) == "--process") {
        string infile = (argc > 2) ? argv[2] : "lenna.img";
        unsigned int qual = (argc > 3) ? static_cast<unsigned int>(stoi(argv[3])) : 50;

        ifstream fin(infile);
        if (!fin) { cerr << "Cannot open " << infile << '\n'; return 1; }

        string firstline;
        if (!getline(fin, firstline)) { cerr << "Empty file" << '\n'; return 1; }

        size_t width = 0;
        {
            istringstream iss(firstline);
            int tmp; while (iss >> tmp) ++width;
        }
        if (width == 0) { cerr << "Failed to detect width" << '\n'; return 1; }

        vector<int> vals;
        {
            istringstream iss(firstline);
            int v; while (iss >> v) vals.push_back(v);
            int x; while (fin >> x) vals.push_back(x);
        }
        fin.close();

        if (vals.size() % width != 0) { cerr << "Malformed input" << '\n'; return 1; }
        size_t height = vals.size() / width;

        cout << "Loaded " << infile << " (" << width << "x" << height << "), showing first 8x8 block with Quality=" << qual << "\n";
        cCompression::setQualiteGlobale(qual);

        const int B = 8;
        if (width < B || height < B) {
            cerr << "Image must be at least 8x8.\n";
            return 1;
        }

        // Buffers for pipeline stages
        int original[B][B];     int* original_ptrs[B];
        int shifted[B][B];      int* shifted_ptrs[B];
        double dct[B][B];       double* dct_ptrs[B];
        int quantized[B][B];    int* quant_ptrs[B];
        double dequantized[B][B]; double* dequant_ptrs[B];
        int idct_out[B][B];     int* idct_ptrs[B];
        int reconstructed[B][B]; int* recon_ptrs[B];

        for(int i=0; i<B; ++i) {
            original_ptrs[i] = original[i];
            shifted_ptrs[i] = shifted[i];
            dct_ptrs[i] = dct[i];
            quant_ptrs[i] = quantized[i];
            dequant_ptrs[i] = dequantized[i];
            idct_ptrs[i] = idct_out[i];
            recon_ptrs[i] = reconstructed[i];
        }

        // 1. Original Block
        for (int r = 0; r < B; ++r) {
            for (int c = 0; c < B; ++c) {
                int val = vals[r * width + c];
                original[r][c] = val < 0 ? 0 : (val > 255 ? 255 : val);
            }
        }
        print_int_block("Original 8x8 Block", original_ptrs);

        // 2. Level Shift
        for (int r = 0; r < B; ++r) {
            for (int c = 0; c < B; ++c) {
                shifted[r][c] = original[r][c] - 128;
            }
        }
        print_int_block("After Level Shift (-128)", shifted_ptrs);

        // 3. DCT
        Calcul_DCT_Block(shifted_ptrs, dct_ptrs);
        print_double_block("After DCT", dct_ptrs);

        // 4. Quantization
        quant_JPEG(dct_ptrs, quant_ptrs);
        print_int_block("After Quantization", quant_ptrs);

        // 5. Dequantization
        dequant_JPEG(quant_ptrs, dequant_ptrs);
        print_double_block("After Dequantization", dequant_ptrs);

        // 6. Inverse DCT (IDCT)
        Calcul_IDCT_Block(dequant_ptrs, idct_ptrs);
        print_int_block("After IDCT", idct_ptrs);

        // 7. Reconstruct (add 128 and clamp)
        for (int r = 0; r < B; ++r) {
            for (int c = 0; c < B; ++c) {
                int val = idct_out[r][c] + 128;
                reconstructed[r][c] = val < 0 ? 0 : (val > 255 ? 255 : val);
            }
        }
        print_int_block("Reconstructed Block", recon_ptrs);

        return 0;
    }

	if (argc > 1 && std::string(argv[1]) == "--decompress") {
		const char *inpath = (argc > 2) ? argv[2] : "lenna.huff";
		cCompression compressor;
		unsigned char **rows = compressor.Decompression_JPEG(inpath);
		if (!rows) { std::cerr << "Decompression failed\n"; return 1; }
		// write output PGM using stored dimensions
		unsigned int w = compressor.getLargeur();
		unsigned int h = compressor.getHauteur();
		std::ofstream fout("decomp_lenna.pgm", std::ios::binary);
		if (!fout) { std::cerr << "Cannot write output file\n"; return 1; }
		fout << "P5\n" << w << " " << h << "\n255\n";
		for (unsigned int r = 0; r < h; ++r) fout.write(reinterpret_cast<char*>(rows[r]), w);
		fout.close();
		std::cout << "Wrote decomp_lenna.pgm (" << w << "x" << h << ")\n";
		// free allocated buffer: rows[0] points to buffer
		delete[] rows[0]; delete[] rows;
		return 0;
	}

	if (argc > 1 && std::string(argv[1]) == "--color-compress") {
		// usage: --color-compress input.ppm basename [quality] [mode]
		const char *ppm = (argc > 2) ? argv[2] : "lenna.ppm";
		const char *basename = (argc > 3) ? argv[3] : "lenna_color";
		unsigned int qual = (argc > 4) ? static_cast<unsigned int>(std::stoi(argv[4])) : 50;
		unsigned int mode = (argc > 5) ? static_cast<unsigned int>(std::stoi(argv[5])) : 444;
		cCompressionCouleur cc;
		bool ok = cc.CompressPPM(ppm, basename, qual, mode);
		std::cout << "Compress color result: " << (ok?"OK":"FAIL") << std::endl;
		return ok ? 0 : 1;
	}

	if (argc > 1 && std::string(argv[1]) == "--color-decompress") {
		// usage: --color-decompress basename out.ppm
		const char *basename = (argc > 2) ? argv[2] : "lenna_color";
		const char *outppm = (argc > 3) ? argv[3] : "decomp_color.ppm";
		cCompressionCouleur cc;
		bool ok = cc.DecompressToPPM(basename, outppm);
		std::cout << "Decompress color result: " << (ok?"OK":"FAIL") << std::endl;
		return ok ? 0 : 1;
	}

	string infile = (argc > 1) ? argv[1] : "lenna.img";
	unsigned int qual = (argc > 2) ? static_cast<unsigned int>(stoi(argv[2])) : 50;

	ifstream fin(infile);
	if (!fin) { cerr << "Cannot open " << infile << '\n'; return 1; }

	string firstline;
	if (!getline(fin, firstline)) { cerr << "Empty file" << '\n'; return 1; }

	// detect width by counting integers in first line
	size_t width = 0;
	{
		istringstream iss(firstline);
		int tmp; while (iss >> tmp) ++width;
	}
	if (width == 0) { cerr << "Failed to detect width" << '\n'; return 1; }

	// read all pixel values (first line already read)
	vector<int> vals;
	{
		istringstream iss(firstline);
		int v; while (iss >> v) vals.push_back(v);
		int x; while (fin >> x) vals.push_back(x);
	}
	fin.close();

	if (vals.size() % width != 0) { cerr << "Malformed input" << '\n'; return 1; }
	size_t height = vals.size() / width;

	cout << "Loaded " << infile << " (" << width << "x" << height << ")\n";

	// clamp to byte and build row pointers
	vector<unsigned char> pixels(width * height);
	for (size_t i = 0; i < vals.size(); ++i) {
		int vv = vals[i]; if (vv < 0) vv = 0; if (vv > 255) vv = 255;
		pixels[i] = static_cast<unsigned char>(vv);
	}
	vector<unsigned char*> rows(height);
	for (size_t r = 0; r < height; ++r) rows[r] = pixels.data() + r * width;

	// configure global quality used by quantization helpers
	cCompression::setQualiteGlobale(qual);

	const int B = 8;
	if ((width % B) != 0 || (height % B) != 0) {
		cerr << "Image dimensions must be multiples of 8\n";
		return 1;
	}

	size_t blocks_w = width / B;
	size_t blocks_h = height / B;

	cCompression compressor; // to call EQM, Taux_Compression and RLE

	double sumEQM = 0.0;
	double sumTaux = 0.0;
	size_t nblocks = 0;

	vector<unsigned char> recon(width * height, 0);

	// per-block local buffers
	int block[8][8];
	int* block_ptrs[8];
	double dct[8][8]; double* dct_ptrs[8];
	int quantized[8][8]; int* quant_ptrs[8];
	double deq[8][8]; double* deq_ptrs[8];
	int idct_out[8][8]; int* idct_ptrs[8];

	for (int i = 0; i < 8; ++i) {
		block_ptrs[i] = block[i];
		dct_ptrs[i] = dct[i];
		quant_ptrs[i] = quantized[i];
		deq_ptrs[i] = deq[i];
		idct_ptrs[i] = idct_out[i];
	}

	for (size_t by = 0; by < blocks_h; ++by) {
		for (size_t bx = 0; bx < blocks_w; ++bx) {
			// fill block with spatial values (0..255)
			for (int r = 0; r < B; ++r) for (int c = 0; c < B; ++c) {
				size_t y = by * B + r;
				size_t x = bx * B + c;
				block[r][c] = static_cast<int>(rows[y][x]);
			}

			// Use cCompression helpers (they encapsulate level-shift/DCT/quant internally)
			double blockEQM = compressor.EQM(block_ptrs);
			double blockTaux = compressor.Taux_Compression(block_ptrs);

			sumEQM += blockEQM;
			sumTaux += blockTaux;
			++nblocks;

			// Build reconstruction using the same primitive helpers so output matches EQM calculation
			// Level shift
			int shifted[8][8]; int* shifted_ptrs[8];
			for (int i = 0; i < B; ++i) { shifted_ptrs[i] = shifted[i]; for (int j = 0; j < B; ++j) shifted[i][j] = block[i][j] - 128; }

			// DCT, quantize, dequantize, IDCT
			Calcul_DCT_Block(shifted_ptrs, dct_ptrs);
			quant_JPEG(dct_ptrs, quant_ptrs);
			dequant_JPEG(quant_ptrs, deq_ptrs);
			Calcul_IDCT_Block(deq_ptrs, idct_ptrs);

			// store reconstructed (shift back +128)
			for (int r = 0; r < B; ++r) for (int c = 0; c < B; ++c) {
				int val = idct_out[r][c] + 128;
				if (val < 0) val = 0; if (val > 255) val = 255;
				size_t y = by * B + r;
				size_t x = bx * B + c;
				recon[y * width + x] = static_cast<unsigned char>(val);
			}
		}
	}

	double avgEQM = (nblocks>0) ? sumEQM / static_cast<double>(nblocks) : 0.0;
	double avgTaux = (nblocks>0) ? sumTaux / static_cast<double>(nblocks) : 0.0;

	cout.setf(std::ios::fixed);
	cout << setprecision(6);
	cout << "Quality=" << qual << " Avg EQM(MSE)=" << avgEQM << " Avg Taux=" << avgTaux << "\n";

	// write reconstructed image
	ofstream fout("recon_lenna.pgm", ios::binary);
	if (fout) {
		fout << "P5\n" << width << " " << height << "\n255\n";
		fout.write(reinterpret_cast<char*>(recon.data()), recon.size());
		fout.close();
		cout << "Wrote recon_lenna.pgm\n";
	} else cerr << "Cannot write recon file\n";

	// Now produce the pedagogic RLE trame using cCompression::RLE()
	compressor.setLargeur(static_cast<unsigned int>(width));
	compressor.setHauteur(static_cast<unsigned int>(height));
	compressor.setBuffer(rows.data());

	size_t maxBlocks = blocks_w * blocks_h;
	size_t maxBytes = maxBlocks * 128; // safe upper bound
	int *Trame_RLE = new int[1 + maxBytes];
	compressor.RLE(Trame_RLE);
	int trameLen = Trame_RLE[0];
	if (trameLen > 0) {
		ofstream rf("lenna.rle", ios::binary);
		if (rf) {
			for (int i = 0; i < trameLen; ++i) {
				signed char b = static_cast<signed char>(Trame_RLE[i + 1]);
				rf.put(static_cast<char>(b));
			}
			rf.close();
			cout << "Wrote lenna.rle (" << trameLen << " bytes)\n";
		} else {
			cerr << "Cannot write RLE file\n";
		}
	} else {
		cout << "No RLE data produced\n";
	}
	// additionally compress the RLE trame with Huffman coding and write output file
	compressor.Compression_JPEG(Trame_RLE, "lenna.huff");
	cout << "Called Compression_JPEG to produce lenna.huff (Huffman output)\n";

	delete[] Trame_RLE;

	return 0;
}

