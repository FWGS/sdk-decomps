# Rebuilding the DLLs: shared steps

Step-by-step instructions to rebuild the mods' DLLs and check them against
the retail files. Linux on x86-64 only: the compilers run under Wine, and
the release scripts use Wine drive letters to put the sources at the paths
the retail builds had. Do these shared steps once, then follow a mod's page:

[Point of View](point-of-view.md) · [T2DEF](t2def.md) · [Wanted!](wanted.md) ·
[Gunman Chronicles](gunman-chronicles.md) · [They Hunger](they-hunger.md) ·
[Counter-Strike](counter-strike.md)

Background (why each step is needed): [setup.md](../setup.md),
[compilers.md](../compilers.md), [reproducing.md](../reproducing.md).

## 1. Install the tools

Wine (tested: 11.0, a WoW64 build), libfaketime's `faketime` (0.9.12),
Python 3 (3.14), 7-Zip's `7z`, `cabextract`, `git`, `curl` and GNU
coreutils. Package names vary by distribution.

## 2. Set the folders and get the repository

Run every later command in this shell (or set these again first).

```sh
export WORK=$HOME/hl-rebuild
export DL=$WORK/downloads
export VC_TOOLCHAINS=$WORK/toolchains
export MODS=$WORK/sdk-decomps
mkdir -p "$WORK" "$DL" "$VC_TOOLCHAINS"
git clone https://github.com/FWGS/sdk-decomps "$MODS"
```

## 3. Download the compilers

Only what your mods need:

| File | Needed by |
|---|---|
| Visual C++ 6.0 Professional CD | all mods |
| Visual C++ 6.0 Learning Edition CD | T2DEF |
| Visual Studio 6.0 Service Pack 4 | T2DEF, Counter-Strike |
| Visual Studio 6.0 Service Pack 5, Processor Pack, Visual C++ 5.0 (VS97 disc 3) | Counter-Strike |

```sh
cd "$DL"
curl -LO https://archive.org/download/X04-58170/1999-02_X04-58170_X04-58170_VCP600ENU1.7z
curl -LO https://archive.org/download/X04-58202/1999-02_X04-58202_X04-58202_VCL600ENU1.7z
curl -LO https://archive.org/download/X05-86594/2000-08_X05-86594_X05-86829_vs6sp4.7z
curl -LO https://archive.org/download/vs6sp5/vs6sp5.exe
curl -LO https://archive.org/download/vcpp5/vcpp5.exe
curl -L -o vs97-pro-disc3.iso 'https://archive.org/download/microsoft-visual-studio-97-professional-edition-disc-3/Microsoft%20Visual%20Studio%2097%20Professional%20Edition%20-%20Disc%203.iso'
sha256sum -c --ignore-missing <<'EOF'
4e3fa95110e263fe4a42fe151b3f5208d414db72c006beee999a76c6c0ed4261  1999-02_X04-58170_X04-58170_VCP600ENU1.7z
7ed8ad7ebdbc29e8c1512c6530d267dd48804ac1860344191c99cb7234c3d9c3  1999-02_X04-58202_X04-58202_VCL600ENU1.7z
d67c8795c59fcbf0d67c468cb747bc28a9f15cb88a532896c1449e1e6da7251a  2000-08_X05-86594_X05-86829_vs6sp4.7z
10a834041223ad21181c0e87c0a90930d976498556af2e8f959a044101e6a604  vs6sp5.exe
96817df1bb0cb58e5b1d0fbcc5b993a63548182add1fbff66365401203ecfb6c  vcpp5.exe
99101fb01f564ba9cc50cbcbf862c334cd1be16a2e8c277cb81f9a500cb6413f  vs97-pro-disc3.iso
EOF
```

Every file you downloaded must print `OK`.

## 4. Unpack the compilers

Each block makes one tree in `$VC_TOOLCHAINS`. The disc images are `.mdf`
files inside the `.7z`; `mdf2iso.py` turns one into an ISO that 7-Zip reads.

```sh
mkdir -p "$WORK/unpack" && cd "$WORK/unpack"
```

**4.1 `vc6`** (all mods): Visual C++ 6.0 Professional as on the CD.

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

**4.2 `vc6std`** (T2DEF): the Learning Edition's back end.

```sh
A=1999-02_X04-58202_X04-58202_VCL600ENU1
7z x -so "$DL/$A.7z" "$A/image/VCL600ENU1.mdf" | python3 "$MODS/tools/mdf2iso.py" - vcl600.iso
7z x -ovcl600 vcl600.iso VC98/BIN/C2.DLL VC98/BIN/CL.EXE -r-
rm vcl600.iso
mkdir -p "$VC_TOOLCHAINS/vc6std"
cp -r "$VC_TOOLCHAINS/vc6/VC98" "$VC_TOOLCHAINS/vc6std/"
cp vcl600/VC98/BIN/C2.DLL vcl600/VC98/BIN/CL.EXE "$VC_TOOLCHAINS/vc6std/VC98/BIN/"
```

**4.3 `vc6sp4std` and `vc6sp4`** (T2DEF; Counter-Strike): Service Pack 4.

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

**4.4 `vc6sp5`** (Counter-Strike): Service Pack 5.

```sh
cabextract -q -d vs6sp5 -F 'VS6sp5*.cab' "$DL/vs6sp5.exe"
cabextract -q -d vs6sp5 -F 'vc98/bin/*' -F 'vc98/include/*' -F 'vc98/lib/*' \
    -F VB98/msdis110.dll -F os/system/msvcep.dll -F os/system/msvcrt.dll vs6sp5/VS6sp51.cab
rm vs6sp5/*.cab
mkdir -p "$VC_TOOLCHAINS/vc6sp5"
cp -r "$VC_TOOLCHAINS/vc6/VC98" "$VC_TOOLCHAINS/vc6sp5/"
for d in BIN INCLUDE LIB; do
    for f in vs6sp5/vc98/$(echo $d | tr A-Z a-z)/*; do
        cp "$f" "$VC_TOOLCHAINS/vc6sp5/VC98/$d/$(basename "$f" | tr a-z A-Z)"
    done
done
cp vs6sp5/os/system/msvcep.dll "$VC_TOOLCHAINS/vc6sp5/VC98/BIN/C2.DLL"
cp vs6sp5/VB98/msdis110.dll "$VC_TOOLCHAINS/vc6sp5/VC98/BIN/MSDIS110.DLL"
```

**4.5 `vc6sp5pp`** (Counter-Strike): Service Pack 5 with the Processor Pack.

```sh
cabextract -q -d vcpp5 "$DL/vcpp5.exe"
mkdir -p "$VC_TOOLCHAINS/vc6sp5pp"
cp -r "$VC_TOOLCHAINS/vc6sp5/VC98" "$VC_TOOLCHAINS/vc6sp5pp/"
for f in c2.dll ml.exe ml.err h2inc.exe h2inc.err; do
    cp "vcpp5/$f" "$VC_TOOLCHAINS/vc6sp5pp/VC98/BIN/$(echo $f | tr a-z A-Z)"
done
for f in dvec.h fpieee.h fvec.h ivec.h malloc.h mm3dnow.h mmintrin.h; do
    cp "vcpp5/$f" "$VC_TOOLCHAINS/vc6sp5pp/VC98/INCLUDE/$(echo $f | tr a-z A-Z)"
done
cp vcpp5/emmint_1.h "$VC_TOOLCHAINS/vc6sp5pp/VC98/INCLUDE/EMMINTRIN.H"
cp vcpp5/xmmint_1.h "$VC_TOOLCHAINS/vc6sp5pp/VC98/INCLUDE/XMMINTRIN.H"
```

**4.6 `vc5/rtm`** (Counter-Strike): Visual C++ 5.0 as on the CD.

```sh
7z x -ovs97 "$DL/vs97-pro-disc3.iso" DEVSTUDIO/VC/BIN DEVSTUDIO/VC/INCLUDE DEVSTUDIO/VC/LIB \
    DEVSTUDIO/VC/REDIST/MSVCP50.DLL DEVSTUDIO/VC/REDIST/MSVCRT.DLL \
    DEVSTUDIO/SHAREDIDE/BIN/MSPDB50.DLL DEVSTUDIO/SHAREDIDE/BIN/MSDIS100.DLL \
    DEVSTUDIO/SHAREDIDE/BIN/RC.EXE DEVSTUDIO/SHAREDIDE/BIN/RCDLL.DLL -r-
rm -r vs97/DEVSTUDIO/VC/BIN/WIN95 vs97/DEVSTUDIO/VC/BIN/WINNT
mkdir -p "$VC_TOOLCHAINS/vc5/rtm"
cp -r vs97/DEVSTUDIO/VC/BIN vs97/DEVSTUDIO/VC/INCLUDE vs97/DEVSTUDIO/VC/LIB "$VC_TOOLCHAINS/vc5/rtm/"
cp vs97/DEVSTUDIO/SHAREDIDE/BIN/*.* vs97/DEVSTUDIO/VC/REDIST/MSVCP50.DLL "$VC_TOOLCHAINS/vc5/rtm/BIN/"
```

## 5. Let the linkers run on their own C runtime

Wine replaces `msvcrt.dll` with its own, and LINK's output depends on it.
Each line makes `<tree>/nativecrt`, a copy whose programs load the disc's
runtime. Run the lines for the trees you made:

```sh
cd "$WORK/unpack"
"$MODS/tools/make_vc6_nativecrt.sh" vcp600/OS/SYSTEM/MSVCRT.DLL
"$MODS/tools/make_vc6_nativecrt.sh" -std vcp600/OS/SYSTEM/MSVCRT.DLL
"$MODS/tools/make_vc6_nativecrt.sh" -sp4std vs6sp4/enu/os/system/msvcrt.dll
"$MODS/tools/make_vc6_nativecrt.sh" -sp4 vs6sp4/enu/os/system/msvcrt.dll
"$MODS/tools/make_vc6_nativecrt.sh" -sp5 vs6sp5/os/system/msvcrt.dll
"$MODS/tools/make_vc6_nativecrt.sh" -sp5pp vs6sp5/os/system/msvcrt.dll
"$MODS/tools/make_vc5_nativecrt.sh" -rtm vs97/DEVSTUDIO/VC/REDIST/MSVCRT.DLL
```

## 6. Make the Wine prefix

Its own prefix, without a display and with Wine's debugger off, so a
crashing compiler exits instead of waiting for a click:

```sh
(
unset DISPLAY WAYLAND_DISPLAY
export WINEPREFIX="$VC_TOOLCHAINS/wineprefix" WINEARCH=win64 WINEDEBUG=-all
export WINEDLLOVERRIDES='mscoree=;mshtml=;winemenubuilder.exe=d;winedbg.exe=d'
timeout 300 wineboot --init
timeout 60 wine reg add 'HKCU\Software\Wine\DllOverrides' /v winedbg.exe /t REG_SZ /d '' /f
timeout 60 wine reg add 'HKCU\Software\Wine\WineDbg' /v ShowCrashDialog /t REG_DWORD /d 0 /f
timeout 120 wineserver -w
)
```

## 7. Check

```sh
"$MODS/tools/check_toolchains.sh"
rm -rf "$WORK/unpack"
```

Every tree you made must print `ok`, and the prefix `win64`, `winedbg.exe
disabled`, `crash dialog off`. Trees you skipped print `missing`.

## Drive letters

You don't map drives yourself. Each release script copies the prefix into
its build folder and maps there the drive letter retail's debug record
names (for example `D:` for Point of View, `U:` for Counter-Strike 1.1), so
the sources sit at retail's path and that path goes into the PDB record.
The mod pages name the path.
