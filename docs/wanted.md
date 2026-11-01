# Wanted! 1.5

Wanted! is a Wild West total conversion for Half-Life by Maverick
Developments (UK). Version 1.5, the original WON release of June 2001, has a
single-player campaign, three multiplayer modes (deathmatch, Capture the
Chicken, High Noon) and server-side bots, the "Cow-Bots". It ships two game
DLLs, both built with Visual C++ 6.0 (VC6):

- `dlls/wanted.dll`, the server: the game logic, monsters, weapons, game
  modes and bots;
- `cl_dlls/client.dll`, the client: the HUD and the telegram panel that
  `item_telegram` opens. `liblist.gam` says `cldll "0"`, so the engine
  actually loads Half-Life's own client, but the shipped file is still mod
  code.

The branch [`wanted`](../../../tree/wanted) holds the reconstructed source of
both. It is Valve's Half-Life SDK history up to the tag `v2.0` (`7e9f0c8`,
2000-06-05), plus 164 commits (153 without merges). The first of them adds
the single-player game code from SDK 2.2b, which 2.0 lacks. Each commit
message says what the change matched and why. This page describes commit
`455d243`, the head of the branch.

## Status

The client rebuilds byte for byte. The server rebuilds to within 810 bytes,
all in one function.

| | Server | Client |
|---|---|---|
| File in the mod folder | `dlls/wanted.dll` | `cl_dlls/client.dll` |
| Size | 1,122,380 bytes | 356,352 bytes |
| Linked (PE time stamp, UTC) | 2001-06-05 13:13:19 | 2000-12-02 15:44:22 |
| Project | `dlls/hl.dsp`, "hl - Win32 Release" | `cl_dll/cl_dll.dsp`, "cl_dll - Win32 Release" |
| Retail SHA-256 | `2e3233515c6e5cc7b77a84bb514212583997b10497b6ca96a1ca57c53ac25875` | `584813e7ec904ec6fa596fa1360d3da37bfc87cfb1b1785886b8d868d793e479` |
| Rebuilt SHA-256 | `e4dfb4369731a2a36c2ef71b4680361da7912f932bc78c5bb2a9cc4df696c99f` | `584813e7ec904ec6fa596fa1360d3da37bfc87cfb1b1785886b8d868d793e479` |
| Result | 810 bytes differ | identical |
| First identical at | not yet | `5f7d773` (2026-10-29) |

Verified on 2026-11-01 with fresh release builds of `455d243`, by SHA-256
and `cmp`.

In the server, every byte outside `CBot::Wander` and its `.reloc` block is
retail's: the headers, `.rdata`, `.data`, the debug record and every other
function, each at its retail address. The 810 bytes are:

- **801 in `.text`**, from `CBot::Wander`+0x436 (0x10015b26) to the end of
  the function. At that offset our build multiplies straight from memory,
  `fmul dword [esp+0xa8]`. Retail loads the value first,
  `fld dword [esp+0xa8]; fmul st, st(1)`, and pops the other operand with an
  extra `fstp st(0)` after the store. Retail's form is 4 bytes longer, so
  the rest of our `Wander` sits 4 bytes early. The function's alignment
  padding takes up the difference, so the next function is at its retail
  address again. Both forms compute the same product.
- **9 in `.reloc`**, the base relocations: the fixup block for `Wander`'s
  page lists the shifted addresses.

[What is left](#what-is-left) explains why the compiler picks the shorter
form.

## The retail files

The retail DLLs are not in this repository. They come from the original
WON release on ModDB:

- Page: ModDB download 191299,
  <https://www.moddb.com/mods/wantedhl/downloads/wanted-for-half-life-original-won-version>,
  added 2020-03-26. The uploader notes that "Wanted15.zip is very hard to
  find for some reason and the installer doesn't seem to work".
- File: `wantedsp.zip`, 102,318,115 bytes.
  - MD5 `d46fef0bf40ff132239b38d4228691f6` (the value ModDB lists);
  - SHA-256 `74d02d7cbc167e942f885b2c155b267bb260f67cb7aab67e61161424a0ece065`.

The zip holds the mod folder `wantedsp/` (98 entries). The two DLLs are:

| Zip entry | Size | Date in the zip | SHA-256 |
|---|---|---|---|
| `wantedsp/dlls/wanted.dll` | 1,122,380 | 2001-06-05 10:13 | `2e3233515c6e5cc7b77a84bb514212583997b10497b6ca96a1ca57c53ac25875` |
| `wantedsp/cl_dlls/client.dll` | 356,352 | 2000-12-02 11:44 | `584813e7ec904ec6fa596fa1360d3da37bfc87cfb1b1785886b8d868d793e479` |

`wantedsp/liblist.gam` says `gamedll "dlls/wanted.dll"`, `cldll "0"` and
`hlversion "1104"`. The unpacked `wantedsp/` folder is what the release
script's `RETAIL` variable expects (see below). The code and the mod's
documents call the folder `wantedhl`; that matters for playing, not for
rebuilding.

## Rebuilding

### What you need

- The toolchain folder from [setup.md](setup.md). This mod uses `vc6/`
  (Visual C++ 6.0 Professional, no service pack) and `wineprefix/`.
  [compilers.md](compilers.md) says where the compiler comes from.
- Wine (we used 11.0 with a win64 prefix), libfaketime (we used 0.9.12),
  Python 3 (standard library only), `sha256sum` and `cmp`.
- A checkout of the `wanted` branch.

### Commands

From a checkout of `main`:

```sh
git worktree add ../wanted wanted    # or: git clone -b wanted <repo> wanted
VC_TOOLCHAINS=/path/to/toolchains tools/release/wanted.sh ../wanted ../wanted-build
```

- The third argument picks `server`, `client` or `both` (the default).
- `../wanted-build` is the build folder. The script creates it if needed.
  It holds the build's copy of the Wine prefix and the source tree laid out
  at retail's path.
- To compare with your own retail copy as well, set `RETAIL` to the
  unpacked mod folder:

  ```sh
  RETAIL=/path/to/wantedsp VC_TOOLCHAINS=/path/to/toolchains \
      tools/release/wanted.sh ../wanted ../wanted-build
  ```

For each DLL the script prints the path of the built file, its SHA-256,
retail's SHA-256 and `identical to retail` or `differs from retail`. With
`RETAIL` set it also runs `cmp` against `dlls/wanted.dll` or
`cl_dlls/client.dll` there. It exits with 0 only if every DLL it built is
identical, so for Wanted! `client` exits with 0 and `server` and `both`
don't.

### What the script does for the server

Retail's debug record names its PDB (program database, the file LINK
writes debug information to):
`K:\Wantedhl\Source Code\New Code\Code\dlls\Releasehl\hl.pdb`.
`dlls\Releasehl` is the output folder of the SDK's `hl.dsp` Release
configuration, so Maverick's tree was at `K:\Wantedhl\Source Code\New Code\Code`.
The script rebuilds that setting and the end of the build history:

1. **The tree.** It copies the toolchain's Wine prefix into the build folder
   and maps the copy's drive `K:` to a folder inside the build folder. There
   it lays out the checkout at `K:\Wantedhl\Source Code\New Code\Code`.
2. **A full build.** It builds `hl - Win32 Release` of `dlls\hl.dsp` into
   `dlls\Releasehl`, all 131 sources (128 C++, 3 C) in project order. CL
   creates and reuses one `vc60.pch` (the precompiled header of `/YX`), as
   in the IDE. The IDE-only `/Fr` and `/FD` are left out.
3. **Linked as `hl.dll`.** Retail's export directory names the DLL `hl.dll`
   and its PDB is `hl.pdb`, so Maverick most likely linked `hl.dll` and
   renamed it `wanted.dll`. The project on the branch writes `wanted.dll`,
   the name `liblist.gam` loads (`63c10dd`), so the script links under the
   name `hl.dll` itself and reports the result as `wanted.dll`. (The export name
   comes from `hl.def`'s `LIBRARY hl` either way.)
4. **The last incremental build.** It compiles `bot.cpp`, `colonel.cpp` and
   `combat.cpp` once more, in that order, with the same flags and the same
   `vc60.pch` path. VC6 decides for each file whether to reuse the `.pch` the
   previous compile left or to rebuild it, as the IDE does when you rebuild
   a few edited files. This gives three objects retail's function order,
   which no full build gives (see [Build stand-ins](#build-stand-ins)).
5. **The PDB's history.** It deletes the PDB and links 18 times. Retail's
   debug record names a PDB created on 2001-04-02 10:56:04 UTC (its
   signature) and written by 18 links (its age). The first link runs with
   the clock at that creation time and writes a fresh `hl.pdb`. The other 17
   run with the clock at retail's link time, 2001-06-05 13:13:19 UTC.
6. **The final link's two seconds.** Retail's export directory is stamped
   13:13:17, two seconds before the image. LINK writes `hl.exp`, which holds
   the export directory, before the image, so retail's final link took two
   seconds between the two. In the last link the clock reads 13:13:17 until
   LINK has written `hl.exp`, then 13:13:19.

The clock is libfaketime's, with `TZ=UTC`, and only LINK runs under it.

### What the script does for the client

The client was linked without `/debug`. It has no debug directory, so no
path reaches the image and the source location doesn't matter. The script
builds `cl_dll - Win32 Release` of `cl_dll/cl_dll.dsp` into an empty folder:
53 sources (50 C++, 3 C) in project order with one `vc60.pch`, LINK's clock
at 2000-12-02 15:44:22 UTC. No incremental build is replayed.

The flags, from the two `.dsp` files:

| | Server (`hl.dsp`) | Client (`cl_dll.dsp`) |
|---|---|---|
| Compile | `/G5 /MT /W3 /Zi /O2 /YX`, defines `NDEBUG WIN32 _WINDOWS QUIVER VOXEL QUAKE2 VALVE_DLL`; `weapons.cpp` also `/D CLIENT_WEAPONS` | `/MT /W3 /GX /Zi /O2 /YX`, defines `WIN32 NDEBUG _WINDOWS` |
| Link | `/version:1.1 /subsystem:windows /dll /map /debug /machine:I386 /def:.\hl.def /incremental:no` | `/subsystem:windows /dll /map /machine:I386`, plus SDK 2.0's `utils/vgui/lib/win32_vc6/vgui.lib` |
| Sources | 128 `.cpp` (8 of them placeholders), 3 `.c` | 50 `.cpp`, 3 `.c` |

### Toolchain files

The verified builds used these files. [setup.md](setup.md) explains how to
assemble them.

| File | Version | SHA-256 |
|---|---|---|
| `vc6/VC98/BIN/CL.EXE` | 12.00.8168 | `d9220fdfaf355c81f26e7b03c5dc628ec72ff81c99dc38ed98b1ad4bdbbb309c` |
| `vc6/VC98/BIN/C1.DLL` | 12.00.8168 | `ee14a2603a1339f95db2e396751f7e51b50e421731229fb2416a01f974689d80` |
| `vc6/VC98/BIN/C1XX.DLL` | 12.00.8168 | `4095e9e8de5ac4b13dbfe310abb8de711475b1a92c5f94375919947937f3237e` |
| `vc6/VC98/BIN/C2.DLL` | 12.00.8168 | `45187b0b6288240f73272a7c61e6329c50048a76e57db3bb87b6f0229e09e27d` |
| `vc6/VC98/BIN/LINK.EXE` (as on the CD) | 6.00.8168 | `08d3a4c9701c8f6c871b2c48bd74d07422aa1f86e5d9a4ddb2c9bd6a88b857e7` |
| `vc6/nativecrt/LINK.EXE` (MSVCRT import renamed) | 6.00.8168 | `a48e47ff5c7410276e1fc78c5e7d7107d5202386439b0b60543147d719997336` |
| `vc6/nativecrt/MSVCRX.DLL` (the CD's `MSVCRT.DLL`) | 6.00.8168 | `fb6260b07c05bdbcb6928156da514be5b6b69f095b335692ba46c561d85bd5f8` |
| `vc6/VC98/LIB/LIBCMT.LIB` | | `1ef9c27b4f76031e40fcbdf7d518adc4969f90d6a00dd1ea88697907268c1b2b` |
| `vc6/VC98/LIB/LIBCIMT.LIB` (the old iostream library; server only) | | `a78e4d21e853b385b043a1dfd0d65e8a3030eb0d776f4d66c2f76afeee6c115f` |
| `vc6/VC98/LIB/OLDNAMES.LIB` | | `348a220e94f90bd9f530514ae59ff9b044b76e2056f52b45df406c12d2156afb` |
| `vc6/VC98/LIB/KERNEL32.LIB` | | `cd4a546f71481319aeee455aa0fb51698a094f9d3e6d8339acf445489a263c68` |
| `utils/vgui/lib/win32_vc6/vgui.lib` (in the branch; client only) | | `df1d49f874c87ba1fbcfe0d164bec90d7940e81341c01c001a701b85ef46677a` |

The tools run on the CD's own `MSVCRT.DLL`, not on Wine's builtin one. LINK
uses the runtime's `qsort` to choose which copy of identical functions it
keeps and to order the import tables, and Wine's `qsort` breaks ties
differently. The client depends on this. See [vc6-notes.md](vc6-notes.md).

Not tried: other Wine versions, and Windows itself. On Windows, LINK would
load the system's `msvcrt.dll`; whether its `qsort` breaks ties as the 1998
DLL does is not known.

### If your build differs

| Symptom | Cause |
|---|---|
| Client: 8 bytes differ, at file offsets 0xf0 and 0x44204 | LINK didn't run at retail's time (COFF header and export directory stamps) |
| Client: functions shifted, other copies of identical functions kept | LINK ran on Wine's builtin `msvcrt.dll` |
| Server: 5 bytes of the debug record differ (PDB signature and age) | the 18 links weren't replayed, or an old PDB was left in `dlls\Releasehl` |
| Server: the export directory's stamp equals the COFF header's | the final link ran without the two-second switch |
| Server: the PDB path and the size of the debug record differ | the tree wasn't at `K:\Wantedhl\Source Code\New Code\Code` |
| Server: header inlines in other places in `bot.obj`, `colonel.obj`, `combat.obj` | only a full build, without the last incremental build |

## What the image holds besides code

A DLL also records how it was built. [reproducing.md](reproducing.md)
explains each part in general. For Wanted!:

| Item | `wanted.dll` | `client.dll` | How the build gets it |
|---|---|---|---|
| PDB record: a debug directory entry in CodeView "NB10" format that names the PDB file | 76 bytes after the last section: `K:\Wantedhl\Source Code\New Code\Code\dlls\Releasehl\hl.pdb`, signature 0x3ac85ac4 (2001-04-02 10:56:04 UTC), age 18 | none (no `/debug`) | The tree at that path on drive `K:`; 18 links into a fresh PDB, the first at its creation time |
| Time stamps | COFF header and debug directory 0x3b1cdaef (13:13:19); export directory 0x3b1cdaed (13:13:17) | COFF header and export directory 0x3a2918d6 | LINK under libfaketime; for the server, the clock switch after `hl.exp` |
| Image version | 1.1 | 0.0 | `/version:1.1` in `hl.dsp` (`25f618f`) |
| Rich header: a block LINK writes before the PE header that counts the objects each tool made | Utc12 C 8168 ×127, Utc12 C++ 8168 ×146, Masm 6.13 7299 ×32, AliasObj 7291 ×3, Lib 8034 ×3, Import0 ×81, Linker 6.00 8168 ×1 | Utc12 C ×128, C++ ×61, Masm ×30, AliasObj ×2, Lib 8034 ×9, Import0 ×636, Linker 8168 ×3 | The CD's compiler and libraries, and retail's object count. Server C++: 120 project files, 8 placeholders and 18 library objects. Client C++: 50 project files and 11 LIBCMT objects |
| PE header position (`e_lfanew`) | 0xe0 | 0xe8 | LINK reserves header space by the number of objects; the server needs the placeholders for it |
| Imports | KERNEL32, 81 functions | KERNEL32 65, `vgui.dll` 542, USER32 3, WINMM 3, WSOCK32 1 | The `.dsp` library lists, and LINK on the CD's `MSVCRT.DLL` |
| Exports | 487, as `hl.dll`: `GetEntityAPI`, `GetEntityAPI2`, `GiveFnptrsToDll`, 288 entity factories, 196 decorated Think/Touch/Use members | 454, as `client.dll`: 37 client interface functions, 417 decorated names (stubs, VGUI members, `InvisibleButton`) | `hl.def` and the `EXPORT` macro; the client's `dllexport` declarations |
| Dead code and identical-code folding (ICF: LINK keeps one copy of functions with identical bytes and relocations) | none: `/debug` turns `/OPT:REF` and `/OPT:ICF` off, so every function compiled stays in the image | on, and the choice of kept copy depends on dead functions too | For the client, retail's full set of foldable functions, one dead placeholder included, and LINK on the CD's runtime |
| Function order inside an object | header inlines placed by `/YX`, three objects from an incremental build | header inlines placed by `/YX` in a full build | A full build in project order; for the server, the replayed incremental build |
| `.bss`: uninitialised globals, which take no file space but are addressed by the code | ordered by a hash of their names | the same | Names (and for nine statics, zero initialisers) that give retail's order |
| C runtime | `/MT`, static LIBCMT, plus LIBCIMT for the bots' `ifstream` code | `/MT`, LIBCMT | The CD's `VC98/LIB` |

**Precompiled header and build history.** With `/YX` (VC6's automatic
precompiled header) CL saves the state after a file's leading headers in
`vc60.pch`. A later file with the same leading headers reuses it; another
file rebuilds it. A file that reuses the `.pch` emits the header inline
functions it needs lazily, between its own functions, so the order of
functions in an object shows which `.pch` its source found. That, in turn,
depends on the files compiled before it. [vc6-notes.md](vc6-notes.md) has
the rule.

## What in the source is not known from retail

The image gives every function body that the compiler emitted. It gives
names only through exports and decorated symbols, and nothing at all for
text that compiled to no code. So the source text is one spelling that
gives retail's bytes, not necessarily Maverick's. This section lists:

- placeholders for things the binary only counts;
- `// sic` lines and other source that exist to steer the compiler;
- names chosen for the `.bss` layout;
- other choices the evidence leaves open;
- the build history the script stands in for.

Commit messages give the full reasoning. Some source comments point to
`PLAN.md` or to tools by name: those were the working notes and scripts of
the reconstruction and are not in this repository. This page carries what
they said.

### Placeholders

| Where | What | Why | Evidence |
|---|---|---|---|
| [`dlls/untraced1.cpp`](../../../blob/wanted/dlls/untraced1.cpp) to [`untraced8.cpp`](../../../blob/wanted/dlls/untraced8.cpp), [`dlls/hl.dsp:660-689`](../../../blob/wanted/dlls/hl.dsp#L660-L689) | Eight C++ files without code, last in `hl.dsp`. Each includes exactly the headers of `wtownb.cpp`, the file before them. Names ours (`e66989c`). | Retail's Rich header counts 146 C++ objects; the sources give 138. LINK sizes the PE header by the object count (`e_lfanew`). | No code, data or string of the image belongs to an object we lack, and the library members LINK loads give retail's runtime code exactly, so the eight were project files that compiled to nothing. We put each of the 32 SDK 2.2b sources the project lacks back into `hl.dsp` as an include-only file: 30 change nothing visible, `apache.cpp` and `barney.cpp` would move inlines in `bag1.obj` and `beartrap.obj`, and none explains anything retail has. Their names, places and contents are unknowable. With `wtownb.cpp`'s include list they reuse its `.pch`; an empty file would write an empty `.pch` that the later compiles reuse, which would undo the incremental build's placements. |
| [`dlls/bot.cpp:717-771`](../../../blob/wanted/dlls/bot.cpp#L717-L771) | `BotStandIn`, an inline function of 42 statements that nothing calls. Name ours (`ca08901`). | VC6 compiles an uncalled inline function to its intermediate language (IL) but emits no code. The IL moves where `bot.obj`'s header inlines land: `CBasePlayer`'s inline virtuals follow `FightThink`, `CheckEnemy` and `CheckGoalItem` only if `bot.cpp`'s own IL from its first function to the end of `FightThink` is 28,355 to 28,468 bytes. Without the stand-in it is about 2,960 bytes short. | Retail's `bot.cpp` had that much text between `LookAround` and `FightThink` that compiled to no code. An uncalled inline, an `if ( 0 )` block and statements after a `return` all add IL, leave every function's code unchanged, and leave no string or float constant in the object, so nothing in the image tells which it was. No public bot source we could check shares code with the Cow-Bots (botman's bot, HPB bot, XDM's bots, Valve's CS bot). The stand-in was accepted on 2026-10-30, after the search for the real text failed. With it, and the three functions in the next table, `bot.obj` is identical to retail's. |
| [`cl_dll/hl/hl_weapons.cpp:818-827`](../../../blob/wanted/cl_dll/hl/hl_weapons.cpp#L818-L827) | `HUD_WeaponsUnknown`, an empty function after `HUD_PostRunCmd`. Name and body ours (`5f7d773`). | It compiles to a 16-byte COMDAT (`ret` and 15 `nop`s) with no relocations. Retail's `hl_weapons.obj` had one more dead function of at most 16 bytes there. LINK drops it, but its ICF sort still counts it, and without it LINK kept other copies of identical functions in `ammo.obj` and the VGUI objects: six functions differed, code moved by up to 112 bytes. | A model of LINK's ICF sort, built for Point of View: one such function between `HUD_PostRunCmd` and the deferred `CBaseMonster::CoverRadius` puts all 841 sections at retail's addresses, and nothing else does (no other single insertion, deletion or swap, two to four extra stubs, 121 other `/YX` states, subsets of SDK 2.2b's extra stubs, or other SDK versions of the file). The real link then gives retail's file. No SDK version has a function there. Retail's `.reloc` size fits a body without absolute fixups. |

A COMDAT is a section of an object file that holds one function. VC6's
`/O2` puts every function in its own COMDAT, so LINK can drop unreferenced
ones (`/OPT:REF`) and fold identical ones (`/OPT:ICF`).

### Source that steers the compiler

These lines are needed for retail's bytes, but their text is ours. Retail
had something with the same effect. All are marked `// sic`.

| Where | What | Effect | Evidence |
|---|---|---|---|
| [`dlls/bot.cpp:484-523`](../../../blob/wanted/dlls/bot.cpp#L484-L523) (called at [87](../../../blob/wanted/dlls/bot.cpp#L87), [108](../../../blob/wanted/dlls/bot.cpp#L108), [127](../../../blob/wanted/dlls/bot.cpp#L127); declared in [`bot.h:271-275`](../../../blob/wanted/dlls/bot.h#L271-L275)) | `MakeCamera`, `WantsBetterWeapon`, `CanFireSecondary`: statements of `Spawn` and `PressFire` moved into inline functions defined after their callers. Names ours (`ca08901`). | Same code: VC6 expands them in place. They move 834 bytes of IL out of the stretch before `LookAround`'s end. | Retail's `bot.obj` has the out-of-line `Length` and `Length2D` after `PickGoalItem`, which needs `bot.cpp`'s own IL up to the end of `LookAround` below 10,980 bytes: at least 745 bytes less than with these statements in place. `Spawn` and `PressFire` have inline budget to spare, so the code doesn't show how retail split them. Which statements retail had in separate functions is unknowable. |
| [`dlls/util.h:18-27`](../../../blob/wanted/dlls/util.h#L18-L27) | Ten comment lines above the inline functions (`c5175b2`) | **Function order, not code.** They put the `cbase.h` inlines 28 IL bytes further on (a line record past line 127 takes two more IL bytes), which gives `aflock.obj`, the first object, retail's order. | Wanted's `util.h` had 8 to 12 more lines than the SDK's there, and nothing in `aflock.cpp` gives the same effect cleanly. The rule behind it predicted 55 variants of `aflock.obj` exactly. The client compiles `util.h` too; it is identical with and without the lines. |
| [`dlls/nodes.cpp:27-33`](../../../blob/wanted/dlls/nodes.cpp#L27-L33) | An enum of 27 unused constants, names ours (`d19fb1d`) | VC6 numbers every symbol it declares. The reloads `LinkVisibleNodes` and `BuildNodeGraph` schedule at their loop ends follow those numbers modulo 64; 26 to 30 more symbols give retail's order. The enum declares 28. | Retail's headers declared more than ours before this file's functions; their text is unknown. |
| [`dlls/botattack.cpp:271`](../../../blob/wanted/dlls/botattack.cpp#L271), [`276`](../../../blob/wanted/dlls/botattack.cpp#L276) | `#pragma inline_depth( 1 )` before `CBotMemory::Update`'s four look-direction accessors, `#pragma inline_depth()` after them (`17f79ea`) | A call nested in an inline function gets the depth limit in force where that function was defined. With depth 1, the accessors expand but their return copies stay out of line, as in retail. | Retail's `Update` calls those copies out of line although its inline budget would expand them; no budget arrangement gives that. Allowed on 2026-10-29 as marked steering. Whether Maverick's source had a pragma is unknown. |
| [`dlls/player.cpp:3776`](../../../blob/wanted/dlls/player.cpp#L3776) | `tr.pHit` in a temporary in `CheatImpulseCommands` (impulse 107) (`e0384de`) | Retail's x87 operand order for impulse 202's `pev->origin + pev->view_ofs`. VC6 picks it by the number of locals the function stores to, modulo 8. | One, nine or seventeen more stored locals give retail's order; the function's text is otherwise the SDK's. The temporary stands in for the local we lack and changes no instruction. |
| [`dlls/puma.cpp:644`](../../../blob/wanted/dlls/puma.cpp#L644), [`1230`](../../../blob/wanted/dlls/puma.cpp#L1230) | Unused locals `flDist` (the wander task) and `vecStart` (`StepUp`) (`29c51b7`) | Inline budget: dead code still counts towards how much VC6 inlines | They only move VC6's inlining; the puma's 32 functions match retail. |
| [`dlls/snake.cpp:221-228`](../../../blob/wanted/dlls/snake.cpp#L221-L228) | Four discarded `vecForward.Length()` calls in `CSnake::RunAI` (`8b91e2c`) | Inline budget: with them, VC6 calls the back trace's `Vector` operators out of line, as retail does. Three or six don't work. | Retail's real leftover is unknown. |

The x87 operand order is the order in which VC6 loads the two operands of a
floating-point add or multiply on the FPU's register stack. Dead code can
change it without changing what the code computes. VC6's inliner works with
a budget: twice the caller's size in IL nodes, at least 1000, shared among
the call sites in evaluation order. [vc6-notes.md](vc6-notes.md) has these
rules.

### Names chosen for the `.bss` layout

VC6 lays out a file's uninitialised globals in the walk order of a
1024-bucket hash table, then its zero-initialised globals in definition
order. The key is `h = h*4 + (h >> 4) + c` over the name, bucket
`(h ^ (h >> 16)) % 1024`; a bucket lists its latest declaration first. The
name hashed is the plain identifier for file-scope data, and the full
decorated name for function statics and static class members. The rule
predicts every object's `.bss` in the image, and the order of a ladder of
3,072 test names and of 42 random sets of 30. Names reach the image this
way even though no symbol does, so these names were chosen to give
retail's layout:

| Where | What | Evidence |
|---|---|---|
| [`dlls/botnav.cpp:50-58`](../../../blob/wanted/dlls/botnav.cpp#L50-L58) | `g_BotNavigator`, `g_cFindPaths`, `g_flNavSaveTime`, and `g_iNavUnused` (`// sic`) for 4 bytes that no code reads or writes. Names ours. | Retail's addresses, in the comment at line 50 |
| [`dlls/ctc_gamerules.cpp:58`](../../../blob/wanted/dlls/ctc_gamerules.cpp#L58) | `g_iChickenUnused` (`// sic`), another 4 unreferenced bytes. Name ours. | Retail's `ctc_gamerules.obj` has 4 unreferenced bytes of `.bss` |
| [`dlls/mexbandit.cpp:210`](../../../blob/wanted/dlls/mexbandit.cpp#L210) | `HMEXBANDIT_SENTENCE_TYPES`, the global the copied hgrunt enum declares. Name ours, after hgrunt's `HGRUNT_SENTENCE_TYPES`. | This name gives retail's order of `mexbandit.obj`'s `.bss` |
| [`dlls/wantedrules.cpp:72-77`](../../../blob/wanted/dlls/wantedrules.cpp#L72-L77), [`dlls/highnoon.cpp:69-71`](../../../blob/wanted/dlls/highnoon.cpp#L69-L71) | Zero initialisers (`= 0`, `= NULL`, `= FALSE`, `= ""`) on nine statics (`77d79d0`) | Retail lays these out in our definition order, which no hash walk of these names gives; zero-initialised data is laid out in definition order. Valve's own code uses the same form (teamplay's `num_teams = 0`). `highnoon.cpp`'s `s_szWins` (line 68) stays uninitialised: it is alone in the walk, first. |
| [`dlls/teamplay_gamerules.h:61-69`](../../../blob/wanted/dlls/teamplay_gamerules.h#L61-L69) | The Capture the Chicken rules class is `CHalfLifeCTC` (name ours; earlier commits call it `CCaptureTheChicken`) (`77d79d0`) | Its `Think` keeps the SDK's function statics `last_frags` and `last_time`, whose decorated names contain the class name. Retail has `last_time` first. `CCaptureTheChicken` hashes them to buckets 95 (`last_frags`) and 299 (`last_time`); `CHalfLifeCTC` to 529 and 11, both before `g_pWaitingChickens`' 878. The class name is the only free part of those names. |

### Other choices and guesses

| Where | What | Why, and what is open |
|---|---|---|
| [`dlls/hl.dsp:623-627`](../../../blob/wanted/dlls/hl.dsp#L623-L627) | `/D CLIENT_WEAPONS` on `weapons.cpp` only | Retail's `CanAttack` and `SendWeaponAnim` match only with it, the related functions in `client.cpp`, `player.cpp` and `util.cpp` only without it. The conditionals are Valve's text. Maverick may instead have edited the two functions to their `CLIENT_WEAPONS` branch; both give the same bytes. |
| [`dlls/botmove.cpp:1174`](../../../blob/wanted/dlls/botmove.cpp#L1174) | An unused `TraceResult tr;` in `SeekGoal` (`// sic`, `cf2bcef`) | Retail's `Roam`, which expands `SeekGoal`, calls `TraceResult`'s constructor out of line (at 0x10014a70) for an object no code reads. An unused local is our reading of that call. |
| [`dlls/bot.cpp:645-662`](../../../blob/wanted/dlls/bot.cpp#L645-L662) | `fRightBlocked = *(int *)&vecSpot.x;` and the same for `fLeftBlocked` (`// sic`) | Retail's source left each flag unset when the side was visible, and VC6 then read the stack slot the flag shares with the spot just tested. The branch writes that read out, so the build behaves the same: a stuck bot nearly always turns 180 degrees. The original text was probably shorter. |
| The bots' inline accessors and steps in [`dlls/bot.h`](../../../blob/wanted/dlls/bot.h), [`bot.cpp`](../../../blob/wanted/dlls/bot.cpp), [`botmove.cpp`](../../../blob/wanted/dlls/botmove.cpp), [`botnav.cpp`](../../../blob/wanted/dlls/botnav.cpp), [`botattack.cpp`](../../../blob/wanted/dlls/botattack.cpp) | Small inline functions such as `GetMoveGoals`, `HeadFor`, `BeginFrame`, `IsValidEnemy`, `RandomStep` (names ours), and spellings such as `vecCell[2] += 1` in `Wander` | Retail's inlining decisions show that the bot source was written with small inline functions: `MoveToLead`, for one, can't come from a single function body. Which statements each held is chosen to reproduce those decisions; the code is retail's. Several spellings (a subscript, an accessor, an assignment instead of a copy) were picked because they add or remove a free inline call site or a reference without changing code. The commit messages give the arithmetic. |
| [`dlls/bot.cpp`](../../../blob/wanted/dlls/bot.cpp) | Include and definition order: `player.h` right after `cbase.h`, `CountFrames` after `CanSee`, `BeginFrame` and `IsValidEnemy` after their callers (`a62ebba`) | Follow from where `bot.obj`'s header inlines land. VC6 never reorders top-level functions. |
| [`dlls/horse.cpp:71-78`](../../../blob/wanted/dlls/horse.cpp#L71-L78); [`bear.cpp:108-109`](../../../blob/wanted/dlls/bear.cpp#L108-L109), [`skelly.cpp:89`](../../../blob/wanted/dlls/skelly.cpp#L89), [`player.h:107`](../../../blob/wanted/dlls/player.h#L107), [`weapons.h:348`](../../../blob/wanted/dlls/weapons.h#L348), [`gattlinggun.cpp:68`](../../../blob/wanted/dlls/gattlinggun.cpp#L68) | A table of three sound names no code reads (`CHorse::pAttackHitSounds`); unused fields | Retail has them: the table at the start of `horse.obj`'s data, the fields in the object sizes and layouts. Their names are ours. |
| [`cl_dll/hl/hl_weapons.cpp:48-49`](../../../blob/wanted/cl_dll/hl/hl_weapons.cpp#L48-L49), [`578-580`](../../../blob/wanted/cl_dll/hl/hl_weapons.cpp#L578-L580) | SDK 2.0's client prediction code without the HLDM glock; the `switch` in `HUD_WeaponsPostThink` left empty (warning C4060) | How Maverick removed the glock is unknown; this is the smallest edit. SDK 2.0's `cl_dll.dsp` defines no `CLIENT_WEAPONS`, so this code is dead; the image checks it only through LINK's ICF sort and `.reloc`'s size, and both are equal. |
| [`cl_dll/hl/hl_baseentity.cpp:96-226`](../../../blob/wanted/cl_dll/hl/hl_baseentity.cpp#L96-L226) | `CBaseMonster` stubs: SDK 2.2b's set without its later weapon-prediction additions, plus `HasPumaGibs`, `HasBearGibs`, `HasSnakeGibs` and `SetTurnActivity` | The client's `CBasePlayer` vtable needs every `CBaseMonster` virtual of Wanted's `basemonster.h`. The stubs retail keeps, their order and the 135-slot vtable match; the dead ones are checked only through sizes and relocation counts. |
| [`cl_dll/vgui_telegram.cpp`](../../../blob/wanted/cl_dll/vgui_telegram.cpp), [`cl_dll/vgui_TeamFortressViewport.h`](../../../blob/wanted/cl_dll/vgui_TeamFortressViewport.h) | The telegram panel: the file name, `CTelegramPanel`, `g_szTelegram` and the members are ours; `InvisibleButton` is retail's (exported) | The file name only has to sort between `vgui_teammenu.cpp` and `view.cpp`. All changed viewport functions and the panel's functions match, and `g_szTelegram` lands at retail's 0x10060650. |
| [`dlls/hl.dsp`](../../../blob/wanted/dlls/hl.dsp) | The file names of the mod's sources | The IDE links in sorted file order, and retail's link order is alphabetical with the mod's files sorted in, so each name only has to fall into the right gap (the Gatling gun's file, for one, sorts between `gamerules` and `genericmonster`). |
| Throughout | Names of everything the image doesn't name: the game-rules classes, unexported functions, globals, fields, skill fields, bullet types, gib types | Comments mark many of them `name ours`. Names inside the decorated exports (the Think, Touch and Use members, entity factories) are retail's. Apart from exports, the only way a name reaches the image is the `.bss` hash above. |

### `// sic` lines that keep retail's behaviour

Of the 118 lines that carry `sic` (`git grep -n -w sic wanted`), 14 are the
placeholders, steering lines and stand-ins in the tables above. The other
104 keep retail's own code, data or layout, and retail's code proves each
of them. Most are bugs kept on purpose, for example:

- [`bot.cpp:899`](../../../blob/wanted/dlls/bot.cpp#L899): `FightThink` tests `weapon_knife` twice; the pick was meant.
- [`botnav.cpp:379`](../../../blob/wanted/dlls/botnav.cpp#L379), [`382`](../../../blob/wanted/dlls/botnav.cpp#L382): `Wander` scales the side step by 10, not by the loop counter, and reads the visit grid at (x, y, y).
- [`botmove.cpp:321`](../../../blob/wanted/dlls/botmove.cpp#L321): `BotInit` writes both colours into one pooled `"255"` literal.
- [`colonel.cpp:416`](../../../blob/wanted/dlls/colonel.cpp#L416): with `Scream()` gone, a `break` became an `if`'s body, so three times in four the task runs into the next case.
- [`scorpion.cpp:218`](../../../blob/wanted/dlls/scorpion.cpp#L218): the snark's `if ( pev->movetype = MOVETYPE_FLY )`.
- [`ramone.cpp:2182`](../../../blob/wanted/dlls/ramone.cpp#L2182): the repel creates the nonexistent `monster_human_ramone`.
- [`ctc.cpp:74`](../../../blob/wanted/dlls/ctc.cpp#L74), [`telegram.cpp:58`](../../../blob/wanted/dlls/telegram.cpp#L58): think and touch functions that aren't `EXPORT`, so a saved game drops them.

The rest are of the same kind: unused calls and locals whose code retail
has, schedule and sound names as retail spells them, and unused fields
that retail's object sizes require.

### Preprocessor directives

Compiled code keeps no preprocessor directives, so every conditional in the
branch needs a source of its own. An audit on 2026-10-28 diffed every file
changed since `v2.0` against each SDK release and listed the directives it
adds; nothing invented was found:

- `/D CLIENT_WEAPONS` on `weapons.cpp` only, from the binary (see above); no
  project-wide `CLIENT_WEAPONS`.
- `cl_dll.dsp` keeps SDK 2.0's flags, with `hl_wpn_glock.cpp` out and
  `vgui_telegram.cpp` in: retail has no glock code, and its 61 C++ objects
  are these 50 sources plus 11 LIBCMT objects.
- Two `#if 0` blocks (`ramone.cpp`'s `PainSound`, `snake.cpp`'s
  `SetYawSpeed`) come with the SDK 2.2b text the files copy.
- Include guards in the headers the reconstruction made (`bot.h`,
  `highnoon.h`, `wantedrules.h`) follow Valve's idiom and select no code.

The only directives added since are the `#pragma inline_depth` pair above.

### Build stand-ins

The build reproduces history that the image records only in part.

| Where | What | Why | Evidence and status |
|---|---|---|---|
| [`tools/release/wanted_server.py`](../tools/release/wanted_server.py): the last incremental build | `bot.cpp`, `colonel.cpp` and `combat.cpp` compiled once more after the full build, in that order | Retail's `colonel.obj` (`CTalkMonster::DeclineFollowing` late), `combat.obj` (`CGib::ObjectCaps` between the two `FInViewCone`s) and `bot.obj` (`CBasePlayer`'s inline virtuals spread over the file, so `bot.cpp` found a `.pch` holding `player.h` and `bot.h`) need `.pch` states that no full build in project order gives. Retail's last build was incremental. | One consistent history, not the only one. A `colonel.cpp` whose include list didn't start with `extdll.h`, `util.h`, `cbase.h` would give the same `colonel.obj` and `combat.obj` from a full build. `bot.cpp` compiled once rebuilds the `.pch` from its own list; compiled twice, the second compile reuses it and its inlines land elsewhere. Once was chosen as the simpler history: it needs a stand-in ten statements longer than twice would, but three moved-out functions instead of six or more. |
| [`tools/release/wanted_server.py`](../tools/release/wanted_server.py): 1 + 17 links | The PDB's link history | Retail gives only the PDB's creation time (signature) and the number of links (age 18) | The 16 middle links at retail's link time are a stand-in: their times, and what was compiled between them, are unknown. `hl.pdb` itself is not part of the image. |
| [`tools/release/wanted_server.py`](../tools/release/wanted_server.py): the final link's clock | 13:13:17 until `hl.exp` is written, then 13:13:19 | Retail's export directory is stamped two seconds before its image | Only the two stamps are known. |
| [`tools/release/wanted_server.py`](../tools/release/wanted_server.py): linking `hl.dll` | The output name | Retail's PDB is `Releasehl\hl.pdb` and its export name `hl.dll` | Inferred. A `/pdb:` option in Maverick's project would give the same record; the output name itself doesn't reach the image. |
| The machine | Wine 11.0 and the CD's `MSVCRT.DLL` | Stand in for the Windows system Maverick built on | The identical client shows that Maverick's LINK made the same ICF choices as LINK on the CD's runtime. |

**Known from retail, not stand-ins:** the tree's path and drive (from the
PDB record), the link times, the image version, the compiler and linker
builds (from the Rich header).

## How identity was reached

The work went in this order: functions first, then the layout, then
everything else the image records. [vc6-notes.md](vc6-notes.md) has the
compiler and linker rules this needed.

**Functions** (2026-10-13 to 10-17). We compared retail with builds of every
SDK release. The server matched best as SDK 2.0's multiplayer files plus
2.2b's single-player files, without `/GX`; the client as SDK 2.0's
`cl_dll.dsp`, which already covers 92.7% of retail's client `.text`. Then
Wanted's code was written from the disassembly and decompiled code,
compiled, and compared with retail one function at a time. By 2026-10-17
every retail function had source, and 3,470 of 3,535 server game functions
(93.7% of their bytes) compiled to retail's code. The rest were reviewed
for behaviour.

**The client** became identical first. Running LINK on the CD's
`MSVCRT.DLL` instead of Wine's (2026-10-28) fixed the kept copies of seven
folded stubs: 839 of 845 game functions were exact. A model of LINK's ICF
sort traced the last six to one dead function in `hl_weapons.obj`. With the
placeholder for it, `.text`, `.rdata`, `.data` and `.reloc` were retail's;
with LINK at retail's time, so was the file (`5f7d773`, 2026-10-29).

**The server file.** Whole-file matching began on 2026-10-28. At first only
8.7% of the server's `.text` sat at retail's addresses, because the four bot
objects together were 2,672 bytes longer than retail's and pushed everything
after them. Reading VC6's inliner out of `C2.DLL` explained why: the bot
source used small inline functions. Then the bytes that differ in the whole
file went down like this:

| Commit | Date | Bytes differing | What changed |
|---|---|---|---|
| `c5175b2` | 2026-10-29 | 822,614 | The starting point. Every object outside the bots had retail's size and order (`util.h`'s comment lines fixed `aflock.obj`), the build had the tree, the incremental build, the link time and the placeholders, but code after the bot objects still sat at shifted addresses. |
| `25f618f` | 2026-10-29 | 63,373 | 19 more functions identical (13 bot functions, 6 others) and two put in retail's slots, which ended the address drift: `.text` went from 9.7% to 99.0% identical. `/version:1.1`. |
| `8e9108a` | 2026-10-29 | 6,043 | 7 more functions identical, all with plain source forms. Most of the drop is `.reloc`, whose page counts now agree. Left: 2,592 bytes in four functions, 3,217 from `bot.obj`'s `Length`/`Length2D` placement, 27 in the `CBot` and `CBasePlayer` vtables, 201 in `.reloc`, 5 in the PDB record, 1 in the header. |
| `8e9108a` | | 6,038 | The same source, with the PDB's 18 links replayed: the PDB record's signature and age are retail's. |
| `ca08901` | 2026-10-29 | 2,757 | `BotStandIn` and the three moved-out functions, with `bot.cpp` compiled once in the incremental build: `bot.obj` identical. Left: the four functions and 42 bytes of `.bss` addresses in the rules objects (`.text` 2,637), `.reloc` 118, `.reloc`'s size in the header, the export directory's stamp. |
| `ca08901` | | 2,756 | The final link's two seconds: the export directory's stamp is retail's. |
| `455d243` | 2026-10-30 | 810 | The rules objects' `.bss` order (`77d79d0`, 2,714). `AvoidWalls`, `BotFindProfile` and `CheckAhead` identical. `Wander` gets retail's calls and stack frame. The headers are identical. |

The last steps needed rules read from `C2.DLL`, VC6's code generator: how
it packs locals into stack slots (`AvoidWalls`), how it ranks variables for
registers (`BotFindProfile`, whose fix also closed the last header byte,
`.reloc`'s size), and how it allocates the x87 stack (`CheckAhead`). See
[vc6-notes.md](vc6-notes.md).

## What is left

Only `CBot::Wander`'s 4 bytes, and the shift they cause.

In [`botnav.cpp:379`](../../../blob/wanted/dlls/botnav.cpp#L379),
`vecTest = vecCell + vecFwdCell * iDist + vecRightCell * iSide * 10;`, VC6
hoists `vecFwdCell * iDist` out of the inner loop. `Vector::operator*(float
fl)` is an inline function, and `fl` holds `iDist` on the x87 stack. For the
x component:

- our build treats `fl` as dying at that multiply. VC6 then emits `fld m;
  fmulp st(1)`, and a later peephole pass folds that into `fmul m`;
- retail's compile treated `fl` as still live: `fld m; fmul st, st(1)`, and
  `fl` is popped later with `fstp st(0)`.

Both compute the same product.

Whether `fl` counts as dying is set by the inline expansions earlier in the
function, in a pattern that repeats: every 4 top-level `Vector( x, y, z )`
constructors, every 3 `operator+`, `operator-` or `operator*` expansions,
every 2 unary minus, copy constructor or member setter expansions. Free
functions, getters, subscripts, padding statements, names, the frame size,
`/G3` to `/G6` and `/Os` don't move it. A likely mechanism,
not confirmed: C2 recycles its symbol records. They come 32 to a block with
fixed numbers, and freed records go on per-kind last-in-first-out lists and
keep their numbers, so the record `fl` gets depends on what was freed
before.

In a reduced copy of `Wander`'s sum, retail's form appears if `vecFwdCell`
is assigned rather than built in place (retail's code shows it built in
place), if it is evaluated first, or, without the `* 10`, if it is the last
`Vector` defined. With the `* 10` product between the two products, nothing
tried reached retail's state: 584 sequences of one to three inline
operations before it, loop forms, other spellings, 10 parenthesisations and
63 commuted forms. So Maverick's source differed in a way not found yet.

The next step is to read how `C2.DLL`'s inliner creates and frees parameter
symbols, statically, or to patch `C2.DLL` to log its last-use decisions.

## Check it yourself

1. Check the zip and the retail files against the hashes in
   [The retail files](#the-retail-files):

   ```sh
   sha256sum wantedsp.zip
   unzip wantedsp.zip
   sha256sum wantedsp/dlls/wanted.dll wantedsp/cl_dlls/client.dll
   ```

2. Build and compare:

   ```sh
   RETAIL=$PWD/wantedsp VC_TOOLCHAINS=/path/to/toolchains \
       tools/release/wanted.sh ../wanted ../wanted-build
   ```

   Expected: `identical to retail` for `client.dll`, with `cmp` silent, and
   `differs from retail` for `wanted.dll`, with SHA-256
   `e4dfb4369731a2a36c2ef71b4680361da7912f932bc78c5bb2a9cc4df696c99f`. The
   exit code is not 0, because the server differs.

3. See where the server differs. Use the path of the built `wanted.dll` the
   script printed:

   ```sh
   cmp -l path/to/built/wanted.dll wantedsp/dlls/wanted.dll |
       awk '{ if ($1 <= 999424) { t++; if (!f) f = $1; l = $1 } else r++ }
            END { print t " in .text (offsets " f "-" l "), " r " in .reloc" }'
   ```

   Expected: `801 in .text (offsets 88871-89707), 9 in .reloc`. `cmp`
   counts from 1; offset 88871 is file offset 0x15b26. `.text` starts at
   file offset 0x1000 and address 0x10001000, so that is address 0x10015b26,
   `CBot::Wander` (0x100156f0) + 0x436, and 89707 is the function's last
   byte. File offset 999424 (0xf4000) is where `.reloc` starts.

4. Look at the instruction itself, in both files:

   ```sh
   for f in wantedsp/dlls/wanted.dll path/to/built/wanted.dll; do
       objdump -d -M intel --start-address=0x10015b26 --stop-address=0x10015b60 "$f" |
           grep -E -m 5 '^1001[0-9a-f]+:'
   done
   ```

   Retail shows `fld DWORD PTR [esp+0xa8]`, `fmul st,st(1)`, `fstp DWORD PTR
   [esp+0xa4]`, `fstp st(0)`, then `mov ecx,DWORD PTR [esp+0xa0]` at
   0x10015b38. The build shows `fmul DWORD PTR [esp+0xa8]` and `fstp DWORD
   PTR [esp+0xa4]`, then the same `mov` 4 bytes early, at 0x10015b34.

5. Look at the parts besides code, on retail's files and on yours:

   ```sh
   TZ=UTC objdump -p wanted.dll | grep -E 'Time/Date|format NB10'
   TZ=UTC objdump -p client.dll | grep -E 'Time/Date|format NB10'
   ```

   Expected for `wanted.dll` (objdump prints the signature's bytes in file
   order):

   ```
   Time/Date		Tue Jun  5 13:13:19 2001
   Time/Date stamp 		3b1cdaed
   (format NB10 signature c45ac83a age 18 pdb K:\Wantedhl\Source Code\New Code\Code\dlls\Releasehl\hl.pdb)
   ```

   and for `client.dll` (no PDB line):

   ```
   Time/Date		Sat Dec  2 15:44:22 2000
   Time/Date stamp 		3a2918d6
   ```

The tools we used to compare partial matches are not in this repository: a
function matcher, a per-section image comparison, the reference checkers,
the `.bss` order tester and the models of VC6's inliner and LINK's ICF
sort. For the client, `cmp` is the whole test; for the server, `cmp -l`
shows that all differing bytes lie in one function and its `.reloc` block.
