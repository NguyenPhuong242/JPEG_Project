#!/usr/bin/env python3
"""Generate a small binary PPM (P6) for quick color tests.

Usage:
    python3 tools/make_sample_ppm.py tests/sample_color.ppm --width 8 --height 8

The image is a simple RGB gradient; values stay in [0,255] and the header
uses the P6 format expected by the color CLI.
"""

import argparse


def build_gradient(width: int, height: int) -> bytearray:
    """Create an RGB gradient pattern across X/Y."""
    if width <= 0 or height <= 0:
        raise ValueError("width and height must be positive")
    data = bytearray()
    max_x = max(1, width - 1)
    max_y = max(1, height - 1)
    for y in range(height):
        for x in range(width):
            r = int(255 * x / max_x)
            g = int(255 * y / max_y)
            b = int(255 * (x + y) / (max_x + max_y)) if (max_x + max_y) else 0
            data.extend((r, g, b))
    return data


def write_ppm(path: str, width: int, height: int, data: bytearray) -> None:
    """Write a P6 PPM file."""
    header = f"P6\n{width} {height}\n255\n".encode("ascii")
    with open(path, "wb") as f:
        f.write(header)
        f.write(data)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a tiny color PPM (P6) test image.")
    parser.add_argument("output", help="Output PPM path (e.g., tests/sample_color.ppm)")
    parser.add_argument("--width", type=int, default=8, help="Image width in pixels (default: 8)")
    parser.add_argument("--height", type=int, default=8, help="Image height in pixels (default: 8)")
    args = parser.parse_args()

    data = build_gradient(args.width, args.height)
    write_ppm(args.output, args.width, args.height, data)
    print(f"Wrote {args.output} ({args.width}x{args.height})")


if __name__ == "__main__":
    main()
