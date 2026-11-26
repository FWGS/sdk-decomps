# Gunman Chronicles

Gunman Chronicles is a stand-alone game on the Half-Life engine, made by
Rewolf Software and published by Sierra in November 2000. Its game code is
in two DLLs in the game's `rewolf` folder: the server `dlls\gunman.dll`
and the client `cl_dlls\client.dll`. The branch
[`gunman-chronicles`](../../../tree/gunman-chronicles) holds C++ source
reconstructed from them. It starts from Valve's Half-Life SDK history
(tag `v2.0`, 2000-06-05) and adds 512 commits.

File and line references below are to the branch at commit `6119524`.

## Status

Verified on 2026-11-26 with fresh release builds of `6119524`, by SHA-256
and `cmp` against the retail files.

| DLL | Size | Retail SHA-256 | Release build |
|---|---|---|---|
| `gunman.dll` (server) | 1,388,603 bytes | `e6e8d4497f11787889f12e7356157cbce98d380570805ca55faef3c07ce088f0` | **identical** |
| `client.dll` | 565,248 bytes | `4d2918498fb0d3aa40ede643beabea235925c0eaa6742130a686968686a51aed` | `96ef8769b4872ebbfe7f6c71ab5b8650a80cce19c48039a8839174b85b82062f`: 4 bytes differ |

The server is identical byte for byte: headers, time stamps, PDB record,
every section and the padding. It was first identical at `d4b4311`.

### What the client's differing bytes are

The four bytes are in one function, `CCustomizationMenu::ShowBeamgun` in
[`cl_dll/vgui_CustomizationMenu.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_CustomizationMenu.cpp#L888),
at file offsets `0x3df5d`, `0x3df5f`, `0x3df61` and `0x3df63`. After its
inner loop the function reloads two values from the stack. Retail reloads
`this` first and the row number second; our build reloads them the other
way round. The two instructions are the same, only swapped
([what is left](#what-is-left-in-the-client-and-why)).

Everything else in the file is retail's: the headers, the link time, the
missing debug directory, the Rich header and PE header offset, the import
and export tables, `.rdata`, `.data`, `.reloc` and every other byte of
`.text`.

Read this result with one caveat. The client is linked without `/debug`,
so the linker folds identical functions: when several functions compile to
the same bytes, LINK keeps one copy and points every caller at it. Which
copy it keeps depends on a sort over all functions in the link, including
functions that nothing calls and that LINK then drops. Retail was linked
with dead functions that left nothing in the image. To make the sort come
out as retail's, the branch holds 57 placeholder functions whose sizes,
relocation counts and places were found by a search
([placeholder functions](#placeholder-functions)). They show that the
known code plus some set of dead functions gives retail's file. They are
not Gunman's dead code.

## The retail files

The DLLs come from the retail game, version 1.0.0.0 (`sierra.inf` and
`readme.txt`, the latter dated 11/13/00). The installation used here ran
`H:\rewolf\install.exe` from drive H:, presumably the game CD: a Wise
installer of 200,079,586 bytes dated 11-13-2000. The installer writes
`INSTALL.LOG` into the game folder, with a CRC32 for every file it
copies:

| File in the game folder | Size | CRC32 in `INSTALL.LOG` | SHA-256 |
|---|---|---|---|
| `rewolf\dlls\gunman.dll` | 1,388,603 bytes | `5e59d944` | `e6e8d4497f11787889f12e7356157cbce98d380570805ca55faef3c07ce088f0` |
| `rewolf\cl_dlls\client.dll` | 565,248 bytes | `988a3cec` | `4d2918498fb0d3aa40ede643beabea235925c0eaa6742130a686968686a51aed` |

`INSTALL.LOG` dates both files 11-12-2000 16:58. The DLLs are not in this
repository (copyright). The CD image itself (label, hash) was not
recorded; the CRC32 values above identify the right files.

## Rebuilding

### What you need

- The toolchain folder from [setup.md](setup.md), with Visual C++ 6.0
  Professional without a service pack: `CL.EXE`, `C1.DLL`, `C1XX.DLL`,
  `C2.DLL` 12.00.8168 and `LINK.EXE`, `MSPDB60.DLL` 6.00.8168. Retail's
  Rich header names the same builds (compiler 8168, linker 6.00 8168).
  [compilers.md](compilers.md) says where the CD comes from and gives the
  file hashes.
- LINK must run on the VC6 CD's own `MSVCRT.DLL` 6.00.8168.0, set up as
  [setup.md](setup.md) describes. LINK sorts with the C runtime's `qsort`,
  and Wine's built-in `msvcrt.dll` orders equal keys differently.
  Relinking Gunman's server on Wine's built-in runtime changes 17 bytes of
  `.rdata` and 7 of `.text` in the import table.
- Wine 11.0 (win64 prefix), libfaketime 0.9.12, Python 3 (standard library
  only), `sha256sum` and `cmp`.
- The client also links Valve's `vgui.lib` from SDK 2.0. It is on the
  branch:
  [`utils/vgui/lib/win32_vc6/vgui.lib`](../../../blob/gunman-chronicles/utils/vgui/lib/win32_vc6/vgui.lib),
  SHA-256 `df1d49f874c87ba1fbcfe0d164bec90d7940e81341c01c001a701b85ef46677a`.
- To compare: the retail DLLs, for example an installed game folder.

The sources are stored with CRLF line ends, and the branch has no
`.gitattributes`. Check them out unchanged (`core.autocrlf` unset or
`false`, the default on Linux). Builds from LF files were not tested.

### Commands

From a checkout of `main`:

```sh
git worktree add ../gunman gunman-chronicles
VC_TOOLCHAINS=/path/to/toolchains tools/release/gunman-chronicles.sh ../gunman ../gunman-build
```

To compare with your own retail files as well, point `RETAIL` at the
game's `rewolf` folder, which holds `dlls\gunman.dll` and
`cl_dlls\client.dll`:

```sh
RETAIL=/path/to/Gunman/rewolf VC_TOOLCHAINS=/path/to/toolchains \
    tools/release/gunman-chronicles.sh ../gunman ../gunman-build server
```

The third argument is `server`, `client` or `both` (the default). For each
DLL the script prints the path of the built file, its SHA-256, retail's
SHA-256 and `identical to retail` or `differs from retail`. With `RETAIL`
set it also runs `cmp` and prints how many bytes differ.

Expected: the server `e6e8d449…ce088f0`, identical to retail; the client
`96ef8769…062f`, which differs (with `RETAIL`: 4 of 565248 bytes).
The exit code is 0 only if every DLL built is identical, so today `server`
exits 0 and `client` or `both` exit 1.

The built files are `OUT/wineprefix/drive_c/Gunman/Source/dlls/Releasehl/gunman.dll`
and `OUT/client/client.dll`; the compiler output is in `OUT/server.log`
and `OUT/client.log`.

On the machine used, a server build takes about 25 seconds and a client
build about 20.

### What the script does

For the server:

1. Copies the Wine prefix from `VC_TOOLCHAINS` to `OUT/wineprefix` (once;
   later runs reuse it).
2. Lays out `SRC` at `C:\Gunman\Source` on drive C: of that copy, as
   links into `SRC`; `dlls` and `cl_dll` are real folders of per-file
   links, so build output stays in `OUT` and out of the checkout. Retail's
   server names its PDB `C:\Gunman\Source\dlls\Releasehl\gunman.pdb`. The
   rest of the tree follows the SDK's layout, which the projects' include
   paths (`..\engine`, `..\common`, `..\pm_shared`, `..\game_shared`)
   expect.
3. Deletes `dlls\Releasehl`, so the final link creates a new PDB.
4. Runs a full build of `dlls\hl.dsp`, configuration `hl - Win32 Release`,
   with `tools/build_dsp.py` into `dlls\Releasehl`, output named `gunman`
   (`gunman.dll`, `gunman.pdb`). LINK alone runs under libfaketime with the
   clock at 2000-11-10 22:46:15 UTC (`0x3a0c7ab7`), retail's link time.

For the client: a full build of `SRC/cl_dll/cl_dll.dsp`, configuration
`cl_dll - Win32 Release`, into `OUT/client`, output `client.dll`, with
LINK at 2000-11-12 19:45:26 UTC (`0x3a0ef356`). It runs in the toolchain's
own Wine prefix. Its link line has no `/debug`, as retail's client has no
debug directory, so the client holds no PDB path and the folder it is
built in does not reach the file.

Then the script prints the hashes and compares them.

`build_dsp.py` builds a project the way the VC6 IDE does: it compiles the
sources in project order with the configuration's flags, minus the
IDE-only `/Fr` and `/FD`, and links in object order.

| | Server (`hl.dsp`) | Client (`cl_dll.dsp`) |
|---|---|---|
| Sources | 124: 121 C++, the three `pm_shared` C files | 70: 67 C++, the three `pm_shared` C files |
| Compiler flags | `/nologo /G5 /MT /W3 /Zi /O2 /I..\dlls /I..\engine /I..\common /I..\pm_shared /I..\game_shared /DNDEBUG /DWIN32 /D_WINDOWS /DQUIVER /DVOXEL /DQUAKE2 /DVALVE_DLL /YX` | `/nologo /MT /W3 /GX /Zi /O2 /I..\engine /I..\common /I..\pm_shared /I..\utils\vgui\include /I..\dlls /DWIN32 /DNDEBUG /D_WINDOWS /YX` |
| Per-file flags | `/Fp` for five files, `/YX"schedule.h"` for one (see [build stand-ins](#build-stand-ins)) | `/D "CLIENT_DLL"` for 13 files, `/Fp` for four |
| Link line | `kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:.\hl.def /incremental:no`, plus `/out`, `/map`, `/pdb`, `/implib` | `kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib winmm.lib ../utils/vgui/lib/win32_vc6/vgui.lib wsock32.lib user32.lib /nologo /subsystem:windows /dll /map /machine:I386` |

`/YX` is VC6's automatic precompiled header: the compiler saves its state
after a file's headers into a `.pch` file and reuses it for later files
whose include lists start the same way. As in the IDE, one `vc60.pch`
serves the whole build.

The server build gives one diagnostic, LINK's `warning LNK4070: /OUT:hl.dll
directive in .EXP differs from output filename`. It comes from `hl.def`'s
`LIBRARY hl`: LINK writes `gunman.dll`, and the export directory names
`hl.dll`, as retail's does.

Always build in full, as the script does. An incremental build replays
`/YX` differently and can move header inline functions to other places.

## What the image holds besides code

A DLL records more than its code. [reproducing.md](reproducing.md)
explains each part in general; for Gunman they are:

| Item | Server | Client | How the build gets it |
|---|---|---|---|
| Debug record | NB10 CodeView record `C:\Gunman\Source\dlls\Releasehl\gunman.pdb`, signature `0x3a0c7ab7`, age 1, in the 59 bytes after the last section. The file ends there, so the path's length sets the file size | None | The tree at `C:\Gunman\Source`, output folder `dlls\Releasehl` (the SDK's), name `gunman`, a new PDB from a full build. The client links without `/debug` |
| Time stamps | COFF header, export and debug directories, NB10 signature: `0x3a0c7ab7` (2000-11-10 22:46:15 UTC) | COFF header and export directory: `0x3a0ef356` (2000-11-12 19:45:26 UTC) | LINK runs with the clock frozen at retail's time stamp |
| Rich header (the linker's record of which tools built how many objects) | Utc12 C++ 8168 ×124, C 8168 ×123, Masm 6.13 ×30, and the libraries | Utc12 C++ 8168 ×78, C 8168 ×128, Masm 6.13 ×32, and the libraries | The same CD's compiler, libraries and runtime, plus retail's object count: the [placeholder objects](#placeholder-objects) |
| PE header offset (`e_lfanew`) | `0xe0` (`0xe8` with two objects fewer) | `0xe8` (`0xd8` with one object fewer) | LINK places the PE header by the object count; the placeholder objects give retail's |
| Imports | `KERNEL32.dll` only, 74 functions | `winmm`, `vgui`, `wsock32`, `user32`, `kernel32`, in that order | The client's link line lists `user32.lib` after `wsock32.lib`, which gives retail's order (`790ca3e`); LINK on the CD's runtime |
| Exports | 778 names, DLL name `hl.dll` | 542 names | `hl.def` and the `EXPORT` markers. 17 exported string literals and the export names needed in-class definitions and private callbacks (`320d5ba`, `d039128`) |
| Identical functions | Not folded: `/debug` makes LINK keep everything, so 40 bytes of `turret.obj` that nothing calls are in the image | Folded; LINK keeps retail's copy in all 85 groups that show it | Nothing to do for the server. For the client, the [placeholder functions](#placeholder-functions) |
| Link order | Project order, with Gunman's own files in retail's object order, not sorted | Project order | `build_dsp.py` links in project order |
| Padding | Zero | Zero | Wine's memory is zeroed |

Two more things decide the bytes and are not in the PE header:

- **Precompiled-header state.** What a file's `.pch` holds decides where
  the compiler emits header inline functions in the object. In a full
  build CL creates and reuses `vc60.pch` in project order, so most objects
  come out as in retail. Nine objects show a different state; see
  [build stand-ins](#build-stand-ins).
- **Uninitialised data (`.bss`) order.** VC6 orders a file's
  uninitialised globals by a hash of their names:
  `h = h*4 + (h>>4) + c` over the name, bucket `(h ^ (h>>16)) & 1023`,
  ascending buckets, newest declaration first within a bucket, then the
  zero-initialised ones in declaration order. Names we chose were picked
  to land where retail's globals are. [vc6-notes.md](vc6-notes.md#bss-order)
  has the details.

## What in the source and build is not known from retail

Every server function compiles to retail's bytes at retail's address, but
its source text is only one spelling that does so. This section lists
what exists for another reason: stand-ins for facts the binaries don't
hold, lines that steer the compiler, and retail's own oddities kept on
purpose. The commit messages hold the full reasoning and the experiments.

All such lines carry a `// sic` comment or the word "placeholder":

```sh
git grep -n -w sic gunman-chronicles
git grep -n -i placeholder gunman-chronicles
```

- Server files (everything outside `cl_dll/`): 229 `// sic` lines. 39
  steer code generation, 5 are `.bss` placeholders, 2 mark unused `const`
  globals, and 183 mark retail behaviour. Files marked "both" below are
  also compiled into the client.
- `cl_dll/`: 74 `// sic` lines. 15 steer code generation, 16 introduce
  blocks of placeholder functions, 2 mark what those displaced (the stub
  order and the removed `EV_FireShotGunDouble`), 5 mark `.bss` stand-ins,
  and 36 mark retail behaviour.
- The placeholder search also finds the three placeholder objects, their
  project lines and the placeholder functions. Its other hits are Valve's comments
  (`dlls/player.cpp:4880`, `cl_dll/hl/hl_weapons.cpp:58`).

### Placeholder objects

| File | What | Evidence |
|---|---|---|
| [`dlls/placeholder_object1.cpp`](../../../blob/gunman-chronicles/dlls/placeholder_object1.cpp), [`dlls/placeholder_object2.cpp`](../../../blob/gunman-chronicles/dlls/placeholder_object2.cpp); [`dlls/hl.dsp:708, 712`](../../../blob/gunman-chronicles/dlls/hl.dsp#L708-L712) | Two empty C++ files, last in the project. Names ours | Retail's Rich header counts 124 C++ objects; the server's code shows 122. Relinking the same objects with two empty ones changes only the Rich header and the PE header offset, both to retail's, and every section stays (`f9caa39`). The two sources left no code or data, so their names, contents and places are unknown. They come last because a source without `#include` writes an empty `.pch` that every later file would reuse; placed earlier, they moved header inlines in later objects |
| [`cl_dll/placeholder_object.cpp`](../../../blob/gunman-chronicles/cl_dll/placeholder_object.cpp); [`cl_dll/cl_dll.dsp:496`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L496) | One empty C++ file between `vgui_HudDeathNotice.cpp` and `vgui_HudID.cpp`. Name ours | Retail's Rich header counts 78 C++ objects; the client's code shows 77. Retail's objects from `vgui_HudID` on show the empty-`.pch` state that a source without `#include` leaves, which places it (`0c1e33e`). Recompiling the 11 objects after it with that empty `.pch` reproduces all 11 |

### Placeholder data

Retail has a few bytes of data that nothing reads or writes, and a few
initialisers of globals nothing uses. The source gives each a global of
the right size and kind, with a name of ours unless noted.

| File:line | What | Evidence |
|---|---|---|
| [`dlls/bandit.cpp:42-44`](../../../blob/gunman-chronicles/dlls/bandit.cpp#L42-L44) | `static int s_iUnknown;` | 4 bytes of `.bss` between the two gib sprites (`ccaf4db`) |
| [`dlls/beak.cpp:41-43`](../../../blob/gunman-chronicles/dlls/beak.cpp#L41-L43) | `static int s_iUnknown1;` | 4 bytes after `g_iGoreBirth`, before `critter.obj`'s data (`ccaf4db`) |
| [`dlls/client.cpp:555-557`](../../../blob/gunman-chronicles/dlls/client.cpp#L555-L557) | `static int s_iUnknown;` | 4 bytes after the two sprites (`ccaf4db`) |
| [`dlls/rustbot.cpp:39-41`](../../../blob/gunman-chronicles/dlls/rustbot.cpp#L39-L41) | `static int s_iUnknown;` | 4 bytes after `g_fRustbotQuestion`, before `beak.obj`'s (`ccaf4db`) |
| [`dlls/vehicletank.cpp:40-42`](../../../blob/gunman-chronicles/dlls/vehicletank.cpp#L40-L42) | `static int s_iUnknown;` | 4 bytes before `MoveTank`'s `s_iStuck` (`1545ca2`) |
| [`dlls/rustflier.cpp:415-419`](../../../blob/gunman-chronicles/dlls/rustflier.cpp#L415-L419), [`dlls/torch.cpp:135-138`](../../../blob/gunman-chronicles/dlls/torch.cpp#L135-L138) | Unused `const Vector` globals | Retail has their empty static initialisers. `const` gives them no storage, as retail's two objects have no `.bss` (`96388c8`) |
| [`cl_dll/ammo.cpp:291`](../../../blob/gunman-chronicles/cl_dll/ammo.cpp#L291) | `int m_HUD_glow_aicore;`, never loaded | A 4-byte slot after `gWR`; the `hud.txt` sprite name hashes into it (`41b6aae`) |
| [`cl_dll/ammo.cpp:292`](../../../blob/gunman-chronicles/cl_dll/ammo.cpp#L292) | `static int iSpriteUnknown;` | 4 bytes after `m_HUD_number` (`41b6aae`) |
| [`cl_dll/hud.cpp:37-39`](../../../blob/gunman-chronicles/cl_dll/hud.cpp#L37-L39) | `static int s_iUnknown[3];` | 12 bytes before `cl_lw` (`f08dc72`) |
| [`cl_dll/tri.cpp:53-58`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L53-L58) | `static int s_iUnknown_1[3]; static int s_iSpare[4];` | A 12-byte and a 16-byte slot in `tri.obj` (`2882929`) |
| [`cl_dll/tri.cpp:45-51`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L45-L51) (not marked `sic`) | Two unread `vec3_t` globals and `float g_flUnused = 1.0;` | Retail's two extra static initialisers in `tri.obj` (`8a8d907`), and an initialised 1.0 that nothing reads as the object's first data (`e871d39`) |
| [`cl_dll/entity.cpp:427`](../../../blob/gunman-chronicles/cl_dll/entity.cpp#L427) | The SDK's `static float lasttime;` in the otherwise empty `Beams()` | SDK text, kept: retail has its 4 bytes before `beams[]` (`b90eec6`) |

### Placeholder functions

Retail's client was linked with functions that nothing calls. LINK removed
them, so their code is gone, but four traces remain:

- **`.reloc`'s size.** LINK sizes the base relocation section with 2
  bytes for every absolute address in every function of the link, dead
  ones included ([reproducing.md](reproducing.md#the-reloc-size)).
  Retail's `.reloc` is 160 bytes larger than its 23,755 live relocations
  need, so dead functions held 80 absolute addresses.
- **A constant.** Retail's double 3.0 sits among `ev_hldm.obj`'s
  constants, after `Font`'s vtable and before `EV_FireGauss`'s, but only
  `CGaussPistol::ChargeSniper` uses it. A constant used by a dead
  function stays in the image if live code elsewhere uses the same value,
  and it stays with the first object that defined it. So a dead function
  of `ev_hldm.cpp` used 3.0.
- **Header inlines.** `ev_hldm.cpp` creates the client's `.pch`, and VC6
  then emits the `.pch`'s inline functions as the file's own code grows.
  Retail's order of them needs more intermediate code (the form the
  compiler's front end hands to its optimiser) than the live functions
  have at two places: before `EV_TurretBullets` ends, and by the end of
  `EV_MonsterSentry`.
- **The copies LINK kept.** Retail's image shows, for 85 groups of
  identical functions, which copy LINK kept. That follows from a sort
  over every function in the link, dead ones included
  ([vc6-notes.md](vc6-notes.md#links-identical-code-folding)). With the
  live code and the dead code the SDK supplies, our link kept retail's
  copy in 45 of the 85.

The first three give totals: 80 absolute addresses, one use of 3.0, two
amounts of intermediate code. The fourth says only that retail's sort had
other entries than ours. A dead function enters that sort as a size, a
relocation count and a position in the link, and as nothing else.

The branch has 57 placeholder functions (`6b5c736`): at three places in
`ev_hldm.cpp`, and at the top or the end of ten other client files. Each
block starts with a `// sic: placeholders` comment, and each function
carries its key as `// key (size, relocations)`:

| File:line | Functions | Keys (size, relocations) |
|---|---|---|
| [`cl_dll/ev_hldm.cpp:890`](../../../blob/gunman-chronicles/cl_dll/ev_hldm.cpp#L890) | 2, before `EV_TurretBullets` | (32, 4), (16, 1) |
| [`cl_dll/ev_hldm.cpp:968`](../../../blob/gunman-chronicles/cl_dll/ev_hldm.cpp#L968) | 1, after `EV_MonsterSentry`, padded with unused locals | (16, 0) |
| [`cl_dll/ev_hldm.cpp:1242`](../../../blob/gunman-chronicles/cl_dll/ev_hldm.cpp#L1242) | 13, after `EV_MinigunHeat` | (48, 0), (32, 3), (16, 2), (16, 1), (16, 0), (32, 1), (240, 11) with 11 absolute addresses and the double 3.0, (192, 10), (16, 1), (16, 0), (32, 0), (32, 3), (16, 1) |
| [`cl_dll/hl/hl_baseentity.cpp:64`](../../../blob/gunman-chronicles/cl_dll/hl/hl_baseentity.cpp#L64), [`425`](../../../blob/gunman-chronicles/cl_dll/hl/hl_baseentity.cpp#L425) | 2 before the stubs, 4 after them | (16, 0), (16, 0); (16, 3), (32, 2), (16, 1), (128, 9) |
| [`cl_dll/hl/hl_events.cpp:111`](../../../blob/gunman-chronicles/cl_dll/hl/hl_events.cpp#L111) | 1, at the end | (96, 12) |
| [`cl_dll/zoom.cpp:29`](../../../blob/gunman-chronicles/cl_dll/zoom.cpp#L29), [`272`](../../../blob/gunman-chronicles/cl_dll/zoom.cpp#L272) | 1 at the top, 3 at the end | (16, 0); (16, 1), (16, 1), (32, 4) |
| [`cl_dll/statusbar.cpp:254`](../../../blob/gunman-chronicles/cl_dll/statusbar.cpp#L254) | 5, at the end | (16, 1), (16, 0), (912, 6), (336, 4), (16, 1) |
| [`cl_dll/train.cpp:87`](../../../blob/gunman-chronicles/cl_dll/train.cpp#L87) | 5, at the end | (16, 1), (16, 0), (32, 1), (128, 2), (16, 1) |
| [`cl_dll/util.cpp:135`](../../../blob/gunman-chronicles/cl_dll/util.cpp#L135) | 2, at the end | (48, 0), (1264, 59) with 59 absolute addresses |
| [`cl_dll/vgui_CustomizationMenu.cpp:43`](../../../blob/gunman-chronicles/cl_dll/vgui_CustomizationMenu.cpp#L43) | 4, at the top | (16, 0), (1184, 40) with 10 absolute addresses, (16, 2), (16, 1) |
| [`cl_dll/vgui_HudDeathNotice.cpp:42`](../../../blob/gunman-chronicles/cl_dll/vgui_HudDeathNotice.cpp#L42), [`321`](../../../blob/gunman-chronicles/cl_dll/vgui_HudDeathNotice.cpp#L321) | 1 at the top, 9 at the end | (16, 0); (96, 12), (80, 7), (112, 14), (656, 1), (16, 1), (32, 4), (800, 1), (480, 5), (80, 6) |
| [`cl_dll/vgui_teammenu.cpp:391`](../../../blob/gunman-chronicles/cl_dll/vgui_teammenu.cpp#L391) | 1, at the end | (16, 1) |
| [`cl_dll/view.cpp:1025`](../../../blob/gunman-chronicles/cl_dll/view.cpp#L1025) | 3, at the end | (16, 1), (48, 0), (16, 3) |

The bodies mean nothing, and nothing runs them. A call of `InitInput()`
supplies one relative relocation, a read of a word of `gEngfuncs` one
absolute address, stores through the argument fill the size, and unused
locals add intermediate code that compiles to nothing. Together the
placeholders hold the 80 absolute addresses (11 + 59 + 10, in the three
functions marked above) and the use of 3.0, and they stay inside what the
inline order allows. Measured for this set: at most 274 bytes of
intermediate code before `EV_TurretBullets` (283 emit `TextPanel`'s copy
constructor too early), and a cumulative 812 by the end of the one after
`EV_MonsterSentry` (848 emits `InputStream::operator=` too early). With
them LINK keeps retail's copy in all 85 groups.

All of them are in existing files because of one more trace: retail's
Rich header counts 78 C++ objects, which the client already has, so no
object could be added.

The keys and places come from a search with a model of LINK's sort
([how it was done](#which-copy-of-identical-functions-link-keeps)). They
are one fit among very many. Other sets of dead functions would do as
well, and nothing says that retail's had these sizes or sat in these
files. Two things changed for the fit:

- SDK 2.0's `EV_FireShotGunDouble`, which the branch had kept as dead
  code (`1eeeef3`), is gone. `events/shotgun2.sc` ships with the game, but
  `Game_HookEvents` doesn't hook it, and nothing in retail's image needs
  the function: LINK drops a dead function's string literals with it. The
  placeholders were fitted without it. Three earlier placeholders
  (`6e7096e`) are replaced.
- The stubs of
  [`cl_dll/hl/hl_baseentity.cpp`](../../../blob/gunman-chronicles/cl_dll/hl/hl_baseentity.cpp) are no
  longer in SDK 2.2b's order
  ([other choices](#other-choices-the-evidence-leaves-open)).

Twelve event scripts ship that nothing hooks (`shotgun2`, `shotgunidle`,
`gaussidle`, `minigunidle`, `minigunspin`, `minigunbody`, `sniperbody`,
`swingfists`, `zoom`, `rapidfire`, `chargefire`, `beamcharge`), so retail
probably had handlers for some of them. No SDK handler fits the gaps in
`ev_hldm.cpp`: SDK 2.0's are 2,189 to 9,505 bytes of intermediate code,
too big for the early gaps; SDK 2.1's `EV_StopPreviousGauss` fits the
first gap but moves two strings into `.data` early.

### Build stand-ins

| Where | What | Why | Evidence |
|---|---|---|---|
| [`dlls/hl.dsp:231`](../../../blob/gunman-chronicles/dlls/hl.dsp#L231), [`468`](../../../blob/gunman-chronicles/dlls/hl.dsp#L468), [`581`](../../../blob/gunman-chronicles/dlls/hl.dsp#L581), [`598`](../../../blob/gunman-chronicles/dlls/hl.dsp#L598), [`619`](../../../blob/gunman-chronicles/dlls/hl.dsp#L619) | `# ADD CPP /Fp".\Releasehl\<file>.pch"` for `ggrenade.cpp`, `microraptor.cpp`, `demoman.cpp`, `friendlygunman.cpp`, `banditgrunt.cpp`; `microraptor.cpp` also `/YX"schedule.h"` (a per-file "through header") | Retail compiled these five after other files than their project-order neighbours. With a `.pch` of their own, made fresh at each compile, the first four put their vtable-only header inlines at the end of the object, as retail does. `microraptor.cpp`, through `schedule.h`, compiles `squadmonster.h` after the `.pch`, so `CSquadMonster::MySquadMonsterPointer` follows the factory function, as in retail | `bd5439f`: `.text` at retail's addresses 58.4% → 59.4%. The natural cause is an incremental final build, but no history of up to three incremental builds fits all five, and a real three-step build fixed four while moving three other objects. So the history is not reproduced; this stands in for it. Retail's project is not known to have had these lines |
| [`cl_dll/cl_dll.dsp:199`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L199), [`214`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L214), [`229`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L229), [`244`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L244) | `/Fp` of their own for `gausspistol.cpp`, `minigun.cpp`, `shotgun.cpp`, `spchemicalgun.cpp` | Retail compiled each with its whole include list in the `.pch`, which compiling the file alone gives; project order reuses `fists.cpp`'s | `f8ffba2`: from `beamgun.obj` to `hl_baseentity.obj` the code became one run in retail's order (281 functions) |
| [`cl_dll/cl_dll.dsp:124`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L124) and the other Release blocks to [`304`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L304) | Per-file `/D "CLIENT_DLL"` for `ev_hldm.cpp`, the four `hl_*.cpp` files and the eight weapon files | The weapon files' `#ifndef CLIENT_DLL` guards around save tables and server-only code. The structure is shown by the binaries: the client's copies of these files lack exactly those parts | The macro is not Valve's for this project: the `cl_dll.dsp` of SDK 2.0 and 2.1 defines no `CLIENT_DLL`, SDK 2.2 and later define it project-wide (never per file), and the client's `pm_shared.c` was built without it. Guarding with `VALVE_DLL` (Valve's server define) would carry the same split without a project change; not tried |
| [`dlls/hl.dsp:708, 712`](../../../blob/gunman-chronicles/dlls/hl.dsp#L708-L712), [`cl_dll/cl_dll.dsp:496`](../../../blob/gunman-chronicles/cl_dll/cl_dll.dsp#L496) | The placeholder objects' positions | See [placeholder objects](#placeholder-objects) | |
| The release script | The tree at `C:\Gunman\Source` | Only `C:\Gunman\Source\dlls\Releasehl` is attested, by the server's PDB path. The rest is the SDK's layout | Other folders don't reach the image |
| The release script | A deleted `Releasehl` and a full build | Retail's PDB has age 1, so the final link created it | Gives retail's NB10 record. What else was in Gunman's `Releasehl` is unknown |
| The release script | The two link times | Retail's own `TimeDateStamp` values | |
| The toolchain | LINK on the VC6 CD's `MSVCRT.DLL` | LINK's `qsort` decides the server's import order | Which runtime Gunman's LINK ran on is unknown; this one gives retail's file |

### Code-generation steering (`// sic` lines)

These lines are needed for retail's bytes, but their text is ours: retail
had something with the same effect that the binary doesn't show. None
changes behaviour; each is dead code, an unused local, a no-op or an
equivalent spelling. Many work through VC6's inline budget: the compiler
inlines small functions such as `Vector` operators only while a
per-function budget lasts, and dead statements spend it
([vc6-notes.md](vc6-notes.md#the-inliners-budget) explains it and the other
rules behind these lines).

**Server** (39 lines):

| File | Lines | Function | What | Effect | Commit |
|---|---|---|---|---|---|
| [`dlls/bandit.cpp`](../../../blob/gunman-chronicles/dlls/bandit.cpp#L621) | 621 | `CBaseBandit::FireGauss` | `TraceResult tr, beam_tr;`, `beam_tr` unused (from the SDK's `CGauss::Fire`) | Its constructor spends the budget, so the beam end's sum calls the out-of-line `Vector( float, float, float )` | `c190819` |
| [`dlls/bandit.cpp`](../../../blob/gunman-chronicles/dlls/bandit.cpp#L635) | 635 | `CBaseBandit::FireGauss` | Unused muzzle point, initialised where declared | Budget, with `beam_tr` | `c190819` |
| [`dlls/beak.cpp`](../../../blob/gunman-chronicles/dlls/beak.cpp#L637-L640) | 637-640, 699 | `CBeak::StartTask` | A file-scope `inline float VecY( const Vector &vec )` and an unused `float flEnemyY = VecY( vecEnemyVelocity );` | x87 operand order: y gets a number in an earlier 32-bit word than z, which gives retail's y, z, x in the inlined `Length()` and z ahead of the length in `Normalize()` | `d4b4311` |
| [`dlls/beak.cpp`](../../../blob/gunman-chronicles/dlls/beak.cpp#L709-L711) | 709-711 | `CBeak::StartTask` | `vecJump.z = vecJump.z * 1;` | Term order of `vecJump.Length()` | `dd4508a` |
| [`dlls/beamgun.cpp`](../../../blob/gunman-chronicles/dlls/beamgun.cpp#L1788-L1795) (both) | 1788-1795 | `CLightningBug::TrackTargetBug` | `int iDead;` and five dead stores | Budget: `Length()` inlined, `operator*` called. Four or six give other code | `ca93bf1` |
| [`dlls/butterfly.cpp`](../../../blob/gunman-chronicles/dlls/butterfly.cpp#L319) | 319 | `CDragonfly::FlockLeaderThink` | Dead `vecDir = pev->velocity;` | Budget for the inlined `operator+` in the ground bump and its `Length` order | `3e7c9be` |
| [`dlls/camera.cpp`](../../../blob/gunman-chronicles/dlls/camera.cpp#L117-L137) | 117, 123-137 | `CBaseTurn::TurnTo` | The current angle read through `int *piAngle`, the float sum from the member; the copy turned in place | VC6 never merges a member read with a read through a local pointer to it, so it loads the member twice (`fild [m]`, `mov edi, [m]`) as retail does | `b68bb86` |
| [`dlls/chopper.cpp`](../../../blob/gunman-chronicles/dlls/chopper.cpp#L464) | 464 | `CChopper::HuntThink` | Unused `vecEst`, the SDK's `CApache::HuntThink` text | Budget | `750e83d` |
| [`dlls/chopper.cpp`](../../../blob/gunman-chronicles/dlls/chopper.cpp#L1064) | 1064 | `CMantaShip::MantaThink` | Dead `vecEst = pev->velocity;` at the end | Budget that keeps the steering `Normalize()`s inlined | `9a9a296` |
| [`dlls/cluster.cpp`](../../../blob/gunman-chronicles/dlls/cluster.cpp#L85) | 85, 200 | `CClusterGod::ShootClusterGrenades`, `CClusterExplosion::ClusterExplode` | Unused `vecEnd` | Budget that the initialised angles cost | `abd1879`, `2fc473b` |
| [`dlls/dml.cpp`](../../../blob/gunman-chronicles/dlls/dml.cpp#L950-L952) (both) | 950-952 | `CDml::FindTarget` | Dead `Vector vecDir = vecForward;` | Operand order in the x sum of `v_angle + punchangle` | `d09f17b` |
| [`dlls/dmlgrenade.cpp`](../../../blob/gunman-chronicles/dlls/dmlgrenade.cpp#L392) (both) | 392, 419 | `CDmlHandGrenade::ShootClusters` | Unused `vecEnd` | Budget, as in `cluster.cpp` | `abd1879`, `2fc473b` |
| [`dlls/dmlgrenade.cpp`](../../../blob/gunman-chronicles/dlls/dmlgrenade.cpp#L663-L672) (both) | 663-672, 725-729 | `CDmlGrenade::WeaponIdle` | The SDK hand grenade's throw-animation choice under `if ( 0 )` (a guess), and the SDK's tail after the returns | A sweep of dead stores showed retail needs 23 to 27 more units of dead code | `fbba331` |
| [`dlls/dmlrocket.cpp`](../../../blob/gunman-chronicles/dlls/dmlrocket.cpp#L310) | 310, 395-396 | `CDmlRocket::TrackTarget` | Dead `float flDelta = 0;`; unused `Vector vecVelocity( pev->velocity );` | Budget | `1589aee` |
| [`dlls/dmlrocket.cpp`](../../../blob/gunman-chronicles/dlls/dmlrocket.cpp#L1081) | 1081 | `CDmlRocket::BecomeTripmine` | `tr2` in `TraceResult tr, tr2;`, unused | Its constructor spends the budget; retail calls `operator-` out of line in the last statement | `3f29d7c` |
| [`dlls/endboss.cpp`](../../../blob/gunman-chronicles/dlls/endboss.cpp#L157-L161) | 157-161 | `CEndBossKataball::KataTouch` | The event's angles spelled `(float *)&g_vecZero[0]` (same address) | Written as `(float *)&g_vecZero` twice, VC6 shares one address constant, which swaps the registers of `this` and the class-name pointer | `ec294f4` |
| [`dlls/endboss.cpp`](../../../blob/gunman-chronicles/dlls/endboss.cpp#L228-L235) | 228-235 | `CEndBossKataball::AccelerateThink` | `vecDir.x = vecDir.x * 1; vecDir.y = vecDir.y * 1;` in one skill case | Naming the components gives retail's multiplies in the inlined `Normalize()` calls | `22ca208` |
| [`dlls/endboss.cpp`](../../../blob/gunman-chronicles/dlls/endboss.cpp#L398) | 398 | `CEndBossRocket::AccelerateThink` | `vecDir.x = vecDir.x * 1;` | The same, in the `Normalize()` after it | `22ca208` |
| [`dlls/gausspistol.cpp`](../../../blob/gunman-chronicles/dlls/gausspistol.cpp#L978) | 978 | `CGaussPistol::FireBolt` | Unused `Vector vecAiming = gpGlobals->v_forward;` (the SDK gauss's) | The angles for `UTIL_MakeVectors` go through the out-of-line copy constructor | `0148f4b` |
| [`dlls/gausspistol.cpp`](../../../blob/gunman-chronicles/dlls/gausspistol.cpp#L1138) | 1138 | `GaussShot` | `TraceResult tr, beam_tr;`, as in `CGauss::Fire` | `tr`'s stack slot; out-of-line constructors for the muzzle point | `c70f629` |
| [`dlls/grenade_chem.cpp`](../../../blob/gunman-chronicles/dlls/grenade_chem.cpp#L485-L487) | 485-487 | `CChemGrenade::BouncyChemicalTouch` | `pev->velocity * 1 / iDamp` | The `* 1` folds away but leaves retail's extra `Vector` temporary before the division | `82c677d` |
| [`dlls/gunner.cpp`](../../../blob/gunman-chronicles/dlls/gunner.cpp#L812) | 812 | `CGunner::HandleAnimEvent` | Unused `vecForward = vecForward * 100;`, reusing the local | The function matches only with the local reused | `a81304a` |
| [`dlls/gunner.cpp`](../../../blob/gunman-chronicles/dlls/gunner.cpp#L937) | 937 | `CGunner::GibMonster` | `vecDir.Normalize();`, result discarded | Retail's layout of the function | `616a6d3` |
| [`dlls/maggot.cpp`](../../../blob/gunman-chronicles/dlls/maggot.cpp#L335) | 335 | `CMaggot::RunAI` | `vecDir.x = vecDir.x * 1;` | Operand order in the dot product | `c30852c` |
| [`dlls/player.cpp`](../../../blob/gunman-chronicles/dlls/player.cpp#L3960-L3963) | 3960-3963 | `CBasePlayer::CheatImpulseCommands` | Seven dead int stores | VC6 loads `view_ofs.x` before `origin.x` in impulse 202's start point only with this much more code; exactly seven work | `2ffd851` |
| [`dlls/rustbot.cpp`](../../../blob/gunman-chronicles/dlls/rustbot.cpp#L1428-L1431) | 1428-1431 | `CRustbot::AimHead` | `int *piHeadTurn = &m_iHeadTurn; int iHeadTurn = *piHeadTurn;` | Two loads of the member, as in `TurnTo` | `b68bb86` |
| [`dlls/rustflier.cpp`](../../../blob/gunman-chronicles/dlls/rustflier.cpp#L383-L384) | 383-384 | `CRustFlier::HuntThink` | `if ( 0 ) ResetStrafe( );` | Retail inlines the unused estimate's `Normalize()` only with one more call in the function | `2ee1c13` |
| [`dlls/rustflier.cpp`](../../../blob/gunman-chronicles/dlls/rustflier.cpp#L392) | 392 | `CRustFlier::HuntThink` | The chopper's unused `vecEst` | Retail calls `Length` out of line for its `Normalize()`, so this one shows in retail's code | `1717452`, `2ee1c13` |
| [`dlls/tube.cpp`](../../../blob/gunman-chronicles/dlls/tube.cpp#L412) | 412 | `CTube::ShootRocket` | Unused `Vector vecOrigin = pev->origin;` | Leaves the copy of `v_forward * 500` out of line | `98b4fff` |
| [`dlls/tuberocket.cpp`](../../../blob/gunman-chronicles/dlls/tuberocket.cpp#L281) | 281 | `CTubeRocket::TrackTarget` | Dead `Vector vecVelocity = pev->velocity;` | Leaves the sum's copy out of line | `1bb48dd` |
| [`dlls/vehicletank.cpp`](../../../blob/gunman-chronicles/dlls/vehicletank.cpp#L734-L735) | 734-735 | `CVehicleTank::Drive` | A dead hand-written dot product into `flDot`, overwritten on the next line | About 11 budget units, inside a 3-unit window; retail's x87 orders of the `Length()` after `turned:` and of the bump dot product | `05659cb` |
| [`dlls/vehicletank.cpp`](../../../blob/gunman-chronicles/dlls/vehicletank.cpp#L1210-L1212) | 1210-1212, 1219 | `CVehicleTank::Slide` | `vecVelocity.z = vecVelocity.z * 1;`, `vecAlong.z = vecAlong.z * 1;` | x87 operand orders in the inlined `Normalize`, `Length` and dot products | `8ca0db5` |

Several of these are Valve's text rather than ours: `beam_tr` and the
gauss pistol's two lines come from the SDK's gauss gun, and the chopper's
`vecEst` from the SDK's `CApache::HuntThink`. Retail still needs them only
for their effect on the code, so they are listed here.

**Client** (15 lines):

| File | Lines | Function | What | Effect | Commit |
|---|---|---|---|---|---|
| [`cl_dll/ammo.cpp`](../../../blob/gunman-chronicles/cl_dll/ammo.cpp#L1808-L1809) | 1808-1809 | `CHudAmmo::DrawWList` | `if ( 1 ) goto slot;` into the bucket loop | A second entry makes the loop "improper" to VC6, which stops constant propagation through it: retail's unfolded x and unthreaded test. A plain `goto` is folded too early to do this | `84d8397` |
| [`cl_dll/ev_hldm.cpp`](../../../blob/gunman-chronicles/cl_dll/ev_hldm.cpp#L1621-L1624) | 1621-1624 | `EV_FireGauss` | `VectorAdd` written out, `.x` on the first normal | Retail's operand order; only these of all 64 `[i]`/`.x` combinations give it | `3e1ad1f` |
| [`cl_dll/health.cpp`](../../../blob/gunman-chronicles/cl_dll/health.cpp#L236-L237) | 236-237 | `CHudHealth::Draw` | The SDK's `ScaleColors` call under `if ( 0 )` | The dead call keeps `r`, `g` and `b` on the stack, as retail has them | `bce4841` |
| [`cl_dll/tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L283-L293) | 283-293 | `ShockCorner`, used by `DrawShocks` | A file-local inline helper for the second shock corner (name and form ours) | Retail calls `operator+`'s `Vector( float, float, float )` out of line while inlining the product's; VC6 does that only with `operator+` one inline level down | `58d3bcf` |
| [`cl_dll/tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L334-L339) | 334-339 | `DrawShocks` | Two discarded `vecOrigin.Length();` | Budget: with `ShockCorner`, every operator and copy inlines or is called as in retail. Exactly two | `58d3bcf` |
| [`cl_dll/tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L350-L356) | 350-356, 643-649 | `DrawShocks`, `DrawSprites` | The third corner's differences written as functional casts, `float( right.x - up.x )`, not in parentheses | VC6 schedules a long block 81 instructions at a time, and a parenthesised float expression counts as one although it leaves no code. With three fewer, the set-up of the next `RenderMode` call lands where retail has it ([vc6-notes.md](vc6-notes.md#the-schedulers-81-instruction-chunks)) | `7bd4f25` |
| [`cl_dll/tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L476-L480) | 476-480 | `DrawEffect8` | The direction passed as `*(Vector *)(float *)vecDir` | No code; the one inline `operator float *()` it costs leaves `Normalize()`'s constructor out of line, as in retail | `a92cccb` |
| [`cl_dll/tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L631-L634) | 631-634 | `DrawSprites` | A discarded `vecOrigin[0];` | Pays for the copy-initialised origin's extra inline unit, which keeps retail's inlining of the corners | `2622033` |
| [`cl_dll/tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L719-L725) | 719-725 | `DrawFlames` | `CEffectList::Remove` called through a member-function pointer | VC6 turns it into a plain call of the out-of-line copy. Retail inlines the operators after it as if the call were no inline candidate, which a direct call always is | `94e5f64` |
| [`cl_dll/tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp#L731-L736) | 731-736 | `DrawFlames` | `flScale * 1` | VC6 loads the scale once and multiplies on the x87 stack, as retail does; the plain variable is used from memory. (The redundant first type test on line 735 is retail's own) | `e33134a` |
| [`cl_dll/vgui_CustomizationMenu.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_CustomizationMenu.cpp#L252-L258) | 252-258, 291 | `CCustomizationMenu::CCustomizationMenu` | Dead zero initialisations of five locals and a named `TextImage *pImage` | VC6 numbers locals by first reference; six more before `iWide` give retail's frame layout and centring order | `bb0c6fe` |
| [`cl_dll/vgui_CustomizationMenu.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_CustomizationMenu.cpp#L556-L558) | 556-558 | `CCustomizationMenu::CCustomizationMenu` | A third of `iLaunchTall` in its own local `iThird` | Both rows compute into `ecx` and share retail's single store | `bb0c6fe` |
| [`cl_dll/vgui_HudID.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_HudID.cpp#L165) | 165 | `CIDPanel::paint` | Three discarded `forward[0]; forward[1]; forward[2];` after the trace | Retail calls the end point's two return copies out of line; with exactly three inline candidates after the trace VC6 does the same | `207af35` |

### Behaviour kept from retail

The remaining `// sic` lines keep retail's bugs and odd spellings. Retail's
code, data or strings show each of them, and they steer nothing. A port
that wants to play like the original must keep them.

One changes behaviour against the SDK on purpose. Retail's
`TEXTURETYPE_PlaySound` (server) and `EV_HLDM_PlayTextureSound` (client)
play the grate sounds (`player/pl_grate1/4.wav`, volume bar 0.5) for wood
and the wood sounds (`debris/wood1-3.wav`, volume bar 0.2) for grates. The
case bodies sit in the SDK's order at retail's addresses; only the two
jump-table entries differ (`12790c0`). Gunman's other server code treats
the letters the same way (the step sounds in `player.cpp`, the tank's
gibs), while `pm_shared` keeps the SDK's mapping in both DLLs.

| File:line | Function |
|---|---|
| [`dlls/sound.cpp:1711`](../../../blob/gunman-chronicles/dlls/sound.cpp#L1711), [`1730`](../../../blob/gunman-chronicles/dlls/sound.cpp#L1730) | `TEXTURETYPE_PlaySound` |
| [`cl_dll/ev_hldm.cpp:185`](../../../blob/gunman-chronicles/cl_dll/ev_hldm.cpp#L185), [`204`](../../../blob/gunman-chronicles/cl_dll/ev_hldm.cpp#L204) | `EV_HLDM_PlayTextureSound` |

One is data only: [`dlls/world.cpp:124`](../../../blob/gunman-chronicles/dlls/world.cpp#L124),
`{balburn`'s decal index initialised to 1 (`5c73380`). `W_Precache`
overwrites every index, so nothing reads it.

<details>
<summary>The other 180 server lines (line: what retail does)</summary>

| File | Lines |
|---|---|
| [`bandit.cpp`](../../../blob/gunman-chronicles/dlls/bandit.cpp) | 647 (`UTIL_VecToAngles` result thrown away); 668 (the direction, not the angles); 732 (`vecForward` unused); 769 (falls through and kicks); 1020 (falls through into the roll); 2001 (always true); 2043 (clears the sentence it just cued) |
| [`banditgrunt.cpp`](../../../blob/gunman-chronicles/dlls/banditgrunt.cpp) | 189 (`vecGunAngles` never set); 249 (attachment thrown away); 512 (the rustbit's claw damage) |
| [`beak.cpp`](../../../blob/gunman-chronicles/dlls/beak.cpp) | 37 (`g_iGibQueen` never precached); 462 (three keep the touch function); 705 (leads the enemy by velocity times distance); 753 (no break: the bite starts the base task twice); 774 (never completes); 1099 (no break); 1161 (the bandits' class) |
| [`beamgun.cpp`](../../../blob/gunman-chronicles/dlls/beamgun.cpp) | 86 (an int saved as a float); 600 (`m_pNoise` cleared, not the beam); 1370 (buzz time set every think) |
| [`bird.cpp`](../../../blob/gunman-chronicles/dlls/bird.cpp) | 265 (nothing sets `m_flLastSeen`) |
| [`boulder.cpp`](../../../blob/gunman-chronicles/dlls/boulder.cpp) | 33 (`DistibuteThinks`, retail's export name) |
| [`butterfly.cpp`](../../../blob/gunman-chronicles/dlls/butterfly.cpp) | 275 (flattens the engine's forward vector) |
| [`camera.cpp`](../../../blob/gunman-chronicles/dlls/camera.cpp) | 77 (stops short within 10 degrees) |
| [`chopper.cpp`](../../../blob/gunman-chronicles/dlls/chopper.cpp) | 285 (crashes into anything it touches); 684 (`flYaw`, `flPitch` uninitialised); 829 (flight model nothing calls); 1035 (`avelocity.y = -100`); 1199 (owner cleared above) |
| [`cluster.cpp`](../../../blob/gunman-chronicles/dlls/cluster.cpp) | 145 (separate ifs) |
| [`cricket.cpp`](../../../blob/gunman-chronicles/dlls/cricket.cpp) | 261 (facing test worked out, never used); 347 (second jump sound) |
| [`critter.cpp`](../../../blob/gunman-chronicles/dlls/critter.cpp) | 590 (checked after use); 607 (undoes the straight spit); 746 (copied from `TakeDamage`) |
| [`darttrap.cpp`](../../../blob/gunman-chronicles/dlls/darttrap.cpp) | 162 (separate ifs); 315 (cases 1-3 for `RANDOM_LONG( 0, 2 )`); 362 (`Normalize` result dropped); 405 (`m_pTargetEntity` never set) |
| [`decore.cpp`](../../../blob/gunman-chronicles/dlls/decore.cpp) | 92 (other keys dropped); 189 (pipes take damage after all); 348 (both picks play hit1); 875 (`m_fAnimEnd` never set); 1068 (`UTIL_VecToAngles` result unused); 1081 (`pMonster` untested); 1404 (every key resets the size); 1463 (a position, not a direction); 1527 (keys marked unhandled); 1974, 1986, 1998 (`BEAM_FSHADEOUT` replaces `BEAM_FSINE`) |
| [`decoreexplode.cpp`](../../../blob/gunman-chronicles/dlls/decoreexplode.cpp) | 198 (skips `CBaseMonster`'s fields); 264 (unhandles keys); 359 (explodes again at every use); 362 (stale `v_right`, `v_up`) |
| [`demoman.cpp`](../../../blob/gunman-chronicles/dlls/demoman.cpp) | 235 (a float saved as an integer); 334 (`Normalize` result thrown away); 495 (`VisionFlags()` asked, then ignored) |
| [`dml.cpp`](../../../blob/gunman-chronicles/dlls/dml.cpp) | 649 (the same animation); 771 (`0.75` is not a time) |
| [`dmlgrenade.cpp`](../../../blob/gunman-chronicles/dlls/dmlgrenade.cpp) | 1072 (`weapon_tripmine`, no such weapon in Gunman) |
| [`dmlrocket.cpp`](../../../blob/gunman-chronicles/dlls/dmlrocket.cpp) | 770, 793 (`Normalize` result thrown away) |
| [`endboss.cpp`](../../../blob/gunman-chronicles/dlls/endboss.cpp) | 203, 395 (not added to the origin); 331 (target never used); 709 (a count saved as a boolean); 855 (both events from one barrel); 958 (falls through); 1372 (pointer arithmetic past the sum) |
| [`friendlygunman.cpp`](../../../blob/gunman-chronicles/dlls/friendlygunman.cpp) | 347 (`vecForward` unused); 356 (`angDir` unused); 558 (the stare key sets the idle group); 760 (angles never set) |
| [`friendlyunarmed.cpp`](../../../blob/gunman-chronicles/dlls/friendlyunarmed.cpp) | 206 (the stare key sets the idle group); 241 (all three set the first) |
| [`func_break.cpp`](../../../blob/gunman-chronicles/dlls/func_break.cpp) | 770 (render mode 7 doesn't exist) |
| [`func_tank.cpp`](../../../blob/gunman-chronicles/dlls/func_tank.cpp) | 1295 (sets the global time) |
| [`funcwind.cpp`](../../../blob/gunman-chronicles/dlls/funcwind.cpp) | 47 (separate ifs; the else marks every key) |
| [`gator.cpp`](../../../blob/gunman-chronicles/dlls/gator.cpp) | 229 (classified as a human soldier) |
| [`gausspistol.cpp`](../../../blob/gunman-chronicles/dlls/gausspistol.cpp) | 627 (`guass_bolt`, retail's classname) |
| [`gunner.cpp`](../../../blob/gunman-chronicles/dlls/gunner.cpp) | 159 (a direction, not a spot); 612 (a distance saved as a time); 813 (`GetArmPosition` result unused); 1120, 1688 (case 0 never happens); 1329 (sets no movetype and others) |
| [`h_cycler.cpp`](../../../blob/gunman-chronicles/dlls/h_cycler.cpp) | 125 (submodel 3 never checked) |
| [`hatchetfish.cpp`](../../../blob/gunman-chronicles/dlls/hatchetfish.cpp) | 170 (no break) |
| [`hologram.cpp`](../../../blob/gunman-chronicles/dlls/hologram.cpp) | 93, 248 (skip `CBaseMonster`'s fields); 156 (under 10 never reaches maxdamage); 359, 696 (forget handled keys); 711 (kind 3 means kind 4) |
| [`hostilescientist.cpp`](../../../blob/gunman-chronicles/dlls/hostilescientist.cpp) | 223 (mixture partly set); 347 (the xenome's melee damage) |
| [`lava.cpp`](../../../blob/gunman-chronicles/dlls/lava.cpp) | 105 (separate ifs) |
| [`leaf.cpp`](../../../blob/gunman-chronicles/dlls/leaf.cpp) | 97 (separate ifs); 155 (`Normalize` result dropped) |
| [`maggot.cpp`](../../../blob/gunman-chronicles/dlls/maggot.cpp) | 373 (always true) |
| [`meteor.cpp`](../../../blob/gunman-chronicles/dlls/meteor.cpp) | 189 (separate ifs); 232 (debris 1 turns debris off); 373 (the sound entity makes no sound); 411 (unused think); 571 (velocity argument unused); 622 (no `pev->dmg` for a large meteor); 634 (no `UTIL_SetOrigin`); 798 (no rock gibs or impact sounds) |
| [`microraptor.cpp`](../../../blob/gunman-chronicles/dlls/microraptor.cpp) | 296 (quiet two times in five); 406 (two keep the touch function); 639 (never completes); 731 (scent found, not used) |
| [`minigun.cpp`](../../../blob/gunman-chronicles/dlls/minigun.cpp) | 71 (`m_flNextSpinAnimation` saved twice) |
| [`monstertank.cpp`](../../../blob/gunman-chronicles/dlls/monstertank.cpp) | 125 (sound flag 8); 207 (unused `vecPlaneNormal * 50`, "retail's", `b77957f`); 418 (empty `IdleSound`); 481 (`FireShell` never called) |
| [`ourano.cpp`](../../../blob/gunman-chronicles/dlls/ourano.cpp) | 208 (unused); 587 (`SetState` never called) |
| [`raptor.cpp`](../../../blob/gunman-chronicles/dlls/raptor.cpp) | 277 (quiet one time in five); 438 (two keep the touch function); 507 (the large scorpion's sting damage); 611 (no breaks); 645 (never completes); 740 (scent found, not used) |
| [`rust.cpp`](../../../blob/gunman-chronicles/dlls/rust.cpp) | 64 (`JustSpoke` first, so it never speaks); 319 (no case 0, no 3); 382 (`Normalize` result thrown away); 383 (x, not z) |
| [`rustbot.cpp`](../../../blob/gunman-chronicles/dlls/rustbot.cpp) | 456 (saved as a time); 713 (`Normalize` result thrown away); 854 (never true) |
| [`rustflier.cpp`](../../../blob/gunman-chronicles/dlls/rustflier.cpp) | 118 (three fields saved twice); 317 (target may not exist); 769 (never takes damage) |
| [`scientist.cpp`](../../../blob/gunman-chronicles/dlls/scientist.cpp) | 620 (no else; keys go on to `CTalkMonster`); 631 (non-zero "IsScared" calms him); 672 (the stare key sets the idle group); 766 (shared friend list); 1093 (the xenome's melee damage); 1129 (`abs()` of a float); 1201 (always walks); 1414 (every hit a bullet); 1446 (index past `m_szFriends`); 1582 (`m_iHead` always 0) |
| [`scorpion.cpp`](../../../blob/gunman-chronicles/dlls/scorpion.cpp) | 331 (bleeds as much as the claw); 413 (height above ground, never used); 614 (no breaks: always 1.5); 627 (`ScamperSound` never called); 658 (flat health 100) |
| [`shotgun.cpp`](../../../blob/gunman-chronicles/dlls/shotgun.cpp) | 49 (`m_iSlidePos` saved as a vector) |
| [`speakers.cpp`](../../../blob/gunman-chronicles/dlls/speakers.cpp) | 76 (key `"niose"`); 147 (`m_fVolume` stays 1) |
| [`sphereexplosion.cpp`](../../../blob/gunman-chronicles/dlls/sphereexplosion.cpp) | 113 (separate ifs) |
| [`spritegod.cpp`](../../../blob/gunman-chronicles/dlls/spritegod.cpp) | 107 (separate ifs); 148 (`spritex` sets z); 168, 214 (integer division); 195 (direction never set without a target) |
| [`tube.cpp`](../../../blob/gunman-chronicles/dlls/tube.cpp) | 369 (`ShootRocket` can return NULL); 606 (not `bits_CAP_SQUAD`); 694 (attack time starts at spawn) |
| [`tubequeen.cpp`](../../../blob/gunman-chronicles/dlls/tubequeen.cpp) | 179 (`UTIL_VecToAngles` result unused); 192 (`pMonster` untested); 514 (fixed spot); 698 (tougher on medium than hard); 874 (third sound never plays); 901 (mortar already away) |
| [`tuberocket.cpp`](../../../blob/gunman-chronicles/dlls/tuberocket.cpp) | 195 (both ends the body target) |
| [`vehicletank.cpp`](../../../blob/gunman-chronicles/dlls/vehicletank.cpp) | 95 (never reached); 883, 884 (wrong vectors for the planes); 1371 (`FL_IMMUNE_LAVA`, not `FL_NOTARGET`) |
| [`vehicletankbsp.cpp`](../../../blob/gunman-chronicles/dlls/vehicletankbsp.cpp) | 438, 747 (unused `vecPlaneNormal * 50`, as in `monstertank.cpp`) |
| [`xenome.cpp`](../../../blob/gunman-chronicles/dlls/xenome.cpp) | 388 (the large scorpion's sound); 458 (the beak's scrape damage); 675 (falls through, adds 10 more); 742 (falls through, restarts the task) |

</details>

<details>
<summary>The other 34 client lines (line: what retail does)</summary>

| File | Lines |
|---|---|
| [`ammo.cpp`](../../../blob/gunman-chronicles/cl_dll/ammo.cpp) | 607 (`gpActiveSel` is NULL here if the attack key just closed the menu); 2038 (item numbers 0, -1, -2) |
| [`ev_hldm.cpp`](../../../blob/gunman-chronicles/cl_dll/ev_hldm.cpp) | 964 (shoots along the angles, not forward); 1028, 1070, 1112 (`vecSrc` and `vecAiming` unused; the bullets start at the origin); 1698, 1751, 1972, 2051 (`vecSrc` never set, `vecDest` unused); 1713 (`vecSpot` unused); 1823 (`vecAngles` unused; the impact keeps the event's angles) |
| [`health.cpp`](../../../blob/gunman-chronicles/cl_dll/health.cpp) | 386 (colour set once, so each pain arrow darkens the next); 643 (computes a colour twenty times and never uses it) |
| [`hl/hl_baseentity.cpp`](../../../blob/gunman-chronicles/cl_dll/hl/hl_baseentity.cpp) | 296, 298 (the `EHANDLE` conversion stubs return nothing; retail's are a bare `ret`) |
| [`tri.cpp`](../../../blob/gunman-chronicles/cl_dll/tri.cpp) | 466 (turns the copy, not the effect in the list) |
| [`vgui_AmmoPanel.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_AmmoPanel.cpp) | 116 (only ever shows the first three) |
| [`vgui_ClassMenu.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_ClassMenu.cpp) | 101 (string built, never used) |
| [`vgui_CustomizationMenu.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_CustomizationMenu.cpp) | 209 (tables never used); 275 (`"Dml Sel"`, not `"Dml Title Text"`); 303 (the shell count shows its title until `ShowShotgun` sets it); 388, 444, 505, 545 (the first title replaces the panel made above); 627 (file never freed); 893 (scheme handle never used) |
| [`vgui_HudDeathNotice.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_HudDeathNotice.cpp) | 150 (`"d_gauss_bolt"` tested twice) |
| [`vgui_HudID.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_HudID.cpp) | 58 (file loaded and dropped); 112 (team info indexed by player) |
| [`vgui_ScorePanel.cpp`](../../../blob/gunman-chronicles/cl_dll/vgui_ScorePanel.cpp) | 292 (buffer short for a line of the config text); 295 (title built, never used) |
| [`zoom.cpp`](../../../blob/gunman-chronicles/cl_dll/zoom.cpp) | 266 (unzoomed, the sensitivity is 1 rather than the player's own) |

</details>

### Other choices the evidence leaves open

| Where | What | Why, and what is open |
|---|---|---|
| [`dlls/gunman_pending.cpp`](../../../blob/gunman-chronicles/dlls/gunman_pending.cpp) | An object without code holding `gGibModels` and `gMonsterTypes`, linked right after `world.cpp`. File name ours | An object's data comes before its strings, so their data after `world.obj`'s strings marks a separate object (`4e17f4e`) |
| [`dlls/vehicletankbsp.cpp`](../../../blob/gunman-chronicles/dlls/vehicletankbsp.cpp), [`dlls/tankshots.h`](../../../blob/gunman-chronicles/dlls/tankshots.h) | The tank's parts, triggers and shots in a second object. File names ours | `CVehicleTankBSP`'s save table follows the strings of `CVehicleTank`'s functions (`2df8cb1`) |
| Gunman's own files in `hl.dsp` | Named after their classes | The image holds no object names; only their order matters, and it is retail's |
| [`dlls/beamgun.h`](../../../blob/gunman-chronicles/dlls/beamgun.h); the include order of [`dlls/dmlgrenade.cpp`](../../../blob/gunman-chronicles/dlls/dmlgrenade.cpp#L28-L30) | `CBeamGun` declared in a header (name ours); `dmlgrenade.h` inside the `.pch`, right after `effects.h` | Read from where the client's `beamgun.obj` and `dmlgrenade.obj` emit `iItemSlot` (`89e2ac8`, `feefcf2`) |
| `.bss` names, e.g. [`dlls/weapons.cpp:48-60`](../../../blob/gunman-chronicles/dlls/weapons.cpp#L48-L60), [`dlls/gausspistol.cpp:62-68`](../../../blob/gunman-chronicles/dlls/gausspistol.cpp#L62-L68), [`dlls/vehicletankbsp.cpp:324-325`](../../../blob/gunman-chronicles/dlls/vehicletankbsp.cpp#L324-L325), the client's `m_HUD_<sprite>` globals | Names chosen so that VC6's name hash puts each global where retail has it | Only names that are ours were renamed. The SDK's unused `g_sModelIndexFireball` has no slot in retail and is gone (`ba1fe75`, `46f7d2a`, `1545ca2`, `ccaf4db`, `41b6aae`) |
| [`dlls/turret.cpp:1222-1224`](../../../blob/gunman-chronicles/dlls/turret.cpp#L1222-L1224) | `CSentry::SentryIdleSound`. Name ours | 40 bytes in retail's `turret.obj` play `sentry/sentry_idle.wav`, and no code or data refers to them. The code is retail's; the name `IdleSound` would override `CBaseMonster`'s virtual (`cdefaf2`, `278d75e`) |
| [`cl_dll/hl/hl_baseentity.cpp`](../../../blob/gunman-chronicles/cl_dll/hl/hl_baseentity.cpp) | The order of the client's 320 one-line stubs | Retail's image shows the order only of the stubs that leave a copy of their own. That order is SDK 2.2b's (`576d590`) and is kept. The other stubs are dead or folded away, and their places feed LINK's sort, so they were placed by the same search as the [placeholder functions](#placeholder-functions) (`6b5c736`). Which dead stubs existed is not shown; the 2.2b stubs this tree lacks stay out |
| [`cl_dll/vgui_CustomObjects.cpp:451`](../../../blob/gunman-chronicles/cl_dll/vgui_CustomObjects.cpp#L451) | `CImageLabel::paint` defined out of line, last in the file | Retail's end of the object needs it (`ad6c75a`). `paint` is Gunman's addition, so where it was written is free |
| [`cl_dll/effectlist.h:135`](../../../blob/gunman-chronicles/cl_dll/effectlist.h#L135), [`dlls/dml.h:22`](../../../blob/gunman-chronicles/dlls/dml.h#L22) | `CEffectList::Remove` inline in its class, before `Add`; `CLaserSpot` declared in `dml.h` | Each gives retail's place for a lazily emitted function (`a1cc15c`, `8b7cbeb`) |
| [`dlls/schedule.cpp:105`](../../../blob/gunman-chronicles/dlls/schedule.cpp#L105) | Valve's `#if 0` schedule trace kept as `#if 1` | The code is in retail; keeping the directive rather than deleting it is not shown either way |
| Unmarked spellings | Subscripts `v[i]` instead of `.x` (`7617548`), `avelocity / 10` instead of `* 0.1` (`0dc77a5`), `FClassnameIs` tests (`ec294f4`), `Vector vecPush = PushAway();` (`05659cb`), case orders, in-class definitions (`320d5ba`) | Same values; chosen because only that spelling gives retail's bytes |
| Names | Classes, members, locals and globals not in exports, save tables, strings or the `.bss` hash are ours. 47 Gunman classes are known only by their entity names | Names reach the image only through those four channels |

## How the server became identical

1. **Base and function matching.** VC6 builds of each SDK release picked
   the base: SDK 2.0's shared files, SDK 2.2b's single-player files
   (`be01e66`), SDK 2.1's engine interface (`dd60d77`), and Gunman's own
   `hl.dsp` (`928626b`) in its Release configuration, without exception
   handling (`/GX`) and without `CLIENT_WEAPONS`. The base alone compiled
   1,338 server functions to retail's bytes exactly, and 290 more apart
   from shifted addresses. After the base classes' layouts and Gunman's
   changes to SDK files, 2,777 of the 2,788 functions in the SDK's objects
   were exact. Then every Gunman file was written. Further checks compared
   what byte matching misses: the strings and tables a function refers to,
   vtable slots, and which stub a call goes to. Before work on the whole
   image, 4,179 of 4,238 game functions were exact (92.3% of their bytes).
2. **The whole image as the goal.** A function identical apart from
   addresses still differs from retail if anything before it moved. At
   `8a31a90`, 44.0% of `.text` was identical at the same address (and the
   code 416 bytes short), 79.6% of `.rdata`, 31.7% of `.data` and 41.7% of
   `.reloc`.
3. **Data and layout.** Exported string literals and export names
   (`320d5ba`, `d039128`), two functions not `static` so they sit where
   retail has them (`a1475d7`) and the uncalled sentry sound (`cdefaf2`)
   brought `.rdata` to 98.0%. Fifteen skill cvars that retail defines and
   never registers (`59d9dd7`) and the gib tables in an object of their own
   (`4e17f4e`) brought `.data` to 98.9%; the tank's second object
   (`2df8cb1`) to 99.8%. `CAicore` inheriting `SecondaryAttack` instead of
   overriding it (`99f0c6f`) put one inline copy where retail has it.
4. **Near misses and `.bss`.** 21 more functions became identical, and
   `.text` reached 58.4% at retail's addresses. VC6's `.bss` order was
   worked out from the hash loop in its front end (`C1XX.DLL`); names were
   chosen and the five unused slots filled (up to `2882929`).
5. **Precompiled headers and object count.** Experiments with real
   compiles showed that retail compiled five objects after other files
   than their project-order neighbours. `build_dsp.py` learned per-file `/Fp`, and `bd5439f` gave the five
   their own `.pch`: 59.4%, with every function up to
   `CBeamGun::WeaponIdle` at retail's address. The two placeholder objects
   (`f9caa39`) gave retail's Rich header and PE header offset. The release
   build environment (the tree at `C:\Gunman\Source`, a new PDB, LINK at
   retail's link time) gave retail's size, PDB record and time stamps.
6. **The remaining near misses, one by one** (up to `5e4b217`). `.text`
   reached retail's size with every function at retail's address, and
   `.rdata`, `.data` and `.reloc` were identical. 346 bytes differed: 340
   inside five functions, 6 in `.reloc`. 4,233 of 4,238 functions were
   exact.
7. **The last five functions.**
   - `CBaseTurn::TurnTo` and `CRustbot::AimHead` (`b68bb86`): retail reads
     an int member twice (`fild [m]` and `mov edi, [m]`). VC6 merges the two
     reads in every form tried, except a member read paired with a read
     through a local pointer that holds the member's address: the compiler
     folds that address back into `[esi+offset]` only after the merge step.
     Of 40 read-pair variants in a test file, only `int *`, `int &`,
     `const int &`, a pointer to member and an inline returning a reference
     through `?:` keep the loads apart
     ([vc6-notes.md](vc6-notes.md#member-reads-through-a-local-pointer)).
   - `CEndBossKataball::KataTouch` (`ec294f4`): `FClassnameIs` tests give
     retail's load order. Which of two values gets `edi` and which `ebp` is
     a near tie: 512 combinations of changes that generate no code didn't
     move it; dropping the shared address constant did.
   - `CVehicleTank::Drive` (`05659cb`): an initialised first push-off places
     a scheduled load; one dead dot product sets two x87 orders.
   - After these (`03d1fbb`), 5 bytes differed, all in `CBeak::StartTask`.
8. **`CBeak::StartTask` and the x87 operand order** (`d4b4311`). VC6's
   optimiser (`C2.DLL`) orders the operands of a commutative x87 operation,
   and the terms of a reassociated sum such as an inlined `Length()`, by
   walking a bit vector of the function's tracked values 32 bits at a time:
   words upwards, bits downwards. Vector components reached only through
   inlined members get late, adjacent numbers, so they come out z, y, x. A
   word boundary between two of them gives the rotations y, x, z or x, z, y.
   About 2,700 small test functions and 250 variants of `beak.cpp` showed
   the rule; addresses and the runtime's allocator play no part. Retail
   needs y ahead of z with both unnamed. Reading y through the inline helper
   `VecY` puts y's number in the word before z's and moves nothing else
   ([vc6-notes.md](vc6-notes.md#x87-operand-order)). The release build was
   identical to retail.

[vc6-notes.md](vc6-notes.md) has these rules and the others found along
the way.

| Release build | Server bytes that differ |
|---|---|
| `5e4b217` | 346 (340 in five functions, 6 in `.reloc`) |
| `03d1fbb` | 5 (`CBeak::StartTask`) |
| `d4b4311` | 0 |
| `fd0e4f5` | 0 (verified 2026-11-01) |
| `6119524` | 0 (verified 2026-11-26) |

Between `d4b4311` and `fd0e4f5` the client work changed three files that
the server compiles too (`dlls/beamgun.cpp`, `dlls/beamgun.h`,
`dlls/dmlgrenade.cpp`); the server stayed identical. Between `fd0e4f5`
and `6119524` six weapon files and their headers gained the client's empty
`UpdateOwner` ([below](#the-six-updateowner-functions)), in the half of an
`#ifndef CLIENT_DLL` block that the server does not compile.

## What is left in the client, and why

Earlier work gave the client retail's file-level facts (placeholder
object, import order), the weapon objects' `.pch` states and the order of
`ev_hldm.obj`, and brought near misses such as the customization menu's
constructor, `CHudAmmo::DrawWList`, `SelectSlot`, `CIDPanel::paint`,
`DrawFlames`, `InitSphere` and `DrawSphere` to retail's code.

| Release build | Client bytes that differ |
|---|---|
| `5e4b217` | 400,495 |
| `05f40fb` | 339,719 |
| `4dc83a3` | 152,042 |
| `fd0e4f5` | 152,034 |
| `29d5c94` | 327,556 (six functions added that retail has; the placeholders of that time no longer fitted) |
| `6119524` | 4 |

One function is left. Three things closed the rest; they follow it.

### One function: `ShowBeamgun`

At the exit of an inner loop, retail's `CCustomizationMenu::ShowBeamgun`
reloads `this` (`mov ebp, [esp+0x38]`) and then the row number
(`mov edx, [esp+0x4c]`). Ours reloads the row number first. Nothing else
in the function differs. What is known about the order:

- It is the register allocator's, not the instruction scheduler's. The
  processor options `/G3`, `/G4` and `/G5` change scheduling and leave it
  alone, and the block is only nine instructions long.
- It follows the parameters' stack positions and moves only with the
  function's live constants (ranked by value modulo 64) and its address
  temporaries. Dead locals, copies, names, parentheses, declaration order,
  other integer types for the parameters, inline helpers and other
  spellings of the loop and of its tests never move it.
- A real use of the row number between `HideAll()` and the loop gives
  retail's order, and changes other code.
- The number of symbols declared before the function, which decides such
  reloads in a few other functions
  ([vc6-notes.md](vc6-notes.md#reloads-at-a-loop-end-symbol-numbers)),
  does not move this one (0 to 63 extra symbols).

About 1,500 variants found no spelling. Retail's order is the usual one,
so ours probably comes from one intermediate temporary or constant more
or fewer that leaves no trace in the code: retail's source differed in a
way the bytes don't show.

### The six `UpdateOwner` functions

Six weapons (`CBeamGun`, `CDml`, `CDmlGrenade`, `CGaussPistol`, `CMinigun`
and `CShotgun`) override `UpdateOwner`, with which the server sends the
weapon's settings to its owner. The client's override is empty, and it is
a function of its own, written out of line (`00d189f` to `72f1dfd`).
Retail's vtables show it: slot `+0xc4` of the six classes points to an
empty out-of-line function, where our build pointed to the inline
`CBaseEntity::UpdateOwner`. LINK folds inline and out-of-line copies
separately, so the slot shows which kind the source had. With the six
functions every relocated reference in the client names the same function
as retail's: 11,635 addresses in data that point into code, and 5,091
references from code.

The byte count went up with them (327,556 at `29d5c94`): they are six
more entries in LINK's sort, and the placeholders of that time had been
fitted without them.

### `DrawShocks` and `DrawSprites`

Both functions had retail's instructions, but the set-up of a
`RenderMode` call (`mov eax` / `push 5`) came two or three x87
instructions too early, and `DrawShocks`' last `RenderMode` call had
`mov ecx` before `push 0`. Dead code can't move that, because it leaves
nothing in the instruction stream; about 400 variants and 220
combinations of redundant parentheses never got closer.

The cause is how VC6's optimiser schedules a long block: at most 81
instructions at a time, counted from the last jump target. Inside such a
chunk it hoists an independent integer instruction, here the load of the
engine's function table and the `push`, to just after the chunk's first
x87 arithmetic instruction, and never across the chunk's start. So the
place where the load lands shows where a chunk starts. Some source text
counts as an instruction there without leaving code: a pair of
parentheses around a float expression, as in `( right.x - up.x ) *
flScale`, is one. Each function had three of these more than retail's
placement allows, and the third corner's three parenthesised differences
were the only three the two functions share. Written as functional casts,
`float( right.x - up.x )`, they count nothing, and both functions are
retail's (`7bd4f25`;
[vc6-notes.md](vc6-notes.md#the-schedulers-81-instruction-chunks)).

### Which copy of identical functions LINK keeps

With `/O2` VC6 puts every function in its own section, a COMDAT, which
the linker can drop or merge on its own. Without `/debug`, LINK 6.00
removes unreferenced COMDATs and folds identical ones (identical-code
folding). To find identical functions it sorts all of them by size and
relocation count with the C runtime's `qsort`, and which copy of a group
survives follows from where its members land in that sort. `qsort` is not
stable: among equal keys, the outcome depends on every entry in the array,
including functions that LINK later drops as unreferenced. Those leave
nothing in the image, but they still move the sort
([vc6-notes.md](vc6-notes.md#links-identical-code-folding)).

Retail's image shows which copy it kept, by where the surviving function
sits: 85 groups in all. A model of LINK's sort reproduces our own links
exactly. So the question was what retail's sort held that ours did not.

These were tested first, with the model and with real builds, to find the
difference in something the evidence supplies:

- a few dead functions in `ev_hldm.cpp`, where the constant and the
  inline order ask for them, at every size, relocation count and position
  those allow;
- the SDK's stub sets for `hl_baseentity.cpp` (2.0, 2.1, 2.2b), and
  millions of random subsets of its dead stubs;
- deleting the client's real dead functions: those Gunman's edits made
  dead, those dead in the SDK too, and those in files Valve shipped
  unchanged; every combination of the 14 candidates in files Gunman
  changed, and 1.3 million combinations of all 32;
- every order of the invisible entries inside each object, that is the
  inline functions the compiler could have emitted elsewhere, up to free
  movement within the object;
- alternative precompiled-header states for every file, in 1,251 real
  compiles;
- single dead functions of any size anywhere, and blocks of equal keys;
- other linkers and runtimes: Service Pack 4's LINK 6.00.8447 keeps other
  copies again. The CD's LINK on SP4's `MSVCRT.DLL` 6.00.8797 gives the
  same file as on the CD's runtime; on Wine's builtin `msvcrt.dll` it
  gives a different one;
- other compilers whose back end writes build 8168: Service Packs 1 and 2
  don't change the compiler, and SP3's front ends (12.00.8472) with RTM's
  back end give the same file as RTM's
  ([compilers.md](compilers.md#summary)).

None reaches retail's choices. The array that LINK sorted for retail
differed from ours in entries the image doesn't show and no SDK text
supplies.

What remained was a search over invented dead functions (`6b5c736`).
Three things made it work:

- **Exact targets instead of a count.** Searches that scored the number
  of groups right stalled at 82 of 85, on arrays nowhere near retail's.
  The part of the sort above the first element `qsort` splits on
  (`CHudBattery::VidInit`, 128 bytes and 12 relocations) depends almost
  only on the larger entries, and a few dead functions in `ev_hldm.cpp`
  make it right. That part fixes which instantiation five copies of
  vgui's `Dar<>` template methods come from, and with them the
  instantiation every other `Dar` copy must come from. Scoring those
  exactly led to all 65 groups that have no member in
  `hl_baseentity.obj`.
- **The stub order as a variable.** The other 20 groups have a member in
  `hl_baseentity.obj`, the client's file of server stubs. Its dead stubs
  are entries of the sort too, and retail doesn't show their order.
  Reordered, they gave the remaining groups.
- **The image's other limits.** The 80 absolute addresses, the constant,
  the intermediate code `ev_hldm.cpp` allows, and no new object: thirteen
  placeholder objects in a first try gave 91 C++ objects in the Rich
  header, where retail has 78, and moved the PE header.

The result was checked with a second implementation of the sort and then
in the real link. All 85 groups keep retail's copy, and the file differs
from retail's only in `ShowBeamgun`'s four bytes.

What this shows: the known code is complete, in the sense that with it
and some set of dead functions retail's linker writes retail's file. What
it doesn't show: which dead functions Gunman had. The outcome of the
groups carries about 63 bits and each freely placed entry about 11, so 57
of them can fit almost any outcome. The
[placeholder functions](#placeholder-functions) are marked as ours for
that reason.

## Checking it yourself

The retail files (reads only; installing the game is not needed if you
have the DLLs):

```sh
G=/path/to/Gunman/rewolf
sha256sum "$G/dlls/gunman.dll" "$G/cl_dlls/client.dll"
python3 -c 'import sys, zlib; print("%08x" % zlib.crc32(open(sys.argv[1], "rb").read()))' "$G/dlls/gunman.dll"
```

Expected: the SHA-256 values in [the retail files](#the-retail-files), and
`5e59d944`, the CRC32 that `INSTALL.LOG` lists (`988a3cec` for
`client.dll`).

The build:

```sh
RETAIL=$G VC_TOOLCHAINS=/path/to/toolchains \
    tools/release/gunman-chronicles.sh ../gunman ../gunman-build
```

The script runs `cmp` itself. By hand:

```sh
B=../gunman-build
cmp "$G/dlls/gunman.dll" "$B/wineprefix/drive_c/Gunman/Source/dlls/Releasehl/gunman.dll" && echo identical
cmp -l "$G/cl_dlls/client.dll" "$B/client/client.dll" | wc -l
```

Expected: `identical`, and `4`.

To see where the client's bytes differ, save this script (standard
library only) as `secdiff.py`. It counts the differing bytes per section:

```python
import struct, sys
a, b = (open(p, 'rb').read() for p in sys.argv[1:3])
e = struct.unpack_from('<I', a, 0x3c)[0]
n, opt = struct.unpack_from('<H', a, e + 6)[0], struct.unpack_from('<H', a, e + 20)[0]
spans = [('headers', 0, struct.unpack_from('<I', a, e + 24 + opt + 20)[0])]
for i in range(n):
    o = e + 24 + opt + 40 * i
    rs, rp = struct.unpack_from('<II', a, o + 16)
    spans.append((a[o:o + 8].rstrip(b'\0').decode(), rp, rp + rs))
for name, lo, hi in spans:
    print(name, sum(x != y for x, y in zip(a[lo:hi], b[lo:hi])))
```

Run as `python3 secdiff.py "$G/cl_dlls/client.dll" "$B/client/client.dll"`. Expected:
`headers 0`, `.text 4`, `.rdata 0`, `.data 0`, `.reloc 0`.
For the server every line is 0.

The tools used during the work to match functions one by one and to
compare the images section by section are not part of this repository.
The file hashes are the proof for the server; for the client, the numbers
above say how far it is.
