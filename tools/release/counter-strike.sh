#!/bin/sh
# Build a Counter-Strike release's DLLs, Beta 1.0 (1999) to 1.5 (2002), as
# the retail files, byte for byte.
#
#   VC_TOOLCHAINS=/path/to/toolchains tools/release/counter-strike.sh TAG SRC OUT [server|client|both]
#
# TAG: a release's tag on branch counter-strike, cstrike-beta1.0 ... cstrike-1.5.
# SRC: a checkout of that tag, inside a clone that has the tags (Betas 2.1,
# 3.0, 3.1 and 6.1 continue an earlier release's build, whose source is taken
# from the repository by its tag). OUT: a build folder.
#
# Compilers: Visual C++ 5.0 as on the CD (Betas 1.0 to 4.1; tree vc5/rtm, CL
# and LINK on the CD's own MSVCRT.DLL), Visual C++ 6.0 as on the CD (Beta 5.0
# to 7.1; vc6), with Service Pack 4 (1.0 to 1.1; vc6sp4), Service Pack 5 (the
# 1.3 server; vc6sp5), SP5 with the Processor Pack (the 1.3 client, 1.4, 1.5;
# vc6sp5pp). docs/counter-strike.md says what each build reproduces.
#
# Server (dlls/mp.dll): counter-strike_server.py, in a copy of the Wine prefix
# in OUT/wineprefix, with the source tree at the path retail's debug record
# names (D:\half-life\code2 and others), the build history the image shows
# and the PDB's links. Client (cl_dlls/client.dll): counter-strike_client.py,
# a full build into OUT/client. 1.4's and 1.5's clients ship in the engine's
# packed format: the built PE file is packed (tools/packed_dll.py) and that is
# compared; they are not identical (docs/counter-strike.md).
#
# With RETAIL=/dir (the release's cstrike folder, with dlls/mp.dll and
# cl_dlls/client.dll) each file is also compared byte by byte.
set -eu
TOOLS=$(cd "$(dirname "$0")/.." && pwd)
. "$TOOLS/release/common.sh"

release_usage() {
	echo "usage: VC_TOOLCHAINS=/path/to/toolchains $0 TAG SRC OUT [server|client|both]" >&2
	echo "  TAG     a release's tag: cstrike-beta1.0 ... cstrike-beta7.1, cstrike-1.0 ... cstrike-1.5" >&2
	echo "  SRC     a checkout of that tag (in a clone that has the tags)" >&2
	echo "  OUT     a build folder, created if missing" >&2
	echo "  RETAIL  (optional) the release's cstrike folder: also cmp with RETAIL/dlls/mp.dll and RETAIL/cl_dlls/client.dll" >&2
	exit 2
}

[ $# -ge 3 ] && [ $# -le 4 ] || release_usage
TAG=$1
shift

# TAG: server recipe, client recipe, retail mp.dll's SHA-256, retail client.dll's SHA-256, and for the
# packed clients the SHA-256 this tree's build gives (the commit message's "ours")
OURS=
case "$TAG" in
cstrike-beta1.0) S=b10 C=b10 SH=f4155737d5f190a385da49308a1ef38446f5dd50aaae2362eccff48addac3bf6 CH=07e181325c522cd612c1059d28c3c173c7200b1c9953f7d9bec418a4d6055564 ;;
cstrike-beta1.1) S=b11 C=b10 SH=577517c643e09c7f9426ff8cde659254df496a79bd18d075a340110bb0a5ecec CH=07e181325c522cd612c1059d28c3c173c7200b1c9953f7d9bec418a4d6055564 ;;
cstrike-beta2.0) S=b20 C=b20 SH=258e016a4b257b759bc42dc71769455508309eb86de188c1dcd6ad3cd4c6debe CH=791242cc4a109773c940962dcd7fbccb79955c8adffc36773d3eff0acfafaf73 ;;
cstrike-beta2.1) S=b21 C=b20 SH=57f9dd8b2cae2072a77e053eb174fd3842d9ef8253031cb4c55475a38f40d844 CH=791242cc4a109773c940962dcd7fbccb79955c8adffc36773d3eff0acfafaf73 ;;
cstrike-beta3.0) S=b30 C=b30 SH=a1c31191a6176be679ee5fa2e6b1e7feaf59f83e26117b346ed3f2263ce4382b CH=a3298a3a5a2a0369340d63d30beb6a271653f17310892e7bc70b1d40c5600c50 ;;
cstrike-beta3.1) S=b31 C=b30 SH=7dac5feda8c0137a7bcdb3855ab3e94ce3c5bcf84e3cb47f9319113b03f0b209 CH=a3298a3a5a2a0369340d63d30beb6a271653f17310892e7bc70b1d40c5600c50 ;;
cstrike-beta4.0) S=b40 C=b40 SH=95312d4de5ca88c387b072f97563911b84a8e0c2c046b850a097bfab8c4dcca6 CH=bd1b1c76bfd79a7614f36508518637bf78716e1414e8592b19bd7bbd44f3f3c7 ;;
cstrike-beta4.1) S=b41 C=b40 SH=f14aed6b873e9a1b31786782d5e33f3d57656f25d72f2a45a0a76bcddb4dc08f CH=bd1b1c76bfd79a7614f36508518637bf78716e1414e8592b19bd7bbd44f3f3c7 ;;
cstrike-beta5.0-leak) S=b50leak C=b50leak SH=2ba7b797f9e9fdad3a88dbbeab61125e664a8777153c27ef4251ad69e8fcc816 CH=62e8489388fa153da1fbdf0d348b33d749a50acb5cbcc4cbda7540e956aefa74 ;;
cstrike-beta5.0) S=b50 C=b50 SH=87ea6dade4035c65338fb8cd95cce5b489b53b3951b8c34323fcd214c50927d7 CH=b3ebbbe9290812cd01b44a19d7b989aba90e8f068051340a33f8dc564ed4c6f0 ;;
cstrike-beta5.2) S=b52 C=b52 SH=af119154607b38b6d0bae8a10efc702c4354d7088166895b47c73e3a75938b07 CH=79730770a318066771ee2299af9266bd99aebb153bda0fa586d0d9baf6cce123 ;;
cstrike-beta6.0) S=b60 C=b60 SH=23796074115c00f308170bdd68c8a40ad0d1ec7e769bf3d9549650d0c3d09f7c CH=bc7412fcbbb20f8f4c1a37a8e5c83b36926d54b86450b0bf52319fe83a7fd80b ;;
cstrike-beta6.1) S=b61 C=b60 SH=d7f05c09d3d9bd1ee44ed2c438ba6830819a57ec532ee20c5b1e860f8debac96 CH=bc7412fcbbb20f8f4c1a37a8e5c83b36926d54b86450b0bf52319fe83a7fd80b ;;
cstrike-beta6.5) S=b65 C=b65 SH=abc1fb0da96654ea3e8a05cedf0ad4ca1d553a44b4b19691ec822c815e2c89cf CH=db55dcbd4316f73412fa6f73828476985f490b1872ddfebfa5f57497a8e1f95a ;;
# Beta 6.5d ships Beta 6.5's two DLLs (tag cstrike-beta6.5 builds them); its commit is the source of its
# Linux library, the state Beta 6.6's client.dll was built from: the client built here is 6.6's
cstrike-beta6.5d) S= C=b65d SH= CH=1db237bb08759f1a7323cf1d1c3a24d912a0893e50638a5a684cd40994bd757d ;;
cstrike-beta6.6) S=b66 C=b66 SH=84fb002a67a9f8dd7e48ec60494c5078d598d2151ddc34409234ee98d87d90da CH=1db237bb08759f1a7323cf1d1c3a24d912a0893e50638a5a684cd40994bd757d ;;
cstrike-beta7.0) S=b70 C=b70 SH=210d739c26423f48693a800d536a9a2e838ebd2c24afa1bbebf97fc8870b56e3 CH=3f16ba1141c172802ed6f32b08bc3de4d1576cb14a2d3d7a3d79a0e20968e496 ;;
cstrike-beta7.1) S=b71 C=b71 SH=41f728a70c0778dff227d88635e10b2e4710fb5136e8545bb199c869ba7ce9b3 CH=62e647eda8ce22b574a52e5f9040a9acc1c449602cc05b125e1f62cafe13178d ;;
cstrike-1.0) S=r10 C=r10 SH=21073277e6e94d947075b6c958a61df2b840d3cd9db394b3349c887457332fea CH=0dab270405a0cd9bf68ebfd2137f013dddf2f7128bd496530af8d82ad76501a2 ;;
cstrike-1.0b) S=r10b C=r10 SH=0fc676a35a0c84a4a5cf50c392d2ca18768397ecb4ebeb939e9bcf73e06dcbdc CH=0dab270405a0cd9bf68ebfd2137f013dddf2f7128bd496530af8d82ad76501a2 ;;
cstrike-1.1|cstrike-1.1b|cstrike-1.1c|cstrike-1.2) S=r11 C=r11 SH=bea6118681f7a9d05b32fa71a2a6f828faee1151eca887f53d96544c96774881 CH=47e8c2ed15a15e199ae7fadb58c4637fb7893fdb6c3a347ddd8fd2631dbc078b ;;
cstrike-1.3) S=r13 C=r13 SH=598c9473cc16386a99bc01bba8f4bb31ffb10473f111434b2926fa3be81c30e1 CH=71fdbe140df3598a74738537b924141f4fb4b0b828737bd99a7223d9e4549460 ;;
cstrike-1.4) S=r14 C=r14 SH=f11016c098bf874f3b8ea9936a0c6760097d177412895de11faa0efbddeaac98 CH=4b8af3606cb440fc40f066abe96b5959fe2036cae6809b7ffdbeeb6c50d3471d
	OURS=c2c6e9b895eba6d17ea9d206e36786893524fe0768e4299be7f6bcaf1f674d56 ;;
cstrike-1.5) S=r15 C=r15 SH=f10cb95979afd46c677bbf21d40be7ec5a9fc179306a03b3b1ce76744ca4d2e8 CH=61228d9c9664c0c67af3fb7717d7fa05b0fee88728a97030e89b45b12b5167ac
	OURS=e8b1191cfa8dc959f900d37fefe8de34c0dd65b84309becf63cd3937286d0009 ;;
*) echo "$0: unknown tag $TAG" >&2; release_usage ;;
esac

RELEASE_SERVER_DSP=dlls/mp.dsp
release_init "$@"
command -v git >/dev/null 2>&1 || release_die "git not found"
case "$S $C" in
*b10* | *b11* | *b20* | *b21* | *b30* | *b31* | *b40* | *b41*)
	[ -d "$VC_TOOLCHAINS/vc5/rtm/nativecrt" ] || release_die "no $VC_TOOLCHAINS/vc5/rtm/nativecrt (docs/setup.md)" ;;
esac
case "$S $C" in *r10* | *r11*) release_need vc6sp4 ;; esac
case "$S $C" in b5* | b6* | b7* | *" b5"* | *" b6"* | *" b7"*) release_need vc6 ;; esac
case "$S" in r13) release_need vc6sp5 ;; r14 | r15) release_need vc6sp5pp ;; esac
case "$C" in r13) release_need vc6sp5pp ;; r14 | r15) release_need vc6sp5pp vc6sp5 ;; esac

# SRC should be the tag's commit; say so when it is not (local changes are built as they are)
want=$(git -C "$SRC" rev-parse --verify --quiet "refs/tags/$TAG^{commit}" 2>/dev/null || true)
have=$(git -C "$SRC" rev-parse --verify --quiet HEAD 2>/dev/null || true)
if [ -z "$want" ]; then
	echo "note: $SRC's repository has no tag $TAG"
elif [ "$want" != "$have" ]; then
	echo "note: $SRC is at ${have:-no commit}, not at $TAG ($want)"
fi

if [ "$WHAT" != client ]; then
	if [ -z "$S" ]; then
		echo "server: none from this commit: Beta 6.5d's package has Beta 6.5's mp.dll (tag cstrike-beta6.5 builds it)"
	else
		release_prefix
		: > "$OUT/server.log"
		echo "server: $TAG's mp.dll; the output is in $OUT/server.log"
		if ! env VC_WINEPREFIX="$OUT/wineprefix" \
			python3 "$TOOLS/release/counter-strike_server.py" "$S" "$SRC" "$OUT/server" >> "$OUT/server.log" 2>&1; then
			tail -n 5 "$OUT/server.log" >&2
			echo "$0: server build failed; every command is in $OUT/server/build.log" >&2
			exit 1
		fi
		grep -E '^(link 1 at|links [0-9]|[0-9]+ links in|cstrike-[^ ]*:|cstrike-[^ ]*'"'"'s build done)' "$OUT/server.log" | sed 's/^/  /'
		release_report server "$OUT/server/mp.dll" "$SH" dlls/mp.dll
	fi
fi

if [ "$WHAT" != server ]; then
	: > "$OUT/client.log"
	if [ "$C" = b65d ]; then
		echo "client: Beta 6.6's client.dll (linked 2000-06-19), which was built from this state;" \
			"with RETAIL, give Beta 6.6's folder"
	fi
	echo "client: $TAG's client.dll; the output is in $OUT/client.log"
	release_run "$OUT/client.log" python3 "$TOOLS/release/counter-strike_client.py" "$C" "$SRC" "$OUT/client"
	release_summary "$OUT/client.log"
	grep -E '^(compiled .* again|client: packed)' "$OUT/client.log" | sed 's/^/  /' || true
	if [ -n "$OURS" ]; then
		echo "  PE file built: $(sha256sum "$OUT/client/client.dll" | cut -d' ' -f1)  $OUT/client/client.dll"
		release_report client "$OUT/client/client.packed.dll" "$CH" cl_dlls/client.dll
		_have=$(sha256sum "$OUT/client/client.packed.dll" | cut -d' ' -f1)
		echo "  (the packed form runs a XOR over the whole file, so one changed byte changes every byte after it)"
		if [ "$_have" = "$OURS" ]; then
			echo "  not identical, as known: this is the SHA-256 the release's commit message records for this tree"
		else
			echo "  not the SHA-256 the release's commit message records for this tree: $OURS"
		fi
	else
		release_report client "$OUT/client/client.dll" "$CH" cl_dlls/client.dll
	fi
fi
exit $RC
