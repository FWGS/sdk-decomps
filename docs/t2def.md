# T2DEF (Todesangst 2) 1.1

T2DEF is a single-player Half-Life mod by Daryn "PeppyFool" Waite, a sequel
to Todesangst. Version 1.0 came out on 6 December 2002. A patch to 1.1
followed in January 2003; it fixed a "double trooper & frozen door bug" and
HUD sprites.

The server DLL is built on Spirit of Half-Life 0.6.1, Laurie Cheers'
extension of the Half-Life SDK for mappers, plus a few pieces of later
Spirit releases (0.6.3's doors, 0.7.1's `multi_manager` and `env_fog`).
The client DLL is SDK 2.2's client plus T2DEF's MP3 player, which uses the
FMOD library, and four VGUI panels (settings, captions, security cameras).

The branch [`t2def`](../../../tree/t2def) holds the reconstructed source of
both DLLs. This page describes its commit `0833c0e`.

## Status

Both DLLs rebuild byte for byte. Verified on 2026-11-01 with fresh release
builds, by SHA-256 and `cmp` against the files of an installed T2DEF 1.1.

| | Server | Client |
|---|---|---|
| File in the mod folder | `dlls/t2def.dll` | `cl_dlls/client.dll` |
| Size | 1,409,089 bytes | 651,264 bytes |
| Linked (PE time stamp) | 2002-12-16 01:56:59 UTC (release 1.1) | 2002-12-06 05:06:23 UTC (release 1.0; 1.1 kept it) |
| Compiler | Visual C++ 6.0 (VC6), first release (RTM), Standard edition, build 8168 | VC6 Service Pack 4, Standard edition, build 8799 |
| Linker | LINK 6.00.8168 | LINK 6.00.8447 |
| Project | `dlls/hl.dsp`, "hl - Win32 Release" | `cl_dll/cl_dll.dsp`, "cl_dll - Win32 Release" |
| Result | identical | identical |

| File | SHA-256 of retail and of the rebuild |
|---|---|
| `t2def.dll` | `ba8fea632d5eca98b9e68078e57a3aa37e07acd872063746d4c597c2ea076135` |
| `client.dll` | `cbd18f9318fe5a626bb35318f70db6a340300d8c6874f8b64e7bdb53f5e91762` |

## The retail files

The retail DLLs are not in this repository. These are the known sources:

- **An installed T2DEF 1.1.** This is what we compared against. Its
  `liblist.gam` says `version "1.1"` and `gamedll "dlls\t2def.dll"`. The
  archive it was installed from was not recorded.
- **The 1.0 → 1.1 patch, `t2def11patch.exe`**: 614,231 bytes, SHA-256
  `4b8695f97bd6cef56224c0e7d05d993091433dc2e1df110261dd4630ef7bce5a`, MD5
  `e3b0c0500759447d5238fa2c31bb541e`. The Wayback Machine has it as capture
  20030220225217 of a mirror listed on peppyfool.com. The raw file is at
  `https://web.archive.org/web/20030220225217id_/http://home.wanadoo.nl:80/bluh/Sander/HL/mapz/t2def11patch.exe`;
  its SHA-1 equals the digest the Wayback Machine publishes for that
  capture.
  - It holds four files: `dlls\t2def.dll` (dated 2002-12-16 01:57),
    `liblist.gam`, `sprites\hud.txt` and the manual. Its `t2def.dll` is
    byte-identical to the installed one.
  - It has no `client.dll`. So 1.1's client is 1.0's, which fits its link
    date: the day 1.0 came out.
  - It is a Clickteam Install Maker package, which 7-Zip cannot open. We
    unpacked it without running it, with an extractor that is not part of
    this repository.
- **GameBanana**, item 59074 (<https://gamebanana.com/mods/59074>):
  `todesangst.7z`, 72,957,689 bytes, MD5 `9c9c03954d9848010b0567e19dc320fd`,
  uploaded in 2022. It is an installed 1.1. Its 7z header lists `t2def.dll`
  with 1,409,089 bytes dated 2002-12-16 and `client.dll` with 651,264 bytes
  dated 2002-12-06, the sizes and dates of the files above. We read only
  the header, with HTTP range requests; the archive was not downloaded, so
  its files were not hashed.
- **ModDB** listed `T2DEFv11.zip`, the full 1.1 (78,096,102 bytes, MD5
  `a8ef1388a6e536809351c8eeb7689be4`). We know this only from a 2008
  Wayback capture of
  <https://www.moddb.com/mods/todesangst-2der-echte-feind/downloads>; the
  file was not checked.
- **Version 1.0** (`t2defv1.0.exe`, about 76.6 MB) was not found. 1.0's
  server is therefore unknown.

## Rebuilding

### What you need

- A checkout of the `t2def` branch.
- The toolchain folder from [setup.md](setup.md), with the trees `vc6std`
  (server) and `vc6sp4std` (client) and the Wine prefix. They are made from
  three discs: `X04-58170` (VC6 Professional), `X04-58202` (VC6 Learning
  Edition) and `X05-86594` (Visual Studio 6.0 Service Pack 4). See
  [compilers.md](compilers.md).
- Linux with Wine, libfaketime and Python 3 (standard library only). The
  verified builds used Wine 11.0 with a win64 prefix and libfaketime
  0.9.12.
- The game files, only if you want the script to `cmp` against them.

The branch stores its sources with CRLF line ends and has no
`.gitattributes`. Leave `core.autocrlf` off (the default on Linux), so git
checks the files out as stored. We haven't tested whether LF line ends
change the build.

### Commands

From a checkout of `main`:

```sh
git worktree add ../t2def t2def
VC_TOOLCHAINS=/path/to/toolchains tools/release/t2def.sh ../t2def ../t2def-build
```

- The third argument is `server`, `client` or `both` (the default).
- The build folder (`../t2def-build`) is created if missing. It gets its
  own copy of the Wine prefix (`wineprefix/`) and the source tree laid out
  at retail's PDB path.
- For each DLL the script prints the path of the built file, its SHA-256
  (`SHA-256 built:`), retail's (`SHA-256 retail:`), and `identical to
  retail` or `differs from retail`.
- It exits with 0 only if every DLL it built is identical to retail.

To compare with your own game files as well, point `RETAIL` at the mod
folder, the one that holds `dlls/t2def.dll` and `cl_dlls/client.dll`. The
script then also runs `cmp` against them and prints `no difference` or the
number of bytes that differ. It only reads that folder:

```sh
RETAIL=/path/to/Half-Life/t2def VC_TOOLCHAINS=/path/to/toolchains \
  tools/release/t2def.sh ../t2def ../t2def-build
```

### The toolchains

Each DLL needs its own compiler. The Rich headers (see below) name the
editions: the server's objects carry RTM Standard stamps, the client's SP4
Standard stamps.

| | Server: `vc6std` | Client: `vc6sp4std` |
|---|---|---|
| `CL.EXE` | 11.99.8168, sha1 `8441a48c0781d5a6ecc7a987f81e01fc696ae38c` | 12.00.8804, sha1 `c8a2dacc7d732e3cd98372d6f23731e9c29a893a` |
| `C1.DLL`, `C1XX.DLL` (front ends) | 12.00.8168, as on the Professional CD | 12.00.8867; `C1XX.DLL` sha1 `3ea2b30216ca28ad63836a0bc5ad61455d364495` |
| `C2.DLL` (back end) | 11.99.8168.0, Standard edition, from the Learning Edition CD; sha1 `27e3050d75b6dfde7e9d86dc5ca13319f176f868` | 11.99.8799.0, Standard edition: SP4's `msse.dll`; sha1 `125778a9cf3400d431d26cbef2b947348edec3b8` |
| `LINK.EXE` | 6.00.8168, sha1 `fba5647145a7261756d897af95ced192784cac50` | 6.00.8447, sha1 `1a3a1b2b18dcd8509305f67f8430f441d627250f` |
| `LIBCMT.LIB` | the Professional CD's, sha1 `97e4b1070327d9d8d33cf2f8ba4fc9fb095f61ef` | SP4's (objects stamped 8797), sha1 `05f7c53a28401bfa36cf3367142cde169aba330a` |
| C runtime the tools run on | Wine's builtin `msvcrt.dll` | SP4's `MSVCRT.DLL` 6.00.8797.0, sha1 `63a4fcd64ecea975c1b91de04702c68a9f2a3c7d` |

The Standard edition's back end ignores `/O2`: it compiles without
optimisation, as `/Od` would. The server's code is therefore unoptimised,
although `hl.dsp` asks for `/O2`. With `/O2`, the client's SP4 Standard
compiler keeps intrinsics (`/Oi`), function-level linking (`/Gy`) and
string pooling (`/Gf`), and drops the optimiser.

The client links against the SDK's VGUI import library,
`utils/vgui/lib/win32_vc6/vgui.lib`, which is on the branch.

### What the script does for the server

Retail's server names its PDB in a CodeView debug record:
`C:\hledit todesangst2\code\dlls\Releasehl\hl.pdb`, with signature
`0x3df56e9d` and age 15. The signature is the time the PDB was created,
2002-12-10 04:33:33 UTC. The age counts how often LINK wrote the PDB. So
`hl.pdb` was created six days before the final link and written by 15
links in all: the release DLL came out of an incremental build, not a
clean one. The script replays such a build:

1. **Prefix and tree.** It copies the toolchain's Wine prefix into the
   build folder and lays out the source tree at
   `C:\hledit todesangst2\code` on that prefix's drive C:, as symbolic
   links into the checkout. `dlls` and `cl_dll` are real folders of
   per-file links, so the build writes into the build folder, never into
   the checkout.
2. **Compiler.** It builds `dlls/hl.dsp`, configuration "hl - Win32
   Release", with [`tools/build_dsp.py`](../tools/build_dsp.py) and the
   `vc6std` tree. The project's flags are used unchanged. `build_dsp.py`
   builds a `.dsp` project as the VC6 IDE does; see
   [reproducing.md](reproducing.md). The output is `t2def.dll`, as the
   project's `/out:".\Releasehl\t2def.dll"` says, and the PDB is `hl.pdb`,
   as its `/pdb:".\Releasehl\hl.pdb"` says.
3. **Runtime.** `CL` and `LINK` run on Wine's builtin `msvcrt.dll`, not on
   the VC6 CD's `MSVCRT.DLL`. This sets the import table's order (see
   below).
4. **The PDB's 15 links**, each with LINK's clock frozen by libfaketime:
   - one full build of all 116 sources (113 C++, 3 C), linked at
     2002-12-10 04:33:33 UTC. This creates `hl.pdb` with age 1;
   - 13 relinks at the same time. Every object is up to date, so only LINK
     runs (ages 2 to 14);
   - the last build session (below);
   - the final link at 2002-12-16 01:56:59 UTC (age 15). It also sets the
     time stamps of the COFF header, the export directory and the debug
     directory.
5. It prints the hashes and compares them with retail.

**The last build session.** VC6 projects use automatic precompiled headers
(`/YX`). A precompiled header (`.pch`) is the compiler's saved state after
a file's leading `#include`s. With `/YX`, each compile looks at the `.pch`
the previous compile left. If it fits the file's include list, the compiler
reuses it; otherwise it rebuilds it from this file's includes. Whether a
header was inside the `.pch` changes where its inline functions and
strings land in the object. So each object depends on which file was
compiled before it.

Retail's `teamplay_gamerules.obj` (whose string "HL Teamplay" comes first)
and `weapons.obj` are what VC6 makes when those two files find a `.pch` of
`extdll.h util.h cbase.h`. A full build in project order never leaves them
that:
- `teamplay_gamerules.cpp` finds the `.pch` of `talkmonster.cpp`. Valve's
  `#if !defined( OEM_BUILD )` before `squeakgrenade.cpp`'s includes makes
  that file rebuild the `.pch`, and `subs.cpp` and `talkmonster.cpp`
  rebuild it again.
- `weapons.cpp` finds the `.pch` of `voice_gamemgr.cpp`, whose include list
  starts with its own header.

So after the 13 relinks the script recompiles three files, in project
order, as one IDE build of a few edited files would: `sound.cpp`,
`teamplay_gamerules.cpp`, `weapons.cpp`. Each is a plain `CL /c` with the
project's flags, sharing the `.pch` the previous compile left; VC6's own
`/YX` logic decides each time.
- The full build ends with `zombie.cpp`'s `.pch`
  (`extdll util cbase monsters schedule`).
- `sound.cpp`, whose list is `extdll util cbase`, rebuilds the `.pch` from
  its list, as it does in the full build. Its object doesn't change.
- `teamplay_gamerules.cpp` and `weapons.cpp` reuse that `.pch` and come out
  as retail's.
- Every other object stays as the full build made it.

Without the session (one full build, one link) 642 bytes of the server
differ from retail. Which files T2DEF really recompiled is not known; see
[Build stand-ins](#build-stand-ins).

### What the script does for the client

The client has no debug directory, so it records no path, PDB or build
history. The script builds `cl_dll/cl_dll.dsp`, configuration "cl_dll -
Win32 Release", in one full build of all 83 sources (80 C++, 3 C):
- with the `vc6sp4std` tree and the project's flags, plus `/Ob0` (no
  inline expansion; see [Build stand-ins](#build-stand-ins));
- with the tools on SP4's own `MSVCRT.DLL`;
- with LINK's clock frozen at 2002-12-06 05:06:23 UTC.

## What the image holds besides code

A DLL records more than its code. [reproducing.md](reproducing.md)
explains each part and how the scripts rebuild it. For T2DEF:

| Part | Server | Client |
|---|---|---|
| PDB path | `C:\hledit todesangst2\code\dlls\Releasehl\hl.pdb`, in a 65-byte CodeView record at the end of the file, so the path's length sets the file size | none |
| PDB signature and age | `0x3df56e9d` (2002-12-10 04:33:33 UTC), age 15 | none |
| Time stamps | COFF header, export and debug directories: `0x3dfd32eb` (2002-12-16 01:56:59 UTC) | COFF header and export directory: `0x3df0304f` (2002-12-06 05:06:23 UTC) |
| Export directory name | `hl.dll`, from `hl.def`'s `LIBRARY hl`, although the file is `t2def.dll` | `client.dll` |
| Rich header | C++ Standard 8168 ×113, C Standard 8168 ×3; LIBCMT's C 8168 ×127, C++ 8168 ×3, assembler 7299 ×39; alias objects 7291 ×2; import stubs 8034 ×3; imports ×75; linker 8168 ×1 | C++ Standard 8799 ×80, C Standard 8799 ×3; LIBCMT's C 8797 ×133, C++ 8797 ×11, assembler 7299 ×38; alias objects 7291 ×3; import stubs 8034 ×9; imports ×645; linker 8447 ×3 |
| PE header offset | `0xf0` | `0xf0` |
| Import order | KERNEL32's thunks start with `CreateDirectoryA`; `TlsAlloc` is slot 37 | as SP4's runtime gives |
| Code generation | unoptimised (Standard edition) | unoptimised, with intrinsics; `/Ob0` |

Notes:
- **Rich header.** VC6's linker writes this undocumented table into the
  DOS stub: one entry per tool and build, with the number of objects that
  tool made. 113 = SDK 2.2b `hl.dsp`'s 98 C++ files, Spirit's `alias.cpp`
  and `movewith.cpp`, and T2DEF's 13 new files. 80 = SDK 2.2 `cl_dll.dsp`'s
  78 and T2DEF's `mp3.cpp` and `vgui_t2def.cpp`. The linker sizes the
  headers from what goes in, so a missing object, or one stamped by
  another edition, can move the PE header and every header byte after it.
- **Import order.** LINK sorts imports with the C runtime's `qsort`, and
  runtimes break ties differently. Linked on the VC6 CD's `MSVCRT.DLL`, the
  server swaps `TlsAlloc` and `CreateDirectoryA` (17 bytes differ). Wine's
  builtin gives retail's order, so T2DEF's linker probably ran on a later
  Windows `msvcrt.dll`. The client links on SP4's runtime; whether it also
  links identically on Wine's builtin was not recorded.
- **Identical-code folding.** LINK merges functions with identical code
  and keeps one copy. Which copy it keeps follows its `qsort` over the
  COMDATs (the per-function sections), so it depends on the COMDAT order
  inside each object and on the runtime. In the client, 11 functions
  depended on where one class sat in a header (see below).

## What is not known from retail

Compiled unoptimised code keeps no names apart from exports, save-table
field names and string literals. In T2DEF's own code, only those names are
T2DEF's; every other name is ours. Code taken from the SDK and Spirit keeps
its authors' names. This section lists what we chose and why.

### Names and other placeholders

The tree marks invented names with comments containing "name ours" or
"names ours": 116 lines in 44 files (list below). A search for
"placeholder" finds only two of Valve's own comments
([`cl_dll/hl/hl_weapons.cpp:51`](../../../blob/t2def/cl_dll/hl/hl_weapons.cpp#L51),
[`dlls/player.cpp:4505`](../../../blob/t2def/dlls/player.cpp#L4505)).

| Where | What | Why it is not known | Evidence used |
|---|---|---|---|
| 116 comment lines in 44 files | Names of classes, functions, members, locals, globals, `#define`s, enums, schedules | Not in the binary | None for the spelling. Some names were chosen for VC6's name hashes (next section) |
| `dlls/` `arcade`, `chumtoad`, `evilscientist`, `fakegordon`, `flyingcrowbar`, `friendly`, `gmanboss`, `joe`, `ka50`, `robolaser`, `securitycam`, `trigger_music`, `trooper` `.cpp`; `cl_dll/` `mp3.cpp`, `vgui_t2def.cpp` | The 15 new file names | The image holds no object names, and the server's PDB isn't available | The VC6 IDE sorts `hl.dsp` by file name, and that is the link order, so each name sorts into the gap where retail has the object. The data shows the object boundaries (`fakegordon`/`flyingcrowbar` and `joe`/`ka50` are two objects each) |
| The new files' `#include` lists | Which headers each file includes | Not in the code | Started from the SDK or Spirit file each resembles. Only where the `.pch` chain shows (`trooper.cpp`, below) is a list evidence |
| Formatting, bracing and comments of T2DEF's code | Text layout | Not in the code | Matters only through the precompiled header's inline order; one place had to be set (`aflock.cpp`, below) |
| [`cl_dll/vgui_TeamFortressViewport.h:1280`](../../../blob/t2def/cl_dll/vgui_TeamFortressViewport.h#L1280) | `Panel *m_pUnused`: its type | A member never set or read | Only its size, from the class layout |
| [`dlls/hl.dsp:46`](../../../blob/t2def/dlls/hl.dsp#L46) | `/O2`, Valve's flag, kept | The Standard edition ignores `/O2`, so T2DEF's own setting can't be seen | The code is unoptimised either way |

<details>
<summary>The "name ours" lines (file: lines)</summary>

- `cl_dll/hud_msg.cpp`: 176, 190
- `cl_dll/mp3.h`: 31
- `cl_dll/tf_defs.h`: 1132
- `cl_dll/vgui_t2def.cpp`: 7 (file, class, member and local names)
- `cl_dll/vgui_TeamFortressViewport.h`: 279, 1256, 1280
- `dlls/aflock.cpp`: 543, 575, 667, 668, 674, 677, 678, 861
- `dlls/arcade.cpp`: 34, 36, 46
- `dlls/basemonster.h`: 331, 342
- `dlls/bmodels.cpp`: 92, 107
- `dlls/cbase.h`: 452
- `dlls/chumtoad.cpp`: 17, 32, 76, 142, 193, 213
- `dlls/defaultai.cpp`: 984
- `dlls/effects.cpp`: 76, 167
- `dlls/evilscientist.cpp`: 17, 32, 33
- `dlls/fakegordon.cpp`: 17
- `dlls/flyingcrowbar.cpp`: 18, 93
- `dlls/friendly.cpp`: 17, 48, 217, 225, 242, 505
- `dlls/globals.cpp`: 40
- `dlls/gmanboss.cpp`: 17, 391, 504
- `dlls/hornet.cpp`: 254
- `dlls/joe.cpp`: 17, 38, 39, 40, 45, 47, 51, 118
- `dlls/ka50.cpp`: 17, 63, 72, 74, 75, 667, 1147, 1191
- `dlls/monstermaker.cpp`: 33, 43, 53, 54, 59, 386, 398
- `dlls/monsters.cpp`: 3484
- `dlls/plats.cpp`: 984
- `dlls/player.cpp`: 256
- `dlls/player.h`: 288
- `dlls/rat.cpp`: 31, 45
- `dlls/robolaser.cpp`: 19 (file, function and `#define` names)
- `dlls/rpg.cpp`: 225
- `dlls/schedule.h`: 76
- `dlls/scientist.cpp`: 32, 61, 62, 778, 829
- `dlls/scripted.cpp`: 42
- `dlls/securitycam.cpp`: 27, 42
- `dlls/skill.h`: 30
- `dlls/sound.cpp`: 2157
- `dlls/talkmonster.cpp`: 1332, 1564
- `dlls/talkmonster.h`: 147
- `dlls/triggers.cpp`: 39, 656, 4154, 4172, 4490, 4570, 4595, 4625, 4798
- `dlls/trooper.cpp`: 19, 48, 49, 344, 714
- `dlls/turret.cpp`: 103, 934, 938, 1030
- `dlls/weapons.cpp`: 1374
- `dlls/weapons.h`: 186
- `dlls/world.cpp`: 48

To list them: `git grep -n -i "name ours\|names ours\|are ours\|is ours\|ours)" t2def`,
minus four lines of SDK text ("ourselves", "ours to delete") in
`combat.cpp`, `player.cpp` and `vgui_SchemeManager.cpp`.
</details>

### Source chosen to steer the compiler

These choices give retail's code or layout. None of them changes
behaviour. The first two carry a `// sic` comment.

| Where | What | Why | Evidence |
|---|---|---|---|
| [`dlls/aflock.cpp:658`](../../../blob/t2def/dlls/aflock.cpp#L658) (`// sic`), `85bcd19` | Braces on single-statement bodies in the Bloater's berserk code (`FlockLeaderThink`) | `aflock.obj` is the first file and creates the `.pch`. VC6 compiles the `.pch`'s inline functions lazily: after each function it advances through them by about the amount of intermediate code it just compiled. With our first text, three header inlines came one step late. T2DEF's bracing is unknown; any three of the five bodies work | Retail's order of header inlines at the end of `aflock.obj`; the object is now byte-identical |
| [`dlls/sound.cpp:22`](../../../blob/t2def/dlls/sound.cpp#L22) (`// sic`), `eeb7ac5` | `extern int gmsgLBCaption;` placed between `cbase.h` and `weapons.h` | A line of code after an `#include` ends the `/YX` region there, so `sound.cpp`'s `.pch` is `extdll util cbase` and `soundent.cpp` reuses it. The declaration is needed anyway; only its place is chosen | Retail's `soundent.obj` ends with `CSoundEnt::ObjectCaps` (0x100b8c40), then `CSound::CSound` (0x100b8c50) |
| [`dlls/trooper.cpp:34-46`](../../../blob/t2def/dlls/trooper.cpp#L34-L46), `0491e40` | No `plane.h` in the include list (unused; the list was copied from `hgrunt.cpp`) | With it, `trooper.cpp`, `turret.cpp` and `util.cpp` each rebuild the `.pch` | Retail's `util.obj` ends with `Vector(float *)` (0x100db120) before the inline `UTIL_AxisRotationToAngles` (0x100db160) |
| [`cl_dll/vgui_TeamFortressViewport.h:465`](../../../blob/t2def/cl_dll/vgui_TeamFortressViewport.h#L465), `d03b7a2` | `CMenuHandler_SecurityCam` declared right after `CMenuHandler_StringCommand`, the class it copies | The order of header inlines in `vgui_TeamFortressViewport.obj`. Retail's header has more inline code between `CCommandMenu` and `CMenuHandler_StringCommandWatch`; this position gives it. Two other positions give 97.8% and 97.2% of the object | All 37,759 bytes of the object in place, and with them LINK's kept copies of 11 identical functions |
| [`dlls/friendly.cpp:260-261`](../../../blob/t2def/dlls/friendly.cpp#L260-L261), `c3e2290` | `CheckMeleeAttack2` declared before `CheckRangeAttack2` | Two identical empty inlines; VC6 emits a class's inlines in declaration order | Known from retail: `CFriendly`'s vtable points to `CheckMeleeAttack2` at 0x1003e1b0 and `CheckRangeAttack2` at 0x1003e1c0 |
| Local names: [`dlls/triggers.cpp:4490-4491`](../../../blob/t2def/dlls/triggers.cpp#L4490-L4491) (`06e60f7`, names and their order), [`dlls/triggers.cpp:656`](../../../blob/t2def/dlls/triggers.cpp#L656) `waittime` (`318af52`), [`dlls/triggers.cpp:4657`](../../../blob/t2def/dlls/triggers.cpp#L4657) the security camera's `Think` (`95b1454`), [`dlls/arcade.cpp:103-109`](../../../blob/t2def/dlls/arcade.cpp#L103-L109) (`9ba4fc8`), [`dlls/hornet.cpp:254`](../../../blob/t2def/dlls/hornet.cpp#L254) (`2548f72`), [`dlls/rpg.cpp:231`](../../../blob/t2def/dlls/rpg.cpp#L231) `pChaffEnt` (`f9e9e57`), [`cl_dll/vgui_t2def.cpp`](../../../blob/t2def/cl_dll/vgui_t2def.cpp#L7) (`5693509`) | Names of local variables | In unoptimised code VC6 orders a function's stack slots by a hash of the locals' names | Retail's frame offsets |
| Global names: [`dlls/globals.cpp:40-43`](../../../blob/t2def/dlls/globals.cpp#L40-L43) (`489cd88`), [`dlls/sound.cpp:2180-2181`](../../../blob/t2def/dlls/sound.cpp#L2180-L2181) the caption arrays (`4e05bb3`), [`dlls/trooper.cpp:48-49`](../../../blob/t2def/dlls/trooper.cpp#L48-L49) and [`:344`](../../../blob/t2def/dlls/trooper.cpp#L344) (`d6e0041`), [`dlls/monstermaker.cpp:53-54`](../../../blob/t2def/dlls/monstermaker.cpp#L53-L54) (`6844c5c`) | Names of new uninitialised globals | VC6 orders them in `.bss` by a hash of their names. For example, `g_iT2DEFMusicOn` would land in the wrong place, hence `g_iT2DefMusicOn` | Retail's addresses, e.g. `g_iT2DefCameraActive` 0x1013c67c, `g_iT2DefMusicOn` 0x1013c694, `g_iT2DefTextLanguage` 0x1013c6a0, `g_iT2DefSpeaker` 0x1013c6ac |

The name hashes and the lazy inline order are described in
[vc6-notes.md](vc6-notes.md).

### `// sic` lines that mark retail's own code

The tree has 24 `// sic` lines: `git grep -n -w sic t2def`. Two are the
steering lines above. The other 22 mark code that looks wrong but is what
retail does, so they are known from retail. They are kept for a later
port.

| Where | Retail's code |
|---|---|
| [`cl_dll/vgui_t2def.cpp:212`](../../../blob/t2def/cl_dll/vgui_t2def.cpp#L212) | Music status indexed by the "on" flag, so "on" reads "Disabled" |
| [`dlls/aflock.cpp:715`](../../../blob/t2def/dlls/aflock.cpp#L715) | `RANDOM_LONG( 0, 10 ) < 0.02`: an integer, so 1 in 11 |
| [`dlls/arcade.cpp:106`](../../../blob/t2def/dlls/arcade.cpp#L106), [`148`](../../../blob/t2def/dlls/arcade.cpp#L148) | `int iLen = 0;` stored, never read |
| [`dlls/arcade.cpp:118`](../../../blob/t2def/dlls/arcade.cpp#L118), [`121`](../../../blob/t2def/dlls/arcade.cpp#L121), [`155`](../../../blob/t2def/dlls/arcade.cpp#L155) | `sprintf` overflows (7 bytes into 6, 4 into 3) |
| [`dlls/chumtoad.cpp:156`](../../../blob/t2def/dlls/chumtoad.cpp#L156) | `pPlayer` used before its NULL test |
| [`dlls/chumtoad.cpp:357`](../../../blob/t2def/dlls/chumtoad.cpp#L357) | `pAttackSounds` indexed past its one entry |
| [`dlls/hornet.cpp:254`](../../../blob/t2def/dlls/hornet.cpp#L254) | An unused copy of `pev->velocity` (its name was chosen for the stack slot, above) |
| [`dlls/joe.cpp:51`](../../../blob/t2def/dlls/joe.cpp#L51) | `SCHED_JOE_HIDE_RELOAD` has the number of `SCHED_CANT_FOLLOW` |
| [`dlls/monstermaker.cpp:351`](../../../blob/t2def/dlls/monstermaker.cpp#L351) | A `!= 0.0` test before storing the time |
| [`dlls/plats.cpp:985`](../../../blob/t2def/dlls/plats.cpp#L985) | `vecAvel.x` tested twice |
| [`dlls/scientist.cpp:839`](../../../blob/t2def/dlls/scientist.cpp#L839) | `lukpain6` precached, never played |
| [`dlls/sound.cpp:2239`](../../../blob/t2def/dlls/sound.cpp#L2239), [`2320`](../../../blob/t2def/dlls/sound.cpp#L2320), [`2333`](../../../blob/t2def/dlls/sound.cpp#L2333), [`2444`](../../../blob/t2def/dlls/sound.cpp#L2444) | Caption loaders: the name is taken from the start of the line, the German loader checks the English count, German captions use the English index |
| [`dlls/trooper.cpp:408`](../../../blob/t2def/dlls/trooper.cpp#L408), [`842`](../../../blob/t2def/dlls/trooper.cpp#L842), [`913`](../../../blob/t2def/dlls/trooper.cpp#L913) | `pGun` unset, the burst events fall into the kick, `iHead` uninitialised |
| [`dlls/turret.cpp:964`](../../../blob/t2def/dlls/turret.cpp#L964) | A spin-down test right after `m_flSpinUpTime` is cleared |

### Build stand-ins

The build reproduces history the image records only in part. These are the
parts we had to choose.

| Where | What | Why | Evidence and status |
|---|---|---|---|
| [`tools/release/t2def.sh`](../tools/release/t2def.sh) and [`t2def_session.py`](../tools/release/t2def_session.py): the last build session | `sound.cpp`, `teamplay_gamerules.cpp` and `weapons.cpp` recompiled, in that order, after the 13 relinks and before the final link | Retail's `teamplay_gamerules.obj` and `weapons.obj` need the `extdll util cbase` `.pch`, which no full build gives them (see above) | Retail's PDB shows an incremental build. Three compiles are the fewest: both objects must be recompiled, and whichever comes first after the full build would rebuild the `.pch` itself, so a third file must go first and leave a `.pch` both can use. These give the same image in `sound.cpp`'s place: `alias`, `arcade`, `h_export`, `lights` and `trigger_music` (list `extdll util cbase`; `trigger_music` sorts after `teamplay_gamerules`, so it would mean an earlier build), and `securitycam` (`extdll util cbase player`). The script uses `sound.cpp` because it is T2DEF's code, sorts before the other two, and its include list has evidence (`soundent.obj`, above). **Not known:** which file T2DEF recompiled, and before which of links 2 to 15. The files that probably hold 1.1's two fixes (`monsterstate.cpp`'s removal of duplicate monsters; `doors.cpp`, which is Spirit 0.6.3's), and also `buttons.cpp`, `monsters.cpp` and `util.cpp`, can join the session without changing any object |
| [`tools/release/t2def.sh`](../tools/release/t2def.sh): 1 + 13 + 1 links | The PDB's link history | Retail gives only the PDB's creation time, its age and the final link time | The 13 relinks at the creation time are a stand-in; only their count is known. The session could have come before any of links 2 to 15; the script puts it before the last. `hl.pdb` itself is not part of the image and is not compared |
| [`tools/release/t2def.sh`](../tools/release/t2def.sh): server tools on Wine's builtin `msvcrt.dll` | The runtime LINK ran on | Its `qsort` gives retail's import order; the VC6 CD's `MSVCRT.DLL` swaps two imports | T2DEF's linker ran on some `msvcrt.dll` whose `qsort` breaks these ties like Wine 11.0's builtin. Which one is not known. Other Wine versions were not tested |
| [`tools/release/t2def.sh`](../tools/release/t2def.sh): client `/Ob0` | `/Ob0` added to `cl_dll.dsp`'s `/O2` | With `/O2` alone, our SP4 Standard compiler expands inline functions; retail has none expanded. Measured on the tree of that time: 862 of 1,728 client functions unmatched (236,167 bytes) with `/O2`, 160 with `/O2 /Ob0`. Now 0 with `/Ob0` | **Open.** Either T2DEF's project added `/Ob0`, or SP4's own front end didn't inline. Ours is `C1XX.DLL` 12.00.8867 from the SP4 disc; the objects' 8799 stamp comes from `C2.DLL` and doesn't identify the front end |
| [`dlls/hl.dsp:56`](../../../blob/t2def/dlls/hl.dsp#L56): `/pdb:".\Releasehl\hl.pdb"` (`d2e6f35`) with `/out:".\Releasehl\t2def.dll"` (`6c399e0`) | Output and PDB names | Retail's record names `Releasehl\hl.pdb`. VC6's LINK names the PDB after the output unless `/pdb` is given. So T2DEF's project either set `/pdb` or built `hl.dll` and renamed it | We kept `t2def.dll` (the name `liblist.gam` loads) and added `/pdb`. LINK doesn't write the output name into the image, so both ways give the same file |
| The source tree under `C:\hledit todesangst2\code` | Its layout | Retail attests only `code\dlls\Releasehl` | The rest is the SDK's layout (`hl.dsp` includes `..\engine`, `..\common`, `..\pm_shared`, `..\game_shared`). Other directories don't reach the image |

An earlier commit, `8314fc9`, got the same two objects from a project
change instead: it compiled `squeakgrenade.cpp` and `voice_gamemgr.cpp`
without `/YX`. It gave retail's code in a full build, but nothing suggests
T2DEF's project had that setting; the PDB points to build history instead.
`0833c0e` reverts it, so every source compiles with the project's `/YX`
again.

### Not yet audited

No complete audit has compared every changed file with its closest SDK or
Spirit source, preprocessor directive by directive. A quick check of
`git diff 6c399e0 0833c0e` found only conditionals carried with copied SDK
text (`ka50.cpp`'s `#ifndef OEM_BUILD` and `#if 1` from `apache.cpp`;
`gmanboss.cpp`'s two `#if 0` from `zombie.cpp`) and the include guards of
`cl_dll/mp3.h` and `cl_dll/fmod.h`.

## How identity was reached

The function matcher and the image comparison used during the work are not
included in this repository. "Game code" below means retail's non-library
functions: 4,726 in the server, 1,728 in the client. Percentages of game
code are shares of its bytes in exactly matching functions; "in place"
means bytes of the file identical at the same offset.

1. **Facts from the binaries.**
   - The Rich headers named the compilers and editions: server VC6 RTM
     Standard, client SP4 Standard. They also gave the object counts,
     which showed 15 new C++ files in the server and 2 in the client.
   - The client's code has intrinsics: 388 inline string operations and 59
     calls to `__CIsqrt`, `__CIsin`, `__CIcos` and `__CIatan2`. The server
     calls `strcpy` and `sqrt` as functions.
   - Builds of variants showed the server's flags: `CLIENT_WEAPONS`
     defined, and no exception handling (`/GX`), since 64 functions match
     only without it.
   - Stock SDK builds matched 22.0% of the server's `.text` and, with
     `/Od /Oi /Gy`, 63.9% of the client's.
2. **Link order.** Aligning retail's functions with a stock build's
   objects located all 15 new server files in the gaps between SDK
   objects.
3. **The Spirit base.** Spirit's source archives were fetched from the
   Wayback Machine (table below). T2DEF has 0.6.1's exports
   (`CEnvExplosion::Smoke`, no `explo_smoker`), so the base is 0.6.1, not
   0.6.2. Its `multi_manager` is 0.7.1's, merged in. We built this base
   (`b00697f`) like T2DEF's server and matched it against Laurie Cheers'
   own 0.6.1 `spirit.dll`, from the Wayback copy of his `Spirit061.exe`:
   99.2% of our functions' bytes are exact. At `6c399e0` the server's game
   code was 65.9% matched, the client's 71.8%.
4. **Base-class layouts** (`13e2427`): Spirit's 180-byte `CBaseEntity`,
   T2DEF's two new virtual functions and two player fields. Server 81.7%,
   client 76.2%.
5. **T2DEF's code**, written area by area (monsters, world, triggers,
   weapons, creatures, client): server game code 92.1% at `21bc29c`, 97.0%
   at `2dda381`, and 4,717 of 4,726 functions exact at `7a1ed84`. The
   client reached 1,717 of 1,728 once SP4's Standard compiler was used: it
   builds by-value temporaries in place, which RTM's front end never does.
   The share of each file in place grew with it: 6.6% (server) / 7.7%
   (client) at `13e2427`, 13.8% / 59.6% at `21bc29c`, 99.5% / 60.0% at
   `7a1ed84`.
6. **The server's compiler.** RTM's Standard back end, from the Learning
   Edition CD, gave the server retail's Rich header. Before, with the
   Professional edition's stamps, the header had 7 Rich entries instead of
   9 and only 92.4% of the header bytes matched.
7. **Client identity** (`d03b7a2`): moving one class in the viewport header
   gave retail's inline order in that object, and with it LINK's kept
   copies of 11 identical functions. Client 1,728 of 1,728; the release
   build is SHA-256 identical. Two findings came out of this, both in
   [vc6-notes.md](vc6-notes.md): VC6 compiles `/YX` inlines lazily by the
   amount of code just compiled, and LINK folds identical functions in two
   passes.
8. **Server functions.**
   - `ba63111`, `189ab32`: `Vector::Length2D` restored where we had written
     `Length`. The matcher masks call targets, so a call to the wrong one
     of two inlines of the same shape had looked right.
   - The `aflock.cpp` braces (`85bcd19`), the `friendly.cpp` declaration
     order (`c3e2290`) and the `trooper.cpp` include list (`0491e40`):
     4,722 of 4,726.
   - The last four are in `weapons.obj`. They, and the order of
     `teamplay_gamerules.obj`'s strings, need the `.pch` that the last
     build session gives (or, in a full build, the stand-in `8314fc9`).
9. **The server release build.** The tree at retail's PDB path, `/pdb`
   (`d2e6f35`), the PDB created at 2002-12-10 04:33:33 and linked 15
   times, the last build session, LINK at retail's time stamps, and LINK on
   Wine's builtin runtime for the import order. The result is identical to
   retail.

The Spirit archives the base is built from (fetched raw from
`http://spirit.valve-erc.com:80/files/<name>`, Laurie Cheers' download
directory; each SHA-1 equals the Wayback Machine's digest for the
capture):

| File | Capture | SHA-256 |
|---|---|---|
| `SpiritSource06.zip` | 20060212002402 | `8f4943a66d7978b2239840827fd746fa9d4ca7c20b4423e98cfddfaf14fdfd61` |
| `SpiritSource061.zip` | 20060212002438 | `44fe977b2419edb7032f716c76dab401aa00c544c29aad34958b9e51b133deba` |
| `SpiritSource071_diff.zip` | 20060212002524 | `38a962ea2d8ed350287423b111e665b65b1460d136de2151e680c829ca0e9765` |

The branch's history shows the layers: Valve's SDK releases up to 2.2b
(`d4fc7e4`, from <https://code.idtech.space/valve/halflife-sdk>), SDK 2.2's
shared files (`ddb0949`), Spirit 0.6 (`cd3a2d3`) and 0.6.1 (`6897090`) as
released, 0.7.1's `CMultiManager` (`b00697f`), the project settings
(`6c399e0`), then T2DEF's code.

## Check it yourself

**The retail hashes.** In your T2DEF 1.1 folder:

```sh
sha256sum dlls/t2def.dll cl_dlls/client.dll
```

Expected: `ba8fea63…` and `cbd18f93…` (full values in [Status](#status)).
The server inside the 1.1 patch has the same hash.

**A rebuild against your files.** Run the release script with `RETAIL`, as
in [Commands](#commands), and check its exit status:

```sh
RETAIL=/path/to/Half-Life/t2def VC_TOOLCHAINS=/path/to/toolchains \
  tools/release/t2def.sh ../t2def ../t2def-build; echo "exit $?"
```

Expected for both DLLs: `SHA-256 built:` equal to `SHA-256 retail:`,
`identical to retail`, and `no difference` from `cmp`; then `exit 0`. You
can also run `cmp` yourself on the paths the script prints.

**The build metadata.** This script (Python 3, standard library) prints the
PE header offset, the link time and the CodeView record of a DLL:

```python
import struct, sys, time
d = open(sys.argv[1], "rb").read()
u = lambda fmt, off: struct.unpack_from("<" + fmt, d, off)
utc = lambda s: time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime(s))
pe = u("I", 0x3c)[0]
nsec, stamp = u("HI", pe + 6)
print("PE header at", hex(pe), "- linked", utc(stamp))
dbg = u("I", pe + 24 + 96 + 6 * 8)[0]          # debug directory RVA
for s in range(nsec):                           # RVA to file offset
    vsize, va, rsize, raw = u("4I", pe + 24 + u("H", pe + 20)[0] + 40 * s + 8)
    if dbg and va <= dbg < va + vsize:
        rec = u("I", dbg - va + raw + 24)[0]    # PointerToRawData
        sig, age = u("II", rec + 8)
        name = d[rec + 16:d.index(b"\0", rec + 16)].decode()
        print(d[rec:rec + 4].decode(), name, "- signature", hex(sig), utc(sig), "- age", age)
```

Retail's `t2def.dll`, and the rebuild, give:

```
PE header at 0xf0 - linked 2002-12-16 01:56:59 UTC
NB10 C:\hledit todesangst2\code\dlls\Releasehl\hl.pdb - signature 0x3df56e9d 2002-12-10 04:33:33 UTC - age 15
```

Retail's `client.dll`, and the rebuild, have no debug record:

```
PE header at 0xf0 - linked 2002-12-06 05:06:23 UTC
```
