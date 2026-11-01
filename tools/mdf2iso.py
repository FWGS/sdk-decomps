#!/usr/bin/env python3
"""Turn a raw CD image (.mdf with 2448-byte sectors) into an ISO that 7-Zip reads.

    mdf2iso.py IMAGE.mdf OUT.iso       (IMAGE.mdf may be - for standard input)

The archive.org disc images of Visual C++ 6.0 and its service pack are Alcohol
120% .mdf files: every sector holds 2352 bytes of raw Mode 1 data plus 96 bytes
of subchannel data. The 2048 bytes of user data start at offset 16 of each
sector (after the 12-byte sync pattern and the 4-byte header). The script
copies those and prints the MD5 of the input, which you can compare with the
archive's checksums.md5.
"""
import hashlib
import sys

RAW, OFFSET, USER = 2448, 16, 2048


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip().split("\n\n")[1])
    src = sys.stdin.buffer if sys.argv[1] == "-" else open(sys.argv[1], "rb")
    md5, sectors = hashlib.md5(), 0
    with open(sys.argv[2], "wb") as out:
        while True:
            block = src.read(RAW * 512)
            if not block:
                break
            md5.update(block)
            if len(block) % RAW:
                more = src.read(RAW - len(block) % RAW)
                md5.update(more)
                block += more
                if len(block) % RAW:
                    sys.exit(f"{sys.argv[1]}: size is not a multiple of {RAW} bytes")
            for i in range(0, len(block), RAW):
                out.write(block[i + OFFSET:i + OFFSET + USER])
            sectors += len(block) // RAW
    print(f"{sectors} sectors; input md5 {md5.hexdigest()}")


if __name__ == "__main__":
    main()
