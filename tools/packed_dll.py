#!/usr/bin/env python3
"""The engine's packed DLL format, as Counter-Strike 1.4's and 1.5's client.dll
ship: read it, and write it from a PE file.

    packed_dll.py info FILE                  the header, the section table, the export directory
    packed_dll.py pe FILE OUT                a PE file around the decoded sections (for PE tools)
    packed_dll.py pack PE OUT NUMBER [BASE]  pack a PE file the same way

The layout follows oxiKKK's decryptor
(https://github.com/oxiKKK/goldsource-dll-decryptor), whose field names are
used below, and was checked against the two retail files:

- 68 plain bytes shaped like a DOS header: 60 bytes of text (all zero in
  the retail files), the offset of the signature where e_lfanew would be
  (0x40), and the signature 0x12345678;
- then everything else under a running XOR: the key starts at 'W' and adds
  each decoded byte plus 'W' (modulo 256);
- decoded, a 24-byte header: nRandom, the number of sections minus one, the
  address of the export "F", the image base, the entry point and the import
  directory, each but the first two disguised by a constant;
- one 20-byte record per section (address, memory size, size in the file,
  offset in the file, a flag), then the sections' bytes.

What a packed file keeps of its PE file: every section but the relocations,
each cut to min(VirtualSize, SizeOfRawData) bytes with max() of the two as
its memory size, all addresses moved to the header's image base by the PE
file's own relocation table. Not kept: the PE headers and the relocation
table. So "pack" can turn a rebuilt PE file into the packed form and the
result can be compared with the shipped file byte for byte. One value has to
come from the shipped file because nothing else determines it: nRandom
(0x6035 in Counter-Strike 1.4, 0x08D5 in 1.5; both below 0x8000, as a
rand() result would be).

"pe" writes PE headers of its own around the decoded sections; only the
sections and the time stamp (from the export directory) are the shipped
file's. Needs only Python 3's standard library.
"""
import struct
import sys
import time

ALIGN = 0x1000
XOR_F, XOR_BASE, XOR_IMPORTS = 0x7A32BC85, 0x49C042D1, 0x872C3D47


def decode(data):
    out = bytearray(data)
    k = ord("W")
    for i in range(68, len(out)):
        out[i] ^= k
        k = (k + out[i] + ord("W")) & 0xFF
    return bytes(out)


def encode(data):
    out = bytearray(data)
    k = ord("W")
    for i in range(68, len(out)):
        p = out[i]
        out[i] = p ^ k
        k = (k + p + ord("W")) & 0xFF
    return bytes(out)


def parse(data):
    """The decoded file, its header (a dict) and its section records."""
    misc, dboff, sig = struct.unpack_from("<60sII", data, 0)
    if sig != 0x12345678:
        raise SystemExit("not a packed DLL (signature %#x)" % sig)
    d = decode(data)
    rnd, n, f, base, entry, imp = struct.unpack_from("<6I", d, 68)
    hdr = dict(misc=misc, dboff=dboff, random=rnd, sections=n + 1,
               f=f ^ XOR_F, base=base ^ XOR_BASE,
               entry=(entry - 12) & 0xFFFFFFFF, imports=imp ^ XOR_IMPORTS)
    secs = [struct.unpack_from("<4IB", d, 92 + 20 * i) for i in range(n + 1)]
    return d, hdr, secs


def reader(d, hdr, secs):
    def at(rva, n):
        for va, msz, fsz, off, flag in secs:
            o = rva - (va - hdr["base"])
            if 0 <= o < fsz:
                return d[off + o:off + min(o + n, fsz)]
        raise SystemExit("rva %#x is in no section" % rva)
    return at


def find_exports(d, hdr, secs):
    """The export directory, found by LINK's layout (its three tables follow it).
    Returns (rva, size, time stamp, functions, names) or None."""
    at = reader(d, hdr, secs)
    for va, msz, fsz, off, flag in secs:
        for o in range(0, fsz - 40, 4):
            ch, ts, ver, name, ob, nf, nn, af, an, ao = struct.unpack_from("<10I", d, off + o)
            rva = va - hdr["base"] + o
            if ch == 0 and ver == 0 and nf and nn <= nf and af == rva + 40 \
                    and an == af + 4 * nf and ao == an + 4 * nn:
                strs = [name] + list(struct.unpack("<%dI" % nn, at(an, 4 * nn)))
                end = max(s + at(s, 256).index(b"\0") + 1 for s in strs)
                return rva, end - rva, ts, nf, nn
    return None


def to_pe(d, hdr, secs):
    """A PE file around the decoded sections (headers of our own)."""
    base = hdr["base"]
    if len(secs) != 3:
        raise SystemExit("expected LINK's three sections, found %d" % len(secs))
    at = reader(d, hdr, secs)
    recs, body, fo, sizes = b"", b"", ALIGN, []
    for (va, msz, fsz, off, flag), name, ch in zip(
            secs, (b".text", b".rdata", b".data"), (0x60000020, 0x40000040, 0xC0000040)):
        # the packer's min()/max(), undone: SizeOfRawData is the aligned one
        vsize, raw = (msz, fsz) if fsz % ALIGN == 0 and msz >= fsz else (fsz, msz)
        if raw % ALIGN:
            raise SystemExit("section %s: neither size is file-aligned" % name.decode())
        recs += struct.pack("<8sIIIIIIHHI", name, vsize, va - base, raw, fo, 0, 0, 0, 0, ch)
        body += d[off:off + fsz].ljust(raw, b"\0")
        sizes.append((va - base, vsize, raw))
        fo += raw
    dirs = [(0, 0)] * 16
    exp = find_exports(d, hdr, secs)
    if exp:
        dirs[0] = exp[:2]
    imp, n = hdr["imports"] - base, 0
    while any(at(imp + 20 * n, 20)):
        n += 1
    dirs[1] = (imp, 20 * (n + 1))
    image = -(-(sizes[2][0] + sizes[2][1]) // ALIGN) * ALIGN
    coff = struct.pack("<HHIIIHH", 0x14C, 3, exp[2] if exp else 0, 0, 0, 0xE0, 0x210F)
    opt = struct.pack("<HBB9I6H4I2H6I", 0x10B, 6, 0,
                      sizes[0][2], sizes[1][2] + sizes[2][2], 0, hdr["entry"] - base,
                      sizes[0][0], sizes[1][0], base, ALIGN, ALIGN,
                      4, 0, 0, 0, 4, 0,
                      0, image, ALIGN, 0,
                      2, 0,
                      0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    head = b"MZ" + bytes(58) + struct.pack("<I", 0x80) + bytes(0x40) + b"PE\0\0" + coff + opt
    head += b"".join(struct.pack("<II", *x) for x in dirs) + recs
    return head.ljust(ALIGN, b"\0") + body


class PE:
    """The parts of a 32-bit PE file that packing needs."""

    def __init__(self, data):
        if data[:2] != b"MZ":
            raise SystemExit("not a PE file")
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            raise SystemExit("not a PE file")
        nsec, opt_size = struct.unpack_from("<H", data, pe + 6)[0], struct.unpack_from("<H", data, pe + 20)[0]
        opt = pe + 24
        if struct.unpack_from("<H", data, opt)[0] != 0x10B:
            raise SystemExit("not a 32-bit PE file")
        self.data = data
        self.entry = struct.unpack_from("<I", data, opt + 16)[0]
        self.image_base = struct.unpack_from("<I", data, opt + 28)[0]
        ndirs = struct.unpack_from("<I", data, opt + 92)[0]
        self.dirs = [struct.unpack_from("<II", data, opt + 96 + 8 * i) for i in range(ndirs)]
        self.sections = []
        for i in range(nsec):
            o = opt + opt_size + 40 * i
            name, vsize, va, raw_size, raw_ptr = struct.unpack_from("<8sIIII", data, o)
            self.sections.append(dict(name=name.rstrip(b"\0"), vsize=vsize, va=va, raw_size=raw_size,
                                      raw_ptr=raw_ptr))

    def read(self, rva, n):
        for s in self.sections:
            o = rva - s["va"]
            if 0 <= o < s["raw_size"]:
                return self.data[s["raw_ptr"] + o:s["raw_ptr"] + o + n]
        raise SystemExit("rva %#x is in no section" % rva)

    def cstring(self, rva):
        b = self.read(rva, 512)
        return b[:b.index(b"\0")]

    def exports(self):
        """{name: rva} of the export directory."""
        rva, size = self.dirs[0]
        if not rva:
            return {}
        nf, nn, af, an, ao = struct.unpack_from("<5I", self.read(rva + 20, 20))
        funcs = struct.unpack("<%dI" % nf, self.read(af, 4 * nf))
        names = struct.unpack("<%dI" % nn, self.read(an, 4 * nn))
        ords = struct.unpack("<%dH" % nn, self.read(ao, 2 * nn))
        return {self.cstring(n): funcs[o] for n, o in zip(names, ords)}

    def relocations(self):
        """(rva, type) of every base relocation."""
        rva, size = self.dirs[5] if len(self.dirs) > 5 else (0, 0)
        out, o = [], 0
        while o < size:
            page, block = struct.unpack_from("<II", self.read(rva + o, 8))
            if block < 8:
                raise SystemExit("bad relocation block at %#x" % (rva + o))
            for k in range((block - 8) // 2):
                e = struct.unpack_from("<H", self.read(rva + o + 8 + 2 * k, 2))[0]
                out.append((page + (e & 0xFFF), e >> 12))
            o += block
        return out


def moved(pe, base):
    """The sections a packed file keeps, cut to their bytes and moved to base."""
    reloc = pe.dirs[5][0] if len(pe.dirs) > 5 else 0
    secs = [s for s in pe.sections if not reloc or s["va"] != reloc]
    datas = [bytearray(pe.data[s["raw_ptr"]:s["raw_ptr"] + min(s["vsize"], s["raw_size"])]) for s in secs]
    delta = (base - pe.image_base) & 0xFFFFFFFF
    if delta:
        if not reloc:
            raise SystemExit("no relocations: the image cannot be moved to %#x" % base)
        for rva, typ in pe.relocations():
            if typ == 0:
                continue
            if typ != 3:
                raise SystemExit("relocation type %d at %#x" % (typ, rva))
            for s, sd in zip(secs, datas):
                o = rva - s["va"]
                if 0 <= o <= len(sd) - 4:
                    v = struct.unpack_from("<I", sd, o)[0]
                    struct.pack_into("<I", sd, o, (v + delta) & 0xFFFFFFFF)
                    break
            else:
                raise SystemExit("relocation at %#x is outside the packed bytes" % rva)
    return secs, datas


def pack(data, rnd, base=None):
    """The packed form of the PE file DATA, at image base BASE (default: the PE's own)."""
    pe = PE(data)
    if base is None:
        base = pe.image_base
    secs, datas = moved(pe, base)
    f = pe.exports().get(b"F")
    if f is None:
        raise SystemExit("no export F")
    off = 68 + 24 + 20 * len(secs)
    out = struct.pack("<60sII", b"", 0x40, 0x12345678)
    out += struct.pack("<6I", rnd, len(secs) - 1, (base + f) ^ XOR_F, base ^ XOR_BASE,
                       (base + pe.entry + 12) & 0xFFFFFFFF,
                       (base + pe.dirs[1][0]) ^ XOR_IMPORTS)
    for s, sd in zip(secs, datas):
        out += struct.pack("<5I", base + s["va"], max(s["vsize"], s["raw_size"]), len(sd), off, 0)
        off += len(sd)
    return encode(out + b"".join(datas))


def main(argv):
    if len(argv) < 2 or argv[0] not in ("info", "pe", "pack") or (argv[0] != "info" and len(argv) < 3) \
            or (argv[0] == "pack" and len(argv) < 4):
        sys.exit(__doc__.split("\n\n")[1])
    cmd, path = argv[0], argv[1]
    data = open(path, "rb").read()
    if cmd == "pack":
        base = int(argv[4], 0) if len(argv) > 4 else None
        open(argv[2], "wb").write(pack(data, int(argv[3], 0), base))
        return
    d, hdr, secs = parse(data)
    if cmd == "info":
        print("size %d  text %r  offset field %#x" % (len(data), hdr["misc"].rstrip(b"\0"), hdr["dboff"]))
        print("number %#06x  sections %d  F %#x  base %#x  entry %#x  imports %#x" % (
            hdr["random"], hdr["sections"], hdr["f"], hdr["base"], hdr["entry"], hdr["imports"]))
        for i, (va, msz, fsz, off, flag) in enumerate(secs):
            print("  %d: va %#x  mem %#x  file %#x  at %#x  flag %d" % (i, va, msz, fsz, off, flag))
        exp = find_exports(d, hdr, secs)
        if exp:
            print("export directory at rva %#x, %#x bytes: %d functions, %d names, time stamp %#x (%s UTC)" % (
                exp[0], exp[1], exp[3], exp[4], exp[2],
                time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(exp[2]))))
    elif cmd == "pe":
        open(argv[2], "wb").write(to_pe(d, hdr, secs))


if __name__ == "__main__":
    main(sys.argv[1:])
