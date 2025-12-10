#include <iostream>
#include "core/cCompression.h"
#include "dct/dct.h"
#include "quantification/quantification.h"
#include <fstream>
#include <vector>
#include <iterator>
#include <array>
#include <cstring>

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
	signed char block_trame[128]; for (int i=0;i<128;++i) block_trame[i]=0;
	comp.RLE_Block(quant_ptrs, 0, block_trame);
	std::cout << "RLE block trame (pairs until EOB):\n";
	for (int i=0;i<128;i+=2) {
		int a = static_cast<unsigned char>(block_trame[i]);
		int b = static_cast<signed char>(block_trame[i+1]);
		std::cout << "(" << a << "," << b << ") ";
		if (block_trame[i]==0 && block_trame[i+1]==0) { std::cout << "<EOB>\n"; break; }
	}

	// Build a Trame_RLE int[] and call Compression_JPEG to produce a .huff file
	int trame_len = 0;
	for (int i=0;i<128;i+=2) {
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
		// Fallback: manually parse header and decode Huffman payload
		std::ifstream in(outname, std::ios::binary);
		if (!in) { std::cerr << "Cannot open " << outname << std::endl; return 0; }
		// Read full file into vector
		std::vector<unsigned char> filedata((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		size_t pos = 0;
		if (filedata.size() < 4) { std::cerr << "Invalid file" << std::endl; return 0; }
		std::string magic(reinterpret_cast<char*>(filedata.data()), 4);
		if (magic != "HUF1") { std::cerr << "Unknown format: " << magic << std::endl; return 0; }
		pos += 4;
		uint16_t nbSym16 = 0; std::memcpy(&nbSym16, filedata.data()+pos, sizeof(nbSym16)); pos += sizeof(nbSym16);
		unsigned int nbSym = nbSym16;
		std::vector<char> symbols(nbSym);
		std::vector<double> freqs(nbSym);
		for (unsigned int i=0;i<nbSym;++i) {
			symbols[i] = static_cast<char>(filedata[pos]); pos += 1;
			uint32_t cnt=0; std::memcpy(&cnt, filedata.data()+pos, sizeof(cnt)); pos += sizeof(cnt);
			freqs[i] = static_cast<double>(cnt);
		}
		uint32_t payload_bytes = 0; std::memcpy(&payload_bytes, filedata.data()+pos, sizeof(payload_bytes)); pos += sizeof(payload_bytes);
		uint32_t payload_bits32 = 0; std::memcpy(&payload_bits32, filedata.data()+pos, sizeof(payload_bits32)); pos += sizeof(payload_bits32);
		uint64_t payload_bits = static_cast<uint64_t>(payload_bits32);
		if (pos + payload_bytes > filedata.size()) { std::cerr << "Truncated payload" << std::endl; return 0; }
		unsigned char *payload = filedata.data() + pos;

		// Build Huffman tree
		cHuffman h;
		h.HuffmanCodes(symbols.data(), freqs.data(), nbSym);
		sNoeud *root = h.getRacine(); if (!root) { std::cerr << "Failed to build Huffman tree" << std::endl; return 0; }

		// Decode bits into trameDec
		std::vector<char> trameDec;
		sNoeud *cursor = root;
		uint64_t valid_bits = (payload_bits > 0) ? payload_bits : static_cast<uint64_t>(payload_bytes) * 8ULL;
		for (uint64_t bitIndex = 0; bitIndex < valid_bits; ++bitIndex) {
			int bit = 7 - static_cast<int>(bitIndex % 8ULL);
			unsigned char byte = payload[bitIndex / 8ULL];
			int val = ((byte >> bit) & 1);
			cursor = (val == 0) ? cursor->mgauche : cursor->mdroit;
			if (!cursor) { std::cerr << "Invalid bitstream during manual decode" << std::endl; return 0; }
			if (!cursor->mgauche && !cursor->mdroit) {
				trameDec.push_back(cursor->mdonnee);
				cursor = root;
			}
		}
		if (trameDec.empty()) { std::cerr << "Empty decoded RLE" << std::endl; return 0; }

		// Parse RLE into quant block(s)
		static const int zigzag[64] = {
			 0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
			12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
			35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
			58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
		};
		size_t p2 = 0; std::array<int,64> q; q.fill(0); int previous_DC = 0;
		signed char dc_diff = static_cast<signed char>(trameDec[p2++]);
		int DC = static_cast<int>(dc_diff) + previous_DC; q[0] = DC; previous_DC = DC;
		int idx = 1;
		while (p2 + 1 <= trameDec.size() && idx < 64) {
			signed char run = static_cast<signed char>(trameDec[p2++]);
			signed char val = static_cast<signed char>(trameDec[p2++]);
			if (run == 0 && val == 0) break;
			idx += static_cast<int>(run);
			if (idx >= 64) break;
			q[zigzag[idx]] = static_cast<int>(val);
			idx++;
		}

		// Convert q (linear) to quant matrix
		int quant_matrix[8][8]; for (int k=0;k<64;++k) quant_matrix[k/8][k%8] = q[k];

		// Dequantize + IDCT to reconstruct
		double dequant_block[8][8]; double* dequant_ptrs[8];
		int recon_block[8][8]; int* recon_ptrs[8]; int* quant_ptrs_local[8];
		for (int i=0;i<8;++i) { dequant_ptrs[i]=dequant_block[i]; recon_ptrs[i]=recon_block[i]; quant_ptrs_local[i]=quant_matrix[i]; }
		dequant_JPEG(quant_ptrs_local, dequant_ptrs);
		Calcul_IDCT_Block(dequant_ptrs, recon_ptrs);
		for (int r=0;r<8;++r) for (int c=0;c<8;++c) {
			int val = recon_block[r][c] + 128; if (val<0) val=0; if (val>255) val=255; rec[r][c]=val;
		}
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

	return 0;
}
