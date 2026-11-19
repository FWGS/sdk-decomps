#!/bin/sh
# Build Scientist Slaughterhouse 1.0's DLLs (August 2002) as the retail
# files, byte for byte.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/scientist-slaughterhouse.sh SRC OUT [server|client|both]
#
# SRC: a checkout of branch scientist-slaughterhouse. OUT: a build folder.
# Compiler: Visual C++ 6.0 Professional as on the CD (tree vc6), running on
# the CD's MSVCRT.DLL (vc6.sh's default).
#
# Both DLLs are built in one copy of the Wine prefix, OUT/wineprefix, laid
# out as the original machine was:
#  - the source tree at C:\Microsoft Visual Studio\MyProjects\Source Code
#    (the CodeView records name ...\dlls\Profilehl\hl.pdb and
#    ...\cl_dll\Debug\client.pdb, and the client's assert messages and
#    anonymous namespaces carry the path): symlinks into SRC, in drive_c;
#  - the compiler at C:\Microsoft Visual Studio\VC98 (vc6.sh's VC6_ROOT):
#    symlinks to the tree vc6's nativecrt, VC98/INCLUDE and VC98/LIB;
#  - TMP and TEMP pinned to C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp (Windows
#    2000's and XP's for the Administrator account) in the prefix's
#    HKCU\Environment.
# The client's RTTI records come out in an order that the lengths of the
# compiler's, the headers' and the temporary files' paths move; with these
# the build no longer depends on where the toolchain folder is or on the
# user's name.
# Server (hl.dll): a full build of hl.dsp "Win32 Profile", the last build
#   session, then the PDB's 1,433 links (scientist-slaughterhouse_server.py;
#   about a quarter of an hour, nearly all of it the links).
# Client (client.dll): the history of the incrementally linked Debug build:
#   a full build, 456 relinks, a full link, a last session and an
#   incremental relink (scientist-slaughterhouse_client.py; two minutes).
#   Until the full link, sword.cpp is an earlier text that ships with this
#   script (scientist-slaughterhouse/sword.0701.cpp).
#
# With RETAIL=/dir (the mod's folder, ssh, with DLLS/hl.dll and
# cl_dlls/client.dll) each file is also compared byte by byte.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"
release_init "$@"
release_need vc6
for d in nativecrt VC98/INCLUDE VC98/LIB; do
	[ -d "$VC_TOOLCHAINS/vc6/$d" ] || release_die "no $VC_TOOLCHAINS/vc6/$d (docs/setup.md)"
done

VC6_ROOT='C:\Microsoft Visual Studio'
TEMP_DIR='C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp'
export VC6_ROOT

# retail_name NAME...: the first of these paths that RETAIL has (the mod's
# download spells the server's folder DLLS), else the first
retail_name() {
	for _n in "$@"; do
		if [ -n "${RETAIL:-}" ] && [ -f "$RETAIL/$_n" ]; then
			echo "$_n"
			return
		fi
	done
	echo "$1"
}

# prefix_wine ARGS...: a Wine program in the prefix copy, headless (as vc6.sh runs them)
prefix_wine() {
	env -u DISPLAY -u WAYLAND_DISPLAY WINEPREFIX="$OUT/wineprefix" WINEDEBUG=-all \
		WINEDLLOVERRIDES='mscoree=;mshtml=;winemenubuilder.exe=d;winedbg.exe=d' timeout 120 wine "$@"
}

release_prefix
C=$OUT/wineprefix/drive_c
T="$C/Microsoft Visual Studio/MyProjects/Source Code"
release_tree "$T"
V="$C/Microsoft Visual Studio/VC98"
mkdir -p "$V"
ln -sfn "$VC_TOOLCHAINS/vc6/nativecrt" "$V/BIN"
ln -sfn "$VC_TOOLCHAINS/vc6/VC98/INCLUDE" "$V/INCLUDE"
ln -sfn "$VC_TOOLCHAINS/vc6/VC98/LIB" "$V/LIB"
mkdir -p "$C/DOCUME~1/ADMINI~1/LOCALS~1/Temp"
: > "$OUT/setup.log"
for v in TMP TEMP; do
	release_run "$OUT/setup.log" prefix_wine reg add 'HKCU\Environment' /v "$v" /t REG_SZ /d "$TEMP_DIR" /f
done
release_run "$OUT/setup.log" prefix_wine reg query 'HKCU\Environment'
[ "$(grep -c -F "$TEMP_DIR" "$OUT/setup.log")" -ge 2 ] || release_die "TMP and TEMP not set in the prefix copy (see $OUT/setup.log)"
echo "environment: the tree at C:\\Microsoft Visual Studio\\MyProjects\\Source Code, the compiler at $VC6_ROOT\\VC98, TMP=TEMP=$TEMP_DIR"

if [ "$WHAT" != client ]; then
	O=$T/dlls/Profilehl
	if ! env VC_WINEPREFIX="$OUT/wineprefix" \
		python3 "$TOOLS/release/scientist-slaughterhouse_server.py" "$T/dlls/hl.dsp" "$O" "$C"; then
		echo "$0: server build failed; every command is in $O/build.log and session.log" >&2
		exit 1
	fi
	release_report server "$O/hl.dll" \
		8e4c150e8d8d57f13bbf7f6b5a5e4ebc37258838ccb7708c760bb7b3c139d4ae "$(retail_name DLLS/hl.dll dlls/hl.dll)"
fi

if [ "$WHAT" != server ]; then
	O=$T/cl_dll/Debug
	if ! env VC_WINEPREFIX="$OUT/wineprefix" \
		python3 "$TOOLS/release/scientist-slaughterhouse_client.py" "$T/cl_dll/cl_dll.dsp" "$O" "$C" \
		"$TOOLS/release/scientist-slaughterhouse/sword.0701.cpp"; then
		echo "$0: client build failed; every command is in $O/build.log" >&2
		exit 1
	fi
	release_report client "$O/client.dll" \
		1e4f2d5c2a6d92ac84f8bdf1fb46abbffabcb78107909aa44c2a75f84baf1365 cl_dlls/client.dll
fi
exit $RC
