#!/bin/sh
# Check the toolchain folder against the trees the release builds were made
# with, and the Wine prefix's settings (docs/setup.md).
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/check_toolchains.sh
#
# For each compiler tree it prints a digest of every file in it (the SHA-256
# of the sorted list of the files' SHA-256 sums) and compares it with the
# digest of the tree that built the retail-identical DLLs. Exit code 0 when
# every tree the six mods need matches and the prefix is set up.
set -eu
: "${VC_TOOLCHAINS:?set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)}"
tc=$(cd "$VC_TOOLCHAINS" && pwd)
rc=0

digest() {
	(cd "$1" && find . -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | sha256sum | cut -c1-64)
}

# tree, expected digest (VC98 or BIN, INCLUDE, LIB, and nativecrt), needed by the six mods (yes/no):
# vc6sp4, vc6sp5, vc6sp5pp and vc5/rtm are Counter-Strike's
while read -r tree want need; do
	if [ ! -d "$tc/$tree" ]; then
		if [ "$need" = yes ]; then echo "$tree: missing (needed)"; rc=1; else echo "$tree: not present (optional)"; fi
		continue
	fi
	if [ ! -d "$tc/$tree/nativecrt" ]; then
		case "$tree" in vc5/*) echo "$tree: no nativecrt (run make_vc5_nativecrt.sh)" ;;
		*) echo "$tree: no nativecrt (run make_vc6_nativecrt.sh)" ;; esac
		rc=1
		continue
	fi
	have=$(digest "$tc/$tree")
	if [ "$have" = "$want" ]; then
		echo "$tree: ok"
	else
		echo "$tree: differs (digest $have, expected $want)"
		rc=1
	fi
done <<'EOF'
vc6 0a5cba66bdf0ed7bdf98a797c90f7954cd5c089aa9f668cead7e52a06ebad391 yes
vc6std 807d9e2fe7f1b179b4670d8298b3a326b8e97c16ccd3422e57d50bb472b134d2 yes
vc6sp4std 359ffb6276ad76d1fa0bd32ecba3f876a3d12a626d03ea953cc91b5be3eb587d yes
vc6sp4 f2461043c1ec4f62fd775bd81911ca8a6e69d240ed834b79002f9869794d00ff yes
vc6sp5 a9c0ecd8b48d124d592c58e979fee23a03033d1f7abedc2f7a07d0a6766a30ac yes
vc6sp5pp 6a9e4308740d03c55a4b751c6f184a892cc2d3e78d16a532d35dfb9c15b627b8 yes
vc5/rtm a129b7cd4bba11a6c3fced02f684c00f5267fc5579c7599adcfb5956746c9699 yes
EOF

p=$tc/wineprefix
if [ ! -f "$p/system.reg" ] || [ ! -f "$p/user.reg" ]; then
	echo "wineprefix: missing"
	rc=1
else
	if grep -q '^#arch=win64' "$p/system.reg"; then echo "wineprefix: win64"; else echo "wineprefix: not win64"; rc=1; fi
	if awk '/^\[Software\\\\Wine\\\\DllOverrides\]/{s=1;next} /^\[/{s=0} s&&/^"winedbg.exe"=""$/{f=1} END{exit !f}' "$p/user.reg"; then
		echo "wineprefix: winedbg.exe disabled"
	else
		echo "wineprefix: winedbg.exe not disabled (HKCU\\Software\\Wine\\DllOverrides winedbg.exe=\"\")"
		rc=1
	fi
	if awk '/^\[Software\\\\Wine\\\\WineDbg\]/{s=1;next} /^\[/{s=0} s&&/^"ShowCrashDialog"=dword:00000000$/{f=1} END{exit !f}' "$p/user.reg"; then
		echo "wineprefix: crash dialog off"
	else
		echo "wineprefix: crash dialog not off (HKCU\\Software\\Wine\\WineDbg ShowCrashDialog=0)"
		rc=1
	fi
fi

for c in wine wineserver faketime python3 7z cabextract git; do
	command -v "$c" >/dev/null 2>&1 || { echo "$c: not found"; rc=1; }
done
command -v wine >/dev/null 2>&1 && echo "wine: $(WINEPREFIX="$p" wine --version 2>/dev/null) (tested: wine-11.0)"
command -v faketime >/dev/null 2>&1 && echo "faketime: $(faketime --version 2>&1 | sed -n 's/.*Version //p' | head -n 1) (tested: 0.9.12)"
command -v python3 >/dev/null 2>&1 && echo "python3: $(python3 -c 'import platform; print(platform.python_version())') (tested: 3.14)"
exit $rc
