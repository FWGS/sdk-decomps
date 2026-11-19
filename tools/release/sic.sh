#!/bin/sh
# Build Survive in Catacombs' DLLs (GAME 1, May 2004) or Survive in
# Catacombs 2's (GAME 2, December 2004) as the retail files, byte for byte,
# from one tree: branch sic, whose projects have a configuration per game.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/sic.sh GAME SRC OUT [server|client|both]
#
# GAME: 1 or 2. SRC: a checkout of branch sic (or of its historical trees,
# ec3024d for GAME 1 and 21fc2ea for GAME 2, which build the same files).
# OUT: a build folder.
# Compiler: Visual C++ 6.0 Professional as on the CD (tree vc6). LINK runs on
# the CD's MSVCRT.DLL for SiC 1 (vc6.sh's default) and on Wine's builtin
# msvcrt for SiC 2 (VC6_CRT=wine), whose import order and kept copies of
# identical functions only that runtime's qsort gives.
#
# Both DLLs are built in one source tree at retail's path, made of symlinks
# into SRC inside a copy of the Wine prefix in OUT/wineprefix, with the
# server folder DLLS in upper case as the server's PDB path has it:
#  - SiC 1: F:\SicEngineCodes (the PDB is
#    F:\SicEngineCodes\DLLS\Releasehl\spirit.pdb); F: is OUT/root;
#  - SiC 2: C:\bloody\bloodyEnginecodes (...\DLLS\Releasehl\bloodbath.pdb),
#    in the prefix copy's drive_c.
# The configuration is the game's: "hl - Win32 Release SIC1" and "cl_dll -
# Win32 Release SIC1" for SiC 1 (/D "SIC1" selects its code), "hl - Win32
# Release" and "cl_dll - Win32 Release" for SiC 2 (and for SiC 1 in a tree
# without the SIC1 configurations). Every compile keeps the projects'
# browse information (/Fr, /FR), which decides code.
# Server (sic.dll, built as spirit.dll; bloodbath.dll): a full build into
# DLLS\Releasehl, the last build session (weapons.h edited, its dependents
# recompiled) and the PDB's links, 6 or 8, the first at the PDB's creation
# time (sic_server.py has the details).
# Client (client.dll). No debug directory, so only the link time matters,
# 2004-05-19 19:04:12 UTC (SiC 1) or 2004-12-17 10:31:35 UTC (SiC 2): a full
# build into cl_dll\Release.
#
# With RETAIL=/dir (the mod's folder: Catacombs or Bloodbath, with Dlls/ and
# Cl_dlls/ as in the mod's download, or dlls/ and cl_dlls/) each file is also
# compared byte by byte.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"

release_usage() {
	echo "usage: VC_TOOLCHAINS=/path/to/toolchains $0 GAME SRC OUT [server|client|both]" >&2
	echo "  GAME    1 (Survive in Catacombs) or 2 (Survive in Catacombs 2)" >&2
	echo "  SRC     a checkout of branch sic" >&2
	echo "  OUT     a build folder, created if missing" >&2
	echo "  RETAIL  (optional) the mod's folder, Catacombs or Bloodbath: also cmp with its Dlls/ and Cl_dlls/ files" >&2
	exit 2
}

[ $# -ge 3 ] && [ $# -le 4 ] || release_usage
GAME=$1
shift
case "$GAME" in
1)	SERVER=sic.dll NAME=spirit CTIME='2004-05-19 19:04:12' CRT=native
	SH=eab217809f7c656acb7678b2035a4dc7f82a08db276febaceef72dfa55509359
	CH=0cb9fd2beed7c6ed327545a280826ecaaa5269ae3539c9a0602522cca3e6527a ;;
2)	SERVER=bloodbath.dll NAME=bloodbath CTIME='2004-12-17 10:31:35' CRT=wine
	SH=673f7e4ef7c75fdf551c87470f6e4f8a5a66f16fe98bfb44ca6436d14c88ba57
	CH=76cc187b538b7fa0ae6bea5842e1484369fb64c6d7184851fb77de96cedbe21a ;;
*)	release_usage ;;
esac
release_init "$@"
release_need vc6

# config DSP NAME...: the first configuration of NAME... that DSP declares
config() {
	_dsp=$1
	shift
	for _c in "$@"; do
		if grep -q "^# Name \"$_c\"" "$_dsp"; then
			echo "$_c"
			return
		fi
	done
	release_die "$_dsp has no configuration $*"
}

# retail_name NAME...: the first of these paths that RETAIL has (the mod's
# download spells the folders Dlls and Cl_dlls), else the first
retail_name() {
	for _n in "$@"; do
		if [ -n "${RETAIL:-}" ] && [ -f "$RETAIL/$_n" ]; then
			echo "$_n"
			return
		fi
	done
	echo "$1"
}

release_prefix
if [ "$GAME" = 1 ]; then
	mkdir -p "$OUT/root"
	ln -sfn "$OUT/root" "$OUT/wineprefix/dosdevices/f:"
	T=$OUT/root/SicEngineCodes
	DRIVE="F:=$OUT/root"
	SCONFIG=$(config "$SRC/dlls/hl.dsp" "hl - Win32 Release SIC1" "hl - Win32 Release")
	CCONFIG=$(config "$SRC/cl_dll/cl_dll.dsp" "cl_dll - Win32 Release SIC1" "cl_dll - Win32 Release")
else
	T=$OUT/wineprefix/drive_c/bloody/bloodyEnginecodes
	DRIVE="C:=$OUT/wineprefix/drive_c"
	SCONFIG=$(config "$SRC/dlls/hl.dsp" "hl - Win32 Release")
	CCONFIG=$(config "$SRC/cl_dll/cl_dll.dsp" "cl_dll - Win32 Release")
fi
release_tree "$T"
mv "$T/dlls" "$T/DLLS"

if [ "$WHAT" != client ]; then
	O=$T/DLLS/Releasehl
	if ! env VC_WINEPREFIX="$OUT/wineprefix" \
		python3 "$TOOLS/release/sic_server.py" "$GAME" "$T/DLLS/hl.dsp" "$SCONFIG" "$O" "$DRIVE"; then
		echo "$0: server build failed; every command is in $O/build.log and session.log" >&2
		exit 1
	fi
	release_report server "$O/$NAME.dll" "$SH" "$(retail_name "Dlls/$SERVER" "dlls/$SERVER")"
fi

if [ "$WHAT" != server ]; then
	O=$T/cl_dll/Release
	: > "$OUT/client.log"
	echo "client: full build, \"$CCONFIG\", with browse information, LINK at $CTIME UTC on" \
		"$([ "$CRT" = native ] && echo "the VC6 CD's MSVCRT.DLL" || echo "Wine's builtin msvcrt")"
	release_run "$OUT/client.log" env VC_WINEPREFIX="$OUT/wineprefix" VC6_CRT="$CRT" \
		python3 "$TOOLS/build_dsp.py" "$T/cl_dll/cl_dll.dsp" "$CCONFIG" "$O" \
		--name client --drive "$DRIVE" --link-time "$CTIME" --full --browse-info
	release_summary "$OUT/client.log"
	release_report client "$O/client.dll" "$CH" "$(retail_name Cl_dlls/client.dll cl_dlls/client.dll)"
fi
exit $RC
