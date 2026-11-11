# They Hunger (episodes 1 to 3)

They Hunger is a zombie-horror single-player campaign for Half-Life in three
episodes, by Neil Manke at Black Widow Games, presented by PC Gamer. The
mod's own text files credit Einar Saukas with design and coding and Jack
Cooper with models and skins. The pack that combines the three episodes was
zipped on 27 March 2001 and adds five deathmatch maps by Dave Waters and
Neil Manke. It ships two game DLLs, both built with Visual C++ 6.0 (VC6):

- `dlls/einar.dll`, the server: the game logic, monsters and weapons;
- `cl_dlls/client.dll`, the client: the HUD and the client side of the
  weapons (`liblist.gam` says `cldll "1"`).

The branch [`they-hunger`](../../../tree/they-hunger) holds C++ source that
compiles back into both files, byte for byte. It is Valve's Half-Life SDK
history up to the tag `v2.1` (`e84d041`, 2000-11-07), plus 76 commits (67
without merges). The first of them adds the single-player game code from SDK
2.2b, which 2.1 lacks. Each commit message says what the change matched and
why. This page describes commit `a2276df`, the head of the branch.

## Why this mod

They Hunger has been recreated in source form by the community more than
once. This reconstruction uses none of that work: no community source of
They Hunger was read or copied at any point. Everything on the branch comes
from the two retail DLLs, the mod's own data files (the maps' entity lists,
the deathmatch maps' `thdm.fgd`, model sequence and sound names, the text
files in the paks) and Valve's public SDKs.

So the source can be checked against the game itself: it compiles to the
retail machine code, function for function, and the release build gives the
retail files. What it says about the game's behaviour is what the shipped
binary does. Where the binary keeps a bug or an oddity, the
source keeps it too, marked `// sic`. Where the binary doesn't record
something (a name, a line that compiled to nothing), this page says so.

## Status

Both DLLs are identical to retail.

| | Server `einar.dll` | Client `client.dll` |
|---|---|---|
| File in the mod folder | `dlls/einar.dll` | `cl_dlls/client.dll` |
| Size | 1,056,817 bytes | 385,024 bytes |
| SHA-256 | `0b9c21c87dab0eec02b323eaab434cf77a2f69982bbf0a3f3f6a316fa95159d3` | `09e8de1a62f4f1fae32b040678fc5b97fe5ba8f20ec44db14b29aedcf37db105` |
| Link time (UTC) | 2001-03-26 05:43:20 | 2001-03-26 05:49:33 |
| Project | `dlls/hl.dsp`, "hl - Win32 Release" | `cl_dll/cl_dll.dsp`, "cl_dll - Win32 Release" |
| First identical at | `a2276df` (2026-11-11) | `a2276df` (2026-11-11) |

Verified on 2026-11-11: release builds from a fresh clone of `a2276df`, the
head of the branch, have retail's SHA-256, and `cmp` finds no difference.
The last commit that touches a file the client compiles is `2b0dedb`.

**What "identical" means here.** It means the files: every byte of both
DLLs, with the headers, time stamps, debug record and padding. It does not
mean that every line of the source is the mod's own text. A compiled DLL
doesn't record names, comments, or text that compiled to no code, and for a
few bytes of each DLL the cause in retail's source left no other trace. In
those places the branch has a marked stand-in:

- two inline functions that nothing calls, one in `dlls/weapons.h` and one
  in `cl_dll/hl/hl_weapons.cpp`;
- two unused enums, in `dlls/nodes.cpp` and `cl_dll/vgui_ClassMenu.cpp`;
- one function, `CEgon::PrimaryAttack`, written with unused locals so that
  the compiler gives retail's code;
- names for five global objects, chosen for where the compiler puts them;
- in the server's build: 71 of the PDB's 73 links, and the last build
  session.

[What in the source is not known from retail](#what-in-the-source-is-not-known-from-retail)
lists each one with what is known and what is not.

## What the code is

**Built from Valve's full game source.** The server's debug record names
`C:\hlsrc11\dlls\Releasehl\hl.pdb`: the Release folder of Valve's `hl`
project, in a tree called "hlsrc11". The DLL has every Half-Life monster
with its AI, which the public SDKs of 2000 leave out. Point of View, Wanted!
and Gunman Chronicles were built from the same kind of tree.

**The base, by comparing retail with builds of every SDK release:**

- The server's shared files match SDK 2.1, not 2.0: the engine interface has
  141 functions (`GiveFnptrsToDll` copies 141 pointers), and `PM_Duck`,
  `PM_UnDuck` and `CGrenade::UseSatchelCharges` match only 2.1's build.
- The single-player files match SDK 2.2b's, compiled against 2.1's headers.
  2.2b is the first public release that has them (`9955f36`).
- The server was compiled without `/GX` (51 functions match only a build
  without it) and without `CLIENT_WEAPONS` (`GetWeaponData`,
  `UpdateClientData`, `CanAttack` and `SendWeaponAnim` match only without
  it), and linked with `/debug` but without incremental-link thunks
  (`719a27e`).
- The client is 2.1's without the studio model renderer: it has 2.1's
  prediction hooks (`HUD_PostRunCmd`, `HUD_TxferPredictionData`,
  `CL_CreateMove`, `V_CalcNormalRefdef`, `PM_Duck`) but no
  `HUD_GetStudioModelInterface` and none of `StudioModelRenderer.cpp`,
  `GameStudioModelRenderer.cpp` and `studio_util.cpp` (`86449a3`).

With nothing of the mod written, that tree already gave 2,993 of retail's
3,428 server game functions (84.1% of their bytes) and 828 of the client's
994 (85.1%).

**Signs that the tree was Valve's own, not the public SDK's text:**

- `CHudMessage::MessageAdd` is SDK 2.0's loop with one thing of 2.1's, the
  skipping of a leading `#`. The three globals 2.1 added for the rest have
  their place in retail's data, unused (`9fc784d`).
- `EV_FireGauss` lacks the `EV_KillEvents( idx, "events/gaussspin.sc" )`
  line, which SDK 2.2 removed from 2.1's text; the rest of the function is
  2.1's (`db7f3b1`).
- Five monsters read gravity with `CVAR_GET_FLOAT( "sv_gravity" )` where
  2.2b has `g_psv_gravity->value` (big momma, assassin, bullsquid, headcrab,
  houndeye).
- `client.cpp` defines `Sys_Error` before `GetGameDescription`, the reverse
  of the SDK's order, as Point of View's, Wanted!'s and Gunman Chronicles'
  do (`2db505d`).
- The client's stub file, `hl_baseentity.cpp`, has its stubs in SDK 2.2b's
  order, without what 2.2 added for its predicted weapons (`d55d13c`).

**No source files of its own.** LINK's Rich header counts every object. A
build of 2.2b's `hl.dsp` file list without `voice_gamemgr.cpp` gives exactly
retail's counts, and so does 2.1's `cl_dll.dsp` without the three studio
files. One more or one fewer `.cpp` would change the C++ count. So all of
the mod's code sits inside Valve's files, and retail's code follows Valve's
project order object for object:

| File | The mod's code in it |
|---|---|
| `agrunt.cpp` | `monster_alien_grunt` became `CEinarZork`; `monster_th_cyberfranklin` at the file's end |
| `apache.cpp` | the helicopter without rockets; `monster_th_boss` |
| `bullsquid.cpp` | `monster_th_zombiebull` (`CEinarZombieBull`), `monster_th_megasquid` |
| `crowbar.cpp` | `weapon_th_shovel`, `weapon_th_spanner` |
| `egon.cpp` | `weapon_egon` is the flamethrower, with its projectile `einar_flame`; `weapon_th_medkit` |
| `headcrab.cpp` | `einar_hand`, `monster_th_chicken` |
| `islave.cpp` | Kelly (`monster_alien_slave`), `monster_th_babykelly` |
| `scientist.cpp` | `einar_civ`, `einar_civ_dead`, `einar_civ_sit` |
| `wpn_shared/hl_wpn_glock.cpp` (compiled into both DLLs) | the pistol's silencer; `weapon_th_ap9`, `weapon_th_taurus`, `weapon_th_sniper`, `weapon_einar1`, `weapon_th_chaingun` and their ammo |
| `cl_dll/ev_hldm.cpp`, `hl_weapons.cpp`, the HUD files | the five guns' events; the red HUD of "red mode" |

Many other files have smaller changes (red blood for the aliens, the zombie
cop and zombie soldier, map-given models and health, the dynamite, the track
train sounds, and so on). The commit messages describe each one. No base
class changed: `CBaseEntity`, `CBaseMonster`, `CBasePlayer` and the weapon
bases have the SDK's layout and virtual tables, and of the save tables only
`CBreakable`'s gained a field (`einar_sMaster`).

## The retail files

The retail DLLs are not in this repository. They come from the mod's
installer on ModDB:

- Page: <https://www.moddb.com/mods/they-hunger/downloads/they-hunger-1-2-and-3>.
- File: `TheyHunger3.exe`, 70,321,876 bytes, SHA-256
  `8449f0a64ad0ee49390ec32821f9a5bd1eecf91dfac59bd5553ef2f3f19057b9`.

`TheyHunger3.exe` is a self-extracting zip. 7-Zip unpacks it without running
it (`7z x TheyHunger3.exe`): 74 files, all in the mod folder `Hunger/`. The
two DLLs are:

| Entry | Size | Date in the zip | SHA-256 |
|---|---|---|---|
| `Hunger/dlls/einar.dll` | 1,056,817 | 2001-03-26 02:43:22 | `0b9c21c87dab0eec02b323eaab434cf77a2f69982bbf0a3f3f6a316fa95159d3` |
| `Hunger/cl_dlls/client.dll` | 385,024 | 2001-03-26 02:49:32 | `09e8de1a62f4f1fae32b040678fc5b97fe5ba8f20ec44db14b29aedcf37db105` |

`Hunger/liblist.gam` says `gamedll "dlls\einar.dll"`, `cldll "1"`,
`version "3.0"` and `hlversion "1100"`. The unpacked `Hunger/` folder is
what the release script's `RETAIL` variable expects (see below).

## Rebuilding

### What you need

- The toolchain folder from [setup.md](setup.md). This mod uses `vc6/`
  (Visual C++ 6.0 Professional, no service pack) and `wineprefix/`.
  [compilers.md](compilers.md) says where the compiler comes from.
- Wine (we used 11.0 with a win64 prefix), libfaketime's `faketime` (we used
  0.9.12), Python 3 (standard library only), `sha256sum` and `cmp`.
- A checkout of the `they-hunger` branch.

The branch stores its sources with CRLF line ends and has no
`.gitattributes`. Leave `core.autocrlf` off (the default on Linux), so git
checks the files out as stored. We haven't tested whether LF line ends
change the build.

### Commands

From a checkout of `main`:

```sh
git worktree add ../hunger they-hunger    # or: git clone -b they-hunger <repo> hunger
VC_TOOLCHAINS=/path/to/toolchains tools/release/they-hunger.sh ../hunger ../hunger-build
```

- The third argument picks `server`, `client` or `both` (the default).
- `../hunger-build` is the build folder. The script creates it if needed.
  It holds the build's copy of the Wine prefix, with the source tree laid
  out at retail's path inside it.
- To compare with your own retail copy as well, set `RETAIL` to the
  unpacked mod folder:

  ```sh
  RETAIL=/path/to/Hunger VC_TOOLCHAINS=/path/to/toolchains \
      tools/release/they-hunger.sh ../hunger ../hunger-build
  ```

For each DLL the script prints the path of the built file, its SHA-256,
retail's SHA-256 and `identical to retail` or `differs from retail`. With
`RETAIL` set it also runs `cmp` against `dlls/einar.dll` or
`cl_dlls/client.dll` there. It exits with 0 only if every DLL it built is
identical. The server takes longest, about five minutes on our machine: it
compiles 202 files and links 73 times.

### What the script does for the server

Retail's debug record names its PDB (program database, the file LINK
writes debug information to): `C:\hlsrc11\dlls\Releasehl\hl.pdb`.
`dlls\Releasehl` is the output folder of the `hl.dsp` Release configuration,
so the tree was at `C:\hlsrc11`. The script rebuilds that setting and the
end of the build history:

1. **The tree.** It copies the toolchain's Wine prefix into the build folder
   and lays out the checkout at `C:\hlsrc11` inside the copy's `drive_c`, as
   symlinks, so the build writes nothing into the checkout.
2. **A full build, linked as `hl.dll`.** It builds `hl - Win32 Release` of
   `dlls\hl.dsp` into `dlls\Releasehl`: all 100 sources (97 C++, 3 C) in
   project order. CL creates and reuses one `vc60.pch` (the precompiled
   header of `/YX`), as in the IDE. Retail's export directory names the DLL
   `hl.dll` and its PDB is `hl.pdb`, so the DLL was linked under the
   project's own name and renamed `einar.dll` for the release. The script
   builds `hl.dll` and compares it with retail's `einar.dll`.
3. **The last incremental builds.** It gives `weapons.h` a new time stamp
   and compiles the 51 sources that include it once more, `agrunt.cpp` to
   `world.cpp` in project order, with the same flags and the same `vc60.pch`
   path. VC6 decides for each file whether to reuse the `.pch` the previous
   compile left or to rebuild it, as the IDE does after a header edit. Then
   it does the same a second time. This gives five objects retail's function
   order, which no full build gives (see [Build stand-ins](#build-stand-ins)).
4. **The PDB's history.** It deletes `hl.pdb` and links 73 times. Retail's
   debug record names a PDB created on 2001-03-10 06:25:12 UTC (its
   signature) and written by 73 links (its age). The first link runs with
   the clock at that creation time and writes a fresh `hl.pdb`. The other 72
   run with the clock at retail's link time, 2001-03-26 05:43:20 UTC.

The clock is libfaketime's, with `TZ=UTC`, and only LINK runs under it.
`tools/release/they-hunger_server.py` does steps 2 to 4.

### What the script does for the client

The client was linked without `/debug`. It has no debug directory, so no
path reaches the image and the source location doesn't matter. The script
builds `cl_dll - Win32 Release` of `cl_dll/cl_dll.dsp` into an empty folder:
53 sources (50 C++, 3 C) in project order with one `vc60.pch`, LINK's clock
at 2001-03-26 05:49:33 UTC. No incremental build is replayed.

The flags, from the two `.dsp` files:

| | Server (`hl.dsp`) | Client (`cl_dll.dsp`) |
|---|---|---|
| Compile | `/G5 /MT /W3 /Zi /O2 /YX`, defines `NDEBUG WIN32 _WINDOWS QUIVER VOXEL QUAKE2 VALVE_DLL` | `/MT /W3 /GX /Zi /O2 /YX`, defines `WIN32 NDEBUG _WINDOWS` |
| Link | `/subsystem:windows /dll /map /debug /machine:I386 /def:.\hl.def /incremental:no` | `/subsystem:windows /dll /map /machine:I386`, plus the SDK's `utils/vgui/lib/win32_vc6/vgui.lib` |
| Sources | 97 `.cpp`, 3 `.c` | 50 `.cpp`, 3 `.c` |

The script leaves out the IDE-only `/Fr` and `/FD`.

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
| `utils/vgui/lib/win32_vc6/vgui.lib` (in the branch; client only) | | `df1d49f874c87ba1fbcfe0d164bec90d7940e81341c01c001a701b85ef46677a` |

The tools run on the CD's own `MSVCRT.DLL`, not on Wine's builtin one. LINK
uses the runtime's `qsort` to order the import tables and to choose which
copy of identical functions it keeps, and Wine's `qsort` breaks ties
differently. The client depends on this. See [vc6-notes.md](vc6-notes.md).

Not tried: other Wine versions, and Windows itself.

### If your build differs

| Symptom | Cause |
|---|---|
| Only the time stamps differ (COFF header and export directory; in the server the debug directory too) | LINK didn't run at retail's time |
| Client: functions in another order inside `vgui.dll`'s and KERNEL32's import lists (14 of 542 and 11 of 65 thunks) | LINK ran on Wine's builtin `msvcrt.dll` |
| Server: the debug record's signature and age differ | the 73 links weren't replayed, or an old `hl.pdb` was left in `dlls\Releasehl` |
| Server: the PDB path and the size of the debug record differ | the tree wasn't at `C:\hlsrc11` |
| Server: 12 functions at other places, in `agrunt.obj`, `barney.obj`, `client.obj`, `combat.obj` and `effects.obj` | only a full build, without the last incremental builds |
| Server: 621 bytes differ, around 0x10047ee0 to 0x100480af and in the tables that point there | `weapons.h` without `TH_StandIn` |
| Server: 6 bytes differ in `CGraph::LinkVisibleNodes` | `nodes.cpp` without its enum |
| Client: about 170,000 bytes differ | `hl_weapons.cpp` without `TH_ClientStandIn` (169,747 at `d662520`, which lacks it) |
| Client: 4 bytes differ, at 0x1001e04f to 0x1001e055 | `vgui_ClassMenu.cpp` without its enum |

## What the image holds besides code

A DLL also records how it was built. [reproducing.md](reproducing.md)
explains each part in general. For They Hunger:

| Item | `einar.dll` | `client.dll` | How the build gets it |
|---|---|---|---|
| PDB record: a debug directory entry in CodeView "NB10" format that names the PDB file | 49 bytes after the last section, at file offset 0x102000: `C:\hlsrc11\dlls\Releasehl\hl.pdb`, signature 0x3aa9c8c8 (2001-03-10 06:25:12 UTC), age 73 | none (no `/debug`) | The tree at that path in the prefix copy's `drive_c`; 73 links into a fresh PDB, the first at its creation time |
| Time stamps | COFF header, export directory and debug directory: 0x3abed6f8 | COFF header and export directory: 0x3abed86d | LINK runs with the clock frozen at retail's link time. |
| Rich header: a block LINK writes before the PE header that counts the objects each tool made | Utc12 C 8168 ×122, Utc12 C++ 8168 ×100, Masm 6.13 7299 ×30, AliasObj 7291 ×3, Lib 8034 ×3, Import0 ×74, Linker 6.00 8168 ×1 | Utc12 C ×128, C++ ×61, Masm ×30, AliasObj 7291 ×2, Lib 8034 ×9, Import0 ×636, Linker 8168 ×3 | The CD's compiler and libraries, and Valve's own file lists. Server C++: 97 project files and 3 library objects. Client C++: 50 project files and 11 library objects. No placeholder object is needed. |
| PE header position (`e_lfanew`) | 0xe8 | 0xe8 | Follows from the number of objects. |
| Imports | KERNEL32, 74 functions | USER32 3, WINMM 3, `vgui.dll` 542, WSOCK32 1, KERNEL32 65, in that order | The `.dsp` library lists, and LINK on the CD's `MSVCRT.DLL`. |
| Exports | 514, as `hl.dll`: `GetEntityAPI`, `GetEntityAPI2`, `GiveFnptrsToDll`, 266 entity factories, 245 decorated Think/Touch/Use members | 462, as `client.dll`: 37 client interface functions, 13 weapon and ammo factories, 412 decorated names | `hl.def` (`LIBRARY hl`) and the `EXPORT` macro; the client's `dllexport` declarations |
| Dead code and identical-code folding (ICF: LINK keeps one copy of functions with identical bytes and relocations) | none: `/debug` turns `/OPT:REF` and `/OPT:ICF` off, so every function compiled stays in the image | on, and the choice of kept copy depends on dead functions too | For the client: SDK 2.2b's stub file with its two dead stubs, retail's section order in `ammo.obj` and `hl_weapons.obj`, and LINK on the CD's runtime. |
| Function order inside an object | header inlines placed by `/YX`; five objects show an incremental build, and `hl_wpn_glock.obj` the amount of inline code in `weapons.h` | header inlines placed by `/YX` in a full build; `ammo.obj` and `hl_weapons.obj` come from files that create the `.pch`, so the amount of their own text decides | The full build, the replayed incremental builds, and the two stand-in functions |
| `.reloc` virtual size | 0x16ce4 | 0x8824, which also counts the absolute fixups of functions LINK dropped | Follows from the code, dead code included. |
| `.bss`: uninitialised globals, which take no file space but are addressed by the code | ordered by a hash of their names | the same | Names that give retail's order |
| Link order | `hl.dsp` order: every file in name order, `aflock` to `zombie` | SDK 2.1's `cl_dll.dsp` order, without the three studio files | `build_dsp.py` links in project order. |
| C runtime | `/MT`, static LIBCMT; no `/GX`, so no C++ exception runtime | `/MT`, LIBCMT with `/GX`'s exception objects | The CD's `VC98/LIB` |
| Padding | zero | zero | Wine hands LINK zeroed memory. |

**Precompiled header and build history.** With `/YX` (VC6's automatic
precompiled header) CL saves the state after a file's leading headers in
`vc60.pch`. A later file with the same leading headers reuses it; another
file rebuilds it. Whether a header is inside the `.pch` a file found, and
whether the file created that `.pch` or reused it, decides where the
header's inline functions land in the file's object. When a file creates
the `.pch`, they are emitted lazily, between the file's own functions, as
far as the compiler's intermediate code for the file's own text has
advanced. So the order of functions in an object shows which `.pch` its
source found, and sometimes how much text the source held.
[vc6-notes.md](vc6-notes.md#where-header-inline-functions-land) has the
rule.

## What in the source is not known from retail

The image gives every function body that the compiler emitted. It gives
names only through exports and strings, and nothing at all for text that
compiled to no code. So the source text is one spelling that gives retail's
bytes, not necessarily the mod's own. This section lists:

- placeholders for text the binary only measures;
- source that exists to steer the compiler;
- names chosen for the `.bss` layout;
- other choices the evidence leaves open, and names we made up;
- the build history the script stands in for.

Commit messages give the full reasoning. IL below is the intermediate
language that VC6's front end writes for the back end; its size is what the
compiler's placement decisions follow. We measured it with the compiler's
`/Bk` option.

### Placeholders

Both are inline functions that nothing calls. VC6 compiles such a function
to IL and emits nothing for it: no code, no data, no relocation, no symbol.
The IL still counts where the compiler decides the order of an object's
functions.

| Where | What | Why | Evidence |
|---|---|---|---|
| [`dlls/weapons.h:574-641`](../../../blob/they-hunger/dlls/weapons.h#L574-L641) | `TH_StandIn`, an inline function of 60 empty statements, between the AP9's class and the sniper rifle's. Name ours (`0aab512`). | It is 366 bytes of IL in front of the sniper rifle's class. In the server's last build `hl_wpn_glock.cpp` creates the `.pch` from its seven headers, so the guns' inline `iItemSlot` and `UseDecrement` are emitted lazily: after each of the file's own functions, every one whose position in the headers' IL is below the file's own IL so far. Retail has four of them (the glock's and the AP9's) after `weapon_einar1`'s `SecondaryAttack`, five after its `Reload` and one after its `WeaponIdle`. Without the stand-in the tree gives eight, two and none, and 621 bytes of the file differ. | The file's own IL up to the end of that `SecondaryAttack` is 32,428 bytes, and `CWeaponEinarSniper::iItemSlot` sits at 32,103 in the header stream; retail's order needs it between 32,428 and 32,506. So retail's `weapons.h` had 325 to 403 bytes more IL in front of the sniper rifle's class than ours, or its `hl_wpn_glock.cpp` that much less before that point, or a mix. The image doesn't show which text it was. 52 to 67 empty statements work, 51 and 68 don't. Two unused classes of the guns' shape (190 bytes each) work too; one or three don't. Ruled out: the shovel's or the medkit's class declared in `weapons.h` (`crowbar.obj` and `egon.obj` then no longer fit), dropping the `flags` blocks the guns copy from the glock (104 bytes, not enough), and every `.pch` state. The other objects limit where the IL can be: anywhere in `weapons.h` from `CBasePlayerItem` to here works; in front of `CGrenade` it moves `apache.obj`, and 71 bytes at the end of `extdll.h`, `util.h`, `schedule.h`, `basemonster.h` or `cbase.h` already move another object. The client compiles `weapons.h` too and is the same file with and without the stand-in. |
| [`cl_dll/hl/hl_weapons.cpp:654-714`](../../../blob/they-hunger/cl_dll/hl/hl_weapons.cpp#L654-L714) | `TH_ClientStandIn`, an inline function of 44 statements, before `HUD_InitClientWeapons`. Name and body ours (`e553179`). | It is 3,149 bytes of IL. `hl_weapons.cpp` creates the `.pch` it is compiled with, so the headers' inlines are emitted lazily here too. Retail's `hl_weapons.obj` has `CBaseEntity::IsAlive`, `IsBSPModel`, `ReflectGauss`, `HasTarget`, `TeamID`, `Think`, `Touch`, `Use` and `Blocked` right after `IsMoving`, before the initialiser of `g_clorg` (0x100046c0 to 0x1000482f), and `Center` and `EyePosition` after `HUD_PostRunCmd`. SDK 2.1's text with our lines for the five guns emits them later. The order of this object's sections decides which copies of identical functions LINK keeps across the whole client: at `d662520`, before the stand-in, 169,747 bytes of the file differ. | Retail's text had 3,095 to 3,290 bytes more IL between the includes and `g_rseq` than SDK 2.1's with our five guns (measured in the image with 619 to 658 empty-statement lines of 5 bytes; 618 and 659 fail), at least about 1,960 of it before `g_clorg`'s initialiser. 44 to 46 statements of this form work; 40 and 48 don't. What is known about the missing text: it had no absolute address (retail's `.reloc` virtual size, 0x8824, is ours exactly; a dead function with three made it 0x882a), it added no function (one more dead function of any of 98 shapes tried moves at least 36,169 bytes in a model of LINK's folding) and no static (`.bss` is equal). An inline or static function nobody called, or statements after a `return`, would leave exactly that: nothing. Tried and ruled out: SDK 2.1's own commented-out animation block (2,361 bytes if compiled; it adds five functions and needs an engine interface retail's client lacks), compiling without `/Zi` (moves the cursor by 400 to 450 bytes), every other `.pch` state and build session we tried (64,099 to 112,089 bytes differ), and the project's headers included after the header region (none is the right size). |

They Hunger needs no placeholder objects: Valve's file lists give retail's
object counts exactly.

### Source that steers the compiler

These lines are needed for retail's bytes, but their text is ours. Retail
had something with the same effect. All are marked `// sic`.

| Where | What | Effect | Evidence |
|---|---|---|---|
| [`dlls/nodes.cpp:27-32`](../../../blob/they-hunger/dlls/nodes.cpp#L27-L32) | An enum of 19 unused constants after the includes, names ours (`0ffe1b9`) | VC6 numbers every symbol a file declares. Three register reloads at a loop end of `CGraph::LinkVisibleNodes` (6 bytes of the file) follow those numbers modulo 64. Retail's order needs 19 to 54 more symbols than the tree's headers declare. `CTestHull::BuildNodeGraph` follows the same count and allows 0, 5 to 16, 20 to 24, 34 to 47 and 56 to 63. The enum declares 20, the fewest that give both. | Retail's headers declared more than ours before this file's functions; what they declared is not known. We compiled every server source with 0 to 63 extra symbols in front: only these two functions change. Two other accounts give the same count and the same file, and exclude each other and this enum: SDK 2.1's `eiface.h` without its `NEW_DLL_FUNCTIONS` block, which no code of retail uses (20 symbols); or the mod's eight weapon ids as an enum and its three helpers' prototypes in `cdll_dll.h` (23 symbols in the server, 24 in the client, which fixes the client's constructor below as well). About one count in six fits by chance, so neither is established, and the branch has the plain stand-in. |
| [`cl_dll/vgui_ClassMenu.cpp:37-43`](../../../blob/they-hunger/cl_dll/vgui_ClassMenu.cpp#L37-L43) | An enum of 4 unused constants after the includes, names ours (`2b0dedb`) | The same effect in `CClassMenuPanel`'s constructor: two register reloads at +0x8be, where retail reloads `pSchemes` before `this` (4 bytes of the file). 5 to 39 or 46 to 59 more symbols here give retail's order. The enum declares 5, the fewest. | The constructor's text is the SDK's. The client has one other function that follows the count, `CMessageWindowPanel`'s constructor, which is retail's with and without the enum. SDK 2.0's and 2.1's texts of every header this file includes give the same count. 2.2b's `VGUI_Label.h` and `VGUI_LineBorder.h` together would give retail's order (13 symbols), but the client would then export 465 names, and retail has 462. An inline colour helper in `cl_util.h` for the red HUD would give it too (7 symbols), but its IL moves header inlines in six objects. |
| [`dlls/egon.cpp:449-497`](../../../blob/they-hunger/dlls/egon.cpp#L449-L497) | `CEgon::PrimaryAttack` (the flamethrower): an unused `TraceResult tr;`, a reference `iAmmo` for the ammo count with `UseAmmo( 1 )`'s statement written out, three unused `Vector` copies, and `HasAmmo()` for the final test (`a2276df`) | With the SDK's `HasAmmo()` and `UseAmmo( 1 )` the function is retail's except for its first 0x48 bytes (835 bytes against 833; 762 bytes of the file differ, and 27 more in `.reloc`). Retail takes the address of the ammo count first, loads the count once and keeps it in a register from the test to the decrement. VC6 does that only when both go through one pointer or reference variable. But without the two inline calls the inliner has budget left and expands `Vector( x, y, z )` in the last two sums of the muzzle position, which retail calls. The unused locals and the `HasAmmo()` use that budget up again. | Retail's text read the count through a pointer or reference and had other inline calls that used the budget; which ones can't be known. 102 to 177 units of budget must be spent before the sums (`HasAmmo()` and `UseAmmo( 1 )` cost 46 + 84 = 130, which is why the SDK form has retail's tail). This is one of several forms that compile to retail's bytes: a pointer instead of the reference, or four `Vector` copies without the `TraceResult`, do too. The lines are ones the function's neighbours have (`CCrossbow::FireBolt`'s `TraceResult`, `CEgon::Attack`'s `vecAiming`). Behaviour is unchanged, and the other functions of `egon.obj` keep their places. |

VC6's inliner works with a budget: twice the caller's size, at least 1000,
shared among the call sites in evaluation order.
[vc6-notes.md](vc6-notes.md) has that rule and the symbol-number rule.

### Names chosen for the `.bss` layout

VC6 lays out a file's uninitialised globals in the walk order of a
1024-bucket hash of their names
([reproducing.md](reproducing.md#bss-order)). Names reach the image this way
even though no symbol does, so these names were chosen to give retail's
layout:

| Where | What | Evidence |
|---|---|---|
| [`cl_dll/hl/hl_weapons.cpp:50-60`](../../../blob/they-hunger/cl_dll/hl/hl_weapons.cpp#L50-L60) | The client's five gun objects: `g_Ap9`, `g_ClTaurus`, `g_Hkg36`, `g_TfcSniper`, `g_Chain`. Names ours (`e90c20f`). | Retail's static initialisers (0x10004100 to 0x1000421f) give the objects' addresses: the chaingun's first, then `g_clorg` (bucket 60), the AP9's, `g_pWpns` (171), the sniper rifle's, `lasthealth` (520), `ev`, `g_Glock`, `player`, `Globals` (737), the Taurus's, `weapon_einar1`'s, `g_clang` (820). So the five names must hash below 60, to 60-171, to 172-519, to 738-818, and above the Taurus's up to 819. These hash to 4, 137, 354, 753 and 817. No naming scheme fits all five: the SDK's idiom with our class names fits two (`g_WeaponEinarAP9` 93, `g_WeaponEinarChaingun` 19) and not the other three. So retail's names are not known; only their buckets' windows are. |
| [`dlls/apache.cpp:855`](../../../blob/they-hunger/dlls/apache.cpp#L855), [`1166`](../../../blob/they-hunger/dlls/apache.cpp#L1166), [`1085`](../../../blob/they-hunger/dlls/apache.cpp#L1085) | The two gun counters, both `static int cnt`, and the class name `CEinarBoss`. Names ours (`1e4cdca`). | Function statics are hashed by their decorated names, which contain the function's and the class's name. With these names the order is retail's: `FireRocket`'s unused `count`, the apache's counter, the boss's. |

### Other choices and guesses

| Where | What | Why, and what is open |
|---|---|---|
| [`cl_dll/hl/hl_weapons.cpp:759-766`](../../../blob/they-hunger/cl_dll/hl/hl_weapons.cpp#L759-L766), [`800-819`](../../../blob/they-hunger/cl_dll/hl/hl_weapons.cpp#L800-L819) | The five guns' `HUD_PrepEntity` calls in `HUD_InitClientWeapons` and their `case`s in `HUD_WeaponsPostThink` (`831095b` and the commits after it) | A guess. Neither project defines `CLIENT_WEAPONS`, so nothing calls these two functions, and LINK dropped them from retail's client. What the image does show: retail constructs the five objects, and its `.reloc` virtual size equals ours, so retail's dead code had exactly as many absolute addresses as these lines add (15: two per `HUD_PrepEntity` line, one per `case`), whatever its text was. |
| [`cl_dll/ammo.cpp:550`](../../../blob/they-hunger/cl_dll/ammo.cpp#L550), [`617-636`](../../../blob/they-hunger/cl_dll/ammo.cpp#L617-L636), [`884`](../../../blob/they-hunger/cl_dll/ammo.cpp#L884), [`916`](../../../blob/they-hunger/cl_dll/ammo.cpp#L916), [`992`](../../../blob/they-hunger/cl_dll/ammo.cpp#L992) | The red-mode `if`/`else` statements without braces around their single statements (`4040aea`) | `ammo.cpp` creates its `.pch`, so the amount of its own IL places the VGUI headers' inline members in `ammo.obj`. Retail's text had 172 to 206 bytes less IL by the end of `GetSpriteList` than ours had with braces. A brace pair around one statement is 18 bytes, and the eleven pairs we had written are 198. Ten pairs fit as well (keeping the one in `MsgFunc_HideWeapon`), and so would other text of the same size. No braces is the smallest difference from the SDK's own style. The code of the functions is the same either way. |
| [`cl_dll/hl/hl_baseentity.cpp`](../../../blob/they-hunger/cl_dll/hl/hl_baseentity.cpp) ([119](../../../blob/they-hunger/cl_dll/hl/hl_baseentity.cpp#L119), [228](../../../blob/they-hunger/cl_dll/hl/hl_baseentity.cpp#L228), [240-241](../../../blob/they-hunger/cl_dll/hl/hl_baseentity.cpp#L240-L241)) | The client's stub file: SDK 2.2b's, without what SDK 2.2 added for its predicted weapons, with 2.1's two step-sound stubs (`d55d13c`) | The stubs retail keeps that have no identical twin come in 2.2b's order (`CBaseMonster::FTriangulate` before `ChangeYaw`, `FireBullets` and `GetDeathActivity`). Dead stubs take part in LINK's folding sort: without `ShouldSimplify` or without `CBasePlayer::PackDeadPlayerItems`, both dead, 101,466 bytes of the file differ, and without both 214,594 (measured with everything else as in the identical build). So both were in retail's file. The rest of the dead stubs are checked only through that sort and `.reloc`'s size. |
| [`cl_dll/message.cpp:420-443`](../../../blob/they-hunger/cl_dll/message.cpp#L420-L443) | `CHudMessage::MessageAdd` as SDK 2.0's loop with 2.1's skipping of a leading `#` (`9fc784d`) | The function is retail's. Whether it was cut back from 2.1's text or comes from a Valve state between the two releases is open. The three globals 2.1 added stay, unused, because retail's data has them. |
| [`dlls/weapons.h:559-566`](../../../blob/they-hunger/dlls/weapons.h#L559-L566) and the four classes after it; [`dlls/wpn_shared/hl_wpn_glock.cpp:477-481`](../../../blob/they-hunger/dlls/wpn_shared/hl_wpn_glock.cpp#L477-L481) and four more | The five guns' `UseDecrement` and the `flags` blocks of their fire functions, with the glock's `#if defined( CLIENT_WEAPONS )` | Copied from the glock's SDK text, which the guns' code follows. Neither project defines the macro, so both DLLs compile the `#else` branch, and a plain `return FALSE` and `flags = 0` would give the same code. Only the IL size could tell them apart. The medkit, which only the server has, got the plain form (`5193d32`). |
| [`dlls/apache.cpp:763-775`](../../../blob/they-hunger/dlls/apache.cpp#L763-L775) | `CApache::FireRocket`: `m_iRockets--; return;`, with Valve's body left behind the `return` | Retail's function only counts a rocket off. That Valve's body was still compiled is proved: both of its statics are in retail's data, and only with it does the inline `CGrenade::BloodColor` land where retail has it. Where exactly the `return` stood is ours. |
| [`dlls/apache.cpp:32`](../../../blob/they-hunger/dlls/apache.cpp#L32) | `friend class CEinarBoss;` in `CApache` | The boss calls the apache's private members. Retail's export table still has the apache's think functions as private, so the class wasn't simply made public. A friend declaration is our way to get that. |
| [`cl_dll/hl/hl_weapons.cpp:1123-1134`](../../../blob/they-hunger/cl_dll/hl/hl_weapons.cpp#L1123-L1134) | The client's `TH_IsMultiplayer` and the two empty screen fades, at the end of the file | The shared weapon file calls three functions that each DLL defines for itself. The client's fades are empty, so LINK folds them into another one-byte function. Their place still matters through the folding sort: in a model of it, the two must follow `TH_IsMultiplayer` at the end of `hl_weapons.cpp` (before it, 2,087 bytes of the image differ; elsewhere more). |
| [`dlls/zombie.cpp:337`](../../../blob/they-hunger/dlls/zombie.cpp#L337), [`dlls/agrunt.cpp:1579`](../../../blob/they-hunger/dlls/agrunt.cpp#L1579) | The sizes of two sound-name buffers | 49 to 52 bytes compile to the same code. |
| [`dlls/hl.dsp:57`](../../../blob/they-hunger/dlls/hl.dsp#L57) | `/incremental:no` on the server's link line | Inferred: retail was linked with `/debug` but has no incremental-link thunks. `/profile` or `/opt:ref` would have the same effect. |
| Throughout | Names of everything the image doesn't name | See below. |

**Names.** Retail names a class only where a decorated export or a
save-table string carries the name. Of the mod's own classes that gives
five: `CEinarFlameRocket`, `CWeaponEinarShovel` and `CWeaponEinarSpanner`
(each exports member functions), `CEinarZork` and `CEinarZombieBull` (each
has a save table, which holds the class's name as a string). The exported
member names (`FlameTouch`, `Smack`, `SwingAgain`), the names of saved
fields (for example `einar_sMaster` in `CBreakable` and the flame's
`m_iTrail`) and all entity names are retail's too. Every other name the mod
added is ours, formed after those five:

- the classes `CEinarCyberFranklin` (it saves under its base class's name,
  `CAGrunt`), `CEinarBoss`, `CEinarMegasquid`, `CEinarHand`,
  `CEinarChicken`, `CEinarKelly`, `CEinarBabyKelly`, `CEinarCiv`,
  `CEinarCivDead`, `CEinarCivSit`, `CWeaponEinarMedkit`, the five gun
  classes in `weapons.h` (`CWeaponEinarAP9`, `CWeaponEinarSniper`,
  `CWeaponEinarTFCSniper`, `CWeaponEinarTaurus`, `CWeaponEinarChaingun`) and
  the three ammo classes (`CAmmoEinar...`), with all their members;
- the weapon ids (`WEAPON_TH_...`, `WEAPON_EINAR1`) and the guns'
  constants. Their values are retail's;
- `HIDEHUD_REDMODE` and `SF_SUIT_REDMODE` for "red mode", the monster flags
  `SF_MONSTER_REDBLOOD`, `SF_MONSTER_ZOMBIECOP`, `SF_MONSTER_ZSOLDIER`,
  `SF_MONSTER_NOZAP` and `SF_MONSTER_FASTMODEZOMBIE`, `SF_TRACKTRAIN_SOUNDS`
  and `SF_GRUNTREPEL_PARACHUTE`. They are named after the flags' names in
  the mod's fgd; their values are retail's;
- the helpers `TH_IsMultiplayer`, `TH_ScreenFadeIn` and `TH_ScreenFadeOut`;
- sequence enums, locals, and unexported functions.

Comments mark many of them `name ours`
(`git grep -n -i "names\? ours\|ours)" they-hunger` lists 43 lines in 24
files). Apart from exports and strings, the only way a name reaches the
image is the `.bss` hash above.

### `// sic` lines that keep retail's behaviour

These lines keep retail's own oddities. Retail's code or data proves each
of them, and they steer nothing.

| Where | Retail behaviour |
|---|---|
| [`cl_dll/ev_hldm.cpp:1329`](../../../blob/they-hunger/cl_dll/ev_hldm.cpp#L1329), [`1384`](../../../blob/they-hunger/cl_dll/ev_hldm.cpp#L1384) | Both sniper rifles' unzoomed view kick asks for a random number from -2.0 to -4.0, the bounds reversed |
| [`dlls/apache.cpp:769`](../../../blob/they-hunger/dlls/apache.cpp#L769) | `FireRocket`'s body after the `return` is compiled and thrown away |
| [`dlls/bullsquid.cpp:129`](../../../blob/they-hunger/dlls/bullsquid.cpp#L129) | The sized spit sets spawnflag 32 before `Spawn`, which doesn't read it |
| [`dlls/bullsquid.cpp:721`](../../../blob/they-hunger/dlls/bullsquid.cpp#L721) | `Precache` sets the spit sprite's index twice; the red sprite replaces the first |
| [`dlls/func_break.cpp:571`](../../../blob/they-hunger/dlls/func_break.cpp#L571) | A breakable asks its master with itself as the activator |
| [`dlls/headcrab.cpp:738`](../../../blob/they-hunger/dlls/headcrab.cpp#L738), [`746`](../../../blob/they-hunger/dlls/headcrab.cpp#L746) | The chicken's pain sounds start with `ch_attack2.wav` and its attack sounds end with `ch_pain1.wav` |
| [`dlls/player.cpp:3879`](../../../blob/they-hunger/dlls/player.cpp#L3879) | A second crowbar, shovel or spanner is "picked up" and attached to the player but not put on the item list |
| [`dlls/wpn_shared/hl_wpn_glock.cpp:670`](../../../blob/they-hunger/dlls/wpn_shared/hl_wpn_glock.cpp#L670) | The Taurus's `Deploy` draws a random number it doesn't use |
| [`dlls/zombie.cpp:343`](../../../blob/they-hunger/dlls/zombie.cpp#L343) | A zombie with a map-given sound directory doesn't precache its claw hit and miss sounds |

That makes 22 lines that carry `sic` (`git grep -n -w sic they-hunger`):
these 11, the four comments on the stand-in functions and enums, and seven
in `CEgon::PrimaryAttack`.

### Preprocessor directives

Compiled code keeps no preprocessor directives, so every conditional in the
branch needs a source of its own.

- No project-wide `/D` was added, and no source file has a `/D` of its own.
  `hl.dsp`'s Release configuration has no `CLIENT_WEAPONS`, and the binary
  shows it (see [What the code is](#what-the-code-is)). `cl_dll.dsp` keeps
  SDK 2.1's flags.
- The only conditionals the mod's code adds are ten copies of the glock's
  `#if defined( CLIENT_WEAPONS )`: five in `weapons.h` (the guns'
  `UseDecrement`) and five in `hl_wpn_glock.cpp` (the `flags` of their fire
  functions). They come with the SDK text the guns copy, and select nothing
  ([Other choices and guesses](#other-choices-and-guesses)).
- `hl_wpn_glock.cpp` is compiled into both DLLs, and the five guns' code is
  the same in both. Where the DLLs must differ, they call a function each
  DLL defines for itself (the three `TH_` helpers), so no conditional on the
  DLL was written.
- One of Valve's conditionals went with its content: the
  `#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )` around the hornet
  gun's precache line in `W_Precache`, which retail doesn't have.
- No `#pragma` was added.

### Build stand-ins

The build reproduces history that the image records only in part.

| Where | What | Why | Evidence and status |
|---|---|---|---|
| [`tools/release/they-hunger_server.py`](../tools/release/they-hunger_server.py): the last incremental builds | `weapons.h` gets a new time stamp and the 51 sources that include it are compiled once more in project order; then the same again | Retail's `agrunt.obj`, `barney.obj`, `client.obj`, `combat.obj` and `effects.obj` have their header inlines where VC6 puts them when the `.pch` holds more of the file's include list than a full build gives it: `agrunt.cpp` needs at least its first 7 headers in it and gets 4, `barney.cpp` at least 5 and gets 4, and `client.cpp`, `combat.cpp` and `effects.cpp` create the `.pch` from their own lists where a full build has them reuse one of 3 headers. So after a full build 12 functions sit at other places. | We compiled every server source in every `.pch` state it can meet (1,430 compiles) and fitted each object against retail. 75 objects come out the same in every state; of the 21 that don't, a full build gives 15 as retail has them. One cause gives the other five and disturbs none of the 15: `weapons.h` was edited, and the build recompiled its dependents, the first of them (`agrunt.cpp`) finding no usable `.pch`. `weapons.h` is the only single header whose dependents' rebuild makes all five right. VC6 does rebuild a `.pch` when a header in it has a new time stamp. **Known:** the last build's first file found no usable `.pch`, and at least those five files were recompiled. **Stand-in:** how the `.pch` came to be unusable. The full build leaves a `.pch` without `weapons.h`, which the first rebuild's `agrunt.cpp` would reuse; so the script rebuilds twice, and the second rebuild starts from a `.pch` the new time stamp has made stale. One compile of `world.cpp` before a single rebuild, or just the five files after any incompatible `.pch`, are equivalent by our measurements. |
| [`tools/release/they-hunger_server.py`](../tools/release/they-hunger_server.py): 1 + 72 links | The PDB's link history | Retail gives only the PDB's creation time (signature) and the number of links (age 73) | The 71 middle links at retail's link time are a stand-in: their times, and what was compiled between them, are unknown. `hl.pdb` itself is not part of the image. |
| [`tools/release/they-hunger_server.py`](../tools/release/they-hunger_server.py): linking `hl.dll` | The output name | Retail's PDB is `Releasehl\hl.pdb` and its export name `hl.dll` | Inferred: it is the project's own output name. The name of the output file doesn't reach the image except through the PDB's name. |
| The client's build history | One full build | The client has no debug record, so nothing dates its builds | A full build gives every client object as retail has it. Replaying a "`weapons.h` edited" build for the client (its three dependents recompiled) gives the same objects, so the client neither confirms nor refutes the server's last build. |
| The machine | Wine 11.0 and the CD's `MSVCRT.DLL` | Stand in for the Windows system the mod was built on | The identical client shows that the mod's LINK made the same choices as LINK on the CD's runtime. |

**Known from retail, not stand-ins:** the tree's path and drive (from the
PDB record), the link times, the PDB's creation time and age, the compiler
and linker builds and the object counts (from the Rich header).

## How identity was reached

The work went in this order: the base, then functions, then everything else
the image records. All of it was done on 2026-11-11.
[vc6-notes.md](vc6-notes.md) has the compiler and linker rules this needed.

**Functions.** The mod's changes were written from the disassembly and
decompiled code, compiled, and compared with retail one function at a time,
file by file. "Game functions" are retail's functions outside the C
runtime. "Exact" means byte for byte, with addresses masked, in a plain full
build.

| Commit | Server: of 3,428 game functions exact | Client: of 994 exact | What it added |
|---|---|---|---|
| `86449a3` (the base) | 2,993 | 828 | Valve's files arranged as retail's projects |
| `1d48e32` | 3,028 | | breakables, health kits and chargers, doors, trains |
| `e665503` | | 844 | the red HUD, `MessageAdd` |
| `f86756c` | 3,115 | | the player and shared code; barney, scientists, zombies, grunts |
| `dc2c8c4` | 3,165 | | the alien grunt family, the helicopter and the boss |
| `ab8750c` | 3,262 | | the bullsquid family, houndeye, headcrab family, the Kellys |
| `f665dd2` | 3,324 | | crowbar, shovel, spanner, flamethrower, medkit, dynamite |
| `0c00aec` | 3,408 | 967 | the pistol's silencer and the five guns, in both DLLs |
| `2db505d` | 3,409 | 967 | `Sys_Error` before `GetGameDescription` |
| `d662520` | 3,409 | 992 | the client's stub order, `ammo.cpp`'s braces, the gun objects' names |

At `0c00aec` every retail function had source. One commit later
(`10681ce`) every string, reference, vtable, data object and export of both
DLLs was retail's. What was left were three functions a few bytes off, and
correct code at the wrong place.

**The server file.** A release build (the tree at `C:\hlsrc11`, linked as
`hl.dll`, the PDB's 73 links, retail's link time) had retail's size from
the start, and identical headers, `.data`, import and export tables and
debug record. Then the bytes that differ in the whole file went down like
this:

| Commit | Bytes differing | What changed |
|---|---|---|
| `2db505d` | 1,416 | With the last incremental builds replayed and `Sys_Error` in its place, `agrunt.obj`, `barney.obj`, `client.obj`, `combat.obj` and `effects.obj` are retail's. Left: `CEgon::PrimaryAttack` 762, the inline order of `hl_wpn_glock.obj` 427, the vtable slots that point there 10, `CGraph::LinkVisibleNodes` 6, `.reloc` 211. |
| `0aab512` | 795 | The stand-in in `weapons.h`: `hl_wpn_glock.obj` is retail's. Left: the two functions and 27 bytes of `.reloc`. |
| `a2276df` | 0 | The enum in `nodes.cpp` and the steered `CEgon::PrimaryAttack`. |

Finding the last build took the most compiles: every source in every
`.pch` state, then real compiles of the 51 dependents from a stale `.pch`.
`hl_wpn_glock.obj` fits in no state, which is how the missing IL in
`weapons.h` was found.

**The client file.** The client had retail's time stamps, Rich header,
import tables (all 614 thunks in order), export names and ordinals and
`.reloc` size from the first release build, with LINK on the CD's runtime. What moved its bytes was which copies of
identical functions LINK kept. A model of LINK's folding reproduced our own
link exactly (1,745 of 1,745 named sections at the map's addresses), and
showed that 29 of 65 folded groups kept another copy than retail's. All of
them are decided together by one sort, and three inputs to it were wrong:

| Commit | Bytes differing | What changed |
|---|---|---|
| `10681ce` | 211,648 | The starting point; `.text` 16 bytes short. |
| `d55d13c` | 169,043 | The stub file in SDK 2.2b's order, with its two dead stubs. `.text` has retail's size. |
| `4040aea` | 169,759 | `ammo.cpp` without the braces: `ammo.obj`'s 128 sections are in retail's order (21 were not). |
| `e90c20f` | 169,747 | Names for the five gun objects: all eighteen static initialisers are retail's bytes. |
| `e553179` | 4 | The stand-in in `hl_weapons.cpp`: that object's sections are in retail's order, and with them every kept copy. |
| `a2276df` | 0 | The enum in `vgui_ClassMenu.cpp` (`2b0dedb`). |

The counts stay near 170,000 until the last input is right, because one
section out of order in an early object changes which copies are kept and
shifts everything after it. With the other two inputs right and one wrong,
the image differed in 101,466 bytes (a dead stub missing), 4,463 (the
braces) or 169,759 (the IL in `hl_weapons.cpp`).

**The declaration count.** Two functions, one in each DLL, differed from
retail only in the order of register reloads. We found that VC6 orders
those by the numbers its front end gives the function's labels and
temporaries, so by the count of everything declared before the function,
modulo 64, and measured what each kind of declaration costs. Only four
functions of the two DLLs follow that count at all. The image holds no
other trace of declarations, so the two enums are stand-ins.

## Check it yourself

1. Check the installer and the retail files against the hashes in
   [The retail files](#the-retail-files):

   ```sh
   sha256sum TheyHunger3.exe
   7z x TheyHunger3.exe
   sha256sum Hunger/dlls/einar.dll Hunger/cl_dlls/client.dll
   ```

2. Build and compare:

   ```sh
   RETAIL=$PWD/Hunger VC_TOOLCHAINS=/path/to/toolchains \
       tools/release/they-hunger.sh ../hunger ../hunger-build
   echo $?
   ```

   Expected: `identical to retail` for both DLLs, `cmp` reporting no
   difference, exit code 0. The server part prints a full build of 100
   sources, two sessions of 51 sources each, link 1 of 73 at 2001-03-10
   06:25:12 UTC and links 2 to 73 at 2001-03-26 05:43:20 UTC.

3. Look at the parts besides code with GNU binutils, on retail's files and
   on yours:

   ```sh
   TZ=UTC objdump -p einar.dll | grep -E 'Time/Date|format NB10'
   TZ=UTC objdump -p client.dll | grep -E 'Time/Date|format NB10'
   ```

   Expected for `einar.dll` (objdump prints the signature's bytes in file
   order):

   ```
   Time/Date		Mon Mar 26 05:43:20 2001
   Time/Date stamp 		3abed6f8
   (format NB10 signature c8c8a93a age 73 pdb C:\hlsrc11\dlls\Releasehl\hl.pdb)
   ```

   and for `client.dll` (no PDB line):

   ```
   Time/Date		Mon Mar 26 05:49:33 2001
   Time/Date stamp 		3abed86d
   ```

4. See what a stand-in does: delete `TH_StandIn` from `dlls/weapons.h` in
   your checkout and run the script for the server again. The build then
   differs from retail: six small inline functions and two functions of
   `weapon_einar1` sit in another order in the guns' object, and the tables
   that point at them differ.

The tools we used to compare partial matches are not in this repository: a
function matcher, a per-section image comparison, the reference checkers,
and the models of VC6's lazy inline emission and LINK's folding sort. For
identical files, `cmp` is the whole test.
