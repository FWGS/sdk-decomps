# Point of View 1.1

Point of View is a single-player mod for Half-Life from 2003. The player is
an alien slave. Version 1.1 ships two game DLLs, both built with Visual C++
6.0:

- `dlls/hl.dll`, the server: the game logic, monsters and weapons;
- `cl_dlls/client.dll`, the client: the HUD, input and the story journal
  window.

The [`point-of-view`](../../../tree/point-of-view) branch holds C++ source
that compiles back into both files, byte for byte. It is Valve's Half-Life
SDK history up to the tag `v2.0`, plus 143 commits. Each commit message says
what the change matched and why.

## Status

Both DLLs are identical to retail.

| | Server `hl.dll` | Client `client.dll` |
|---|---|---|
| Size | 1,204,294 bytes | 364,544 bytes |
| SHA-256 | `0f42bad4664a87cf3793efd213088b9c2c5dd0ac230b45db3544fd7c344f7ccd` | `c0a40333d725c4a8d66e2640e78929ebc69bd118c28d3b79e0aab35d69608ea1` |
| Link time (UTC) | 2003-05-06 07:56:13 | 2003-05-02 01:24:25 |
| First identical at | `a7a4d18` (2026-10-28) | `469339a` (2026-10-29) |

Verified on 2026-11-01: fresh release builds of `469339a`, the head of the
branch, have retail's SHA-256, and `cmp` finds no difference. No server file
changed between `a7a4d18` and `469339a`.

## Retail files

The retail DLLs are not in this repository. They come from the mod's
installer:

- Page: ModDB, "Point of View 1.1" (download 5474),
  <https://www.moddb.com/mods/point-of-view/downloads/point-of-view-11>,
  added 2006-12-18 by DAVLevels.
- File: `pov.exe`, 52,058,200 bytes.
  - MD5 `e16a1bb8970d9e5f2263147ab9ec4157` (the value ModDB lists);
  - SHA-256 `ab89aa83f31c34d7f4ae35d7b3925059b3c01a69640f191e53c171c5dad5631e`.

`pov.exe` is an old Clickteam installer with its own variant of deflate.
7-Zip and the usual unpackers can't open it. We unpacked it without running
it, with our own script, which is not in this repository. Every file decoded
to exactly its listed size; the installer's own per-file checksums were not
checked. Running the installer should give the same files, but we didn't
try it. Either way, check the hashes above.

The two DLLs sit in the mod folder `pov/` as follows (`liblist.gam` says
`gamedll "dlls\hl.dll"` and `cldll "1"`):

```
pov/dlls/hl.dll
pov/cl_dlls/client.dll
```

That `pov/` folder is what the release script's `RETAIL` variable expects
(see below).

## Rebuilding

### What you need

- The toolchain folder from [setup.md](setup.md). This mod uses `vc6/`
  (Visual C++ 6.0 Professional, no service pack) and `wineprefix/`.
  [compilers.md](compilers.md) says where the compiler comes from.
- Wine (we used 11.0 with a win64 prefix), libfaketime's `faketime` (we used
  0.9.12), Python 3 (standard library only), `sha256sum` and `cmp`.
- A checkout of the `point-of-view` branch.

The branch stores its sources with CRLF line ends and has no
`.gitattributes`. Leave `core.autocrlf` off (the default on Linux), so git
checks the files out as stored. We haven't tested whether LF line ends
change the build.

### Commands

From a checkout of `main`:

```sh
git worktree add ../pov point-of-view    # or: git clone -b point-of-view <repo> pov
VC_TOOLCHAINS=/path/to/toolchains tools/release/point-of-view.sh ../pov ../pov-build
```

- The third argument picks `server`, `client` or `both` (the default).
- `../pov-build` is the build folder. The script creates it if needed.
- To compare with your own retail copy as well, set `RETAIL` to the mod
  folder:

  ```sh
  RETAIL=/path/to/Half-Life/pov VC_TOOLCHAINS=/path/to/toolchains \
      tools/release/point-of-view.sh ../pov ../pov-build
  ```

For each DLL the script prints the path of the built file, its SHA-256,
retail's SHA-256 and `identical to retail` or `differs from retail`. With
`RETAIL` set it also runs `cmp` against `dlls/hl.dll` or `cl_dlls/client.dll`
there. It exits with 0 only if every DLL it built is identical. A full
server build took about 20 seconds on our machine.

### What the script does

Both builds are full builds: every file is compiled again, in project order,
and the output folder starts empty.

**Server.** Retail's debug record names
`D:\Editors\Levels\POVSourceCode\dlls\Releasehl\hl.pdb`. `dlls\Releasehl` is
the output folder of the SDK's `hl.dsp` Release configuration, so POV's tree
was at `D:\Editors\Levels\POVSourceCode`. The script rebuilds that setting:

1. It copies the toolchain's Wine prefix into the build folder once, and
   maps the copy's drive `D:` to a folder inside the build folder.
2. It lays out `D:\Editors\Levels\POVSourceCode` as symlinks into the
   checkout. `dlls` and `cl_dll` are real folders of per-file symlinks, so
   the build output stays out of the checkout.
3. It deletes `dlls\Releasehl`, so LINK writes a new PDB (age 1).
4. It builds the configuration `hl - Win32 Release` of `dlls\hl.dsp` with
   `tools/build_dsp.py`. Paths go to the compiler and linker as `D:\...`,
   and LINK runs with the clock frozen at 2003-05-06 07:56:13 UTC.

**Client.** The client was linked without `/debug`. It has no debug
directory, so no path reaches the image and the source location doesn't
matter. The script builds the configuration `cl_dll - Win32 Release` of
`cl_dll/cl_dll.dsp` into an empty folder, with LINK's clock frozen at
2003-05-02 01:24:25 UTC.

**What `build_dsp.py` does with a `.dsp`.** It works like the VC6 IDE's
build:

- It compiles the sources in project order with the configuration's flags.
  It leaves out the IDE-only `/FD`, `/Fr`, `/FR` and `/Gm`.
- CL creates and reuses one `vc60.pch` per build, as in the IDE.
- It links with the configuration's `LINK32` line, objects in project order,
  and its own output names in the output folder.
- With `--link-time` it runs only LINK under `faketime`, with `TZ=UTC`.

The flags, from the two `.dsp` files:

| | Server (`hl.dsp`) | Client (`cl_dll.dsp`) |
|---|---|---|
| Compile | `/G5 /MT /W3 /Zi /O2 /YX`, defines `NDEBUG WIN32 _WINDOWS QUIVER VOXEL QUAKE2 VALVE_DLL` | `/MT /W3 /GX /Zi /O2 /YX`, defines `WIN32 NDEBUG _WINDOWS` |
| Link | `/subsystem:windows /dll /map /debug /machine:I386 /def:.\hl.def /incremental:no` | `/subsystem:windows /dll /map /machine:I386`, plus SDK 2.0's `utils/vgui/lib/win32_vc6/vgui.lib` |
| Sources | 139 `.cpp`, 3 `.c` | 51 `.cpp`, 3 `.c` |

### Toolchain files

These builds used the files below. [setup.md](setup.md) explains how to
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
| `vc6/VC98/LIB/OLDNAMES.LIB` | | `348a220e94f90bd9f530514ae59ff9b044b76e2056f52b45df406c12d2156afb` |
| `vc6/VC98/LIB/KERNEL32.LIB` | | `cd4a546f71481319aeee455aa0fb51698a094f9d3e6d8339acf445489a263c68` |
| `utils/vgui/lib/win32_vc6/vgui.lib` (in the branch) | | `df1d49f874c87ba1fbcfe0d164bec90d7940e81341c01c001a701b85ef46677a` |

The tools run on the CD's own `MSVCRT.DLL`, not on Wine's builtin one. Wine's
`qsort` orders equal keys differently from the 1998 DLL, and LINK uses
`qsort` to pick which copy of identical functions it keeps and to order the
import table. The compiler doesn't depend on it. See
[vc6-notes.md](vc6-notes.md).

### If your build differs

These are the differences we met along the way, and their causes:

| Symptom | Cause |
|---|---|
| Server: `TlsGetValue` and `CreateDirectoryA` swapped in the import table (7 bytes of `.text`, 4 of the import tables differ) | LINK ran on Wine's builtin `msvcrt.dll` |
| Client: functions shifted, other copies of identical functions kept | the same |
| Server: the PDB path and the size of the debug record differ | the tree wasn't at `D:\Editors\Levels\POVSourceCode` |
| PDB age 2 instead of 1 | an old PDB was left in `dlls\Releasehl` |
| Time stamps differ | LINK didn't run under `faketime` |
| A few functions differ after a header edit | an incremental build; once this lost 6 server functions that a full build matched. The script always builds in full. |

## What the image holds besides code

A DLL also records how it was built. [reproducing.md](reproducing.md)
explains each part in general. For POV:

| Item | `hl.dll` | `client.dll` | How the build gets it |
|---|---|---|---|
| PDB record: a debug directory entry in CodeView "NB10" format that names the PDB file | 70 bytes after the last section, at file offset 0x126000: `D:\Editors\Levels\POVSourceCode\dlls\Releasehl\hl.pdb`, age 1 | none (no `/debug`) | The tree at that path on drive `D:`; an empty output folder, so the PDB is new. The record's size follows from the path. |
| Time stamps | COFF header, export directory, debug directory and NB10 signature: `0x3eb76a9d` | COFF header and export directory: `0x3eb1c8c9` | LINK runs with the clock frozen at retail's link time. |
| Rich header: a block LINK writes before the PE header that counts the objects each tool made | Utc12 C 8168 ×122, Utc12 C++ 8168 ×142, Masm 6.13 7299 ×30, AliasObj 7291 ×3, Lib 8034 ×3, Import0 ×74, Linker 6.00 8168 ×1 | Utc12 C ×128, C++ ×62, Masm ×30, AliasObj 7291 ×2, Lib 8034 ×9, Import0 ×636, Linker 8168 ×3 | The CD's compiler, libraries and C runtime, and retail's exact object list. Server C++: 138 project files, a placeholder and 3 LIBCMT objects. Client C++: 50 project files, a placeholder and 11 LIBCMT objects. Two of the client's Linker entries come from SDK 2.0's `vgui.lib`. |
| PE header position (`e_lfanew`) | 0xe8 | 0xd8 | LINK reserves header space by the number of objects. Without the placeholders the two would be 0xd8 and 0xe8. |
| Imports | KERNEL32, 74 functions | USER32 3, WINMM 3, `vgui.dll` 542, WSOCK32 1, KERNEL32 65, in that order | The `.dsp` library lists, and LINK on the CD's `MSVCRT.DLL`. |
| Exports | 570: 304 entity factories, 263 decorated Think/Touch/Use/Blocked members, `GetEntityAPI`, `GetEntityAPI2`, `GiveFnptrsToDll` | 449: 37 client interface functions, 398 VGUI template members, 14 `hl_baseentity.cpp` stubs | Server: `hl.def` and the `EXPORT` macro, with retail's names (the class stays `CDecal`, `b67b08f`; `CGenericMonster::MonsterThink` is `EXPORT`ed, `b2b36c0`). Client: `dllexport` declarations, VGUI's included. |
| Identical-code folding (ICF): LINK keeps one copy of functions with identical bytes and relocations | none: `/debug` turns `/OPT:REF` and `/OPT:ICF` off by default | on: one copy of each group is kept | LINK on the CD's `MSVCRT.DLL`, and retail's full set of foldable functions, dead ones included (see the placeholder in `hl_weapons.cpp` below). |
| `.reloc` virtual size | 0x1a2e2 | 0x7e72, which also counts the 4 fixups of a function LINK dropped | Follows from the code; for the client, also from the placeholder in `hl_weapons.cpp`. |
| Link order | `hl.dsp` order: alphabetical, POV's `pov_*.cpp` between `pm_shared.c` and `python.cpp`, the placeholder last | SDK 2.0's `cl_dll.dsp` order, `pov_icons.cpp` between `pm_shared.c` and `saytext.cpp`, the placeholder last | `build_dsp.py` links in project order. |
| C runtime | `/MT`, static LIBCMT; no `/GX`, so no C++ exception runtime | `/MT`, LIBCMT with `/GX`'s exception objects | The CD's `VC98/LIB`. |
| Padding | zero | zero | Wine hands LINK zeroed memory. Since the files are identical, retail's padding was zero too. |

**Data.** Each object's initialised data comes first, in definition order,
then its string literals. Uninitialised globals (`.bss`) are ordered by a
hash of their names. The source reproduces both as it is.

**Precompiled header and build history.** With `/YX` (VC6's automatic
precompiled header) CL saves the state after a file's headers in
`vc60.pch` and reuses it in later files that include the same headers. Which
headers a file finds in the `.pch` decides where header inline functions
land in its object, at the start or at the end. The `.pch`, in turn, depends
on the files compiled before. POV's own build history is unknown. One full
build in project order stands in for it, with CL creating and reusing the
`.pch` itself. The release builds simulate no other `.pch` state.

## What in the source is not known from retail

Every function of retail's two DLLs comes out of the branch byte for byte,
at its retail address. But the source text is only one spelling that does
so. This section lists the lines that exist for another reason:

- placeholders for facts the binary doesn't hold;
- `// sic` lines that steer the compiler;
- choices the evidence leaves open, and names we made up.

The commit messages give the full reasoning. A few source comments point to
`PLAN.md` or `tools/bss_try.py`: those were the working notes and scripts of
the reconstruction and are not in this repository. This page carries what
they said.

### Placeholders

| Where | What | Why | Evidence |
|---|---|---|---|
| [`dlls/placeholder_object.cpp:1-10`](../../../blob/point-of-view/dlls/placeholder_object.cpp#L1-L10), [`dlls/hl.dsp:719`](../../../blob/point-of-view/dlls/hl.dsp#L719) | An empty C++ file, last in `hl.dsp`. The name is ours. | Retail's Rich header counts 142 C++ objects; the code shows 141. LINK sizes the PE header by the object count: 0xd8 without it, retail's 0xe8 with it. | `a7a4d18`. The missing object left no code and no data. The server links with `/debug`, so `/OPT:REF` is off and code couldn't have been dropped. What it was is open: a file of declarations only, or a split our file list doesn't make (beretta, m41a, `weapon_loader`, `area_*` or `env_spawnereffect`). |
| [`cl_dll/placeholder_object.cpp:1-13`](../../../blob/point-of-view/cl_dll/placeholder_object.cpp#L1-L13), [`cl_dll/cl_dll.dsp:333`](../../../blob/point-of-view/cl_dll/cl_dll.dsp#L333) | The same for the client. The name is ours. | Retail counts 62 C++ objects; the code shows 61. PE header 0xe8 without it, retail's 0xd8 with it. | `a1f9d66`. An empty object appended to the link changes neither `.text` nor the kept copies. SDK 2.0's project builds one more C++ file, `..\dlls\wpn_shared\hl_wpn_glock.cpp`, but nothing proves it was that one. Building it empty in the client would need a preprocessor guard that rests on an object count only, so it isn't done. |
| [`cl_dll/hl/hl_weapons.cpp:109-114`](../../../blob/point-of-view/cl_dll/hl/hl_weapons.cpp#L109-L114) | `Vector __pov_unknown_stub( void ) { return g_vecZero * gpGlobals->time; }`, right after `CBaseEntity::Killed`. Name and body are ours. | Retail compiled a function here that nothing calls. `/OPT:REF` dropped it, but it still sits in LINK's sort array and changes which identical copy LINK keeps. | LINK's sort, with `ammo.obj`'s order, leaves a few COMDAT shapes, all in `hl_weapons.obj`. `.reloc`'s virtual size gives four absolute (dir32) fixups: 0x7e6a with none, 0x7e70 with three, retail 0x7e72. That leaves 80 bytes with 4 relocations right after `Killed` (committed), or a 96-byte function further on. One of the latter also gives retail's SHA-256: `return a * gpGlobals->time + g_vecZero;` with a `const Vector &a`, after `DefaultDeploy`. |

A COMDAT is a section of an object file that holds one function. VC6's
`/O2` puts every function in its own COMDAT, so LINK can drop unreferenced
ones (`/OPT:REF`) and fold identical ones (`/OPT:ICF`).

### `// sic` lines that steer the compiler

These lines are needed for retail's bytes, but their text is ours. Retail
had something with the same effect.

| Where | What | Effect | Evidence |
|---|---|---|---|
| [`cl_dll/ammo.cpp:835-845`](../../../blob/point-of-view/cl_dll/ammo.cpp#L835-L845) | Nine empty statements at the top of `CHudAmmo::Draw` (comment on 835-836, `;` on 837-845) | **Section order, not code.** They advance the compiler's position in its intermediate language (IL), so `Dar<IntChangeSignal>::operator=` and the late precompiled-header inlines land in `ammo.obj` where retail has them. That decides LINK's kept copies. | Release builds with 8, 9 or 10 `;` give retail's file; 7 and 11 don't. They stand in for 98-112 IL bytes of unknown text in `Draw` or the functions after it; with the `if` spelling below (+60 IL) the `;` (5 IL each) cover the 38-52 left. No plausible text was found. |
| [`dlls/func_tank.cpp:820`](../../../blob/point-of-view/dlls/func_tank.cpp#L820) | Unused `BOOL fire = FALSE;` in `CFuncTank::TrackTarget` | Flips the x87 operand order of an inlined vector add | The function matches only with it (`7c45126`). |
| [`dlls/player.cpp:3960`](../../../blob/point-of-view/dlls/player.cpp#L3960) | Dead `int x = 0;` in `CBasePlayer::CheatImpulseCommands` (impulse 107) | Retail's operand order for the x component of `pev->origin + pev->view_ofs` | `7504140` |
| [`dlls/pov_loader.cpp:831`](../../../blob/point-of-view/dlls/pov_loader.cpp#L831) | Unused `Vector vecAngles = pev->angles;` in `CALoader::LaunchGrenade` (name ours) | Inline budget: dead code still counts towards how much VC6 inlines | `2ece5b9` |
| [`dlls/pov_loader.cpp:886`](../../../blob/point-of-view/dlls/pov_loader.cpp#L886) | Parentheses around `gSkillData.gargantuaHealth * 1.2` in `CALoader::Spawn` | VC6 keeps them as a node in its expression tree; the x87 store stays before the next pushes | `396fe00` |
| [`dlls/pov_stukabat.cpp:1279-1281`](../../../blob/point-of-view/dlls/pov_stukabat.cpp#L1279-L1281) | Parentheses around the yaw in `CStukabat::StukabatIdle` | The x87 store comes after the pushes | `d7a3267` |
| [`dlls/pov_toad.cpp:784-788`](../../../blob/point-of-view/dlls/pov_toad.cpp#L784-L788) | Unused `vecSrc` and the `pEntity` copy in `CToad::ToadIdle` | Add order in the first `Length()` | `3ded2c6` |
| [`dlls/pov_toad.cpp:861-863`](../../../blob/point-of-view/dlls/pov_toad.cpp#L861-L863) | Parentheses around the yaw in `CToad::ToadIdle` | The x87 store comes after the pushes | `3ded2c6` |

The x87 operand order is the order in which VC6 loads the two operands of a
floating-point add or multiply. Dead code and parentheses can change it
without changing what the code computes. [vc6-notes.md](vc6-notes.md) has
the rules we found.

### Other choices and guesses

| Where | What | Why, and what is open | Evidence |
|---|---|---|---|
| [`cl_dll/ammo.cpp:944`](../../../blob/point-of-view/cl_dll/ammo.cpp#L944) | POV's secondary-ammo edit written `if (pw->iAmmo2Type != 0)`: the SDK's test without its count | The same code: VC6 drops the test under the enclosing `> 0`. It adds 60 IL bytes towards the budget of the `;` lines above. POV's own text is unknown. | `469339a` |
| [`cl_dll/hl/hl_baseentity.cpp`](../../../blob/point-of-view/cl_dll/hl/hl_baseentity.cpp) (after 95; 121, 244-245, 288-289) | SDK 2.2b's stub file with POV's changes: 2.2b's block after `CGrenade::Spawn` (line 95) removed, `ShouldSimplify` (121) kept, 2.0's step-sound stubs (244-245) kept, `BloodColor` before `GetGunPosition` (288-289) | Retail's 17 folded stubs come in 2.2b's order. Dead stubs count only through LINK's sort. Removing any six of `ShootTimed`, `ShootContact`, `UTIL_Remove`, `UTIL_SetSize`, `UTIL_FindEntityInSphere`, `UTIL_VecToAngles` and `ShouldSimplify` gives the same result; we removed the contiguous block. | `5e0b02d`: retail has no `DetonateUse` export and no `gSkillData` in the client's `.bss` |
| [`cl_dll/vgui_TeamFortressViewport.h:580, 595, 610, 625, 640`](../../../blob/point-of-view/cl_dll/vgui_TeamFortressViewport.h#L577-L644) | The five `CMenuHandler_Story*` classes (names ours) with empty `( void )` constructors | They change register choice in the journal window's constructor and the COMDAT order of six objects, which moves kept copies. Four or five of the five give retail's code; all five is the natural edit. | `a1f3097` |
| [`dlls/globals.cpp:40-42`](../../../blob/point-of-view/dlls/globals.cpp#L40-L42) | `g_fLevelLoaded`, `g_flFadeEndTime`, `g_fSilentPickup` (names ours) | VC6 orders uninitialised globals by a hash of their names. These names give retail's `.bss`. | `d05f31b` |
| [`dlls/func_break.cpp:27`](../../../blob/point-of-view/dlls/func_break.cpp#L27) | `g_vecAttackDir`'s declaration before `#include "func_break.h"` | It ends the `/YX` header region, so `func_break.h` stays out of the `.pch` and `CBreakable::ObjectCaps` follows the factory as in retail. One of several spellings. A no-code file between `flyingmonster.cpp` and `func_break.cpp`, perhaps the missing server object, would do the same. | `b527b76` |
| [`dlls/barney.cpp:20-23`](../../../blob/point-of-view/dlls/barney.cpp#L20-L23), [`dlls/buttons.cpp:23-29`](../../../blob/point-of-view/dlls/buttons.cpp#L23-L29), [`dlls/pov_barniel.cpp:21-32`](../../../blob/point-of-view/dlls/pov_barniel.cpp#L21-L32) | Include lists | Chosen for where header inlines land through `/YX`. In `barney.cpp` and `buttons.cpp` they land in the file's own object; for `buttons.cpp` this is the one include set that works. `pov_barniel.cpp`'s list puts `CScientist::ObjectCaps` at the end of the next object, `pov_brave.obj`. The barniel's own object doesn't show its includes. | `6b906f5`, `9b6137e` |
| [`dlls/monsters.cpp:2226`](../../../blob/point-of-view/dlls/monsters.cpp#L2226) | `stricmp`, not `strcmpi` | Retail's Rich header has 3 AliasObj entries (`stricmp`, `strnicmp`, `itoa`); `strcmpi` made 4. Each spelling loads its own alias object from `OLDNAMES.LIB`. Same code. | `2fbd2e5` |
| [`dlls/pov_hev_spforce.cpp:218-230`](../../../blob/point-of-view/dlls/pov_hev_spforce.cpp#L218-L230) | The two tests in `CHEVSPForce::CheckRangeAttack2` | Retail keeps only the load of `m_iWeaponUsed`. Which weapons are tested is a guess: VC6 drops the compares. | the function's comment |
| [`dlls/hl.dsp:439-607`](../../../blob/point-of-view/dlls/hl.dsp#L439-L607), [`cl_dll/cl_dll.dsp:253`](../../../blob/point-of-view/cl_dll/cl_dll.dsp#L253) | The file names `dlls/pov_<word>.cpp` (43 files) and `cl_dll/pov_icons.cpp` | The IDE links in sorted file order, so the names fix the link order. The `pov_` prefix and the words are ours. Several splits are choices: gordon and the two HEV barneys have no data of their own; beretta and m41a could end other files. | the link order (below) |
| [`dlls/triggers.cpp:3232`](../../../blob/point-of-view/dlls/triggers.cpp#L3232), [`3413`](../../../blob/point-of-view/dlls/triggers.cpp#L3413), [`dlls/effects.cpp:2297`](../../../blob/point-of-view/dlls/effects.cpp#L2297) | `area_break` and `area_story` at the end of `triggers.cpp`; `env_spawnereffect` in `effects.cpp` | Files of their own are also possible. Their save-table strings sit inside `triggers.cpp`'s data. | open |
| 184 lines in 67 files | Names ours: classes without exported members, fields, locals, constants | The binary doesn't name them. Names inside decorated exports (the Think, Touch, Use and Blocked members) are retail's. The known ways a name reaches the image are exports and the `.bss` hash. Class and member names were tested and don't change `ammo.obj`'s order. | `git grep -n -i "names\? ours\|ours)" point-of-view` |

### `// sic` lines that keep retail's behaviour

These lines keep retail's bugs and odd spellings. Retail's code or strings
prove them, and they steer nothing.

| Where | Retail behaviour |
|---|---|
| [`dlls/pov_barn.cpp:279`](../../../blob/point-of-view/dlls/pov_barn.cpp#L279) | The shotgun case precaches `barney/ba_attack2.wav` |
| [`dlls/pov_barn.cpp:823`](../../../blob/point-of-view/dlls/pov_barn.cpp#L823) | `PlaySentence` gets an absolute time as a duration |
| [`dlls/pov_hassault.cpp:183`](../../../blob/point-of-view/dlls/pov_hassault.cpp#L183) | A schedule has the next schedule's name |
| [`dlls/pov_hev_bluehelmet.cpp:84`](../../../blob/point-of-view/dlls/pov_hev_bluehelmet.cpp#L84) | `weaponused` is never read |
| [`dlls/pov_hev_orangehelmet_xtbu.cpp:178`](../../../blob/point-of-view/dlls/pov_hev_orangehelmet_xtbu.cpp#L178), [`dlls/pov_hev_spfxtbu.cpp:182`](../../../blob/point-of-view/dlls/pov_hev_spfxtbu.cpp#L182), [`dlls/pov_xtbu.cpp:603`](../../../blob/point-of-view/dlls/pov_xtbu.cpp#L603), [`dlls/triggers.cpp:754`](../../../blob/point-of-view/dlls/triggers.cpp#L754) | `strcpy` into a shared `""` literal |
| [`dlls/pov_loader.cpp:304`](../../../blob/point-of-view/dlls/pov_loader.cpp#L304) | The range-attack schedule is named "ALoaderEstablishLineOfFire" |
| [`dlls/pov_loader.cpp:1332`](../../../blob/point-of-view/dlls/pov_loader.cpp#L1332) | No `break`: the combat case falls into the idle code |
| [`dlls/pov_object.cpp:116, 122, 128, 134, 140, 146`](../../../blob/point-of-view/dlls/pov_object.cpp#L116-L146) | Every can gets skin 0 |
| [`dlls/pov_relationship.cpp:220`](../../../blob/point-of-view/dlls/pov_relationship.cpp#L220) | The "didn't found" message prints when the target was found |
| [`dlls/pov_scientist.cpp:348`](../../../blob/point-of-view/dlls/pov_scientist.cpp#L348) | Friend slot 7 is skipped; the list runs into the 16th slot |
| [`dlls/pov_scientist.cpp:608`](../../../blob/point-of-view/dlls/pov_scientist.cpp#L608) | `Talk( m_painTime )`: a time, not a duration |
| [`dlls/pov_spfsci.cpp:70`](../../../blob/point-of-view/dlls/pov_spfsci.cpp#L70) | `MonsterInit` is called twice |
| [`dlls/pov_workerbrave.cpp:102`](../../../blob/point-of-view/dlls/pov_workerbrave.cpp#L102) | The voice pitch is set, then overwritten |
| [`dlls/triggers.cpp:2847`](../../../blob/point-of-view/dlls/triggers.cpp#L2847) | `m_bFistOnCamera`: retail's save-table string |

That makes 30 `// sic` lines: the 8 steering lines above and these 22
(`git grep -n -w sic point-of-view`).

### Preprocessor directives

Compiled code keeps no preprocessor directives, so every conditional in the
branch needs a source of its own. Each one comes from Valve's SDK text that
the file copies (`OEM_BUILD`/`HLDEMO_BUILD` guards, `#if 0` blocks), or from
Valve's header idiom in headers the reconstruction made (`basebarn.h`,
`scientist.h`, `zombie.h`).

- The tree has no invented guards. Two `VALVE_DLL` guards that an earlier
  commit made up were removed (`45845b2`).
- No project-wide `/D` was added. `hl.dsp` lacks `CLIENT_WEAPONS`, and the
  binary shows it: `CanAttack`, `SendWeaponAnim`, `GetWeaponData` and
  `UpdateClientData` match only without it. `cl_dll.dsp` keeps SDK 2.0's
  flags, and no source file has a `/D` of its own.

### Build stand-ins

- **Build history.** One full build in project order stands in for POV's
  unknown sequence of builds (see "Precompiled header and build history"
  above). The placeholders come last, so the empty `.pch` they write can't
  affect any earlier file.
- **The machine.** Wine 11.0 and the CD's `MSVCRT.DLL` stand in for the
  Windows system POV was built on. The identical output shows that POV's
  LINK made the same choices as LINK on the CD's runtime.
- **Known from retail, not stand-ins:** the tree's path and drive (from the
  PDB record), the link times, the compiler build (from the Rich header).

## How identity was reached

The work went in this order: functions first, then the layout, then
everything else the image records. [vc6-notes.md](vc6-notes.md) has the
compiler and linker rules this needed.

| Commit | Date | Server: retail game functions exact | Client: retail game functions exact |
|---|---|---|---|
| `3c2be0d` (the base) | 2026-10-13 | 2,383 of 3,920 | 706 of 861 |
| `7504140` | 2026-10-16 | 3,104 | 832 |
| `71dc28b` | 2026-10-17 | 3,912 | 832 |
| `396fe00` | 2026-10-17 | 3,920 (all) | 832 |
| `a1f3097` | 2026-10-17 | 3,920 | 845 |
| `a7a4d18` | 2026-10-28 | file identical | 851 |
| `469339a` | 2026-10-29 | file identical | file identical |

"Game functions" are retail's functions outside the C runtime. "Exact" means
byte for byte, with addresses masked.

1. **The base.** We compared retail with builds of every SDK release,
   function by function. The best server base is SDK 2.0 with SDK 2.2b's
   single-player files compiled against 2.0's headers, linked with
   `/incremental:no`: 57.5% of retail's `.text` matched. The best client base
   is SDK 2.0's `cl_dll` built with `/YX`: 72.0%. The same mix of 2.0 and
   2.2b matched Gunman Chronicles' server.
2. **Function by function.** POV's changes were written from the
   disassembly and decompiled code, compiled, and compared with retail one
   function at a time. The last near misses needed the steering lines
   listed above.
3. **Link order.** The IDE links in sorted file order, and `hl.dsp` lists
   its files alphabetically. POV's own code sits between `pm_shared.c` and
   `python.cpp` (0x100707f0-0x100981b0, 162,240 bytes), so it became 43
   `pov_*.cpp` files. The data fixes the file boundaries: each object's
   initialised data comes first, then its strings, so a table after another
   file's strings starts a new object. At `b2b36c0` the server's `.text` had
   retail's size, every exact function sat at its retail address, and
   `.data` was byte for byte retail's.
4. **Precompiled header.** The include list of a file, and of the file
   before it, decides where header inlines land. In `func_break.obj` every
   function after the factory was 16 bytes off until `func_break.h` left
   the `.pch` (`b527b76`).
5. **Data.** Names for POV's globals that give retail's `.bss` (`d05f31b`),
   `skill.h` with retail's 0x178-byte layout (`46bdead`), and no `gSkillData`
   in the client, which had shifted its `.bss` by 0x178 (`5e0b02d`).
6. **The linker's C runtime.** LINK sorts every foldable COMDAT, live or
   dead, by size and relocation count with `qsort`, and keeps the first live
   copy of each group of identical functions. Wine's `qsort` breaks ties
   differently from the 1998 DLL. With the CD's DLL (2026-10-28) the server's
   import table came out right, and the server differed from retail only in
   time stamps, the PDB record and the Rich header.
7. **The server file.** The tree at the PDB path on drive `D:`, a fresh PDB,
   the frozen link time, `stricmp` (`2fbd2e5`) and the placeholder object
   made `hl.dll` retail's file on 2026-10-28 (`a7a4d18`).
8. **The client's kept copies.** At `a7a4d18` 184,894 of 218,694 bytes of
   the client's `.text` were identical at the same address (84.54%). A model
   of LINK's sort predicted all 857 addresses we checked in our own builds.
   So retail's kept copies became facts about retail's sort array: the tree
   lacked one dead function. With a stand-in for it and the first `ammo.cpp`
   stand-ins, `0c5b304` had 218,383 bytes identical (99.86%) and `.data`
   100%. One tie was left.
9. **The client's object count.** The client placeholder (`a1f9d66`) fixed
   the Rich header and the PE header position. 415 bytes still differed,
   all from the last tie: 311 in `.text`, 103 in `.rdata`, one in the
   `.reloc` size.
10. **Lazy inline emission in `ammo.obj`.** Retail exports all 398 VGUI
    template members, so their addresses show the order of 111 unfolded
    ones. Ours broke that order in four places. `ammo.cpp` is the file that
    creates the `.pch` in a full build, and then VC6 emits the `.pch`'s
    inline functions lazily, driven by the IL size of each function compiled.
    Blank lines, empty statements and `{}` all work the same way: they move
    the compiler's IL position. 5,580 compiles gave only 27 orders. A rule
    found while matching [Wanted!](wanted.md) predicts the order exactly on
    12 of 12 variants: after each of the file's own functions, VC6 emits
    every header inline whose IL offset is below the function's end minus a
    fixed offset. Retail needs the IL position after `GetSpriteList` at
    27,902-27,916:

    | `ammo.cpp` | after `NextWeapon` | after `PrevWeapon` | after `GetSpriteList` |
    |---|---|---|---|
    | needed | 18,914-18,977 | 19,768-19,831 | 27,902-27,916 |
    | SDK layout and POV's edits, no `sic` | 18,944 | 19,798 | 27,804 |
    | `a1f9d66` (15 comment lines on top, 10 `;`) | 18,998 | 19,852 | 27,908 |
    | `469339a` (`if` spelling, 9 `;`) | 18,944 | 19,798 | 27,909 |

11. **`.reloc` reveals the dead function.** A search over the stand-in's
    size, relocation count and position left a few shapes, all in
    `hl_weapons.obj`: 80 bytes with 4 relocations after `Killed`, 80 with 6
    after `DefaultDeploy`, `PlayEmptySound` or `ResetEmptySound`, or 96 or
    112 bytes further on. LINK apparently sizes `.reloc` before it drops dead
    functions, 2 bytes per dir32 fixup: 0x7e6a with no fixups, 0x7e70 with
    three, retail 0x7e72. So the function had four.
12. **Both together.** The `ammo.obj` split and the 80-byte, 4-relocation
    stand-in after `CBaseEntity::Killed` fix the last two groups (`Dar<T>`'s
    `operator[]` and `removeElement`); neither does alone. At `469339a` all
    1,326 client functions are exact, `.text`, `.data` and `.reloc` are 100%,
    and the release build gives retail's SHA-256.

## Check it yourself

1. Check your retail files against the hashes in [Status](#status):

   ```sh
   sha256sum /path/to/Half-Life/pov/dlls/hl.dll /path/to/Half-Life/pov/cl_dlls/client.dll
   ```

2. Build and compare:

   ```sh
   RETAIL=/path/to/Half-Life/pov VC_TOOLCHAINS=/path/to/toolchains \
       tools/release/point-of-view.sh ../pov ../pov-build
   echo $?
   ```

   Expected: `identical to retail` for both DLLs, `cmp` silent, exit code 0.

3. Look at the parts besides code with GNU binutils, on retail's files and on
   yours:

   ```sh
   TZ=UTC objdump -p hl.dll | grep -E 'Time/Date|pdb'
   TZ=UTC objdump -p client.dll | grep -E 'Time/Date|pdb'
   ```

   Expected for `hl.dll`:

   ```
   Time/Date		Tue May  6 07:56:13 2003
   Time/Date stamp 		3eb76a9d
   (format NB10 signature 9d6ab73e age 1 pdb D:\Editors\Levels\POVSourceCode\dlls\Releasehl\hl.pdb)
   ```

   and for `client.dll` (no PDB line):

   ```
   Time/Date		Fri May  2 01:24:25 2003
   Time/Date stamp 		3eb1c8c9
   ```

The tools we used to compare partial matches are not in this repository:
a function matcher, a per-section image comparison and the reference
checkers. For identical files, `cmp` is the whole test.
