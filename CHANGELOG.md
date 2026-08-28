# Changelog

## 0.2.0 - 2026-08-28

- Fix grayscale JPEG round-trip decoding for complete 8x8 block streams.
- Fix RLE/Huffman handling, including single-symbol Huffman streams.
- Store decompression quality metadata in HUF1 output.
- Split decompression into dedicated `cDecompression` and `cDecompressionCouleur` classes.
- Harden decompression against malformed payload metadata, invalid color quality metadata, and failed PPM writes.
- Update CLI and tests for the new decompression flow.

## 0.1.0 - 2026-08-28

- Baseline project state before the JPEG round-trip and decompression fixes.
