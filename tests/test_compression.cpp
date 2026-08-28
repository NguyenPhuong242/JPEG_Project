#include <iostream>
#include "core/cCompression.h"
#include "dct/dct.h"
#include "quantification/quantification.h"
#include <fstream>
#include <vector>
#include <iterator>
#include <array>
#include <cstring>
#include <cstdio>

int main()
{
	// Provided 8x8 sample block (typical JPEG test block)
	static const int blockVals[8][8] = {
		{139, 144, 149, 153, 155, 155, 155, 155},
		{144, 151, 153, 156, 159, 156, 156, 156},
		{150, 155, 160, 163, 158, 156, 156, 156},
		{159, 161, 162, 160, 160, 159, 159, 159},
		{159, 160, 161, 162, 162, 155, 155, 155},
		{161, 161, 161, 161, 160, 157, 157, 157},
		{162, 162, 161, 163, 162, 157, 157, 157},
		{162, 162, 161, 161, 163, 158, 158, 158}
	};

	// Build an int** view expected by the API
	int *rows[8];
	int storage[8][8];
	for (int i = 0; i < 8; ++i) {
		rows[i] = storage[i];
		for (int j = 0; j < 8; ++j) storage[i][j] = blockVals[i][j];
	}

	// Set a reasonable global quality for quantization
	cCompression::setQualiteGlobale(50);

	cCompression comp;

    // Show block original:
    std::cout << "Original Block (8x8):\n";
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) std::cout << storage[i][j] << (j==7?"":" ");
        std::cout << "\n";
    }   

	double eqm = comp.EQM(rows);
	double taux = comp.Taux_Compression(rows);

	std::cout << "EQM (MSE) = " << eqm << std::endl;
	std::cout << "Taux de compression (fraction zeros) = " << taux << std::endl;

	// --- Additional diagnostics: DCT -> Quant -> RLE -> Huffman file ---
	int shifted[8][8]; double dct[8][8]; int quant[8][8];
	int *dct_ptrs[8]; int *quant_ptrs_int[8]; double *dct_ptrs_d[8];
	for (int i = 0; i < 8; ++i) { dct_ptrs[i] = reinterpret_cast<int*>(dct[i]); dct_ptrs_d[i] = dct[i]; quant_ptrs_int[i] = quant[i]; }

	for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) shifted[i][j] = storage[i][j] - 128;
	// Note: Calcul_DCT_Block expects int*[] -> double*[]; adapt via intermediate pointers
	double dct_work[8][8]; double* dct_work_ptrs[8];
	for (int i=0;i<8;++i) dct_work_ptrs[i] = dct_work[i];
	int shifted_ptrs_i[8][8]; int* shifted_ptrs[8];
	for (int i=0;i<8;++i) { for (int j=0;j<8;++j) shifted_ptrs_i[i][j]=shifted[i][j]; shifted_ptrs[i]=shifted_ptrs_i[i]; }

	Calcul_DCT_Block(shifted_ptrs, dct_work_ptrs);

	// Prepare pointers for quant_JPEG (double** -> int**)
	int quant_mat[8][8]; int* quant_ptrs[8];
	for (int i=0;i<8;++i) quant_ptrs[i]=quant_mat[i];
	quant_JPEG(dct_work_ptrs, quant_ptrs);

	std::cout << "Quantized coefficients (8x8):\n";
	for (int i=0;i<8;++i) {
		for (int j=0;j<8;++j) std::cout << quant_mat[i][j] << (j==7?"":" ");
		std::cout << "\n";
	}

	// Zig-zag linearization
	static const int zigzag[64] = {
		 0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
	};
	int linear[64];
	for (int k=0;k<64;++k) linear[k] = quant_mat[zigzag[k]/8][zigzag[k]%8];
	std::cout << "Zigzag linear order:\n";
	for (int k=0;k<64;++k) { std::cout << linear[k] << (k%8==7?"\n":" "); }

	// RLE the block using cCompression::RLE_Block
	signed char block_trame[129]; for (int i=0;i<129;++i) block_trame[i]=0;
	comp.RLE_Block(quant_ptrs, 0, block_trame);
	std::cout << "RLE block trame (pairs until EOB):\n";
	std::cout << static_cast<int>(block_trame[0]) << " ";
	for (int i=1;i+1<129;i+=2) {
		int a = static_cast<unsigned char>(block_trame[i]);
		int b = static_cast<signed char>(block_trame[i+1]);
		std::cout << "(" << a << "," << b << ") ";
		if (block_trame[i]==0 && block_trame[i+1]==0) { std::cout << "<EOB>\n"; break; }
	}

	// Build a Trame_RLE int[] and call Compression_JPEG to produce a .huff file
	int trame_len = 0;
	trame_len += 1;
	for (int i=1;i+1<129;i+=2) {
		trame_len += 2;
		if (block_trame[i]==0 && block_trame[i+1]==0) break;
	}
	int *Trame_RLE = new int[1 + trame_len];
	Trame_RLE[0] = trame_len;
	for (int i=0;i<trame_len;++i) Trame_RLE[i+1] = static_cast<int>(block_trame[i]);

	const char *outname = "block_sample.huff";
	comp.Compression_JPEG(Trame_RLE, outname);
	std::cout << "Wrote Huffman file: " << outname << std::endl;
	delete[] Trame_RLE;

	// Attempt to use library decompression first
	cCompression comp2;
	unsigned char **rows_out = comp2.Decompression_JPEG(outname);
	int rec[8][8]; bool used_lib = false;
	if (rows_out) {
		unsigned int out_w = comp2.getLargeur();
		unsigned int out_h = comp2.getHauteur();
		std::cout << "Decompressed image size (library): " << out_w << "x" << out_h << std::endl;
		for (int r = 0; r < 8; ++r) for (int c = 0; c < 8; ++c) {
			unsigned char v = 0;
			if (r < (int)out_h && c < (int)out_w) v = rows_out[r][c];
			rec[r][c] = static_cast<int>(v);
		}
		delete[] rows_out[0]; delete[] rows_out;
		used_lib = true;
	} else {
		std::cerr << "Library decompression failed" << std::endl;
		return 1;
	}

	// Print recovered block and MSE
	std::cout << "Recovered block (8x8) (used_lib=" << (used_lib?"yes":"no") << "):\n";
	double mse_rec = 0.0;
	for (int i=0;i<8;++i) {
		for (int j=0;j<8;++j) {
			std::cout << rec[i][j] << (j==7?"":" ");
			double diff = static_cast<double>(storage[i][j] - rec[i][j]); mse_rec += diff*diff;
		}
		std::cout << "\n";
	}
	mse_rec /= 64.0;
	std::cout << "Recovered block MSE = " << mse_rec << std::endl;
	if (mse_rec > 10.0) {
		std::cerr << "Recovered block MSE too high" << std::endl;
		return 1;
	}

	int single_symbol_trame[] = {3, 0, 0, 0};
	cCompression single_symbol_comp(8, 8, 50, nullptr);
	single_symbol_comp.Compression_JPEG(single_symbol_trame, "single_symbol_block.huff");

	cCompression single_symbol_dec;
	unsigned char **single_rows = single_symbol_dec.Decompression_JPEG("single_symbol_block.huff");
	if (!single_rows || single_symbol_dec.getLargeur() != 8 || single_symbol_dec.getHauteur() != 8) {
		std::cerr << "Single-symbol Huffman round-trip failed" << std::endl;
		return 1;
	}
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			if (single_rows[r][c] != 128) {
				std::cerr << "Unexpected single-symbol reconstruction value" << std::endl;
				delete[] single_rows[0];
				delete[] single_rows;
				return 1;
			}
		}
	}
	delete[] single_rows[0];
	delete[] single_rows;
	std::remove("single_symbol_block.huff");

	return 0;
}
