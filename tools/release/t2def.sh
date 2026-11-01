#!/bin/sh
# Build T2DEF 1.1's DLLs as the retail files, byte for byte.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/t2def.sh SRC OUT [server|client|both]
#
# SRC: a checkout of branch t2def. OUT: a build folder.
#
# Server (t2def.dll): Visual C++ 6.0 RTM with the Standard edition's back end
# (tree vc6std), with hl.dsp's own flags. The build reproduces the build
# environment, not just the code:
#  - the source tree at retail's PDB path, C:\hledit todesangst2\code (the
#    CodeView record names ...\dlls\Releasehl\hl.pdb): a symlink tree inside
#    drive_c of a copy of the Wine prefix in OUT/wineprefix;
#  - a full build into dlls\Releasehl, deleted first, as hl.dsp names it:
#    t2def.dll (its /out) with the PDB hl.pdb (its /pdb);
#  - CL and LINK on Wine's builtin msvcrt (VC6_CRT=wine). With the VC6 CD's
#    MSVCRT, LINK's qsort swaps TlsAlloc and CreateDirectoryA in the import
#    table; retail has the builtin's order (T2DEF's linker probably ran on a
#    later Windows msvcrt). Only Wine 11.0 was tested;
#  - the PDB's history. Retail's CodeView record has signature 0x3df56e9d
#    (the PDB was created 2002-12-10 04:33:33 UTC) and age 15, so hl.pdb was
#    written by 15 links. Replayed as one full build linked at the creation
#    time, 13 relinks at that time, and the final link at retail's link time,
#    2002-12-16 01:56:59 UTC, which also sets the COFF, export and debug
#    directory time stamps (LINK under libfaketime);
#  - the last build session before the final link (t2def_session.py):
#    sound.cpp, teamplay_gamerules.cpp and weapons.cpp recompiled in that
#    order with the .pch the full build left, VC6's own /YX deciding each
#    .pch. Retail's teamplay_gamerules.obj and weapons.obj are what VC6 makes
#    when those files find the [extdll util cbase] .pch, which a full build
#    in project order never leaves them. sound.cpp, whose list is [extdll
#    util cbase], rebuilds the .pch from that list (its own object is
#    unchanged); the other two reuse it. Another file with that list, or
#    which of links 2 to 15 the session came before, gives the same image.
#
# Client (client.dll): Visual C++ 6.0 Service Pack 4 with the Standard
# edition's back end (tree vc6sp4std), its LINK 6.00.8447 on SP4's MSVCRT.DLL,
# /Ob0 added to the project's flags. No debug directory, so only the link
# time matters: 2002-12-06 05:06:23 UTC. A full build into OUT/client in the
# toolchain's own Wine prefix.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"
release_init "$@"

if [ "$WHAT" != client ]; then
	release_need vc6std
	release_prefix
	T="$OUT/wineprefix/drive_c/hledit todesangst2/code"
	release_tree "$T"
	rm -rf "$T/dlls/Releasehl"
	: > "$OUT/server.log"
	build() {
		release_run "$OUT/server.log" env VC_WINEPREFIX="$OUT/wineprefix" VC6_CRT=wine \
			python3 "$TOOLS/build_dsp.py" "$T/dlls/hl.dsp" "hl - Win32 Release" "$T/dlls/Releasehl" \
			--name t2def --drive "C:=$OUT/wineprefix/drive_c" --compiler vc6std --link-time "$@"
	}
	echo "server: full build, link 1 of 15 at 2002-12-10 04:33:33 UTC (creates hl.pdb)"
	build '2002-12-10 04:33:33' --full
	release_summary "$OUT/server.log"
	echo "server: links 2 to 14 at 2002-12-10 04:33:33 UTC"
	n=2
	while [ $n -lt 15 ]; do
		build '2002-12-10 04:33:33'
		n=$((n + 1))
	done
	echo "server: last session: sound.cpp, teamplay_gamerules.cpp, weapons.cpp"
	release_run "$OUT/server.log" env VC_WINEPREFIX="$OUT/wineprefix" VC6_CRT=wine \
		python3 "$TOOLS/release/t2def_session.py" "$T/dlls/hl.dsp" "$T/dlls/Releasehl" \
		--drive "C:=$OUT/wineprefix/drive_c" sound.cpp teamplay_gamerules.cpp weapons.cpp
	echo "server: link 15 of 15 at 2002-12-16 01:56:59 UTC"
	build '2002-12-16 01:56:59'
	release_report server "$T/dlls/Releasehl/t2def.dll" \
		ba8fea632d5eca98b9e68078e57a3aa37e07acd872063746d4c597c2ea076135 dlls/t2def.dll
fi

if [ "$WHAT" != server ]; then
	release_need vc6sp4std
	O=$OUT/client
	rm -rf "$O"
	mkdir -p "$O"
	: > "$OUT/client.log"
	echo "client: full build, LINK at 2002-12-06 05:06:23 UTC"
	release_run "$OUT/client.log" \
		python3 "$TOOLS/build_dsp.py" "$SRC/cl_dll/cl_dll.dsp" "cl_dll - Win32 Release" "$O" \
		--name client --compiler vc6sp4std --cpp-add /Ob0 --link-time '2002-12-06 05:06:23' --full
	release_summary "$OUT/client.log"
	release_report client "$O/client.dll" \
		cbd18f9318fe5a626bb35318f70db6a340300d8c6874f8b64e7bdb53f5e91762 cl_dlls/client.dll
fi
exit $RC
