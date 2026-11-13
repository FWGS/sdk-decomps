#!/bin/sh
# Build $VC_TOOLCHAINS/vc5/<tree>/nativecrt: a copy of vc5/<tree>/BIN whose
# programs load the C runtime shipped with that release instead of Wine's
# builtin msvcrt.dll. `VC5_CRT=native vc5.sh ...` and build_dsp.py's
# vc5-rtm-n and vc5-n run from it.
#
#   make_vc5_nativecrt.sh -rtm MSVCRT.DLL   (vc5/rtm: Visual C++ 5.0 as on the CD)
#   make_vc5_nativecrt.sh -sp3 MSVCRT.DLL   (vc5/sp3: with Service Pack 3)
#
# RTM: DEVSTUDIO/VC/REDIST/MSVCRT.DLL of the Visual Studio 97 Professional
# disc 3 (5.00.7022, 271632 bytes, sha1 56c58e43281a14356bc038280bc4ac4267a9a581).
# SP3: VS97_SP3/all/vc/redist/msvcrt.dll of the service pack's cabinets
# (5.00.7303, 277776 bytes, sha1 e1eb5125f645e70ef9190b092a5d0a4bf2d86d61).
#
# Wine always maps msvcrt.dll to its builtin, even with a native copy next to
# the program. So every "msvcrt.dll" import name in the copied EXEs and DLLs
# is renamed to "MSVCRX.dll" (same length), and the DLL is installed under
# that name. In both trees every such string is an import name or a bound
# import name; nothing else matches. Why it matters: LINK 5.00 sorts with the
# C runtime's qsort, whose order of equal keys differs between Wine's builtin
# and the 1997 DLL (docs/compilers.md).
set -eu

: "${VC_TOOLCHAINS:?make_vc5_nativecrt.sh: set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)}"
tc=$(cd "$VC_TOOLCHAINS" && pwd)
sp=sp3
want=e1eb5125f645e70ef9190b092a5d0a4bf2d86d61
case "${1:-}" in
-rtm) sp=rtm; want=56c58e43281a14356bc038280bc4ac4267a9a581; shift ;;
-sp3) shift ;;
esac
crt=${1:?usage: make_vc5_nativecrt.sh [-rtm|-sp3] MSVCRT.DLL}
have=$(sha1sum "$crt" | cut -d' ' -f1)
[ "$have" = "$want" ] || { echo "$crt: sha1 $have, expected $want (vc5/$sp)" >&2; exit 1; }
[ -d "$tc/vc5/$sp/BIN" ] || { echo "no $tc/vc5/$sp/BIN" >&2; exit 1; }

dst=$tc/vc5/$sp/nativecrt
rm -rf "$dst"
mkdir -p "$dst"
cp -p "$tc/vc5/$sp"/BIN/* "$dst"/
cp -p "$crt" "$dst/MSVCRX.DLL"
python3 -I - "$dst" <<'EOF'
import os, re, sys
d = sys.argv[1]
files = 0
for f in sorted(os.listdir(d)):
    if not f.upper().endswith((".EXE", ".DLL")) or f.upper() == "MSVCRX.DLL":
        continue
    p = os.path.join(d, f)
    b = bytearray(open(p, "rb").read())
    n = 0
    for m in re.finditer(rb"(?i)msvcrt\.dll\x00", bytes(b)):
        b[m.start():m.start() + 10] = b"MSVCRX.dll"
        n += 1
    if n:
        open(p, "wb").write(b)
        print(f"{f}: {n} import name(s) renamed")
        files += 1
if not files:
    sys.exit("no msvcrt.dll import found: is BIN already patched?")
EOF
