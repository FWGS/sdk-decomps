#!/bin/sh
# Build Wanted! 1.5's DLLs the way the retail files were built.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/wanted.sh SRC OUT [server|client|both]
#
# SRC: a checkout of branch wanted. OUT: a build folder.
# Compiler: Visual C++ 6.0 Professional as on the CD (tree vc6), its LINK
# running on the CD's MSVCRT.DLL (vc6.sh's default).
#
# Server (wanted.dll, built as hl.dll): not yet identical to retail; 810
# bytes differ, all in CBot::Wander (docs/wanted.md). The build environment
# is reproduced in full:
#  - the source tree at retail's PDB path, K:\Wantedhl\Source Code\New Code\
#    Code (the CodeView record names ...\dlls\Releasehl\hl.pdb): a symlink
#    tree in OUT/root, which is drive K: of a copy of the Wine prefix in
#    OUT/wineprefix;
#  - a full build into dlls\Releasehl, then the last build session and the
#    PDB's 18 links, the last with the export directory stamped two seconds
#    before the image (wanted_server.py has the details).
# Client (client.dll): identical to retail. No debug directory, so only the
# link time matters, 2000-12-02 15:44:22 UTC. A full build into OUT/client
# in the toolchain's own Wine prefix.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"
release_init "$@"
release_need vc6

if [ "$WHAT" != client ]; then
	release_prefix
	mkdir -p "$OUT/root"
	ln -sfn "$OUT/root" "$OUT/wineprefix/dosdevices/k:"
	T="$OUT/root/Wantedhl/Source Code/New Code/Code"
	release_tree "$T"
	if ! env VC_WINEPREFIX="$OUT/wineprefix" \
		python3 "$TOOLS/release/wanted_server.py" "$T/dlls/hl.dsp" "$T/dlls/Releasehl" "$OUT/root"; then
		echo "$0: server build failed; every command is in $T/dlls/Releasehl/build.log and session.log" >&2
		exit 1
	fi
	release_report server "$T/dlls/Releasehl/hl.dll" \
		2e3233515c6e5cc7b77a84bb514212583997b10497b6ca96a1ca57c53ac25875 dlls/wanted.dll
fi

if [ "$WHAT" != server ]; then
	O=$OUT/client
	rm -rf "$O"
	mkdir -p "$O"
	: > "$OUT/client.log"
	echo "client: full build, LINK at 2000-12-02 15:44:22 UTC"
	release_run "$OUT/client.log" \
		python3 "$TOOLS/build_dsp.py" "$SRC/cl_dll/cl_dll.dsp" "cl_dll - Win32 Release" "$O" \
		--name client --link-time '2000-12-02 15:44:22' --full
	release_summary "$OUT/client.log"
	release_report client "$O/client.dll" \
		584813e7ec904ec6fa596fa1360d3da37bfc87cfb1b1785886b8d868d793e479 cl_dlls/client.dll
fi
exit $RC
