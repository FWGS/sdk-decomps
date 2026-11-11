#!/bin/sh
# Build They Hunger's DLLs (episodes 1 to 3, March 2001) as the retail files,
# byte for byte.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/they-hunger.sh SRC OUT [server|client|both]
#
# SRC: a checkout of branch they-hunger. OUT: a build folder.
# Compiler: Visual C++ 6.0 Professional as on the CD (tree vc6), its LINK
# running on the CD's MSVCRT.DLL (vc6.sh's default).
#
# Server (einar.dll, built as hl.dll). The build reproduces the build
# environment, not just the code:
#  - the source tree at retail's PDB path, C:\hlsrc11 (the CodeView record
#    names C:\hlsrc11\dlls\Releasehl\hl.pdb): a symlink tree inside drive_c
#    of a copy of the Wine prefix in OUT/wineprefix;
#  - a full build into dlls\Releasehl, then the last build sessions (the
#    sources that include weapons.h recompiled after an edit of it) and the
#    PDB's 73 links, the first at the PDB's creation time, 2001-03-10
#    06:25:12 UTC, the others at retail's link time, 2001-03-26 05:43:20 UTC
#    (they-hunger_server.py has the details).
# Client (client.dll). No debug directory, so only the link time matters,
# 2001-03-26 05:49:33 UTC. A full build into OUT/client in the toolchain's
# own Wine prefix.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"
release_init "$@"
release_need vc6

if [ "$WHAT" != client ]; then
	release_prefix
	T="$OUT/wineprefix/drive_c/hlsrc11"
	release_tree "$T"
	if ! env VC_WINEPREFIX="$OUT/wineprefix" \
		python3 "$TOOLS/release/they-hunger_server.py" "$T/dlls/hl.dsp" "$T/dlls/Releasehl" "$OUT/wineprefix/drive_c"; then
		echo "$0: server build failed; every command is in $T/dlls/Releasehl/build.log and session.log" >&2
		exit 1
	fi
	release_report server "$T/dlls/Releasehl/hl.dll" \
		0b9c21c87dab0eec02b323eaab434cf77a2f69982bbf0a3f3f6a316fa95159d3 dlls/einar.dll
fi

if [ "$WHAT" != server ]; then
	O=$OUT/client
	rm -rf "$O"
	mkdir -p "$O"
	: > "$OUT/client.log"
	echo "client: full build, LINK at 2001-03-26 05:49:33 UTC"
	release_run "$OUT/client.log" \
		python3 "$TOOLS/build_dsp.py" "$SRC/cl_dll/cl_dll.dsp" "cl_dll - Win32 Release" "$O" \
		--name client --link-time '2001-03-26 05:49:33' --full
	release_summary "$OUT/client.log"
	release_report client "$O/client.dll" \
		09e8de1a62f4f1fae32b040678fc5b97fe5ba8f20ec44db14b29aedcf37db105 cl_dlls/client.dll
fi
exit $RC
