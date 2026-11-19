#!/bin/sh
# Run a Visual C++ 6.0 tool under Wine, without a display.
#
#   vc6.sh CL /nologo /c /O2 foo.cpp        (RTM, Professional, 12.00.8168)
#   vc6.sh -std CL ...                      (RTM, Standard edition back end, 11.99.8168)
#   vc6.sh -sp4 CL ...                      (Service Pack 4, Professional, 12.00.8799)
#   vc6.sh -sp4std CL ...                   (Service Pack 4, Standard edition back end)
#   vc6.sh -sp5 CL ...                      (Service Pack 5, Professional, 12.00.8966)
#   vc6.sh -sp5pp CL ...                    (Service Pack 5 with the Processor Pack, 13.00.9044)
#   vc6.sh LINK /nologo /dll ...
#
# The first argument names a program in the tree's VC98/BIN (case-insensitive,
# .EXE optional). Pass paths outside the current directory as Z:\...
#
# Environment:
#   VC_TOOLCHAINS  the toolchain folder (required; see docs/setup.md). It holds
#                  the trees vc6, vc6std, vc6sp4, vc6sp4std, vc6sp5, vc6sp5pp
#                  and the Wine prefix wineprefix.
#   VC_WINEPREFIX  a Wine prefix to use instead of $VC_TOOLCHAINS/wineprefix,
#                  e.g. a per-build copy with an extra drive letter.
#   VC6_CRT        native (default): run the copy of the programs in
#                  <tree>/nativecrt, which loads the MSVCRT.DLL shipped with that
#                  release instead of Wine's builtin msvcrt (docs/compilers.md);
#                  wine: run the unpatched VC98/BIN on Wine's builtin msvcrt.
#   VC6_ROOT       (opt-in, tree vc6 only) a DOS path inside the prefix, such
#                  as 'C:\Microsoft Visual Studio', under which the caller has
#                  laid out VC98\BIN, VC98\INCLUDE and VC98\LIB (links to
#                  vc6/nativecrt, vc6/VC98/INCLUDE, vc6/VC98/LIB). The program
#                  then runs as VC6_ROOT\VC98\BIN\TOOL.EXE, with INCLUDE and
#                  LIB under VC6_ROOT, and no path of the host reaches the
#                  compiler. The front end keeps the full path of every header
#                  it opens in its memory, and their lengths move the addresses
#                  that decide the order of the RTTI records (Scientist
#                  Slaughterhouse's client). Unset: the tree's own paths, Z:\...
set -eu

: "${VC_TOOLCHAINS:?vc6.sh: set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)}"
tc=$(cd "$VC_TOOLCHAINS" && pwd)
win_tc="Z:$(printf '%s' "$tc" | sed 's|/|\\|g')"

tree=vc6
case "${1:-}" in -sp4) tree=vc6sp4; shift ;; -sp4std) tree=vc6sp4std; shift ;; -std) tree=vc6std; shift ;;
-sp5) tree=vc6sp5; shift ;; -sp5pp) tree=vc6sp5pp; shift ;; -rtm) shift ;; esac

tool=$(printf '%s' "${1:?usage: vc6.sh [-rtm|-std|-sp4|-sp4std|-sp5|-sp5pp] TOOL [args...]}" | tr '[:lower:]' '[:upper:]')
shift
case "$tool" in *.EXE) ;; *) tool="$tool.EXE" ;; esac

[ -d "$tc/$tree/VC98/BIN" ] || { echo "vc6.sh: no $tc/$tree/VC98/BIN (docs/setup.md)" >&2; exit 2; }

export WINEPREFIX="${VC_WINEPREFIX:-$tc/wineprefix}"
[ -f "$WINEPREFIX/system.reg" ] || { echo "vc6.sh: no Wine prefix at $WINEPREFIX (docs/setup.md)" >&2; exit 2; }
export WINEDEBUG=-all
# No windows: a crashing program exits with an error code instead of opening
# winedbg's crash dialog, which would wait for a click forever.
unset DISPLAY WAYLAND_DISPLAY
export WINEDLLOVERRIDES='mscoree=;mshtml=;winemenubuilder.exe=d;winedbg.exe=d'
export INCLUDE="$win_tc\\$tree\\VC98\\INCLUDE"
export LIB="$win_tc\\$tree\\VC98\\LIB"

bin="$tc/$tree/VC98/BIN"
case "${VC6_CRT:-native}" in
native) bin="$tc/$tree/nativecrt"
	[ -d "$bin" ] || { echo "vc6.sh: no $bin; run make_vc6_nativecrt.sh (docs/setup.md)" >&2; exit 2; } ;;
wine) ;;
*) echo "vc6.sh: VC6_CRT must be native or wine" >&2; exit 2 ;;
esac

if [ -n "${VC6_ROOT:-}" ]; then
	[ "$tree" = vc6 ] || { echo "vc6.sh: VC6_ROOT is for the tree vc6 only" >&2; exit 2; }
	[ "${VC6_CRT:-native}" = native ] || { echo "vc6.sh: VC6_ROOT runs what its VC98\\BIN holds; VC6_CRT=wine does not apply" >&2; exit 2; }
	case "$VC6_ROOT" in [A-Za-z]:\\*) ;; *) echo "vc6.sh: VC6_ROOT must be a DOS path (C:\\...)" >&2; exit 2 ;; esac
	root=${VC6_ROOT%\\}
	export INCLUDE="$root\\VC98\\INCLUDE"
	export LIB="$root\\VC98\\LIB"
	exec wine "$root\\VC98\\BIN\\$tool" "$@"
fi
exec wine "$bin/$tool" "$@"
