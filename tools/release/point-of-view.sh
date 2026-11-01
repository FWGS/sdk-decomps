#!/bin/sh
# Build Point of View 1.1's DLLs as the retail files, byte for byte.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/point-of-view.sh SRC OUT [server|client|both]
#
# SRC: a checkout of branch point-of-view. OUT: a build folder.
# Compiler: Visual C++ 6.0 Professional as on the CD (tree vc6), its LINK
# running on the CD's MSVCRT.DLL (vc6.sh's default).
#
# Server (hl.dll). Besides the code, the image records where and when it
# was linked; the build reproduces both:
#  - the source tree at retail's PDB path, D:\Editors\Levels\POVSourceCode
#    (the CodeView record names ...\dlls\Releasehl\hl.pdb): a symlink tree
#    in OUT/root, which is drive D: of a copy of the Wine prefix in
#    OUT/wineprefix;
#  - a full build into dlls\Releasehl, deleted first, so the PDB is new
#    (age 1);
#  - LINK with the clock frozen at retail's link time, 2003-05-06 07:56:13
#    UTC (libfaketime), which sets the COFF, export, debug directory and
#    CodeView time stamps.
# Client (client.dll): no debug directory, so only the link time matters,
# 2003-05-02 01:24:25 UTC. A full build into OUT/client in the toolchain's
# own Wine prefix.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"
release_init "$@"
release_need vc6

if [ "$WHAT" != client ]; then
	release_prefix
	mkdir -p "$OUT/root"
	ln -sfn "$OUT/root" "$OUT/wineprefix/dosdevices/d:"
	T=$OUT/root/Editors/Levels/POVSourceCode
	release_tree "$T"
	rm -rf "$T/dlls/Releasehl"
	: > "$OUT/server.log"
	echo "server: full build, LINK at 2003-05-06 07:56:13 UTC"
	release_run "$OUT/server.log" env VC_WINEPREFIX="$OUT/wineprefix" \
		python3 "$TOOLS/build_dsp.py" "$T/dlls/hl.dsp" "hl - Win32 Release" "$T/dlls/Releasehl" \
		--name hl --drive "D:=$OUT/root" --link-time '2003-05-06 07:56:13' --full
	release_summary "$OUT/server.log"
	release_report server "$T/dlls/Releasehl/hl.dll" \
		0f42bad4664a87cf3793efd213088b9c2c5dd0ac230b45db3544fd7c344f7ccd dlls/hl.dll
fi

if [ "$WHAT" != server ]; then
	O=$OUT/client
	rm -rf "$O"
	mkdir -p "$O"
	: > "$OUT/client.log"
	echo "client: full build, LINK at 2003-05-02 01:24:25 UTC"
	release_run "$OUT/client.log" \
		python3 "$TOOLS/build_dsp.py" "$SRC/cl_dll/cl_dll.dsp" "cl_dll - Win32 Release" "$O" \
		--name client --link-time '2003-05-02 01:24:25' --full
	release_summary "$OUT/client.log"
	release_report client "$O/client.dll" \
		c0a40333d725c4a8d66e2640e78929ebc69bd118c28d3b79e0aab35d69608ea1 cl_dlls/client.dll
fi
exit $RC
