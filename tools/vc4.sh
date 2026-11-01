#!/bin/sh
# Run a Visual C++ 4.x tool under Wine, without a display.
#
#   vc4.sh CL /nologo /c /Od foo.cpp        (4.2, 10.20.6166)
#   vc4.sh -41 CL /nologo /c /Od foo.cpp    (4.1, 10.10.6038; -40 for 4.0, 10.00.5270)
#   vc4.sh LINK /nologo /dll ...
#
# The first argument names a program in $VC_TOOLCHAINS/vc4/{42,41,40}/BIN
# (case-insensitive, .EXE optional). Pass paths outside the current directory
# as Z:\...
#
# Environment: VC_TOOLCHAINS (required) and VC_WINEPREFIX, as for vc6.sh.
set -eu

: "${VC_TOOLCHAINS:?vc4.sh: set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)}"
tc=$(cd "$VC_TOOLCHAINS" && pwd)
win_tc="Z:$(printf '%s' "$tc" | sed 's|/|\\|g')"

ver=42
case "${1:-}" in -40|-41|-42) ver=${1#-}; shift ;; esac

tool=$(printf '%s' "${1:?usage: vc4.sh [-40|-41|-42] TOOL [args...]}" | tr '[:lower:]' '[:upper:]')
shift
case "$tool" in *.EXE) ;; *) tool="$tool.EXE" ;; esac

[ -d "$tc/vc4/$ver/BIN" ] || { echo "vc4.sh: no $tc/vc4/$ver/BIN (docs/setup.md)" >&2; exit 2; }

export WINEPREFIX="${VC_WINEPREFIX:-$tc/wineprefix}"
[ -f "$WINEPREFIX/system.reg" ] || { echo "vc4.sh: no Wine prefix at $WINEPREFIX (docs/setup.md)" >&2; exit 2; }
export WINEDEBUG=-all
# No windows: a crashing program exits with an error code instead of opening
# winedbg's crash dialog, which would wait for a click forever.
unset DISPLAY WAYLAND_DISPLAY
export WINEDLLOVERRIDES='mscoree=;mshtml=;winemenubuilder.exe=d;winedbg.exe=d'
export INCLUDE="$win_tc\\vc4\\$ver\\INCLUDE"
export LIB="$win_tc\\vc4\\$ver\\LIB"

exec wine "$tc/vc4/$ver/BIN/$tool" "$@"
