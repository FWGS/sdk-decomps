# common.sh: the part the release scripts share. Sourced by
# tools/release/<mod>.sh after it has set TOOLS (the tools folder).
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/<mod>.sh SRC OUT [server|client|both]
#
# SRC is a checkout of the mod's branch, OUT a build folder (created if
# missing). With RETAIL=/dir set, each DLL is also compared byte by byte with
# the shipped file in /dir/dlls or /dir/cl_dlls (the mod's folder in the game).

release_usage() {
	echo "usage: VC_TOOLCHAINS=/path/to/toolchains $0 SRC OUT [server|client|both]" >&2
	echo "  SRC     a checkout of the mod's branch" >&2
	echo "  OUT     a build folder, created if missing" >&2
	echo "  RETAIL  (optional) the mod's game folder: also cmp with RETAIL/dlls/... and RETAIL/cl_dlls/client.dll" >&2
	exit 2
}

release_die() {
	echo "$0: $*" >&2
	exit 2
}

# release_init "$@": check the arguments and the environment; sets SRC, OUT,
# WHAT and VC_TOOLCHAINS (absolute), RC=0.
release_init() {
	[ $# -ge 2 ] && [ $# -le 3 ] || release_usage
	WHAT=${3:-both}
	case "$WHAT" in server|client|both) ;; *) release_usage ;; esac
	[ -n "${VC_TOOLCHAINS:-}" ] || release_die "set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)"
	[ -d "$VC_TOOLCHAINS" ] || release_die "VC_TOOLCHAINS: no folder $VC_TOOLCHAINS"
	VC_TOOLCHAINS=$(cd "$VC_TOOLCHAINS" && pwd)
	export VC_TOOLCHAINS
	[ -f "$VC_TOOLCHAINS/wineprefix/system.reg" ] || release_die "no Wine prefix at $VC_TOOLCHAINS/wineprefix (docs/setup.md)"
	[ -d "$1" ] || release_die "SRC: no folder $1"
	SRC=$(cd "$1" && pwd)
	# the server's project: dlls/hl.dsp, or the one the mod's script names (RELEASE_SERVER_DSP)
	[ -f "$SRC/${RELEASE_SERVER_DSP:-dlls/hl.dsp}" ] && [ -f "$SRC/cl_dll/cl_dll.dsp" ] ||
		release_die "$SRC has no ${RELEASE_SERVER_DSP:-dlls/hl.dsp} and cl_dll/cl_dll.dsp: not a checkout of a mod branch"
	mkdir -p "$2"
	OUT=$(cd "$2" && pwd)
	for c in wine wineserver faketime python3 sha256sum cmp; do
		command -v "$c" >/dev/null 2>&1 || release_die "$c not found (docs/setup.md)"
	done
	RC=0
	trap release_stop_prefix EXIT
}

# release_need TREE...: fail early when a compiler tree is missing.
release_need() {
	for t in "$@"; do
		[ -d "$VC_TOOLCHAINS/$t/VC98/BIN" ] || release_die "no $VC_TOOLCHAINS/$t/VC98/BIN (docs/setup.md)"
	done
}

# release_prefix: the per-build copy of the Wine prefix, $OUT/wineprefix
# (made once, then reused).
release_prefix() {
	if [ ! -f "$OUT/wineprefix/system.reg" ]; then
		rm -rf "$OUT/wineprefix"
		cp -a --reflink=auto "$VC_TOOLCHAINS/wineprefix" "$OUT/wineprefix"
	fi
}

# release_stop_prefix: stop the wineserver of the per-build prefix copy, if
# any. build_dsp.py keeps one running across its compiles; with Wine's
# background services it would otherwise stay after the build. Runs when the
# script exits (release_init sets the trap).
release_stop_prefix() {
	if [ -n "${OUT:-}" ] && [ -f "$OUT/wineprefix/system.reg" ]; then
		WINEPREFIX="$OUT/wineprefix" timeout 60 wineserver -k >/dev/null 2>&1 || true
	fi
}

# release_tree DEST: lay out DEST as the mod's source tree was laid out on
# the original machine, made of symlinks into SRC, so that the build writes
# its output under OUT and never into the checkout. Every top-level entry of
# SRC except .git is linked; the project folders dlls and cl_dll are real
# folders of per-file symlinks, so each gets its own output folder.
release_tree() {
	rm -rf "$1"
	mkdir -p "$1"
	for e in "$SRC"/* "$SRC"/.[!.]*; do
		[ -e "$e" ] || continue
		n=$(basename "$e")
		case "$n" in
		.git) ;;
		dlls|cl_dll)
			mkdir -p "$1/$n"
			for f in "$e"/* "$e"/.[!.]*; do
				[ -e "$f" ] && ln -s "$f" "$1/$n/$(basename "$f")"
			done ;;
		*) ln -s "$e" "$1/$n" ;;
		esac
	done
}

# release_run LOG COMMAND...: run a build step, its output appended to LOG;
# on failure show the end of LOG and stop.
release_run() {
	_log=$1
	shift
	if ! "$@" >> "$_log" 2>&1; then
		tail -n 5 "$_log" >&2
		echo "$0: build step failed; the full output is in $_log" >&2
		exit 1
	fi
}

# release_summary LOG: the last line build_dsp.py printed into LOG.
release_summary() {
	grep '^compiled ' "$1" | tail -n 1 | sed 's/^/  /'
}

# release_report LABEL FILE RETAIL_SHA256 RETAIL_NAME: print the built file,
# its SHA-256, retail's, and whether they are equal; with RETAIL set, cmp
# with $RETAIL/RETAIL_NAME. Sets RC=1 on any difference.
release_report() {
	if [ ! -f "$2" ]; then
		echo "$1: $2 was not built"
		RC=1
		return
	fi
	_have=$(sha256sum "$2" | cut -d' ' -f1)
	echo "$1: $2"
	echo "  SHA-256 built:  $_have"
	echo "  SHA-256 retail: $3"
	if [ "$_have" = "$3" ]; then
		echo "  identical to retail"
	else
		echo "  differs from retail"
		RC=1
	fi
	if [ -n "${RETAIL:-}" ]; then
		if [ ! -f "$RETAIL/$4" ]; then
			echo "  cmp: no $RETAIL/$4"
			RC=1
		elif cmp -s "$RETAIL/$4" "$2"; then
			echo "  cmp $RETAIL/$4: no difference"
		else
			_n=$(cmp -l "$RETAIL/$4" "$2" 2>/dev/null | wc -l)
			_sz=$(wc -c < "$RETAIL/$4")
			echo "  cmp $RETAIL/$4: $_n of $_sz bytes differ"
			RC=1
		fi
	fi
}
