#!/bin/sh
# Build $VC_TOOLCHAINS/<tree>/nativecrt: a copy of <tree>/VC98/BIN whose
# programs load the C runtime shipped with that release instead of Wine's
# builtin msvcrt.dll.
#
#   make_vc6_nativecrt.sh MSVCRT.DLL          (vc6: RTM, Professional)
#   make_vc6_nativecrt.sh -std MSVCRT.DLL     (vc6std: RTM, Standard edition back end)
#   make_vc6_nativecrt.sh -sp4 MSVCRT.DLL     (vc6sp4: Service Pack 4, Professional)
#   make_vc6_nativecrt.sh -sp4std MSVCRT.DLL  (vc6sp4std: SP4, Standard edition back end)
#
# RTM trees: the VC6 CD's OS/SYSTEM/MSVCRT.DLL (6.00.8168.0, 254005 bytes,
# sha1 11179154ec7375e7dc50f2b349c571d01a8856b9).
# SP4 trees: the SP4 disc's enu/os/system/msvcrt.dll (6.00.8797.0, 278581
# bytes, sha1 63a4fcd64ecea975c1b91de04702c68a9f2a3c7d).
#
# Wine always maps msvcrt.dll to its builtin, even with a native copy next to
# the program. So every "msvcrt.dll" import name in the copied EXEs and DLLs
# is renamed to "MSVCRX.dll" (same length), and the DLL is installed under
# that name. Why it matters: LINK sorts with the C runtime's qsort, whose order
# of equal keys differs between Wine's builtin and the 1998 DLL
# (docs/compilers.md).
set -eu

: "${VC_TOOLCHAINS:?make_vc6_nativecrt.sh: set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)}"
tc=$(cd "$VC_TOOLCHAINS" && pwd)
tree=vc6
want=11179154ec7375e7dc50f2b349c571d01a8856b9
case "${1:-}" in
-sp4) tree=vc6sp4; want=63a4fcd64ecea975c1b91de04702c68a9f2a3c7d; shift ;;
-sp4std) tree=vc6sp4std; want=63a4fcd64ecea975c1b91de04702c68a9f2a3c7d; shift ;;
-std) tree=vc6std; shift ;;
-rtm) shift ;;
esac
crt=${1:?usage: make_vc6_nativecrt.sh [-std|-sp4|-sp4std] MSVCRT.DLL}
have=$(sha1sum "$crt" | cut -d' ' -f1)
[ "$have" = "$want" ] || { echo "$crt: sha1 $have, expected $want" >&2; exit 1; }
[ -d "$tc/$tree/VC98/BIN" ] || { echo "no $tc/$tree/VC98/BIN" >&2; exit 1; }

dst=$tc/$tree/nativecrt
rm -rf "$dst"
mkdir -p "$dst"
cp -p "$tc/$tree"/VC98/BIN/* "$dst"/
cp -p "$crt" "$dst/MSVCRX.DLL"
python3 -I - "$dst" <<'EOF'
import os, re, sys
d = sys.argv[1]
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
EOF
