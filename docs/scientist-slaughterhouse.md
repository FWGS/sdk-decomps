# Scientist Slaughterhouse 1.0

Scientist Slaughterhouse is a single-player Half-Life mod by one author (its
readme speaks as "I (the mod author)"). Version 1.0 was linked in August 2002.
The readme calls it "a scientist killing mod": detailed death messages for
scientists and a scoreboard that counts each of four scientists' deaths;
fire, electric, radiation and cold damage effects; heads shot off, scientists
cut in half by an energy disk or turned into skeletons by acid; a target
identification HUD with a "battle power" readout; new weapons with a third
attack (`+attack3`) and HUD slots 6 and 7; the Nephilim Alliance monsters,
turrets, beam and teleport entities; weapons and players that respawn in
single player. It has 23 maps (`ddi_a1` to `ddi_a22` without 13, and two
training maps). `liblist.gam` says `type "singleplayer_only"`,
`gamedll "dlls\hl.dll"` and `cldll "1"`. The mod ships two game DLLs, both
built with Visual C++ 6.0 (VC6):

- `dlls/hl.dll`, the server: the game logic, monsters and weapons;
- `cl_dlls/client.dll`, the client: the HUD, the mod's visual effects and
  the client side of 22 predicted weapons.

The branch [`scientist-slaughterhouse`](../../../tree/scientist-slaughterhouse)
holds C++ source that compiles back into both files, byte for byte. It is
Valve's Half-Life SDK history up to SDK 2.2 (`4ec16cc`, 2001-11-08),
plus 599 commits (451 without merges). The first of them adds the
single-player game code from SDK 2.2b, which 2.2 lacks. Each commit message
says what the change matched and why. This page describes commit `c7f292c`,
the head of the branch.

## Sources used

No community source of Scientist Slaughterhouse and no community recreation
of Half-Life's code was read or copied. Everything on the branch comes from
the two retail DLLs, the mod's own data files (the maps' entity lists,
`ssh.fgd`, the entity reference `ssh_ent.html`, `skill.cfg`, the sentences,
titles, HUD sprite lists and event files, `readme.txt` and `whatsnew.txt`)
and Valve's public SDKs.

So the source can be checked against the game itself: it compiles to the
retail machine code, function for function, and the release build gives the
retail files. What it says about the game's behaviour is what the shipped
binary does. Where the binary keeps a bug or an oddity, the source keeps it
too, marked `// sic`. Where the binary doesn't record something (a name, a
line that compiled to nothing, a step of the build history), this page says
so.

## Status

Both DLLs are identical to retail.

| | Server `hl.dll` | Client `client.dll` |
|---|---|---|
| File in the mod folder | `DLLS/hl.dll` | `cl_dlls/client.dll` |
| Size | 1,495,128 bytes | 938,074 bytes |
| SHA-256 | `8e4c150e8d8d57f13bbf7f6b5a5e4ebc37258838ccb7708c760bb7b3c139d4ae` | `1e4f2d5c2a6d92ac84f8bdf1fb46abbffabcb78107909aa44c2a75f84baf1365` |
| Link time (UTC) | 2002-08-17 07:56:57 | 2002-08-11 22:30:25 (last link); 2002-07-01 15:12:23 (last full link) |
| Project | `dlls/hl.dsp`, "hl - Win32 Profile" | `cl_dll/cl_dll.dsp`, "cl_dll - Win32 Debug", optimised |
| Linking | not incremental (`/profile /debug`) | incremental, with a history of 459 links |
| First identical at | `c7f292c` (2026-11-19) | `089e19c`, on the main line from `48193ac` (2026-11-19) |

Verified on 2026-11-19 from a fresh clone of the repository, following
[the rebuild page](rebuild/scientist-slaughterhouse.md) as written: both
files built by `tools/release/scientist-slaughterhouse.sh` from `c7f292c`,
the head of the branch, have retail's SHA-256, and `cmp` finds no
difference. The last commit that touches a file the client compiles is
`1a33030`.

**What "identical" means here.** It means the files: every byte of both
DLLs, with the headers, time stamps, debug records, padding, the client's
incremental-link thunk table and its reserve. It does not mean that every
line of the source is the mod's own text. A compiled DLL doesn't record
names, comments, or text that compiled to no code, and for some bytes the
cause in retail's source or build left no other trace. In those places the
branch has a marked stand-in:

- unused inline functions, unused enums and unused macros whose only effect
  is on the compiler: where it places inline functions, which stack slot it
  picks, in which order it writes the client's type information;
- a few unused locals, statics and data items that steer the compiler or
  that retail's data shows without a user;
- names for symbols the image doesn't name but whose hash places them: the
  client's thunk table and both DLLs' `.bss`;
- one conditional: `CLaserPulse`'s empty constructor exists only in the
  client's compile of `weapons.h`;
- in the build: 1,431 of the server's 1,433 links, 456 of the client's 459,
  the last build sessions, an earlier text of `sword.cpp` for the client's
  last full link, and the paths of the compiler and of its temporary files.

[What in the source is not known from retail](#what-in-the-source-is-not-known-from-retail)
lists them by kind, with the commits that hold the evidence.

## What the code is

**One source tree for both DLLs.** Both debug records name a PDB under
`C:\Microsoft Visual Studio\MyProjects\Source Code`: the server's in
`dlls\Profilehl`, the client's in `cl_dll\Debug`. Both PDBs were created on
2002-04-12, within half an hour (their signatures). `whatsnew.txt` says of
version 0.9: "Transfered code to HL SDK 2.2", "Moved some weapons
clientside".

**The base, by comparing retail with builds of every SDK release:**

- SDK 2.2's shared code with SDK 2.2b's single-player files. 2.2 ships only
  the multiplayer project; the server has every Half-Life monster, so the
  single-player files and 2.2b's `hl.dsp` were added (`bb82f5d`, the same 50
  files as in the Gunman Chronicles, Wanted! and They Hunger
  reconstructions). In the server, `AllowLagCompensation`, `CGlock::Deploy`
  and `CVoiceGameMgr::Init` match only 2.2's text; in the client
  `Initialize`, `CHudSpectator::Draw`, `DrawOverviewEntities`,
  `EV_FireCrossbow2`, `CCrowbar::Deploy`, `CGlock::Deploy`, the glock's ammo
  factories and `CVoiceStatus::Init`. No function matches only 2.2b's text.
  The engine interfaces are 2.2's: `GiveFnptrsToDll` copies 143 pointers,
  the client's `Initialize` 98.
- **The server is `hl.dsp`'s Profile configuration** (`e15cd8f`). The PDB
  folder `Profilehl` is that configuration's output folder (SDK 2.2's
  `mp.dsp` would say `Profilemp`). Its link is `/profile /debug`, which is
  not incremental: a Profile build of SDK 2.2b matches 1,776 retail
  functions, a build of the incrementally linked Release configuration
  1,093. No `/GX`: the server has no exception tables and no RTTI.
- **The mod added `CLIENT_WEAPONS` to that configuration.**
  `UTIL_WeaponTimeBase`, `CanAttack` and eleven `UseDecrement` and
  `iItemSlot` inlines match only builds with it; nothing matches only
  without it.
- **No `teamplay_gamerules.cpp`.** None of its code or strings is in retail,
  no `CHalfLifeTeamplay` vtable is among the 361 the server has, and LIBCMT's
  `strtok.obj`, which only that file pulls in, is missing. Without it the
  C object count is retail's 125. `InstallGameRules` lost its teamplay
  branch (`cd1f2df`).
- **The client is `cl_dll.dsp`'s Debug configuration, edited to optimise**
  (`3518377`). Its PDB names the Debug folder; it links the debug runtime
  LIBCMTD; its asserts are compiled in (`_DEBUG`, in `pm_shared.c`,
  `voice_status.cpp`, `vgui_grid.cpp`, `bitvec.h`); it has RTTI (168 type
  descriptors) and exception tables; and it is linked incrementally. But its
  code is optimised: the shipped `/Od /ZI` reproduces 471 functions, `/O1`
  523, `/O2` 1,901. `/O2` with `/ZI` is a command-line error (D2016) and
  retail has no `.textbss`, so the objects were compiled with `/Zi`.
- The client compiles the single-player `basemonster.h`: it exports
  `CBaseMonster::MonsterUse`, `MonsterInitThink`, `CallMonsterThink` and
  `CorpseFallThink`, and `hl_baseentity.cpp` has 2.2b's `CBaseMonster`
  stubs back.

With nothing of the mod written, that tree gave 1,203 of retail's 4,778
server game functions (13.8% of their 937,482 bytes) and 1,461 of the
client's 2,581 (59.6% of 328,766 bytes). "Game functions" are retail's
functions outside the C runtime and the import thunks.

**The mod changed the base classes.** Unlike They Hunger, Scientist
Slaughterhouse added virtual functions and fields to Valve's base classes,
which is why 957 retail server functions (227 KB) matched the SDK's code
only with other offsets at the start. `CBaseEntity` has 72 virtual slots
against the SDK's 57 and a new saved field `m_iszKillDesc`;
`CBaseMonster`'s vtable has 147 slots against 125 and the object 0x14 bytes
more; `CBasePlayer` is 0x48 bytes bigger (among others the per-scientist
death and gib counts); the weapon base classes have a third attack,
`WeaponSpecial`, with its timer `m_flNextWeaponSpecial`.

**New source files.** LINK's Rich header counts every object. The server
has 131 C++ objects where SDK 2.2b's file list without
`teamplay_gamerules.cpp` gives 100; the client 105 where SDK 2.2's
`cl_dll.dsp` gives 89, with the same runtime objects. So the mod added 31
server files and 16 client files. Valve's projects list their files in name
order, and LINK lays the objects out in that order, so each new file's code
sits where its name sorts. Nothing in the binaries names a file: the names
on the branch are ours, chosen after the main class or entity and checked
to sort into retail's place.

- Server (175,206 bytes of code in 800 functions): `beams`, `black_hole`,
  `camera`, `cluster_bomb`, `crane`, `dd_sphere`, `ddisk`, `displacer`,
  `elemental`, `entity_link`, `fire`, `fireball`, `flamethrower`,
  `flyingobject`, `freezethrower`, `func_turret`, `headmonster`, `lpistol`,
  `masters`, `pulse`, `random_scis`, `reflector`, `repulsor`, `rocket`,
  `sniper`, `softgun`, `sword`, `teslacoil`, `teslamonster`, `volcano`,
  `vortex`, plus the headers `beams.h`, `entity_link.h` and `flyingobject.h`.
  Where each one starts is decided by the data: each object's initialised
  data comes first, then its string literals, so a save table after another
  object's strings starts a new object, and a mod class whose save table
  sits inside an SDK file's data block was written into that SDK file. Some
  splits rest only on the object count.
- Client: the eight weapon files it compiles from `dlls` (displacer,
  flamethrower, laser pistol, repulsor, softgun, sword, tesla coil, vortex
  launcher; their client code shows which file holds which weapon) and
  eight of its own: `bloodspray`, `cl_effects`, `damagefx`, `effects`,
  `ident`, `rain`, `sphere`, `tractorbeam`, plus `particlesys.h`.

The mod's code inside Valve's files is larger still: new monsters at the
end of `agrunt.cpp`, `apache.cpp`, `barney.cpp`, `hassassin.cpp`,
`islave.cpp`, `osprey.cpp`, `scientist.cpp`, `turret.cpp` and
`zombie.cpp`, new entities in `effects.cpp`, `func_break.cpp`,
`triggers.cpp` and others, and changes all through `combat.cpp`,
`player.cpp`, `monsters.cpp`. In all the server exports 368 entity
factories: 244 of the SDK's classnames (all but `monster_tentaclemaw`) and
124 new ones, among them 12 weapons (ids 16 to 27); it adds 8 console
variables and 4 user messages. The commit messages describe each part.

## The retail files

The retail DLLs are not in this repository. They come from the mod's
installer:

- Where to get it: not given here yet; the file is identified by its size
  and SHA-256.
- File: `ssh10.exe`, 21,307,392 bytes, SHA-256
  `98497a52522c5e7bb06f2c9a6a473c9cb8c80de5015a2efb4c23a74315466aeb`.

`ssh10.exe` is a WinZip self-extractor. `unzip` or 7-Zip unpacks it
without running it (`unzip ssh10.exe`, or `7z x ssh10.exe`, which warns
about data after the end of the archive): 354 files, all in the mod folder
`ssh/`. The two
DLLs are:

| Entry | Size | Date in the zip | SHA-256 |
|---|---|---|---|
| `ssh/DLLS/hl.dll` | 1,495,128 | 2002-08-17 10:57 | `8e4c150e8d8d57f13bbf7f6b5a5e4ebc37258838ccb7708c760bb7b3c139d4ae` |
| `ssh/cl_dlls/client.dll` | 938,074 | 2002-08-12 01:30 | `1e4f2d5c2a6d92ac84f8bdf1fb46abbffabcb78107909aa44c2a75f84baf1365` |

`ssh/LIBLIST.GAM` says `game "Scientist Slaughterhouse"`, `version "1.0"`,
`gamedll "dlls\hl.dll"` and `cldll "1"`. The unpacked `ssh/` folder is what
the release script's `RETAIL` variable expects (see below).

## Rebuilding

### What you need

- The toolchain folder from [setup.md](setup.md). This mod uses `vc6/`
  (Visual C++ 6.0 Professional, no service pack: `nativecrt`,
  `VC98/INCLUDE` and `VC98/LIB`) and `wineprefix/`.
  [compilers.md](compilers.md) says where the compiler comes from.
- Wine (we used 11.0 with a win64 prefix, the only version we measured),
  libfaketime's `faketime` (we used 0.9.12), Python 3 (standard library
  only), `sha256sum` and `cmp`.
- A checkout of the `scientist-slaughterhouse` branch.

The branch stores its sources with CRLF line ends and has no
`.gitattributes`. Leave `core.autocrlf` off (the default on Linux), so git
checks the files out as stored. We haven't tested whether LF line ends
change the build.

### Commands

From a checkout of `main`:

```sh
git worktree add ../ssh scientist-slaughterhouse    # or: git clone -b scientist-slaughterhouse <repo> ssh
VC_TOOLCHAINS=/path/to/toolchains tools/release/scientist-slaughterhouse.sh ../ssh ../ssh-build
```

- The third argument picks `server`, `client` or `both` (the default).
- `../ssh-build` is the build folder. The script creates it if needed. It
  holds the build's copy of the Wine prefix, with the source tree and the
  compiler laid out at retail's paths inside it.
- To compare with your own retail copy as well, set `RETAIL` to the
  unpacked mod folder:

  ```sh
  RETAIL=/path/to/ssh VC_TOOLCHAINS=/path/to/toolchains \
      tools/release/scientist-slaughterhouse.sh ../ssh ../ssh-build
  ```

For each DLL the script prints the path of the built file, its SHA-256,
retail's SHA-256 and `identical to retail` or `differs from retail`. With
`RETAIL` set it also runs `cmp` against `DLLS/hl.dll` or
`cl_dlls/client.dll` there. It exits with 0 only if every DLL it built is
identical. The server takes about a quarter of an hour on our machine,
nearly all of it its 1,433 links (about half a second each); the client
about two minutes.

### The build environment

Both DLLs are built in one copy of the toolchain's Wine prefix, laid out as
the original machine is known or assumed to have been:

1. **The tree** at `C:\Microsoft Visual Studio\MyProjects\Source Code`, as
   symlinks into the checkout, so the build writes nothing there. Both
   debug records name that path, the client's asserts carry it in their
   `__FILE__` strings, and the client's anonymous namespaces carry it in
   their names.
2. **The compiler** at `C:\Microsoft Visual Studio\VC98`: `BIN`, `INCLUDE`
   and `LIB` are links to the toolchain's `vc6/nativecrt`, `vc6/VC98/INCLUDE`
   and `vc6/VC98/LIB`, and CL runs as
   `C:\Microsoft Visual Studio\VC98\BIN\CL.EXE` (`tools/vc6.sh`'s
   `VC6_ROOT`).
3. **`TMP` and `TEMP`** set to `C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp` (the
   Administrator's default on Windows 2000 and XP) in the prefix's
   `HKCU\Environment`.

Steps 2 and 3 are there for the client: the order in which VC6 writes its
type information depends on the lengths of these paths
([below](#the-clients-type-information-and-the-pinned-paths)). With them
the build doesn't depend on where the toolchain folder is or on the user's
name. They are stand-ins: retail's own compiler and temporary folders are
not recorded.

### What the script does for the server

`tools/release/scientist-slaughterhouse_server.py`:

1. **A full build** of `hl - Win32 Profile` of `dlls\hl.dsp` into
   `dlls\Profilehl`: all 131 sources (128 C++, 3 C) in project order, each
   handed to CL by its full path as the IDE does (for the server this
   changes nothing in the image). CL creates and reuses one `vc60.pch`, the
   precompiled header of `/YX`, as in the IDE.
2. **The last build session.** Where VC6 puts the header inline functions an
   object needs depends on the `.pch` its last compile found
   ([below](#where-header-inline-functions-land)), and a full build in
   project order leaves several objects in other orders than retail's. The
   script replays edits and compiles that give each object the state
   retail's shows: `flyingobject.h` gets a new time stamp and its 2
   dependents are recompiled (`dd_sphere.cpp` then reuses the three-header
   `.pch` the full build left); `game.h` gets one and its 17 dependents are
   recompiled (each rebuilds the `.pch` from its own list:
   `bullsquid.obj`, `client.obj`); then `islave.cpp`, `beams.cpp`,
   `aflock.cpp` and `bigmomma.cpp` are compiled twice each (the second
   compile reuses the file's own `.pch`) and `freezethrower.cpp` and
   `func_break.cpp` once. Each is a plain `CL /c` with the project's and the
   file's flags and the same `/Fp` and `/Fd`, so VC6 decides each `.pch`
   itself. Only the objects' final states are evidence; the edits and their
   order are a stand-in that produces them.
3. **The PDB's history.** It deletes `hl.pdb` and links 1,433 times.
   Retail's debug record names a PDB created on 2002-04-12 23:45:57 UTC (its
   signature) and written by 1,433 links (its age). The first link runs
   with the clock at that creation time and writes a fresh `hl.pdb`. The
   other 1,432 run at retail's link time, 2002-08-17 07:56:57 UTC, which
   sets the COFF header's, the export directory's and the debug directory's
   time stamps. Each full link into an existing PDB keeps its signature and
   raises its age by one.

The clock is libfaketime's, with `TZ=UTC`, and only LINK runs under it.

### What the script does for the client

Retail's client was linked incrementally, and its time stamps and PDB age
record a history that the image shows in places
([below](#the-incrementally-linked-client)).
`tools/release/scientist-slaughterhouse_client.py` replays it in four
steps:

1. **A full build** of `cl_dll - Win32 Debug` of `cl_dll\cl_dll.dsp` into
   `cl_dll\Debug`: 97 sources (94 C++, 3 C; `overview.cpp` is excluded, as
   in the SDK) in project order with one `vc60.pch`, each by its full path,
   and with the project's browse information (`/FR`), which changes where
   VC6 emits lazily placed inline functions. The five files with an
   anonymous namespace are compiled at the seconds their names give. The
   first link creates the PDB at 2002-04-12 23:18:18 UTC, retail's PDB
   signature.
2. **456 relinks** that change nothing but the PDB's age: before each, one
   object's file time and COFF time stamp move on by a second, and LINK
   relinks it in place.
3. **The last full link.** `vgui_ServerBrowser.cpp` is compiled twice (the
   second compile reuses its own `.pch`, which gives retail's order of its
   type information) and `hl_weapons.cpp` once; then the `.ilk` is deleted
   and LINK makes a full link at 2002-07-01 15:12:23 UTC, the time of
   retail's export and debug directories.
4. **The last incremental relink.** `hl_weapons.cpp` is compiled again (now
   reusing its own `.pch`, which moves one inline function ahead of
   `HUD_InitClientWeapons`) and `sword.cpp` with the tree's own text; then
   LINK relinks incrementally at 2002-08-11 22:30:25 UTC, the COFF header's
   time. LINK keeps the old layout and moves what no longer fits. Age 459.

In steps 1 to 3 `dlls\sword.cpp` is replaced by
`tools/release/scientist-slaughterhouse/sword.0701.cpp`, an earlier text of
the file that ships with the script, not with the branch
([Build stand-ins](#build-stand-ins)).

The flags, from the two `.dsp` files:

| | Server (`hl.dsp`, Profile) | Client (`cl_dll.dsp`, Debug) |
|---|---|---|
| Compile | `/G5 /MT /W3 /Zi /O2 /YX`, defines `NDEBUG WIN32 _WINDOWS QUIVER VOXEL QUAKE2 VALVE_DLL CLIENT_WEAPONS` | `/G5 /MTd /W3 /GR /GX /Zi /O2 /FR /YX`, defines `_DEBUG _MBCS WIN32 _WINDOWS CLIENT_DLL CLIENT_WEAPONS` |
| Link | `/subsystem:windows /dll /profile /debug /machine:I386 /def:.\hl.def` | `/subsystem:windows /dll /debug /machine:I386`, incremental, plus the SDK's `utils/vgui/lib/win32_vc6/vgui.lib` and `wsock32.lib` |
| Sources | 128 `.cpp`, 3 `.c` | 94 `.cpp`, 3 `.c` built |

The script leaves out the IDE-only `/FD` and `/Gm`; it keeps the client's
`/FR`, writing the browse files into the output folder as the IDE does.

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
| `vc6/VC98/LIB/LIBCMT.LIB` (server) | | `1ef9c27b4f76031e40fcbdf7d518adc4969f90d6a00dd1ea88697907268c1b2b` |
| `vc6/VC98/LIB/LIBCMTD.LIB` (client) | | `36ed299ff236596e5037d352a0bea17b531ad75dca7c4eb6f1ea64cbd06029e5` |
| `vc6/VC98/LIB/OLDNAMES.LIB` | | `348a220e94f90bd9f530514ae59ff9b044b76e2056f52b45df406c12d2156afb` |
| `vc6/VC98/LIB/KERNEL32.LIB` | | `cd4a546f71481319aeee455aa0fb51698a094f9d3e6d8339acf445489a263c68` |
| `vc6/VC98/LIB/USER32.LIB` (client) | | `0a3c34c4daec09c1b9fd163d5d0b2ee9a6cbe0b3e863ae81a1fae3b9f936fd19` |
| `vc6/VC98/LIB/WINMM.LIB` (client) | | `bb098679a802afdbc6fdaac03dfe1ea81453a36f6d4768b1c1cd092b1dbe9342` |
| `vc6/VC98/LIB/WSOCK32.LIB` (client) | | `3ac0ae32d0c9ea77813db7f4d2ea8eba23ae420b40a24845cbe36f6702fca4ba` |
| `utils/vgui/lib/win32_vc6/vgui.lib` (in the branch; client only) | | `2f3a070def04fb0d542c1a10c2cc005d18d15fdd32d547a91e0c8e43bfb99785` |

The compiler and LINK run from `nativecrt`, on the CD's own `MSVCRT.DLL`,
not on Wine's builtin one, as for the other mods. Neither DLL folds
identical functions (both link with `/debug`), so this mod doesn't test
LINK's folding sort.

Not tried: other Wine versions, and Windows itself.

### If your build differs

| Symptom | Cause |
|---|---|
| Only the time stamps differ | LINK didn't run at retail's times |
| Server: the debug record's signature and age differ | the 1,433 links weren't replayed, or an old `hl.pdb` was left in `dlls\Profilehl` |
| Either DLL: the PDB path and the size of the debug record differ | the tree wasn't at `C:\Microsoft Visual Studio\MyProjects\Source Code` |
| Server: header inline functions in other orders in some of `aflock.obj`, `bigmomma.obj`, `bullsquid.obj`, `client.obj`, `dd_sphere.obj`, `islave.obj`, `beams.obj`, `freezethrower.obj`, `func_break.obj` | a full build only, without the last session |
| Client: the thunk table's order and every call through it differ, and the anonymous namespaces' names in the type information | the five files with anonymous namespaces weren't compiled at their seconds, or not by full path |
| Client: the type descriptors of several VGUI objects and `cdll_int.obj` in other orders | the compiler or `TMP` at paths of other lengths |
| Client: the age differs | the 456 relinks weren't replayed |
| Client: `HUD_InitClientWeapons` inside `hl_weapons.obj` and no hole at 0x10020660 | no incremental relink after the last full link |
| Client: `.data` shifted by 0x30 bytes from `sword.obj`'s strings on | `sword.cpp`'s own text used for the last full link |
| Server: 444 bytes differ, all in `CRocket::FlyThink` and its relocations | `rocket.cpp` before `c7f292c` |

## What the image holds besides code

A DLL also records how it was built. [reproducing.md](reproducing.md)
explains each part in general. For Scientist Slaughterhouse:

| Item | `hl.dll` | `client.dll` | How the build gets it |
|---|---|---|---|
| PDB record (CodeView NB10) at the end of the file | 88 bytes at 0x16d000: `C:\Microsoft Visual Studio\MyProjects\Source Code\dlls\Profilehl\hl.pdb`, signature 0x3cb771b5 (2002-04-12 23:45:57 UTC), age 1433 | 90 bytes at 0xe5000: `...\Source Code\cl_dll\Debug\client.pdb`, signature 0x3cb76b3a (2002-04-12 23:18:18 UTC), age 459 | The tree at that path; the server's 1,433 links into a fresh PDB; the client's history |
| Time stamps | COFF header, export and debug directory: 0x3d5e01c9 | COFF header 0x3d56e581 (the last link); export and debug directory 0x3d207157 (the last full link) | LINK runs with the clock frozen at each link's time |
| Rich header | Utc12 C 8168 ×125, Utc12 C++ 8168 ×131, Masm 6.13 7299 ×31, AliasObj 7291 ×3, Linker 5.12 8034 ×3, Import0 ×75, Linker 6.00 8168 ×1 | Utc12 C ×139, C++ ×105, Masm ×33, AliasObj ×3, Linker 6.00 8447 ×2 (the import objects of SDK 2.2's `vgui.lib`), Linker 5.12 ×9, Import0 ×646, Linker 6.00 8168 ×1 | The CD's compiler and libraries; Valve's file lists with the mod's 31 and 16 files. Server C++: 128 project files and 3 library objects; client: 94 and 11 |
| PE header position (`e_lfanew`) | 0xd8 | 0xe8 | Follows from the Rich header's length |
| Imports | KERNEL32, 75 functions | KERNEL32 82, USER32 3, WINMM 3, `vgui.dll` 557, WSOCK32 1, in that order | The `.dsp` library lists, and LINK on the CD's `MSVCRT.DLL` |
| Exports | 749, as `hl.dll`: `GetEntityAPI`, `GetEntityAPI2`, `GiveFnptrsToDll`, 368 entity factories, 378 decorated members | 545, as `client.dll`: 76 undecorated (the client interface and 30 weapon and ammo factories), 469 decorated; every one points at a thunk | `hl.def` and the `EXPORT` macro; the client's `dllexport` declarations |
| Code layout | non-incremental: objects in link order, functions in emission order | an incremental link: 5 bytes of `int3`, a thunk table of 2,481 `jmp`s (0x10001005 to 0x1000407a), 12,406 bytes of reserve, code from 0x100070f0, every function in a slot of 1.25 times its size | [The incrementally linked client](#the-incrementally-linked-client) |
| Thunk table order | none | LINK's hash of every external symbol's decorated name | Names that hash into retail's slots; the number of external symbols |
| Function order inside an object | header inlines placed by `/YX` and by the amount of intermediate code; nine objects show the last build session | the same, with `/FR`; `hl_weapons.obj` in two orders (the last full link's and the last relink's) | The full build, the replayed sessions, unused inline functions as stand-ins |
| Type information (RTTI) order | none (no `/GR`) | 168 type descriptors; their order in 22 objects follows C1XX's memory layout | The pinned paths and unused macros as stand-ins |
| Anonymous namespaces | none | five, named `?%C:\...\file.cpp<number>`, the number from the compile second; in the thunk table and in RTTI names | Each of the five files compiled at its second |
| `__FILE__` strings | none | the asserts' file names (in `pm_shared.c`, `voice_status.cpp`, `vgui_grid.cpp`, `bitvec.h`), with full paths | CL gets each source by its full path |
| `.bss`: uninitialised globals | ordered by a hash of their names | the same, inside each object's contribution | Names that give retail's order |
| Dead code and identical-code folding | none: `/debug` keeps every function | none | |
| C runtime | `/MT`, LIBCMT: 157 of its objects (SDK build's 158 without `strtok.obj`), in retail's load order | `/MTd`, LIBCMTD, exactly the SDK build's objects | The CD's `VC98/LIB`; the name the mod's code uses for `strnicmp` |
| Code padding | the compiler's `nop`s, each function padded to 16 bytes | the same, and LINK's `int3` in each slot's remaining quarter, in the reserves and in the hole the relink left | Follows from the code and the link history |

The sections below describe the parts this mod needed beyond what the other
mods needed.

## What identity needed

### The Profile server and its 1,433 links

The server was linked with the Profile configuration's `/profile /debug`:
every link is a full one, and each raises the PDB's age by one. 1,433 links
went into a PDB created on 2002-04-12 23:45:57, the last on 2002-08-17
07:56:57. Only the first time, the last time and the count reach the image,
so the script makes one link at the creation time and 1,432 at the last
time. The 1,431 links in between stand in for four months of builds whose
times and contents are unknown. They take nearly all of the build's time:
1,433 links of the full server take 705 seconds on our machine.

The server's code needed nothing else from the build history except the
`.pch` states of its last session (above).

### The incrementally linked client

The client is linked incrementally, as VC6's Debug configurations are. LINK
then lays the image out differently from a normal link, and a later
relink moves only what no longer fits. We worked out LINK 6.00's rules
([vc6-notes.md](vc6-notes.md#incremental-linking)) and found that retail's client is one fresh
incremental link plus one later relink:

- **The fresh layout.** `.text` starts with 5 bytes of `int3` and a thunk
  table (ILT): one `jmp` per external function defined in a project
  object. Then a reserve of 5 bytes per thunk, then the objects' code in
  link order, each function at the previous one's start plus its size plus
  a quarter of its size, rounded up to 16. 2,575 of retail's 2,579 code
  blocks are exactly where that rule puts them. The data sections follow
  the same rule with a fifth.
- **The thunk table's order is LINK's hash order** of every external
  symbol's decorated name: a linear hash table with 1,024 initial buckets
  that splits as it fills, walked bucket by bucket. So every external name
  in the client decides the order of 2,481 thunks, and with them every
  call, every vtable entry and every export, all of which point at thunks.
  The table's shape follows the total number of external symbols: retail's
  has 7,256 to 7,261. Ours had 7,273 at first, with the same functions,
  imports, library members, literals, constants and RTTI classes; fifteen
  file-private globals of the mod's files made `static` gave 7,258
  (`bcba432`). Then each name of ours had to hash into the window of
  buckets its retail slot leaves (see
  [Names chosen for hash orders](#names-chosen-for-hash-orders)).
- **The relink.** Retail's `HUD_InitClientWeapons` (512 bytes) sits at
  0x10017420, in the padding after `EV_FireGauss`, although it belongs to
  `hl_weapons.obj`; its place in `hl_weapons.obj`, 0x10020660, holds the
  16-byte `CBaseEntity::Respawn` stub followed by a 656-byte hole. That is
  what LINK does when it relinks an object whose functions changed order:
  it matches the new functions with the old by position, keeps one in its
  old slot if it fits and moves the other to the first gap that is large
  enough. So at the last full link `hl_weapons.obj` had
  `HUD_InitClientWeapons` first, and at the last relink `Respawn` first.
  Under `/YX` the second order needs no source change: `hl_weapons.cpp`
  builds its `.pch` itself in a full build and reuses its own when it is
  compiled again, and the two states emit `Respawn` on either side of
  `HUD_InitClientWeapons` (with the stand-in of `8873f38`).
- **The history in the time stamps.** The PDB was created on 2002-04-12
  23:18:18 (signature), the last full link was on 2002-07-01 15:12:23
  (export and debug directories), the last link on 2002-08-11 22:30:25
  (COFF header), and 459 links went into the PDB (age). Everything after
  the last full link changed only code of the same size, apart from the
  swap above: the reserve is exactly a fresh link's, and no thunk is stale.
  The script's 456 relinks stand in for the builds between April and July.
- **`sword.obj`'s data.** Retail's `.data` keeps two contributions of
  `sword.obj` at their fresh-link places (21 to 24 and 18 to 20 bytes) and
  has `p_sword.mdl` and `v_sword.mdl` further on, where LINK puts data a
  relink adds. So at the full link `sword.cpp` had two string literals
  fewer than at the last relink. The script compiles an earlier text for
  the full link (`sword.0701.cpp`: the sword with the crowbar's view and
  player models, which `crowbar.obj` already owns, so that its own strings
  are `yelflare1.spr` and `w_sword.mdl`). Only the two sizes and their order
  are recorded; the text is a guess. `whatsnew.txt` lists "Added sword
  weapon" for 1.0.
- **Anonymous namespaces.** VC6 names an anonymous namespace
  `?%<source path as given to CL><number>`, where the number comes from
  the C runtime's `rand()` seeded with the compile time and a checksum of
  the path. Retail's client has five, in `vgui_scrollbar2.cpp`,
  `vgui_slider2.cpp`, `vgui_ControlConfigPanel.cpp`,
  `vgui_ConsolePanel.cpp` and `vgui_ServerBrowser.cpp`, and their names are
  in the image twice: in the RTTI names of their classes and through the
  thunk table's hash. Each number gives exactly one second between 1998 and
  2003: 2002-04-19 01:37:55, 01:37:57 and 01:40:13 for the first three,
  2002-04-22 06:21:41 and 06:56:10 for the last two (UTC). The path is the
  full path, `C:\Microsoft Visual Studio\MyProjects\Source Code\...`: the
  IDE handed CL full paths, which the asserts' `__FILE__` strings show too.
  The script compiles these five files at those seconds.
- **The `.bss` order.** Each object's uninitialised globals are laid out by
  a hash of their names, as in a normal link
  ([reproducing.md](reproducing.md#bss-order)), so the mod's file-scope
  names in the client had to give retail's order too.

### The client's type information and the pinned paths

The client is compiled with `/GR`. At the end of a file, VC6's front end
(C1XX) writes the vtables the file still owes and with them the RTTI
records, in an order it keeps in a list sorted by the low 14 bits of each
class symbol's address in its own memory arena (read from `C1XX.DLL`; the
type descriptors come out in descending order of that key). The arena
holds everything C1XX allocated before the class: identifiers,
declarations, macro text, the full path of every header it opened, and the
path of its temporary file. Its regions are 64 KB aligned, so the key is an
offset that depends only on what came before, not on where Wine put the
memory.

So the order of the type descriptors in an object moves with the lengths of
those paths and with the amount of text before each class. 22 of the
client's objects have two or more type descriptors. We measured:

- With the compiler at whatever path the toolchain folder had, two host
  paths gave two clients 5,086 bytes apart: 9 objects' type information in
  other orders. A test bench with made-up paths of the same lengths
  reproduced all 22 objects' orders: only the lengths matter.
- `TMP` is stored once, early, so its length acts in steps: within a range
  of lengths nothing moves, across a threshold several objects change at
  once. Each header's path is stored twice, at its `#include`. A macro of N
  characters before a class moves it by N bytes until C1XX starts a new
  chunk of memory; past a page boundary everything after a chunk moves by
  4 KB, so the effect of more text is not smooth.

The script therefore pins the compiler at `C:\Microsoft Visual Studio\VC98`
(the include path is 39 characters) and `TMP` at
`C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp` (34 characters), and the branch has
unused macros, strings of dots, where retail's files had more text before a
class than ours: before the includes of `vgui_ControlConfigPanel.cpp` (107
characters), `vgui_ServerBrowser.cpp` (2), `vgui_ScorePanel.cpp` (16), in
`vgui_ClassMenu.cpp` (6 and 384), `vgui_TeamFortressViewport.cpp` (60 and
120), `cdll_int.cpp` (200) and `hud.h` (8,800 before `CHudDeathNotice` and
3,200 before `CHudIdent`, whose declaration moved before `CHudMenu`).
Each was measured in a window (for `hud.h`, about 8,370 to 9,260 and 2,560
to 3,990). They stand in for text retail's files had, and they only work
with the pinned paths: retail's own paths are not known, and a different
choice would need other stand-ins. With them the client is identical when
built from the toolchain at two different host paths (`f858c2d`,
`21a4bdc`, `bca2cf1`, `5fdd4ec`, `79ebc33`, `cfbcf72`, `39e1267`,
`55d32cb`, `e69c478`, `876965f`). The server has no RTTI: built through
two host paths of the toolchain, and with the compiler and `TMP` pinned as
for the client, it gave the same image.

`cdll_int.obj`, the last object, took the longest: its sixteen HUD classes
come from `hud.h`, which about fifteen other objects include, and retail's
order needed the classes from `CHudSpectator` to `CHudDeathNotice` farther
apart than our header's text allows. One more input showed up there: the
two big macros count as declarations under `/FR`
([Source that steers the compiler](#source-that-steers-the-compiler)).

### Where header inline functions land

As in the other mods, VC6 emits a file's own functions in source order and
the header inline functions it needs (vtable-only virtuals, implicit
constructors, template members) lazily, by their position in the
intermediate code (IL) of the file
([vc6-notes.md](vc6-notes.md#where-header-inline-functions-land)). In this
mod's server it took one model for all 130 objects with code: each own function
starts at an IL offset measured with the compiler's `/Bk` switch; a
header inline's position is its offset in the header region plus the end
of the precompiled header when the file created the `.pch`, 1,012 bytes
less when the file reused it, and its plain offset when the header is not
in the `.pch`; before each own function VC6 emits every needed inline whose
position is below that function's start, lowest first, and an emitted
item's references become needed at once. With each object's `.pch` state
from the replayed build, the model gives the order of 126 of the 130
objects with code exactly (the other four differ only in static
initialisers the measurement doesn't place), and against retail it was
wrong for exactly the objects whose text or history differed from
retail's.

From there the model said how much IL was missing and where:

- `aflock.obj` and `bigmomma.obj` show the reuse state: each file compiled
  twice at the end of the last session. With that, retail's headers had
  about 340 to 390 bytes more IL anywhere between `vector.h`'s operators
  and `CBaseEntity::CanBeGrenaded`; an unused inline function of 11
  statements in front of `CBaseEntity` stands in (10 or 11 fit) and puts
  `aflock.obj`, `bigmomma.obj` and `buttons.obj` in order (`a6cbc83`).
- `func_break.h` had about 200 to 300 bytes more in front of `CBreakable`'s
  inline virtuals (8 statements; 6 to 9 fit; `9572855`).
- `beams.cpp` had more text in two places (12 and 10 statements; 11 to 13
  and 4 to 16 fit; `b620502`), `hassassin.cpp` about 9.1 to 9.6 KB more
  somewhere between `CHAssassin::ISoundMask` and `CPowerBeam::Precache`
  (118 statements; 116 to 121 fit; `960fb70`), and `client.cpp` more
  before `CBasePlayer`'s inlines (61 statements, 60 to 62 fit; `02b3728`,
  grown in `a6cbc83`).
- The client compiles `cbase.h` too: `hl_weapons.obj`, which emits
  `CBaseEntity`'s inline virtuals between its own functions, needed a
  second stand-in of 10 statements and its first one grown from 36 to 38
  (`aeba9ff`), and then has retail's orders in both of its build states.

The client's other placements: `cl_effects.obj` emits 299 VGUI template
members and copy functions lazily around its three own functions, and needs
61 bytes more IL (an empty unused inline function, `3ef4948`);
`vgui_TeamFortressViewport.h` one more empty inline (`5995709`, `b7150ac`).

Some precompiled-header states come from a conditional before a file's
includes, which makes VC6 build the `.pch` from that file's own list and
keeps the next file from reusing it. `lpistol.cpp` and `fire.cpp` are
wrapped in the weapon files' own `#if !defined( OEM_BUILD ) &&
!defined( HLDEMO_BUILD )`: `lpistol.obj` then has `CLaserPistol`'s inlines
at its end, and `fireball.cpp`, compiled after `fire.cpp`, builds its own
`.pch` (`709d840`, `06cd04b`). `barnacle.cpp` includes `weapons.h` without
using it, so that `barney.cpp`, compiled next, builds its `.pch` from its
own list (`// sic`).

### `CLaserPulse`'s constructor: the two DLLs saw different text

`weapons.h` is compiled into both DLLs. Its `CLaserPulse` (the mod's laser
projectile, a `CGrenade`) has an empty inline constructor, `CLaserPulse()
{}`, only under `#ifdef CLIENT_DLL` (`1a33030`). The evidence is in where
each DLL places `CBaseEntity`'s implicit constructor:

- A user-declared inline constructor makes VC6 generate the implicit
  constructors of the class's bases (`CGrenade`, `CBaseMonster`,
  `CBaseToggle`, `CBaseAnimating`, `CBaseDelay`, `CBaseEntity`) where the
  header is parsed; without one they are generated where an object is
  first constructed.
- The server's `agrunt.obj` has `CBaseEntity`'s out-of-line constructor
  after `CNAGrunt::MonsterThink`, its last function. `agrunt.cpp` includes
  `weapons.h` and uses `CLaserPulse`, and `weapons.h` is outside its `.pch`
  (`squadmonster.h`'s and `effects.h`'s inlines follow their first use). A
  constructor generated in `weapons.h` would land right after
  `monster_na_grunt`'s factory, as ours did; one generated in `cbase.h` or
  `monsters.h` would land in the middle of the object. So the server's
  `weapons.h` declared no such constructor.
- The client's `hl_weapons.obj` needs those constructors, about 1.4 KB of
  IL, inside its precompiled header: generated at its `CBasePlayer` global
  instead, they move `CBaseEntity::Respawn` out of place in both of its
  build states, and own text can only add IL there. The trigger can't be
  in `player.h` (the server's `client.obj` would then have `CBaseMonster`'s
  constructor before `RegisterEncoders` instead of at its end), and
  `nodes.h` and the client's own headers declare no class derived from
  `CBaseMonster`.

So the two DLLs' `weapons.h` differed here. That is known; the form is not.
An inline constructor of `CLaserPulse` under `CLIENT_DLL` is one text that
gives both DLLs' bytes; any `CBaseMonster`-derived class with an inline
constructor in the client's `weapons.h` (and not in the server's) would do
the same. We kept this one, where the constructor already was, and mark
it `// sic`. Before
(`6ecce58`, `ca9372d`) the constructor was in both DLLs; without it in both
(`aa53e41`) the server's `agrunt.obj` was right and the client's
`hl_weapons.obj` wrong.

Removing the constructor from the server moved one more thing:
`CScientist::DeathMessage` swapped two stack slots, because its reload
order follows the number of symbols declared before it, which the
generated constructors had raised by 34. An unused enum of 17 constants
before it restores retail's slots (5 to 28 or 31 to 50 work; `dba54d2`).

### The last function: `FlyThink`'s repeated test

`CRocket::FlyThink` clamps a rocket's speed to 300 underwater. Retail's code
tests `flSpeed` against 300 once, but places the branch for "less than
300" after the bubble trail's call, jumping back into the multiply. No
spelling of a single clamp gave that: ternaries in both orientations,
`if` clamps, `if`/`else` into a local and inline helpers with one or two
returns either put that branch before the multiply (12 lines from retail)
or changed other code. Reading `C2.DLL`
explained why: C2 first lays out the blocks in reverse postorder, which
can't put that branch after the multiply, and later moves blocks only in
one pass, which needs a jump that ends a branch to survive the branch
optimiser ([vc6-notes.md](vc6-notes.md#block-order-and-the-block-mover)).

The answer was a second test that repeats the first:

```cpp
flSpeed = min( flSpeed, 300 );
pev->velocity = gpGlobals->v_forward * min( flSpeed, 300 );
```

After the first clamp the second test's outcome is known on each path, so
C2 threads both paths past it and drops its compare; the two branches then
end in jumps to different labels, and the block mover places the branch
that is jumped over after the block's closing jump, as retail has it. The
mod's game-rules `DeathNotice`, already identical, shows the same move for
its two `pevInflictor` tests. The other branch of `FlyThink` multiplies by
`min( flSpeed, pev->speed )`, so the repeated `min` is a plausible text,
not an invented one; `if ( flSpeed >= 300 ) flSpeed = 300;` as the first
clamp compiles to the same code, `>` leaves a second compare. Before this,
a dead `if ( 0 ) goto slow;` into the branch also gave retail's code (as a
dead back edge when C2 orders the blocks); it is not on the branch.
`c7f292c` made the server identical.

## What in the source is not known from retail

The image gives every function body that the compiler emitted. It gives
names only through exports, save tables, RTTI and strings, and through the
two hashes above; it gives nothing at all for text that compiled to no
code. So the source text is one spelling that gives retail's bytes, not
necessarily the mod's own. This section summarises, by kind:

- source that exists to steer the compiler;
- names chosen for hash orders;
- other choices the evidence leaves open, and names we made up;
- `// sic` lines that keep retail's behaviour;
- preprocessor directives;
- the build history the scripts stand in for.

The commit messages give the full reasoning and the measured windows.
`git grep -n -w sic scientist-slaughterhouse` lists 157 lines.

### Source that steers the compiler

Retail had something with the same effect in each of these places; the
text is ours, and every one is marked `// sic`.

| Kind | Where | What it moves | Commits |
|---|---|---|---|
| Unused inline functions (IL that compiles to nothing) | `cbase.h`, `func_break.h`, `beams.cpp` (two), `hassassin.cpp`, `client.cpp`, `hl_weapons.cpp` (two), `cl_effects.cpp`, `vgui_TeamFortressViewport.h` | where VC6 emits header inline functions | `a6cbc83`, `9572855`, `b620502`, `960fb70`, `02b3728`, `8873f38`, `aeba9ff`, `3ef4948`, `5995709`, `b7150ac` |
| Unused macros (strings of dots) | `cdll_int.cpp`, `hud.h` (two), `vgui_ClassMenu.cpp` (two), `vgui_ControlConfigPanel.cpp`, `vgui_ScorePanel.cpp`, `vgui_ServerBrowser.cpp`, `vgui_TeamFortressViewport.cpp` (two) | the order of the client's type information | `cfbcf72`, `f858c2d`, `21a4bdc`, `bca2cf1`, `5fdd4ec`, `79ebc33`, `39e1267`, `55d32cb`, `e69c478`, `876965f` |
| Unused enums (declaration counts) | `scientist.cpp` (17 constants), `hud.h` (10), `vgui_MOTDWindow.cpp` (3), `voice_vgui_tweakdlg.h` (1) | stack slots and reload orders that follow the number of symbols declared before a function | `dba54d2`, `bdfba9d`, `53f21b3`, `780ca2a`, `cfbcf72` |
| Unused locals, statements and copies inside functions | several functions, among them `CEgon::IonCannon`, `CDisplacer::WeaponIdle`, `CCrossbowBolt::LaserNet`, `CCannonShootpoint::Fire`, `CBeamTurret::BeamThink`, `CDSphere::RunAttack`, `CNAgent::GetSchedule` | the inliner's budget, the x87 operand order, stack slots | the commits of each function (for example `d1924e7`, `2fa996d`, `febe859`) |
| An unused `#include` | `barnacle.cpp` | the next file's precompiled header | `c37d429` |
| A conditional | `CLaserPulse() {}` under `#ifdef CLIENT_DLL` in `weapons.h` | where each DLL generates `CBaseEntity`'s implicit constructor | `1a33030` |
| Unused members and data | `CHud`'s two last `Vector`s (`m_vecUnused1`, `2`), the viewport's two 64-byte arrays, `headcrab.cpp`'s table of zombie classnames, `combat.cpp`'s two unused `tracerCount` statics | object sizes and the inliner's sites; data and `.bss` that retail has without a user | `bdfba9d`, `b01fd3a`, `d23120b`, `5213d17` |

Some lines are natural forms rather than invented text, and are not marked
as stand-ins:
the repeated clamp in `FlyThink`, the copy-initialised `Vector vecUp` in
`CWaterElemental::HandleAnimEvent` (`9af64eb`; it fixes the x87 operand
order of the last product), file-scope `const float` constants that retail
keeps in their objects' `.rdata` (in `crane.cpp`, `egon.cpp` and
`sniper.cpp`: `a6803a9`, `c826ef6`, `ba3a6f9`; also in `flyingobject.cpp`
and `teslamonster.cpp`), `util.h` declaring two of `util.cpp`'s
helpers (`2e2597a`, a declaration count in `nodes.cpp`), `sound.cpp`
calling `_strnicmp` (`4aa4cf2`, which decides when LINK pulls LIBCMT's
`strnicmp.obj`), `random_scis.cpp`'s `FindNextEntity` not being `static`
(`8ba3037`).

How much text was missing can be measured; what it was can't. The
unused-function stand-ins give windows of IL, the macros windows of text
length, the enums windows of symbol counts modulo 64. None of them adds a
function, a static, a string or an absolute address to the image.

### Names chosen for hash orders

Two hashes put names into the image without the names themselves: LINK's
hash of the client's external symbols, which orders the thunk table, and
VC6's 1024-bucket hash of uninitialised globals' names, which orders `.bss`
in both DLLs. Each slot of retail's thunk table leaves a window of buckets
that the name in it must hash into, a few of 2,048 (one to eight for the
names we had to change), and each global's neighbours leave a window in
`.bss`.

Many names fitted their windows as first written. Where such a name is a
plausible one and fitted at once, the source counts it as retail's: most
of the mod's new virtual functions of the base classes (`HeatSignature`,
`Shielded`, `CanBeRepulsed`, `CanBeTeleported`, `CanBeTractored`,
`CanBeGrenaded`, `GetKillDesc`, `GetKillVerb`, `GetHitGroupDesc`,
`MovementSpeed`, `GetBattlePower`, and `WeaponSpecial`, whose decorated
names fit in nine classes; `7205c2f`), `EV_IsInPVS` (`f99d2f4`),
`EV_Rain` (`a13699a`), `CTractorBeam`'s `Create`, `Delete` and `Render`,
`TractorBeamCreate`, `TractorBeamDelete` and `Matrix`'s constructor
(`f53bbb0`). A window
of a few buckets out of 2,048 makes a chance fit unlikely, but how many
names were weighed before the first fit is not on record, so this is
evidence, not proof. Where a name of ours didn't fit, we searched
systematic candidate lists and took the fitting name closest to what the
code does. Those names are ours: a name found by searching says nothing
about retail's name beyond the window.

Renamed for the client's thunk table (both DLLs where the name is shared):

- `CBaseEntity` and `CBaseMonster`: `GetEntityTriggerState` (the
  `master_watcher` query), `Install( CBaseMonster * )` (the sniper box),
  `FindBackwardRoute`, `FireHeavyShotgunShells` (`86619d4`);
  `TractorBeamRadius` (`d3a3e85`), `AllowsGrenades` (`56408b3`),
  `Resurrect( CBaseMonster * )` (`d833d5e`), and `CEgon::IonCannon`
  (`4caabd5`);
- the client's effects and events: `CalcMuzzlePos`, `GetMuzzleVel` and
  `AddNewParticle` of the particle system (`874334a`), `CParticleStruct`
  (`608205f`), `CRainVolume` with `RainEffectThink` and
  `RainEffectVidInit` (`ef3840c`), `Matrix::operator*` (`5781bc8`),
  `EV_ElectricField` (`722796d`), `EV_DoMeleeAttack`, `AttachGrowEffect`,
  `EmitFallingDrops`, `DrawGravityFieldCone`, `PM_Physics_LockMovement`
  (`9e8a5ae`), `FX_EmitSpriteFlame` (`ffc82b1`), `CL_HurtEffectUpdate`
  (`234c863`), `AddScientistEntries` and `MsgFunc_SciDeathInfo`
  (`99311f1`).

Named for `.bss` order:

- server `weapons.cpp`: `g_sModelIndexFireExplo`, `g_sModelIndexFire`,
  `g_iSphereEvent`, `g_iShockEvent` (`aedd24b`);
- client: the eight predicted weapons' objects in `hl_weapons.cpp`
  (`g_SoftGun` to `g_Sword`, `6982a14`), `g_flCurrentGravity`, `g_forward`,
  `g_right`, `g_up` (`effects.cpp`, `874334a`), `g_sprFlame`, `g_sprLightning`
  (`damagefx.cpp`, `ffc82b1`), `g_ScientistInfo`, `g_ScientistTeamInfo`
  (`vgui_ScorePanel.cpp`, `99311f1`), `g_flSphereEdgeFadeScale`,
  `g_flSphereColorGradient`, `g_pSpheresManager` (`sphere.cpp`,
  `dc71527`), `s_vecConeVerts` (`tractorbeam.cpp`, `587539b`).

The anonymous namespaces are not names but compile seconds; the script
supplies them ([above](#the-incrementally-linked-client)).

### Other choices and guesses

| Where | What | Why, and what is open |
|---|---|---|
| [`dlls/weapons.h:1543-1550`](../../../blob/scientist-slaughterhouse/dlls/weapons.h#L1543-L1550) | `CLaserPulse() {}` only in the client's compile | Known: the two DLLs' text differed so that only the client generated the base constructors in `weapons.h`. The form is a guess that we decided to keep ([above](#claserpulses-constructor-the-two-dlls-saw-different-text)). |
| The 31 server and 8 client file names | ours | Only the sort position is fixed. Some splits between files rest on the object count alone: whether `black_hole`, `crane`, `pulse` and the start of `flyingobject` are files of their own or the tails of neighbours, and whether the repulsor and reflector, and the sniper rifle and softgun, are two files each (the client compiles one of each pair). |
| [`cl_dll/hud.h`](../../../blob/scientist-slaughterhouse/cl_dll/hud.h) | `CHudIdent` declared before `CHudMenu` | A choice for the type-information order; where retail declared it is not known. |
| The fifteen file-private globals made `static` | which ones | The image shows only how many of the mod's globals were not external (12 to 17 of ours, from the thunk table's shape), not which (`bcba432`). |
| `sword.0701.cpp` | the July text | Two string sizes and their order are known; the text is a guess. |
| Names of everything the image doesn't name | ours | See below. |

**Names.** Retail names a server class only where a decorated export or a
save-table string carries it, and every polymorphic client class through
its RTTI name (`.?AV<class>@@`): the eight predicted weapons' classes
(`CDisplacer`, `CFlameThrower`, `CLaserPistol`, `CRepulsor`, `CSoftGun`,
`CSword`, `CTeslaCoil`, `CVortexLauncher`), the HUD classes and the
client's other classes with virtual functions are retail's. Entity names,
exported member names and saved field names are retail's too, and the
names the source counts as retail's because they fitted their thunk-table
windows at once (above) are likely to be. Everything else is ours: the
names renamed into their windows, the game rules' two new virtual
functions, the weapon ids `WEAPON_REPULSOR` to `WEAPON_SWORD` (values
retail's), the file names, the helpers in `util.cpp` and the effect code,
sequence enums, locals, and unexported functions. Comments mark them
`name ours`
(`git grep -n -i "names\? ours\|ours)" scientist-slaughterhouse` lists 422
lines in 100 files).

### `// sic` lines that keep retail's behaviour

Of the 157 `sic` lines, 105 mark retail's own oddities, which its code or
data proves and which steer nothing; 51 belong to the stand-ins above, and
one notes a form of retail's (a file-scope `const float` in
`flyingobject.cpp`, which retail keeps in that object's `.rdata`).
Examples of the oddities:

| Where | Retail behaviour |
|---|---|
| `dlls/beams.cpp` | a wide beam cannon's scorch decal is `DECAL_SCORCH1 + RANDOM_LONG(0,2)`, and 2 gives `DECAL_BLOOD1`; a beam message without its noise byte, so every byte after it shifts by one |
| `dlls/hassassin.cpp` | the NA agent's battle power is the constant 2747083.25; two `switch` cases fall through; a beam that burns through a wall decals its entry point a second time, not the exit point it has just traced |
| `dlls/scientist.cpp` | a scientist cut in half: the lower half gets the same `SetBodygroup( 2, 3 )` call twice |
| `dlls/rocket.cpp` | the heat-seeking rocket skips targets with the same owner as itself, not its owner |
| `dlls/barnacle.cpp` | the cough chance, `RANDOM_LONG(0,99) == max( 5, iChance )`, is 1 in 100 however much the barnacle has eaten |
| `cl_dll/vgui_ScorePanel.cpp` | `m_bHasBeenSorted` indexed by row, not by player |
| `dlls/client.cpp` | chat hands the game rules the edict, not the player |

### Preprocessor directives

Compiled code keeps no preprocessor directives, so every conditional in the
branch needs a source of its own.

- No project-wide `/D` was added beyond `CLIENT_WEAPONS` in the server's
  Profile configuration, which the code shows (see
  [What the code is](#what-the-code-is)). No source file has a `/D` of its
  own.
- Both projects define `CLIENT_WEAPONS`; the server defines `VALVE_DLL`, the
  client `CLIENT_DLL`. The 80 `#ifndef CLIENT_DLL` lines the mod's code
  adds are all in files compiled into both DLLs (the predicted weapons'
  files and `weapons.h`), around code that only the server's image has:
  projectiles, death-message overrides (the client's vtables keep
  `CBaseEntity`'s), entity factories the client doesn't export. Their
  evidence is the difference between the two compiled versions.
- The 15 `#if defined( CLIENT_WEAPONS )` come with the SDK text the mod's
  weapons copy (`UseDecrement`); both DLLs compile the first branch.
- `#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )` around
  `lpistol.cpp` and `fire.cpp` is the weapon files' own SDK guard, placed
  where retail's precompiled-header states need a conditional before the
  includes.
- One conditional has no SDK source: the `#ifdef CLIENT_DLL` around
  `CLaserPulse() {}`. Its evidence is that the two DLLs compiled `weapons.h`
  differently; see above.
- No `#pragma` was added.

### Build stand-ins

The build reproduces history that the images record only in part.

| Where | What | Evidence and status |
|---|---|---|
| Server: 1 + 1,432 links | The PDB's history | Retail gives the creation time, the last link time and the count (age 1433). The 1,431 middle links at the last time are a stand-in. |
| Server: the last session | `flyingobject.h` and `game.h` edited (19 recompiles), then six files compiled once or twice | Each object's final `.pch` state is evidence; the edits and their order are a stand-in that produces those states. |
| Client: 456 relinks | The builds from April to July | Only the age counts them. Each relinks one object in place. |
| Client: the last full link | `vgui_ServerBrowser.cpp` twice and `hl_weapons.cpp` before it | `vgui_ServerBrowser.obj`'s type information and `hl_weapons.obj`'s first order are evidence; which compiles produced them is not. |
| Client: `sword.0701.cpp` | `sword.cpp` at the last full link | Two string sizes and their order are recorded; the text is a guess. |
| Client: compile seconds | Five files at the seconds their anonymous namespaces give | Each number gives exactly one second between 1998 and 2003. Known, not a stand-in. |
| Both: the compiler at `C:\Microsoft Visual Studio\VC98`, `TMP` at `C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp` | The original machine's paths | Only their lengths matter, and the client's type-information stand-ins were tuned for these. Retail's paths are not recorded. |
| The machine | Wine 11.0 and the CD's `MSVCRT.DLL` | Stand in for the Windows system the mod was built on. |

**Known from retail, not stand-ins:** the tree's path and drive (from both
PDB records and the asserts), the configurations, the link times, both
PDBs' creation times and ages, the client's last full link, the compile
seconds of the five files with anonymous namespaces, the compiler and
linker builds and the object counts (from the Rich headers).

## How identity was reached

The work went in this order: the base, then the base classes, then the
mod's code in parallel by area, then everything else the images record. It
was done between 2026-11-17 and 2026-11-19.
[vc6-notes.md](vc6-notes.md) has the compiler and linker rules this
needed.

**Functions.** The mod's changes were written from the disassembly and
decompiled code, compiled, and compared with retail one function at a time,
file by file. "Exact" means byte for byte, with addresses masked, in a
plain full build.

| Commit | Server: of 4,778 game functions exact | Client: of 2,581 exact | What it added |
|---|---|---|---|
| `3518377` (the base) | 1,203 | 1,461 | Valve's files arranged as retail's projects |
| `d11f13f` | 2,643 | 1,802 | the base classes' new virtuals and fields |
| `ea3342a` | 2,921 | 1,830 | the player, game rules and utilities; monster AI and combat; the world entities |
| `09e1dde` | 3,309 | 1,918 | the SDK weapons, the rockets and the cluster bomb |
| `6d13db5` | 3,531 | 1,918 | the flock, alien grunts, apaches, barnacle, barneys, gonarch, bullsquid, controller and crane |
| `4b73019` | 3,531 | 2,092 | the client's HUD, scoreboard, view, input and the mod's client effects |
| `970a434` | 3,754 | 2,092 | grunts, alien slaves, scientists, ospreys, zombies, the tesla tower |
| `c4ce13f` | 3,955 | 2,159 | reflector, repulsor, sniper rifle, softgun, sword, tesla coil, vortex launcher |
| `7365e4b` | 4,105 | 2,201 | laser pistol, displacer, flamethrower, fireball launcher, freeze thrower, black hole, flying objects |
| `9b8c81d` | 4,320 | 2,201 | gargantua, G-Man, assassins, headcrab, houndeye and the mod's drones, disk, elemental, flying head |
| `e337656` | 4,486 | 2,201 | triggers, platforms, map rules, xen, the volcano and the beam entities |
| `ff39950` | 4,486 | 2,515 | the client's events and weapon prediction |
| `b3a3f08` | 4,732 | 2,515 | the turrets and `func_turret` |
| `e6353ea` | 4,745 | 2,550 | the last near misses of the first round (node graph, crane, sniper rifle, beams; the HUD constructor, the tractor beam) |

At `b3a3f08` the whole mod was written: 46 server functions (37,552 bytes)
and 66 client functions (7,936 bytes) still differed, as near misses or as
functions in the wrong place. From then on the release images were the
measure.

**The server file.** The first release build (the tree at retail's path,
the PDB's 1,433 links, retail's link time) already had retail's exports,
debug record, Rich header, imports, `.data` and `.bss` layout and `.rdata`
size; its headers differed only in `.text`'s size. Then the bytes that
differ in the whole file (`cmp -l`) went down like this:

| Commit | Bytes differing | What changed |
|---|---|---|
| `5ce1961` | 886,857 | the first release build: 18 functions' code differs and `.text` is 32 bytes long, which shifts everything after the first of them |
| `f86c393` | 95,436 | 12 of the 18 exact; `.text` has retail's size |
| `d315c96` | 9,596 | the placement model's stand-ins and four more functions; left: `agrunt.obj`'s order, `CWaterElemental::HandleAnimEvent`, `CRocket::FlyThink` |
| `c4fd96f` | 444 | `CLaserPulse`'s constructor in the client only, `HandleAnimEvent`, two zero tests against `g_vecZero`, `CWaterSpray`'s base class; left: `FlyThink` (433 bytes of `.text`, 11 of `.reloc`) |
| `c7f292c` | 0 | `FlyThink`'s repeated test |

`FlyThink` took the most work for its size: several rounds of reading
`C2.DLL`'s block layout before the repeated test was found.

**The client file.** The first release build replayed the history (the
relinks, the full link, the relink) and had retail's time stamps, age,
signature, Rich header, `.idata`, export names and ordinals from the
start, and retail's pattern of 2,579 code blocks: 2,575 in fresh-link
slots, the others where the relink put them.

| Commit | Bytes differing | What changed |
|---|---|---|
| `5ce1961` | 11,437 | the first release build: thunk-table and `.bss` names, a few data items |
| `8887d6f` | 1,131 | names in retail's hash orders; every function exact, every thunk slot and `.bss` item at retail's place; left: type-information order in `cdll_int.obj`, `vgui_ClassMenu.obj`, `vgui_TeamFortressViewport.obj` |
| `9772e9c` | 330 | the compiler and `TMP` at pinned paths, the type-information stand-ins re-tuned for them (the tree before gave 4,442 bytes with those paths); left: `cdll_int.obj`'s HUD classes |
| `48193ac` | 0 | the HUD classes' stand-ins in `hud.h` and `cdll_int.cpp`, and the viewport's `iTeamColors` with its red and blue rows as retail's (`089e19c`) |

Until the last name fitted, the counts stayed high: one name in the wrong
bucket moves the thunks after it, and every call and vtable entry that
goes through them.

## Check it yourself

1. Check the installer and the retail files against the hashes in
   [The retail files](#the-retail-files):

   ```sh
   sha256sum ssh10.exe
   unzip ssh10.exe
   sha256sum ssh/DLLS/hl.dll ssh/cl_dlls/client.dll
   ```

2. Build and compare:

   ```sh
   RETAIL=$PWD/ssh VC_TOOLCHAINS=/path/to/toolchains \
       tools/release/scientist-slaughterhouse.sh ../ssh ../ssh-build
   echo $?
   ```

   Expected: `identical to retail` for both DLLs, `cmp` reporting no
   difference, exit code 0. The server part prints a full build of 131
   sources, the last session (`flyingobject.h` edited, 2 sources
   recompiled; `game.h` edited, 17; then `islave.cpp`, `beams.cpp`,
   `freezethrower.cpp`, `func_break.cpp`, `aflock.cpp`, `bigmomma.cpp`),
   link 1 of 1433 at 2002-04-12 23:45:57 UTC and links 2 to 1433 at
   2002-08-17 07:56:57 UTC. The client part prints a full build of 97
   sources, 456 relinks, a full link at 2002-07-01 15:12:23 UTC and an
   incremental relink at 2002-08-11 22:30:25 UTC.

3. Look at the parts besides code with GNU binutils, on retail's files and
   on yours:

   ```sh
   TZ=UTC objdump -p hl.dll | grep -E 'Time/Date|format NB10'
   TZ=UTC objdump -p client.dll | grep -E 'Time/Date|format NB10'
   ```

   Expected for `hl.dll` (objdump prints the signature's bytes in file
   order):

   ```
   Time/Date		Sat Aug 17 07:56:57 2002
   Time/Date stamp 		3d5e01c9
   (format NB10 signature b571b73c age 1433 pdb C:\Microsoft Visual Studio\MyProjects\Source Code\dlls\Profilehl\hl.pdb)
   ```

   and for `client.dll`, whose export directory keeps the last full link's
   time while the COFF header has the last link's:

   ```
   Time/Date		Sun Aug 11 22:30:25 2002
   Time/Date stamp 		3d207157
   (format NB10 signature 3a6bb73c age 459 pdb C:\Microsoft Visual Studio\MyProjects\Source Code\cl_dll\Debug\client.pdb)
   ```

4. Look at the anonymous namespaces' names in retail's client:

   ```sh
   strings -n 6 client.dll | grep -F '?%'
   ```

   Each name ends in the number that the compile second and the path give.

5. See what a stand-in does: in your checkout, revert the last commit
   (`git revert --no-commit c7f292c`) and run the script for the server
   again. The build then differs from retail in 444 bytes: the
   `flSpeed < 300` branch of `CRocket::FlyThink` sits before the multiply,
   and its relocations follow.

The tools we used to compare partial matches are not in this repository: a
function matcher, a per-object order check that pairs functions by their
masked bytes, image comparisons, the model of VC6's lazy inline emission,
the thunk-table hash and its bucket windows, and test benches for the
compiler's arena. For identical files, `cmp` is the whole test.
