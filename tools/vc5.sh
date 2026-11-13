#!/bin/sh
# Run a Visual C++ 5.0 Professional tool under Wine, without a display.
#
#   vc5.sh CL /nologo /c /Od foo.cpp        (Service Pack 3, 11.00.7303)
#   vc5.sh -rtm CL /nologo /c /Od foo.cpp   (as on the CD, 11.00.7022)
#   vc5.sh LINK /nologo /dll ...
#
# The first argument names a program in $VC_TOOLCHAINS/vc5/{sp3,rtm}/BIN
# (case-insensitive, .EXE optional). Pass paths outside the current directory
# as Z:\...
#
# Environment: VC_TOOLCHAINS (required) and VC_WINEPREFIX, as for vc6.sh, and
#   VC5_CRT  wine (default): run the programs in BIN on Wine's builtin msvcrt;
#            native: run the copy in vc5/{sp3,rtm}/nativecrt, which loads the
#            MSVCRT.DLL shipped with that release (make_vc5_nativecrt.sh).
#            A link that has to give a retail file needs native: LINK's
#            output depends on the runtime's qsort (docs/compilers.md). The
#            default is the other way round from vc6.sh's because the
#            compiler alone does not need it.
set -eu

: "${VC_TOOLCHAINS:?vc5.sh: set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)}"
tc=$(cd "$VC_TOOLCHAINS" && pwd)
win_tc="Z:$(printf '%s' "$tc" | sed 's|/|\\|g')"

sp=sp3
case "${1:-}" in -rtm) sp=rtm; shift ;; -sp3) shift ;; esac

tool=$(printf '%s' "${1:?usage: vc5.sh [-rtm|-sp3] TOOL [args...]}" | tr '[:lower:]' '[:upper:]')
shift
case "$tool" in *.EXE) ;; *) tool="$tool.EXE" ;; esac

[ -d "$tc/vc5/$sp/BIN" ] || { echo "vc5.sh: no $tc/vc5/$sp/BIN (docs/setup.md)" >&2; exit 2; }

export WINEPREFIX="${VC_WINEPREFIX:-$tc/wineprefix}"
[ -f "$WINEPREFIX/system.reg" ] || { echo "vc5.sh: no Wine prefix at $WINEPREFIX (docs/setup.md)" >&2; exit 2; }
export WINEDEBUG=-all
# No windows: a crashing program exits with an error code instead of opening
# winedbg's crash dialog, which would wait for a click forever.
unset DISPLAY WAYLAND_DISPLAY
export WINEDLLOVERRIDES='mscoree=;mshtml=;winemenubuilder.exe=d;winedbg.exe=d'
export INCLUDE="$win_tc\\vc5\\$sp\\INCLUDE"
export LIB="$win_tc\\vc5\\$sp\\LIB"

bin="$tc/vc5/$sp/BIN"
case "${VC5_CRT:-wine}" in
wine) ;;
native) bin="$tc/vc5/$sp/nativecrt"
	[ -d "$bin" ] || { echo "vc5.sh: no $bin; run make_vc5_nativecrt.sh (docs/setup.md)" >&2; exit 2; } ;;
*) echo "vc5.sh: VC5_CRT must be native or wine" >&2; exit 2 ;;
esac

exec wine "$bin/$tool" "$@"
