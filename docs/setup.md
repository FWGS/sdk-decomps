# Setting up the toolchain

The release scripts in `tools/release/` rebuild each mod's DLLs with the
compilers that made them, Visual C++ 6.0 in three variants, run under Wine
on Linux. This page builds the folder those scripts need, from the original
Microsoft discs, and the Wine prefix to run the compilers in. It ends with a
first build.

Everything here was tested on Linux x86-64 with Wine 11.0, libfaketime
0.9.12 and Python 3.14 (see [What was tested](#what-was-tested)).
[compilers.md](compilers.md) says which compiler built which DLL and how
that is known.

## What you need

- Linux on x86-64.
- Wine. Only Wine 11.0 was tested (a WoW64 build: 32-bit Windows programs
  run in a 64-bit Wine, with no 32-bit Linux libraries).
- libfaketime: the `faketime` program and its library. Tested: 0.9.12. It
  sets the clock that the linker reads, so the time stamps in the image are
  retail's.
- Python 3. Tested: 3.14. The tools use only the standard library.
- 7-Zip (`7z`), to unpack the downloads and read the disc images.
- GNU coreutils (`cp`, `sha256sum`, `timeout`), `cmp`, `git`, and a POSIX
  shell for the scripts.
- Disk space: about 1 GB for the downloads, 1 GB of temporary space while
  unpacking a disc, 330 MB for the compilers, 340 MB for the Wine prefix,
  and up to 400 MB for each build folder (a server build copies the prefix).

The discs and the retail DLLs are not in this repository (copyright). The
discs are on the Internet Archive; where each mod's retail files come from
is in that mod's page ([Point of View](point-of-view.md), [T2DEF](t2def.md),
[Wanted!](wanted.md), [Gunman Chronicles](gunman-chronicles.md),
[They Hunger](they-hunger.md)).

## The toolchain folder

One folder holds the compilers and the Wine prefix. The scripts find it
through the environment variable `VC_TOOLCHAINS`; it can be anywhere.

```
$VC_TOOLCHAINS/
  vc6/          Visual C++ 6.0 Professional as on the CD (compiler 12.00.8168)
    VC98/BIN/       compiler, linker and tools
    VC98/INCLUDE/   headers
    VC98/LIB/       C runtime and import libraries
    nativecrt/      copy of VC98/BIN that runs on the CD's own MSVCRT.DLL
  vc6std/       the same with the Standard edition's compiler back end (11.99.8168)
  vc6sp4std/    VC6 with Service Pack 4, Standard edition back end (11.99.8799)
  vc6sp4/       VC6 with Service Pack 4, Professional (12.00.8799); optional
  wineprefix/   the Wine prefix (64-bit)
```

| Tree | Builds |
|---|---|
| `vc6` | Point of View, Wanted!, Gunman Chronicles and They Hunger, server and client |
| `vc6std` | the T2DEF server |
| `vc6sp4std` | the T2DEF client |
| `vc6sp4` | none of the five mods; made in the same step, so it is included |

The wrappers `tools/vc5.sh` and `tools/vc4.sh` run Visual C++ 5.0 and 4.x
from `vc5/` and `vc4/` in the same folder. None of the five mods needs them;
see [Older compilers](#older-compilers-optional).

## 1. Download the discs

Three disc images from the Internet Archive, one file each:

| Disc | archive.org item | File | Size (bytes) |
|---|---|---|---|
| Visual C++ 6.0 Professional, English retail CD, 1999-02, part no. X04-58170 | [`X04-58170`](https://archive.org/details/X04-58170) | `1999-02_X04-58170_X04-58170_VCP600ENU1.7z` | 435,242,369 |
| Visual C++ 6.0 Learning Edition, English, 1999-02, part no. X04-58202 | [`X04-58202`](https://archive.org/details/X04-58202) | `1999-02_X04-58202_X04-58202_VCL600ENU1.7z` | 196,626,810 |
| Visual Studio 6.0 Service Pack 4, 2000-08, part no. X05-86594 (matrix X05-86829) | [`X05-86594`](https://archive.org/details/X05-86594) | `2000-08_X05-86594_X05-86829_vs6sp4.7z` | 374,501,304 |

Check them:

| File | SHA-256 | SHA-1 | MD5 |
|---|---|---|---|
| `…VCP600ENU1.7z` | `4e3fa95110e263fe4a42fe151b3f5208d414db72c006beee999a76c6c0ed4261` | `7d476db1e8eaebe5733fbe95cb7a6bd01132c479` | `495caefab7a0a3e461a5840f4ebdc64c` |
| `…VCL600ENU1.7z` | `7ed8ad7ebdbc29e8c1512c6530d267dd48804ac1860344191c99cb7234c3d9c3` | `cec6f224387bb9121f4d91c10818847738647973` | `53787fef30debef314bd36f2b7992545` |
| `…vs6sp4.7z` | `d67c8795c59fcbf0d67c468cb747bc28a9f15cb88a532896c1449e1e6da7251a` | `7ba4001a8e6a3af8f9807d85471b5ea83000eae8` | `aa8654befa6a002947fc180b1bbaf59f` |

The SHA-1 and MD5 values are the ones archive.org lists for the files.

Each archive holds the disc as an `.mdf` image (Alcohol 120% format), its
`.mds` descriptor, scans of the disc, and a `checksums.md5` for these files.
An `.mdf` stores raw CD sectors of 2448 bytes: 2352 bytes of sector data
and 96 bytes of subchannel data. The 2048 bytes of user data start at
offset 16 of each sector. `tools/mdf2iso.py` copies those out into an ISO
image, which 7-Zip reads, and prints the MD5 of the `.mdf` so you can check
it against `checksums.md5`.

## 2. Unpack the compilers

Set three variables first. The commands below use them; run them all in one
shell.

```sh
DL=$HOME/Downloads              # where the three .7z files are
MODS=$HOME/src/mods             # a checkout of this repository (branch main)
export VC_TOOLCHAINS=$HOME/vc-toolchains
mkdir -p "$VC_TOOLCHAINS" "$HOME/vc-unpack"
cd "$HOME/vc-unpack"
```

`$HOME/vc-unpack` is a scratch folder; delete it at the end.

### 2.1 Visual C++ 6.0 Professional: `vc6`

```sh
A=1999-02_X04-58170_X04-58170_VCP600ENU1
7z x -so "$DL/$A.7z" "$A/image/VCP600ENU1.mdf" | python3 "$MODS/tools/mdf2iso.py" - vcp600.iso
7z x -ovcp600 vcp600.iso VC98/BIN VC98/INCLUDE VC98/LIB OS/SYSTEM/MSVCRT.DLL \
    COMMON/MSDEV98/BIN/MSPDB60.DLL COMMON/MSDEV98/BIN/MSOBJ10.DLL COMMON/MSDEV98/BIN/MSDIS110.DLL -r-
rm vcp600.iso
mkdir -p "$VC_TOOLCHAINS/vc6"
cp -r vcp600/VC98 "$VC_TOOLCHAINS/vc6/"
cp vcp600/COMMON/MSDEV98/BIN/*.DLL "$VC_TOOLCHAINS/vc6/VC98/BIN/"
```

`mdf2iso.py` should print `321240 sectors; input md5
c046bd72a38c9ba328c7899a8677cf4a`, the `.mdf` line of `checksums.md5`.

The CD's `VC98` folder is the compiler as setup installs it, uncompressed.
`BIN` holds the compiler driver `CL.EXE`, the C and C++ front ends `C1.DLL`
and `C1XX.DLL`, the back end (code generator) `C2.DLL`, and `LINK.EXE`. The
three DLLs from `COMMON/MSDEV98/BIN` are loaded by these programs:
`MSPDB60.DLL` writes the program databases (`.pdb`) of the compiler and the
linker, the back end loads `MSOBJ10.DLL`, and LINK loads `MSDIS110.DLL` (a
disassembler). `OS/SYSTEM/MSVCRT.DLL` is the C runtime DLL the CD installs
(6.00.8168.0); step 3 uses it.

### 2.2 Learning Edition back end: `vc6std`

The Standard edition of VC6 has a different back end, which ignores
optimisation options: `/O2` gives the same code as `/Od`, and objects are
stamped as Standard edition objects. The T2DEF server was built with it. The
Learning Edition CD carries that back end: its `VC98/BIN` differs from the
Professional CD's in two files only, `C2.DLL` and `CL.EXE` (11.99.8168.0).
`vc6std` is a copy of `vc6` with those two.

```sh
A=1999-02_X04-58202_X04-58202_VCL600ENU1
7z x -so "$DL/$A.7z" "$A/image/VCL600ENU1.mdf" | python3 "$MODS/tools/mdf2iso.py" - vcl600.iso
7z x -ovcl600 vcl600.iso VC98/BIN/C2.DLL VC98/BIN/CL.EXE -r-
rm vcl600.iso
mkdir -p "$VC_TOOLCHAINS/vc6std"
cp -r "$VC_TOOLCHAINS/vc6/VC98" "$VC_TOOLCHAINS/vc6std/"
cp vcl600/VC98/BIN/C2.DLL vcl600/VC98/BIN/CL.EXE "$VC_TOOLCHAINS/vc6std/VC98/BIN/"
```

Expected: `178057 sectors; input md5 7d058d09324670054ef3c6d970537d66`.

### 2.3 Service Pack 4: `vc6sp4std` and `vc6sp4`

The service pack disc keeps its files uncompressed under `enu/` (English),
with lower-case names. Each SP4 tree is a copy of `vc6` with SP4's
compiler, linker, headers and libraries copied over under the upper-case
names the CD uses:

- `BIN`: `C1.DLL`, `C1XX.DLL` (12.00.8867), `CL.EXE` (12.00.8804), `LINK.EXE`
  (6.00.8447), `CVTRES.EXE`, and `MSDIS110.DLL` from `enu/common/msdev98/bin`;
- `INCLUDE`: `FPIEEE.H`, `OLEAUTO.H`, `XTREE`;
- `LIB`: the C runtime libraries (`LIBC`, `LIBCMT`, `MSVCRT` and their debug
  versions), `ATL`, `COMSUPP`, `OLEAUT32`, `UUID` (the `.lib` files only).

SP4 has no file called `C2.DLL`. Its setup installs one of three disguised
files under that name, by the edition it finds: `enu/os/system/msvcep.dll`
(Professional and Enterprise, 12.00.8799), `msse.dll` (Standard, 11.99.8799)
or `intro.dll` (Introductory, 11.98.8798). Without the matching back end the
new front ends stop with `fatal error C1900: Il mismatch between 'P1'
version '19991026' and 'P2' version '19970710'`.

```sh
A=2000-08_X05-86594_X05-86829_vs6sp4
7z x -so "$DL/$A.7z" "$A/image/vs6sp4.mdf" | python3 "$MODS/tools/mdf2iso.py" - vs6sp4.iso
7z x -ovs6sp4 vs6sp4.iso enu/vc98/bin enu/vc98/include enu/vc98/lib enu/common/msdev98/bin/msdis110.dll \
    enu/os/system/msse.dll enu/os/system/msvcep.dll enu/os/system/msvcrt.dll -r-
rm vs6sp4.iso
S=vs6sp4/enu
for t in vc6sp4std vc6sp4; do
    mkdir -p "$VC_TOOLCHAINS/$t"
    cp -r "$VC_TOOLCHAINS/vc6/VC98" "$VC_TOOLCHAINS/$t/"
    for f in $S/vc98/bin/* $S/common/msdev98/bin/msdis110.dll; do
        cp "$f" "$VC_TOOLCHAINS/$t/VC98/BIN/$(basename "$f" | tr a-z A-Z)"
    done
    for f in $S/vc98/include/*; do
        cp "$f" "$VC_TOOLCHAINS/$t/VC98/INCLUDE/$(basename "$f" | tr a-z A-Z)"
    done
    for f in $S/vc98/lib/*.[lL]ib; do
        cp "$f" "$VC_TOOLCHAINS/$t/VC98/LIB/$(basename "$f" | tr a-z A-Z)"
    done
done
cp $S/os/system/msse.dll "$VC_TOOLCHAINS/vc6sp4std/VC98/BIN/C2.DLL"
cp $S/os/system/msvcep.dll "$VC_TOOLCHAINS/vc6sp4/VC98/BIN/C2.DLL"
```

Expected: `265814 sectors; input md5 2afffb48728d1717be52ce265bd73c3f`.
`enu/os/system/msvcrt.dll` is SP4's C runtime DLL (6.00.8797.0), for step 3.

## 3. The `nativecrt` copies

LINK sorts with the C runtime's `qsort` in two places that reach the image:
- the order of the import table;
- which copy it keeps when it folds identical functions into one
  (`/OPT:ICF`, identical-code folding).

`qsort` is not a stable sort: how it orders elements that compare equal
depends on its implementation. Wine always loads its own builtin
`msvcrt.dll`, even when a Windows copy sits next to the program, and the
builtin orders equal elements differently from the 1998 DLL. With the
builtin, LINK keeps other copies of some folded functions and orders some
imports differently. The compiler is not affected: all 62 objects of SDK
2.0's multiplayer server project (`mp.dsp`) come out identical with either
runtime.

`tools/make_vc6_nativecrt.sh` makes `<tree>/nativecrt`: a copy of
`VC98/BIN` in which every program's import of `msvcrt.dll` is renamed
`MSVCRX.dll` (a name of the same length), plus the disc's own `MSVCRT.DLL`
installed as `MSVCRX.DLL`. Wine has no builtin of that name, so the programs
load the disc's runtime. The script checks the DLL's SHA-1 first.

```sh
"$MODS/tools/make_vc6_nativecrt.sh" vcp600/OS/SYSTEM/MSVCRT.DLL
"$MODS/tools/make_vc6_nativecrt.sh" -std vcp600/OS/SYSTEM/MSVCRT.DLL
"$MODS/tools/make_vc6_nativecrt.sh" -sp4std vs6sp4/enu/os/system/msvcrt.dll
"$MODS/tools/make_vc6_nativecrt.sh" -sp4 vs6sp4/enu/os/system/msvcrt.dll
```

`tools/vc6.sh` runs the programs from `nativecrt` by default. With
`VC6_CRT=wine` it runs the unpatched `VC98/BIN` on Wine's builtin runtime
instead. The T2DEF server needs that: its import table has the order
Wine's builtin gives, not the 1998 runtime's ([compilers.md](compilers.md)).

Now check the trees. `tools/check_toolchains.sh` compares a digest of all
files of each tree with the trees that built the retail-identical DLLs:

```sh
"$MODS/tools/check_toolchains.sh"
```

It should print `ok` for `vc6`, `vc6std`, `vc6sp4std` and `vc6sp4`. The
prefix lines come after step 4. Then delete the scratch folder:

```sh
cd && rm -rf "$HOME/vc-unpack"
```

## 4. The Wine prefix

A Wine prefix is the folder that holds one Windows installation for Wine:
its registry and its `C:` drive. The compilers get one of their own, so
nothing touches `~/.wine`. Create it as a 64-bit prefix (`win64`) with
`wineboot`, then turn off Wine's debugger:

```sh
unset DISPLAY WAYLAND_DISPLAY
export WINEPREFIX="$VC_TOOLCHAINS/wineprefix" WINEARCH=win64 WINEDEBUG=-all
export WINEDLLOVERRIDES='mscoree=;mshtml=;winemenubuilder.exe=d;winedbg.exe=d'
timeout 300 wineboot --init
timeout 60 wine reg add 'HKCU\Software\Wine\DllOverrides' /v winedbg.exe /t REG_SZ /d '' /f
timeout 60 wine reg add 'HKCU\Software\Wine\WineDbg' /v ShowCrashDialog /t REG_DWORD /d 0 /f
timeout 120 wineserver -w
unset WINEPREFIX WINEARCH WINEDEBUG WINEDLLOVERRIDES
```

What the settings do:
- No display (`DISPLAY` and `WAYLAND_DISPLAY` unset): Wine can't open
  windows.
- `mscoree=;mshtml=` stops `wineboot` from offering to install Wine Mono
  and Wine Gecko, which the compilers don't need. `winemenubuilder.exe=d`
  keeps Wine from adding menu entries to your desktop.
- `winedbg.exe` disabled and `ShowCrashDialog` 0: when a program crashes,
  Wine normally starts its debugger and shows a crash dialog, which waits
  for a click. A build without a display would wait forever. With these two
  settings a crashing program just exits (with code 5) and the build fails.
  The wrappers also pass `winedbg.exe=d` on every run.
- `wineserver -w` waits until Wine has written the registry to disk.

Nothing is installed in the prefix: the compilers run from the toolchain
folder, and their `INCLUDE` and `LIB` are set by the wrappers. A server
build copies the prefix into its build folder and maps the drive letter
that retail's PDB path names (`D:`, `K:`) to a folder there, or puts the
source tree on its `C:` drive.

Run `"$MODS/tools/check_toolchains.sh"` again: the three `wineprefix` lines
should say `win64`, `winedbg.exe disabled` and `crash dialog off`.

## 5. libfaketime and Python

A linked DLL records its link time in several places: the COFF header, the
export directory and, for a DLL linked with `/debug`, the debug directory
and the PDB signature. The scripts run LINK alone under libfaketime with
the clock frozen at retail's link time, in UTC: `faketime -f 'YYYY-MM-DD
hh:mm:ss'` with `TZ=UTC` and `FAKETIME_DONT_FAKE_MONOTONIC=1`
(`build_dsp.py --link-time`). Install it from your distribution (Debian and
Ubuntu: `faketime`; Gentoo: `sys-libs/libfaketime`). Tested: 0.9.12.

One build loads the library directly. Wanted!'s final link needs a clock
that jumps by two seconds while LINK runs, so `tools/release/wanted_server.py`
sets `LD_PRELOAD` to `libfaketime.so.1` and gives it the time in a file. It
finds the library's path inside the `faketime` program; set `FAKETIME_LIB`
to the path if that fails. LINK is a 32-bit Windows program; in a WoW64
build of Wine it runs in a 64-bit Linux process, so the 64-bit library is
the right one. With a Wine that runs 32-bit programs in 32-bit Linux
processes, the 32-bit library would be needed (not tested).

Python 3 runs `build_dsp.py` and the Python parts of the release scripts. They use only the
standard library. Tested: 3.14.

## 6. A first build

Each mod is a branch of this repository. Get a checkout of one, for
example Point of View: from a clone of the repository, `git worktree add
../pov point-of-view`, or clone that branch alone with `git clone -b
point-of-view <repository URL> pov`. Leave Git's line-end conversion off
(`core.autocrlf` unset or `false`): the sources are stored with CRLF line
ends, and builds from converted files were not tested.

Then run the mod's release script from the `main` checkout:

```sh
cd "$MODS"
VC_TOOLCHAINS=$HOME/vc-toolchains tools/release/point-of-view.sh ../pov ../build-pov
```

The arguments are the checkout (`SRC`), a build folder (`OUT`, created if
missing), and optionally `server`, `client` or `both` (the default). Every
build is a full build; a mod's two DLLs took two to three minutes here. For
each DLL the output gives the built file, its SHA-256, retail's, and the
verdict:

```
server: full build, LINK at 2003-05-06 07:56:13 UTC
  compiled 142 of 142 sources (0 failed); link rc=0; output .../build-pov/root/Editors/Levels/POVSourceCode/dlls/Releasehl/hl.dll
server: .../build-pov/root/Editors/Levels/POVSourceCode/dlls/Releasehl/hl.dll
  SHA-256 built:  0f42bad4664a87cf3793efd213088b9c2c5dd0ac230b45db3544fd7c344f7ccd
  SHA-256 retail: 0f42bad4664a87cf3793efd213088b9c2c5dd0ac230b45db3544fd7c344f7ccd
  identical to retail
client: full build, LINK at 2003-05-02 01:24:25 UTC
  compiled 54 of 54 sources (0 failed); link rc=0; output .../build-pov/client/client.dll
client: .../build-pov/client/client.dll
  SHA-256 built:  c0a40333d725c4a8d66e2640e78929ebc69bd118c28d3b79e0aab35d69608ea1
  SHA-256 retail: c0a40333d725c4a8d66e2640e78929ebc69bd118c28d3b79e0aab35d69608ea1
  identical to retail
```

The exit code is 0 only if every DLL built is identical to retail. The
other scripts work the same way: `t2def.sh`, `wanted.sh`,
`gunman-chronicles.sh` and `they-hunger.sh`. The Wanted! server and the
Gunman Chronicles client are not identical yet, so their scripts end with
`differs from retail` and exit code 1 (the mods' pages give the hashes they
produce). They Hunger's server takes longer than the others, about five
minutes here: its script replays 102 more compiles and 73 links.

To compare byte for byte with your own copy of the retail files, set
`RETAIL` to the mod's folder in the game, the one with `dlls/` and
`cl_dlls/`:

```sh
RETAIL=/path/to/game/pov VC_TOOLCHAINS=$HOME/vc-toolchains tools/release/point-of-view.sh ../pov ../build-pov
```

Each DLL then gets one more line: `cmp …/dlls/hl.dll: no difference`, or
how many bytes differ.

In the build folder: `server.log` and `client.log` hold the build output;
`build.log` next to each DLL lists every compiler and linker command. The
server's source tree is a tree of symbolic links into `SRC`, at the path
retail's PDB names, so the build writes nothing into the checkout. Two
builds can run at the same time if their build folders differ. Running
again with the same build folder reuses its copy of the prefix and rebuilds
everything else from scratch.

### Using the compilers directly

The wrappers run one program of a tree, with `INCLUDE` and `LIB` set:

```sh
export VC_TOOLCHAINS=$HOME/vc-toolchains
tools/vc6.sh CL /nologo /c /O2 foo.cpp          # VC6 Professional
tools/vc6.sh -std CL /nologo /c foo.cpp         # Standard edition back end
tools/vc6.sh -sp4std CL ...                     # SP4, Standard edition
tools/vc6.sh LINK /nologo /dll ...
```

Pass paths outside the current folder as `Z:\...` (Wine's drive `Z:` is
the Linux root). `VC_WINEPREFIX` selects another Wine prefix.
`tools/build_dsp.py` builds a whole `.dsp` project the way the Visual C++ 6
IDE does; its docstring has the details.

## What was tested

On 2026-11-01, on Gentoo Linux x86-64 with Wine 11.0 (`wine-vanilla-11.0`,
a WoW64 build), libfaketime 0.9.12, Python 3.14.10 and 7-Zip 26.03:

- The steps above, run from the three downloads into a new toolchain folder,
  gave trees identical file for file to the ones used throughout the
  reconstruction (`check_toolchains.sh`).
- With that folder and a new prefix made as in step 4, all four release
  scripts gave the same eight files as with the original folder: the six
  retail-identical DLLs (both Point of View DLLs, both T2DEF DLLs, the
  Wanted! client and the Gunman Chronicles server) byte for byte, and the
  known hashes for the other two.

They Hunger was added on 2026-11-11. `they-hunger.sh` gave both of its DLLs
byte for byte from a fresh clone of its branch, with the toolchain folder
used throughout the reconstruction and the same Wine. The steps from the
downloads were not repeated for it.

Not tested:
- **Other Wine versions.** The T2DEF server is linked on Wine's builtin
  `msvcrt.dll`, and its import table depends on how that `qsort` orders
  equal elements. Only Wine 11.0's was tested; another version could order
  them differently. The other DLLs link on the discs' own runtime and
  shouldn't depend on Wine's, but were not tested elsewhere either.
- A Wine that is not a WoW64 build, other libfaketime versions, Windows.

## Older compilers (optional)

`tools/vc5.sh` and `tools/vc4.sh` run Visual C++ 5.0 and 4.x, which were
used to identify the compilers of other mods. None of these five needs
them. Layout and sources, briefly:

| Tree | Contents | Source (archive.org item / file, SHA-1) |
|---|---|---|
| `vc5/rtm/{BIN,INCLUDE,LIB}` | Visual C++ 5.0 Professional (11.00.7022): `DEVSTUDIO/VC/{BIN,INCLUDE,LIB}` without `BIN/WIN95` and `BIN/WINNT`, plus `MSPDB50.DLL`, `MSDIS100.DLL`, `RC.EXE`, `RCDLL.DLL` from `DEVSTUDIO/SHAREDIDE/BIN` and `MSVCP50.DLL` from `VC/REDIST` in `BIN` | `microsoft-visual-studio-97-professional-edition-disc-3`, Visual Studio 97 Professional disc 3, `a3ade9b0681ffab34f058a7991f6b7aa25d342e1` |
| `vc5/sp3/...` | the same with Visual Studio 97 Service Pack 3's English VC files over it (compiler 11.00.7303, linker 5.10.7303), under the upper-case names | `vs97sp3`, `vs97sp3.zip`, `10a818f34b7223d4cbd2c99efb0e52c9efcd3bfc`; `cabextract` unpacks it once the cabinets are also linked under upper-case names |
| `vc4/40/{BIN,INCLUDE,LIB}` | Visual C++ 4.0 (10.00.5270): `MSDEV\BIN` (top-level files), `MSDEV\INCLUDE`, `MSDEV\LIB` | `msvc4x` / `MSVC40.iso`, `81e139ac41d76740a6ba6d474355b37ed2e46c66` |
| `vc4/41/...` | Visual C++ 4.1 (10.10.6038), the same folders of `VC41\MSDEV` | `msvc4x` / `VC_FOR_16.iso`, `2e00df7338b39343a13fc9d5ee36ff86fa21fc33` |
| `vc4/42/...` | Visual C++ 4.2 Professional (10.20.6166), the same folders | `msft-visual-cplusplus-42` / `MSVC42.iso`, `e8b64d5f25d3d58891a62722cfbec371f870823c` |

These steps were not repeated for this page.

## Troubleshooting

- `set VC_TOOLCHAINS to the toolchain folder`: the variable is required;
  `tools/` is not the toolchain folder.
- `no Wine prefix at …`: step 4 was not done for this folder.
- `no …/nativecrt; run make_vc6_nativecrt.sh`: step 3 is missing for that
  tree.
- `C1900: Il mismatch between 'P1' version … and 'P2' version …`: an SP4
  tree without its `C2.DLL` (end of step 2.3).
- `libfaketime.so.1 not found` (Wanted! server): set `FAKETIME_LIB` to the
  library's path (step 5).
- A build stops with `FAILED <file>`: the compiler messages are in
  `build.log` next to the output DLL. A compiler that crashes exits with
  code 5.
- `warning LNK4070: /OUT:hl.dll directive in .EXP differs from output
  filename` in Gunman Chronicles' and T2DEF's server logs is expected: their
  `.def` files say `LIBRARY hl` and the output is `gunman.dll`/`t2def.dll`,
  as in retail.
