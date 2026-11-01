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
# Environment: VC_TOOLCHAINS (required) and VC_WINEPREFIX, as for vc6.sh.
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

exec wine "$tc/vc5/$sp/BIN/$tool" "$@"
