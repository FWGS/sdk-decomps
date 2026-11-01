#!/bin/sh
# Build Gunman Chronicles' DLLs the way the retail files were built.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/gunman-chronicles.sh SRC OUT [server|client|both]
#
# SRC: a checkout of branch gunman-chronicles. OUT: a build folder.
# Compiler: Visual C++ 6.0 Professional as on the CD (tree vc6), its LINK
# running on the CD's MSVCRT.DLL (vc6.sh's default): both import tables come
# out as retail's.
#
# Server (gunman.dll): identical to retail. The build reproduces the build
# environment, not just the code:
#  - the source tree at retail's PDB path, C:\Gunman\Source (the CodeView
#    record names ...\dlls\Releasehl\gunman.pdb): a symlink tree inside
#    drive_c of a copy of the Wine prefix in OUT/wineprefix;
#  - a full build into dlls\Releasehl, deleted first, named gunman, so the
#    PDB is new: retail's CodeView record has age 1 and the link time as its
#    signature;
#  - LINK with the clock frozen at retail's link time, 2000-11-10 22:46:15
#    UTC (libfaketime), which sets the COFF, export and debug time stamps.
# Client (client.dll): not yet identical to retail (docs/gunman-chronicles.md).
# No debug directory, so only the link time matters, 2000-11-12 19:45:26
# UTC. A full build into OUT/client in the toolchain's own Wine prefix.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"
release_init "$@"
release_need vc6

if [ "$WHAT" != client ]; then
	release_prefix
	T=$OUT/wineprefix/drive_c/Gunman/Source
	release_tree "$T"
	rm -rf "$T/dlls/Releasehl"
	: > "$OUT/server.log"
	echo "server: full build, LINK at 2000-11-10 22:46:15 UTC"
	release_run "$OUT/server.log" env VC_WINEPREFIX="$OUT/wineprefix" \
		python3 "$TOOLS/build_dsp.py" "$T/dlls/hl.dsp" "hl - Win32 Release" "$T/dlls/Releasehl" \
		--name gunman --drive "C:=$OUT/wineprefix/drive_c" --link-time '2000-11-10 22:46:15' --full
	release_summary "$OUT/server.log"
	release_report server "$T/dlls/Releasehl/gunman.dll" \
		e6e8d4497f11787889f12e7356157cbce98d380570805ca55faef3c07ce088f0 dlls/gunman.dll
fi

if [ "$WHAT" != server ]; then
	O=$OUT/client
	rm -rf "$O"
	mkdir -p "$O"
	: > "$OUT/client.log"
	echo "client: full build, LINK at 2000-11-12 19:45:26 UTC"
	release_run "$OUT/client.log" \
		python3 "$TOOLS/build_dsp.py" "$SRC/cl_dll/cl_dll.dsp" "cl_dll - Win32 Release" "$O" \
		--name client --link-time '2000-11-12 19:45:26' --full
	release_summary "$OUT/client.log"
	release_report client "$O/client.dll" \
		4d2918498fb0d3aa40ede643beabea235925c0eaa6742130a686968686a51aed cl_dlls/client.dll
fi
exit $RC
