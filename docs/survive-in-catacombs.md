# Survive in Catacombs 1 and 2

Survive in Catacombs is a horror single-player mod for Half-Life by Roman
Rynda, built on Laurie Cheers' Spirit of Half-Life 1.2. It came out in May
2004 (gamedir `Catacombs`). Its readme says it was made for a contest of the
magazine Gamestar, and the sequel's readme thanks Gamestar for the first
place. The sequel, Survive in Catacombs 2 (its readme says "Survive in
Catacombs 2: Fear"), followed in December 2004 (gamedir `Bloodbath`). It is
the first mod's code moved on. The readmes credit Roman Rynda Jr. with the
programming and most of the rest (concept, story, maps, models), and Marek
Rynda with voices, sound and, in the sequel, some models. Below, the two
games are SiC 1 and SiC 2. Each ships two game DLLs, built with Visual C++
6.0 (VC6):

| | Server: game logic, monsters, weapons | Client: HUD, the client side of the weapons, the music player |
|---|---|---|
| SiC 1 | `Catacombs/Dlls/sic.dll` | `Catacombs/Cl_dlls/client.dll` |
| SiC 2 | `Bloodbath/Dlls/bloodbath.dll` | `Bloodbath/Cl_dlls/client.dll` |

Both mods also ship `fmod.dll` (FMOD, which the client's music player loads
at run time; SiC 1 has it in the mod folder, SiC 2 in `Cl_dlls/`). It is
third-party code and not part of this work.

The branch [`sic`](../../../tree/sic) holds C++ source that compiles back
into all four DLL files, byte for byte, from one tree: SiC 2's code is the
default, SiC 1's sits under `#ifdef SIC1`, and each project has a
configuration per game. Its history:

- Valve's Half-Life SDK releases of 1999 to 2001, five commits (`b443b44` to
  `4ec16cc`);
- Spirit of Half-Life 1.0, 1.1 and 1.2 as released (`2f32593`, `be47bc3`,
  `ddefec0`), each commit the release's files exactly;
- 104 commits (94 without merges): 37 for SiC 1 up to `ec3024d`, 63 for
  SiC 2 up to `21fc2ea`, then four, `8f70aae` to `8d34187`, that make one
  tree of the two.

`ec3024d` (SiC 1 as released) and `21fc2ea` (SiC 2) are the historical
trees: each builds its own game's two retail files. Each of the mods'
commit messages says what the change matched and why. This page describes
commit `8d34187`, the head of the branch.

## Contents

- [Sources of truth](#sources-of-truth)
- [Status](#status)
- [What the code is](#what-the-code-is)
- [One tree for both games](#one-tree-for-both-games)
- [The retail files](#the-retail-files)
- [Rebuilding](#rebuilding)
- [What the image holds besides code](#what-the-image-holds-besides-code)
- [What in the source is not known from retail](#what-in-the-source-is-not-known-from-retail)
- [How identity was reached](#how-identity-was-reached)
- [Check it yourself](#check-it-yourself)

## Sources of truth

No community source of either mod, no later port or rewrite of Spirit of
Half-Life, and no recreation of Half-Life code was read at any point.
Everything on the branch comes from:

- the four retail DLLs;
- Valve's SDK text (the SDK commits on the branch);
- Spirit of Half-Life's original source releases, from the Internet Archive
  item [`spirit-of-half-life`](https://archive.org/details/spirit-of-half-life).
  1.2 is the base. 1.0 and 1.1 were built for comparison and are on the
  branch as its history. 1.4 holds a copy of the community MP3-player
  tutorial that SiC 1 also used; its text was the starting point for the
  mod's copy (`cl_dll/mp3.cpp`, `mp3.h`, and 1.4's FMOD headers `fmod.h` and
  `fmod_errors.h`), cut back to what retail has (`0958d87`);
- the mods' own data files: the maps' entity lists, `skill.cfg`,
  `sentences.txt`, `materials.txt`, the sprite lists, `liblist.gam` and the
  readmes.

So the source can be checked against the games themselves: it compiles to
the retail machine code, function for function, and the release builds give
the retail files. What it says about the games' behaviour is what the shipped
binaries do. Where a binary keeps a bug or an oddity, the source keeps it
too, marked `// sic`. Where the binary doesn't record something (a name, a
file name, text that compiled to nothing), this page says so.

## Status

All four DLLs are identical to retail.

| Game | File in the mod folder | Size (bytes) | SHA-256 | Link time (UTC) | Project configuration | First identical at |
|---|---|---|---|---|---|---|
| SiC 1 | `Dlls/sic.dll` (built as `spirit.dll`) | 1,212,476 | `eab217809f7c656acb7678b2035a4dc7f82a08db276febaceef72dfa55509359` | 2004-05-19 21:32:38 | `dlls/hl.dsp`, "hl - Win32 Release SIC1" | `ec3024d` (2026-11-19) |
| SiC 1 | `Cl_dlls/client.dll` | 540,672 | `0cb9fd2beed7c6ed327545a280826ecaaa5269ae3539c9a0602522cca3e6527a` | 2004-05-19 19:04:12 | `cl_dll/cl_dll.dsp`, "cl_dll - Win32 Release SIC1" | `ec3024d` (2026-11-19) |
| SiC 2 | `Dlls/bloodbath.dll` | 1,208,393 | `673f7e4ef7c75fdf551c87470f6e4f8a5a66f16fe98bfb44ca6436d14c88ba57` | 2004-12-19 18:27:24 | `dlls/hl.dsp`, "hl - Win32 Release" | `2d74976` (2026-11-19) |
| SiC 2 | `Cl_dlls/client.dll` | 540,672 | `76cc187b538b7fa0ae6bea5842e1484369fb64c6d7184851fb77de96cedbe21a` | 2004-12-17 10:31:35 | `cl_dll/cl_dll.dsp`, "cl_dll - Win32 Release" | `2d74976` (2026-11-19) |

"First identical at" is the historical commit, where each project has only
its `... Release` configuration. `2d74976` changes only four server sources,
so SiC 2's client builds the same from its parent `4ed7ecc`, which made the
client's last change. From `8d34187` on, one tree builds all four files with
the configurations in the table.

Verified on 2026-11-19, from a fresh clone of the repository at `8d34187`
and a new toolchain folder, by following
[rebuild/survive-in-catacombs.md](rebuild/survive-in-catacombs.md) as
written: release builds of the four files have retail's SHA-256, and `cmp`
finds no difference. The same holds for the historical trees, `ec3024d` and
`21fc2ea` (below).

**What "identical" means here.** It means the files: every byte of all four
DLLs, with the headers, time stamps, debug records and padding. It does not
mean that every line of the source is the mods' own text. A compiled DLL
doesn't record names, comments, file names, or text that compiled to no
code. In those places the branch has a marked stand-in:

- the switch `SIC1` itself and the two `... Release SIC1` configurations,
  which exist only so that one tree holds both games;
- in `dlls/agrunt.cpp`, a `#pragma hdrstop`, and in SiC 1's server
  configuration, no precompiled header for that file: both only so that the
  one tree gives each game's precompiled-header states;
- one declaration, of SiC 2's `UTIL_ExplosionSprite` in `dlls/util.h`,
  which gives one function retail's register order;
- names for eleven sprite-index globals (one of SiC 1, ten of SiC 2), chosen
  for where the compiler puts them;
- the names of SiC 2's four new source files;
- the four files SiC 2 emptied: the image shows that they had no code, not
  whether comments stayed in them;
- in the servers' builds: 4 of the PDB's 6 links (SiC 1) and 6 of its 8
  (SiC 2), and the last build session;
- for SiC 2, the C runtime that LINK ran on.

No unused inline function, placeholder object or `// sic` line that steers
the compiler was needed.
[What in the source is not known from retail](#what-in-the-source-is-not-known-from-retail)
lists each one with what is known and what is not.

## What the code is

"Game functions" below are retail's functions outside the C runtime and the
import thunks. "Exact" means byte for byte, with addresses masked, in a plain
full build.

### Spirit of Half-Life 1.2, built as it ships

- **The names are Spirit's.** Both servers' export directories name the DLL
  `spirit.dll`: Spirit of Half-Life's `hl.def` says `LIBRARY spirit`. SiC 1's
  debug record names `F:\SicEngineCodes\DLLS\Releasehl\spirit.pdb`:
  `Releasehl\spirit.dll` is the Release output of Spirit's `hl.dsp`, so the
  DLL was built under that name and shipped renamed `sic.dll`. Both servers
  have Spirit's cvars `sohl_impulsetarget` and `sohl_mwdebug`.
- **The entities are 1.2's.** SiC 1's server exports 305 entity factories:
  303 of Spirit 1.2's classnames, plus `env_chrome` (1.2's `func_shine`
  under another name, `bb25fa5`) and `trigger_mp3audio` (`d3035e9`).
  `func_shine` is the only one of 1.2's that is missing.
- **1.2, not 1.0 or 1.1.** Each release built in its own Release
  configuration places this much of retail's code (`.text`):

  | Release | Server | Client |
  |---|---|---|
  | Spirit 1.0 | 90.0% | 58.9% |
  | Spirit 1.1 | 91.7% | 85.9% |
  | **Spirit 1.2** | **93.3%** | **91.7%** |
  | Spirit 1.4, with 1.2's configuration | 47.1% | 79.7% |

  Nothing in the server matches only 1.0's or 1.1's build.
- **Not 1.4.** Spirit 1.4 came out in November 2004, after SiC 1. The only
  retail code that matches 1.4's build and not 1.2's is the MP3 player:
  the server's `trigger_mp3audio` and the client's `PlayMP3` message and
  `stopmp3` command. 1.4's source credits that code to a community tutorial
  (Killar's MP3 player), and SiC 1's copy differs from 1.4's where each
  adapted it: the commands are `playmp3` and `stopmp3` (1.4: `playaudio`,
  `stopaudio`), songs come from `sound/mp3/` (1.4: `sound/fmod/`), and the
  messages are Czech. So SiC 1 is 1.2 plus the tutorial plus the mod's own
  changes.
- **No source files of its own in the server.** LINK's Rich header counts
  every object. A build of Spirit 1.2's `hl.dsp` file list gives exactly
  retail's counts: C ×127 (124 runtime objects and the 3 `pm_shared` files)
  and C++ ×104 (3 runtime objects and 101 sources). One more or one fewer
  source would change the C++ count. Retail's code follows the project's
  order object for object, `aflock.cpp` to `zombie.cpp`.
- **One file more in the client.** SiC 1's client is 1.2's `cl_dll.dsp` plus
  the MP3 player (C++ ×94 against 93). Its four functions lie between
  `interface.obj` and `mp5.obj`, exactly where Spirit 1.4's project later
  put the same code as `mp3.cpp` (`0958d87`).
- **The configuration as shipped.** Server `/G5 /MT /W3 /Zi /O2` with
  `CLIENT_WEAPONS`, linked with `/debug` and without incremental-link
  thunks; client `/MT /W3 /GX /Zi /O2`, no `/debug`. Laurie Cheers' own
  build logs of 1.2, on the branch as
  [`dlls/hl.plg`](../../../blob/sic/dlls/hl.plg) and
  [`cl_dll/cl_dll.plg`](../../../blob/sic/cl_dll/cl_dll.plg), show the IDE
  adding `/incremental:no` to the link line, which `6a28b30` writes into
  `hl.dsp`, and passing the browse-information flags `/Fr".\Releasehl/"`
  and `/FR".\Release/"`, which turn out to change code
  ([What the image holds besides code](#what-the-image-holds-besides-code)).
- **Valve's SDK underneath.** Spirit 1.0 was built on SDK 2.2; from 1.1 on
  its files carry SDK 2.3's text. The engine interfaces of all four DLLs are
  2.3's: the servers copy 144 engine functions, the clients 101.

With nothing of the mod written (`6a28b30`), that tree gave 3,675 of SiC 1's
3,755 server game functions (96.0% of their bytes) and 1,465 of the
client's 1,550 (95.1%). These counts were taken without browse information.

No class layout changed in SiC 1: all 141 save tables are Spirit 1.2's.

### What Survive in Catacombs changed

All of SiC 1's code sits in Spirit's files and the new `mp3.cpp`. It was
written in three parts, merged as `c6f0864` (monsters), `887a9e2` (the MP3
player, the player and the HUD) and `ec3024d` (weapons):

| Area | What changed | Commits |
|---|---|---|
| MP3 player | `trigger_mp3audio` in `triggers.cpp`; `playmp3` in `ClientCommand` sends the new `PlayMP3` message; the client's `CMP3` loads `fmod.dll` with `LoadLibrary` and plays a song from `sound/mp3/` once; `CHud::Init` hooks the message and adds `stopmp3` | `d3035e9`, `ceb8100`, `4ca72d2`, `0958d87`, `e1c8c06` |
| HUD | red HUD colour (0xff1010); the health number in its own green at 100 or less | `e1c8c06`, `86affa4` |
| Weapons | ammo and clip sizes (`weapons.h`); the shotgun's ten pellets in a 5° cone; the MP5 fires every 0.07 s; a 2 s grenade fuse; the crowbar's "say" model; monsters' bullets can gib; the client's weapon events (recoil, pellet traces) | `705b978`, `3c9f04a`, `fdf4a49`, `5dbccc2`, `2d990b5`, `a311498`, `5e5b3b9`, `2841032` |
| Explosions | the grenade explosion's shock ring, sparks and smoke puffs; bouncing grenades hurt only above 250 units/s; burning shrapnel from `spark_shower` and `env_explosion`; `cycler_wreckage`'s smoke | `b96add0`, `935d644`, `5812e82`, `a5f5448` |
| Monsters | red blood for the alien monsters, none for the controller; Barney's armour stops no bullets; scientists back to Valve's SDK text, with no healing; the bullsquid, vortigaunt, zombie and controller changed; the alien grunt with the mod's warrior sounds | `911d2c8`, `3434225`, `f3cc938`, `37f58ff`, `a4803de`, `3fa53be`, `b0f40c7`, `9a4c2bf`, `86d05a0`, `29aa44f` |
| Other | `func_shine` as `env_chrome`; human gibs `models/flesh.mdl`; the battery `models/armor.mdl`; the player's muzzle flash never goes out; the game name `SurviveInCatacombs` | `bb25fa5`, `5733eaf`, `a169501`, `7127be6`, `4ca72d2`, `359574b` |

### What Survive in Catacombs 2 changed

SiC 2 is SiC 1's tree moved on. SiC 1's retail DLLs place more of SiC 2's
code than any Spirit build does (server 89.8% of `.text` against 88.2% for
1.2; client 92.8% against 90.6%), and the compiler flags are the same. The
projects changed in three ways:

- **The output.** The server is linked as `bloodbath.dll`, so its PDB is
  `bloodbath.pdb`; `hl.def` still says `LIBRARY spirit`, so the export name
  stays `spirit.dll`. The tree moved from `F:\SicEngineCodes` to
  `C:\bloody\bloodyEnginecodes` (`20fa520`).
- **Four alien monsters gone.** SiC 2 has none of the alien grunt's,
  bullsquid's, human assassin's or vortigaunt's code, save tables or
  classnames. `agrunt.cpp`, `bullsquid.cpp`, `hassassin.cpp` and
  `islave.cpp` stay in the project as empty files (`ca80744`, `2d74976`).
- **Four new files**: `barneyzom.cpp`, `cremator.cpp`, `scutspider.cpp` and
  `z_ghoul.cpp` (`fde2b87`). The Rich header counts C++ ×108: 3 runtime
  objects and 105 sources, which is SiC 1's 101 and four. The data shows
  four new objects: in every VC6 object here an object's initialised globals
  come before its own string literals, so code added to an existing file
  would put its globals among that file's; each of the four has its globals
  after its predecessor's strings. That leaves no room for a file taken out
  of the project, so the alien files stayed in it, compiling to nothing.
  `explo_debris` and `explode_ball`, whose code lies between `effects.cpp`'s
  and `egon.cpp`'s and which have no globals, end `effects.cpp`.

The code changes, merged as `49b53e4` (skill data), `023ff22` (weapons),
`ea1e3d9` (monsters) and `1825f49` (effects, world and player), then
`afecfc0`, `4ed7ecc` and `2d74976`, merged as `21fc2ea`:

| Area | What changed | Commits |
|---|---|---|
| Skill data | `skilldata_t` gains fields for the barney zombie, the cremator and the ghoul (0x170 bytes; every field after Barney's moved); the fork's, MP18's, flare's and TNT's damage take the old weapons' places; twelve new skill cvars | `13a03ed`, `6d6905a`, `7264ae2` |
| New monsters | `monster_barneyzombie` (Barney's code as a zombie); `cremator` and `monster_ghoul` (copies of the zombie); `monster_scutspider` and `monster_babyspider` (copies of the headcrab and the baby headcrab) | `e056784`, `fe982e2`, `7fae775`, `fe7601c` |
| Weapons, renamed in place | the crowbar is `weapon_fork`, the crossbow `weapon_flaregun` with `ammo_flares`, the hand grenade `weapon_tnt`, the MP5 `weapon_bergmann`/`weapon_mp18` with `ammo_bergmann`, each in its old file and class; a double-barrelled shotgun; the 357 in HUD slot 4 | `26e65ca`, `fb94efd`, `ede9e32`, `ef47618`, `3bba16c`, `e9ba980` |
| Explosions and effects | the grenade explosion with fireballs, debris and balls of flame; `explo_debris` and `explode_ball`; flying spark showers; blood clouds and ten gibs; `cycler_baloon`; ten new sprites | `19cd2a7`, `c4f96c9`, `6166dca`, `c2236b1`, `41f0116`, `93f4f6c`, `e770b7d` |
| Monsters changed | the controller as a ghost skeleton; new models for the zombie and the scientists; Barney's 5° cone; the headcrab an alien monster; the big momma lays scut spiders; grunts drop the MP18; monster makers' children always fade; furniture doesn't bleed; the nihilanth's sprites glow | `ab3d23a`, `6d4384c`, `246ead0`, `3abd178`, `c98dc0b`, `79f42f9`, `9050bfd`, `454b620`, `65e9570`, `16ca2c4`, `ec2965f`, `b3ffcdc` |
| World and client | grave texture sounds in both DLLs; impulse 101; breakables' spawn objects; infodecal's dust puffs; `light_glow`'s sprite; the red health HUD; the music player plays from `aud_sic/`; the game name "Survive In Catacombs 2" | `282b555`, `fbe0519`, `5a5c423`, `2508d23`, `e119749`, `7464314`, `f59fa6c`, `1f9f3cb`, `26b8f9c` |

Two save tables are new (`CBaloon`, `CBarneyzom`), five went with the alien
files (`CAGrunt`, `CBullsquid`, `CSquidSpit`, `CHAssassin`, `CISlave`), and
the other 136 are Spirit 1.2's. SiC 1's tree with SiC 2's project changes
(`fde2b87`) gave 3,552 of SiC 2's 3,744 server game functions (92.1% of
their bytes) and 1,486 of the client's 1,550 (96.5%).

## One tree for both games

**The switch.** SiC 2's code is the default, and SiC 1's sits under
`#ifdef SIC1`. `SIC1` is our editorial switch: neither game's source had
it. Every file in which `ec3024d` and `21fc2ea` differ holds both texts, as
`#ifdef SIC1` (SiC 1's text) `#else` (SiC 2's) `#endif`, as `#ifndef SIC1`
around what only SiC 2 has, or as `#ifdef SIC1` around what only SiC 1 has:
217 such lines in 45 files. Blocks one unchanged line apart are one block.
Where a stretch of code changed throughout, each game has it whole: the end
of `CGrenade::Explode` and `CGrenade::Smoke` (`ggrenade.cpp`), the impulse
101 give list (`player.cpp`), and `CWreckage`'s smoke with SiC 2's new
`CBaloon` after it (`h_cycler.cpp`). SiC 2's own conditional, the shotgun's
`#ifdef CLIENT_DLL`, stays inside the `#else`. No directive sits in a
`#define` continuation or a comment. Line ends (CRLF, LF, or mixed, as in
Spirit's files) and the Czech strings' encoding are each file's own. The
four commits: `8f70aae` (the projects and the alien files), `b3cd282`
(weapons and skill data), `ff2b538` (monsters), `8d34187` (the rest).

**The projects.** `dlls/hl.dsp` has a fourth configuration,
[`hl - Win32 Release SIC1`](../../../blob/sic/dlls/hl.dsp#L143-L178):
`ec3024d`'s Release settings plus `/D "SIC1"`, linked as
`.\Releasehl/spirit.dll` (so the PDB is `Releasehl\spirit.pdb`). It leaves
out SiC 2's four new files (`# PROP Exclude_From_Build 1`), so its Rich
header still counts SiC 1's 104 C++ objects, and it has one per-file
setting of ours: `agrunt.cpp` without a precompiled header
([`# SUBTRACT CPP /YX`](../../../blob/sic/dlls/hl.dsp#L204-L206), below).
`cl_dll/cl_dll.dsp` has
[`cl_dll - Win32 Release SIC1`](../../../blob/sic/cl_dll/cl_dll.dsp#L103-L137):
its Release settings plus `/D "SIC1"`. SiC 2's configurations are
`21fc2ea`'s. The four new files are unconditional: SiC 1's configuration
doesn't build them, and an `#ifndef SIC1` before their includes would change
SiC 2's precompiled-header states. The `.dsp` format has no comment the IDE
keeps, so the reason for the per-file setting is written in `agrunt.cpp` and
in `8f70aae`'s message.

**The preprocessor check.** VC6's `CL /EP`, run with each configuration's
flags and defines over every source the configuration builds, gives the
historical tree's output, blank lines aside: with SiC 1's configurations
`ec3024d`'s (104 server and 86 client sources), with SiC 2's `21fc2ea`'s
(108 and 86). The one exception is `agrunt.cpp`'s `#pragma hdrstop`, one
line more in both games. No `__LINE__` or `__FILE__` reaches Release code
(only `util.h`'s `ASSERT`, under `DEBUG`), so the line numbers, which move
with every block, reach no code.

**`agrunt.cpp`.** The four alien files hold SiC 1's text inside
`#ifdef SIC1`; in SiC 2 they compile to nothing.
`bullsquid.cpp`, `hassassin.cpp` and `islave.cpp` need nothing else.
[`agrunt.cpp`](../../../blob/sic/dlls/agrunt.cpp#L1-L18) needs two things
of ours, both for VC6's automatic precompiled header (`/YX`;
[reproducing.md](reproducing.md#precompiled-header-state-full-builds-and-build-sessions)
explains it):

- A `/YX` header region holds every directive before the first line of code.
  A file that starts with `#ifdef SIC1` leaves a `.pch` that holds the
  conditional, and the next source doesn't reuse it: it builds its own.
  SiC 2's build needs what its empty `agrunt.cpp`, the project's second
  source, left: an empty `.pch` that every later source up to `player.cpp`
  reuses (see [SiC 2's empty precompiled header](#sic-2s-empty-precompiled-header)).
  A `#pragma hdrstop` before the `#ifdef` ends the region there and leaves
  that empty `.pch`.
- SiC 1's last build session compiled `agrunt.obj` with a `.pch` cut back to
  `extdll.h util.h cbase.h monsters.h`. With the conditional first, `/YX`
  precompiles all ten of the file's headers; with the pragma, none, and the
  empty `.pch` then passes to every later source of SiC 1's full build too.
  Compiled without `/YX`, the object is retail's, and the `.pch` the
  previous source left goes on to the next. So SiC 1's configuration
  compiles this file without `/YX`.

Measured with release builds of variants of the tree (bytes that differ in
the whole file; a configuration reads only its own settings, so an empty
cell was not built):

| `agrunt.cpp` | SiC 1's setting for it | SiC 2's setting for it | `spirit.dll` | `bloodbath.dll` |
|---|---|---|---|---|
| whole file in `#ifdef SIC1` | none | none | 67,018 (`agrunt.obj`) | 24,008 (`alias.obj`, `func_break.obj`, `nodes.obj`) |
| whole file in `#ifdef SIC1` | `# SUBTRACT CPP /YX` | | 0 | |
| whole file in `#ifdef SIC1` | `/YX"monsters.h"` | | 0 | |
| whole file in `#ifdef SIC1` | | `# SUBTRACT CPP /YX` | | 24,008 |
| whole file in `#ifdef SIC1` | | `/Yc` instead of `/YX` | | 24,008 |
| whole file in `#ifdef SIC1` | | `/FI"extdll.h"` | | 20,837 (`func_break.obj`) |
| `#pragma hdrstop`, then the same | none | | 185,775 | |
| `#pragma hdrstop`, then the same | `# SUBTRACT CPP /YX` | none | **0** | **0** |

With the whole file in `#ifdef SIC1`, no setting of SiC 2's gives its
image. Compile sequences (`aflock.cpp`, `agrunt.cpp`, ..., `alias.cpp`,
with `/Fp` on every compile as the IDE passes it) show why: the `.pch` an
empty file leaves (186,904 bytes) is reused by every later source, while
`#ifdef SIC1 … #endif` (186,928 bytes), a `#define` (186,936), and the
whole-file `#ifdef SIC1` with `/Yc` (252,468), `/YX"name"`, `/U SIC1`,
`/u`, no `/YX` or another `/Fp` all leave a `.pch` the next source rebuilds.
`/Yc"extdll.h"` and `/Yu` don't compile (C2857, C1010). The pragma is the
smallest change that ends the region in front of the conditional.
`bullsquid.cpp`, `hassassin.cpp` and `islave.cpp`, wrapped whole, change
nothing in either game.

**The historical trees.** `ec3024d` and `21fc2ea` are on the branch, each
with only its `... Release` configurations and none of the `SIC1` text.
The release script builds them too: in a tree without the `... Release
SIC1` configurations it builds SiC 1 from the `... Release` ones. A
checkout of `ec3024d` gives SiC 1's two retail files, one of `21fc2ea`
SiC 2's ([rebuild/survive-in-catacombs.md](rebuild/survive-in-catacombs.md),
step 5).

## The retail files

The retail DLLs are not in this repository. They come from the mods'
release archives, as mirrored in the Internet Archive item
[`half-lifeseriesmods_20191215_1013`](https://archive.org/details/half-lifeseriesmods_20191215_1013)
("Hλlf-Life Mods"). The item's MD5 and SHA-1 for these two files match ours.

| File in the item | Size (bytes) | SHA-256 |
|---|---|---|
| `Survive in Catacombs/Survive in Catacombs.rar` | 97,540,639 | `995d3e9dabadaf0031031653fe2972f8dd4e7f6db970745c2da3075510f03c97` |
| `Survive in Catacombs 2.rar` | 131,998,672 | `f045e41022610f77c5f1d56bee6d1f5d92715e8c9657fe99388af36029fb222c` |

Both are RAR archives, each holding the mod folder (34 files for SiC 1, 38
for SiC 2). libarchive's `bsdtar` unpacks them; some builds of 7-Zip list
them but can't unpack them. The DLLs:

| Entry | Size (bytes) | SHA-256 |
|---|---|---|
| `Catacombs/Dlls/sic.dll` | 1,212,476 | `eab217809f7c656acb7678b2035a4dc7f82a08db276febaceef72dfa55509359` |
| `Catacombs/Cl_dlls/client.dll` | 540,672 | `0cb9fd2beed7c6ed327545a280826ecaaa5269ae3539c9a0602522cca3e6527a` |
| `Bloodbath/Dlls/bloodbath.dll` | 1,208,393 | `673f7e4ef7c75fdf551c87470f6e4f8a5a66f16fe98bfb44ca6436d14c88ba57` |
| `Bloodbath/Cl_dlls/client.dll` | 540,672 | `76cc187b538b7fa0ae6bea5842e1484369fb64c6d7184851fb77de96cedbe21a` |

`Catacombs/liblist.gam` says `gamedll "dlls\sic.dll"` and `cldll "1"`;
`Bloodbath/liblist.gam` says `gamedll "dlls/bloodbath.dll"`. The unpacked
`Catacombs/` and `Bloodbath/` folders are what the release script's `RETAIL`
variable expects (see below).

## Rebuilding

[rebuild/survive-in-catacombs.md](rebuild/survive-in-catacombs.md) has the
steps as exact commands. This section explains what they do.

### What you need

- The toolchain folder from [setup.md](setup.md). Both games use `vc6/`
  (Visual C++ 6.0 Professional, no service pack) and `wineprefix/`.
  [compilers.md](compilers.md) says where the compiler comes from.
- Wine (we used 11.0), libfaketime's `faketime`, Python 3 (standard library
  only), `sha256sum` and `cmp`; `bsdtar` to unpack the retail archives.
- A checkout of the `sic` branch.

The branch has no `.gitattributes`, and its line ends are each file's own,
as Spirit of Half-Life's releases had them. Leave `core.autocrlf` off (the
default on Linux), so git checks the files out as stored.

### Commands

From a checkout of `main`:

```sh
git worktree add ../sic sic    # or: git clone -b sic <repo> sic
VC_TOOLCHAINS=/path/to/toolchains tools/release/sic.sh 1 ../sic ../catacombs-build
VC_TOOLCHAINS=/path/to/toolchains tools/release/sic.sh 2 ../sic ../bloodbath-build
```

- The first argument is the game, `1` or `2`; then the checkout and the
  build folder. The script creates the build folder if needed. It holds the
  build's copy of the Wine prefix and the source tree laid out at retail's
  path.
- A fourth argument picks `server`, `client` or `both` (the default).
- To compare with your own retail copy as well, set `RETAIL` to the unpacked
  mod folder, `Catacombs` for game 1 or `Bloodbath` for game 2:

  ```sh
  RETAIL=/path/to/Catacombs VC_TOOLCHAINS=/path/to/toolchains \
      tools/release/sic.sh 1 ../sic ../catacombs-build
  ```

For each DLL the script prints the path of the built file, its SHA-256,
retail's SHA-256 and `identical to retail` or `differs from retail`. With
`RETAIL` set it also runs `cmp` against `Dlls/sic.dll` or
`Dlls/bloodbath.dll` and `Cl_dlls/client.dll` there. It exits with 0 only if
every DLL it built is identical. The built files are:

| | Server | Client |
|---|---|---|
| Game 1 | `OUT/root/SicEngineCodes/DLLS/Releasehl/spirit.dll` | `OUT/root/SicEngineCodes/cl_dll/Release/client.dll` |
| Game 2 | `OUT/wineprefix/drive_c/bloody/bloodyEnginecodes/DLLS/Releasehl/bloodbath.dll` | `OUT/wineprefix/drive_c/bloody/bloodyEnginecodes/cl_dll/Release/client.dll` |

The output for game 1, with `RETAIL` set (paths shortened):

```
server: full build, "hl - Win32 Release SIC1", with browse information
  compiled 104 of 104 sources (0 failed); link rc=0; output …/root/SicEngineCodes/DLLS/Releasehl/spirit.dll
server: session: weapons.h edited, 51 sources recompiled
server: link 1 of 6 at 2004-05-16 13:01:57 UTC (creates spirit.pdb), links 2 to 6 at 2004-05-19 21:32:38 UTC, LINK on the VC6 CD's MSVCRT.DLL
server: …/root/SicEngineCodes/DLLS/Releasehl/spirit.dll
  SHA-256 built:  eab217809f7c656acb7678b2035a4dc7f82a08db276febaceef72dfa55509359
  SHA-256 retail: eab217809f7c656acb7678b2035a4dc7f82a08db276febaceef72dfa55509359
  identical to retail
  cmp …/Catacombs/Dlls/sic.dll: no difference
client: full build, "cl_dll - Win32 Release SIC1", with browse information, LINK at 2004-05-19 19:04:12 UTC on the VC6 CD's MSVCRT.DLL
  compiled 86 of 86 sources (0 failed); link rc=0; output …/root/SicEngineCodes/cl_dll/Release/client.dll
client: …/root/SicEngineCodes/cl_dll/Release/client.dll
  SHA-256 built:  0cb9fd2beed7c6ed327545a280826ecaaa5269ae3539c9a0602522cca3e6527a
  SHA-256 retail: 0cb9fd2beed7c6ed327545a280826ecaaa5269ae3539c9a0602522cca3e6527a
  identical to retail
  cmp …/Catacombs/Cl_dlls/client.dll: no difference
```

Game 2 prints the same lines with its own values: 108 sources, 49
recompiled, 8 links, LINK on Wine's builtin msvcrt. Each game took one and
a half to two and a half minutes here.

### What the script does for the servers

The servers' debug records name their PDBs (program databases, the files
LINK writes debug information to):
`F:\SicEngineCodes\DLLS\Releasehl\spirit.pdb` (SiC 1) and
`C:\bloody\bloodyEnginecodes\DLLS\Releasehl\bloodbath.pdb` (SiC 2).
`DLLS\Releasehl` is the output folder of `hl.dsp`'s Release configuration,
so the trees were at `F:\SicEngineCodes` and `C:\bloody\bloodyEnginecodes`.
The script rebuilds that setting and the end of each build history:

1. **The tree.** It copies the toolchain's Wine prefix into
   `OUT/wineprefix` and lays out the checkout at retail's path, as symlinks,
   so the build writes nothing into the checkout. SiC 1: `F:\SicEngineCodes`,
   drive `F:` being `OUT/root`. SiC 2: `C:\bloody\bloodyEnginecodes`, inside
   the copy's `drive_c`. The server folder is `DLLS` in upper case, as the
   PDB path has it.
2. **A full build.** It builds the game's configuration of `dlls\hl.dsp`
   into `DLLS\Releasehl`, with the projects' browse information kept
   (`build_dsp.py --browse-info`): SiC 1's `hl - Win32 Release SIC1`, all
   104 sources (101 C++, 3 C), linked as `spirit.dll`; SiC 2's
   `hl - Win32 Release`, all 108 sources (105 C++, 3 C), linked as
   `bloodbath.dll`. The sources compile in project order, and CL creates and
   reuses one `vc60.pch`, as in the IDE.
3. **The last build session.** It gives `weapons.h` a new time stamp and
   compiles the sources that include it once more, in project order, each
   with its own per-file settings, starting from the `.pch` the full build
   left: 51 sources for SiC 1, 49 for SiC 2. It reads the sources with the
   `SIC1` conditionals resolved for the configuration, so the alien files
   count only for SiC 1. This gives `barney.obj`, `h_cycler.obj` and
   `handgrenade.obj` retail's function order, and in SiC 2 every `weapons.h`
   dependent that the full build gave the empty `.pch` (see
   [Build stand-ins](#build-stand-ins)).
4. **The PDB's history.** It deletes the PDB and links again, 6 times for
   SiC 1 and 8 for SiC 2. The first link runs with the clock at the PDB's
   creation time (its signature), the others at retail's link time:

   | | PDB created (link 1) | Links 2 to the last | Age |
   |---|---|---|---|
   | SiC 1 | 2004-05-16 13:01:57 UTC | 2004-05-19 21:32:38 UTC | 6 |
   | SiC 2 | 2004-12-17 11:03:37 UTC | 2004-12-19 18:27:24 UTC | 8 |

The clock is libfaketime's, with `TZ=UTC`, and only LINK runs under it.
LINK runs on the VC6 CD's `MSVCRT.DLL` for SiC 1 and on Wine's builtin
`msvcrt` for SiC 2 (see [LINK's C runtime](#links-c-runtime)).

### What the script does for the clients

The clients were linked without `/debug`. They have no debug directory, so
no path reaches the image. The script still builds each in its game's tree
(`F:\SicEngineCodes\cl_dll`, `C:\bloody\bloodyEnginecodes\cl_dll`):

- SiC 1: `cl_dll - Win32 Release SIC1`, one full build of 86 sources (83
  C++, 3 C) in project order with the projects' browse information, one
  link at 2004-05-19 19:04:12 UTC, LINK on the VC6 CD's `MSVCRT.DLL`.
- SiC 2: `cl_dll - Win32 Release`, the same 86 sources, one link at
  2004-12-17 10:31:35 UTC. The whole build, compiles and link, runs on
  Wine's builtin `msvcrt`. The compiles don't depend on it: objects compiled
  on the CD's runtime and linked on Wine's give the same file.

No session is replayed for the clients.

### Project flags

From the two `.dsp` files:

| | Server (`hl.dsp`) | Client (`cl_dll.dsp`) |
|---|---|---|
| Compile | `/G5 /MT /W3 /Zi /O2 /Fr /YX`, defines `NDEBUG WIN32 _WINDOWS QUIVER VOXEL QUAKE2 VALVE_DLL CLIENT_WEAPONS`, and `SIC1` in SiC 1's configuration | `/MT /W3 /GX /Zi /O2 /FR /YX`, defines `WIN32 NDEBUG _WINDOWS CLIENT_DLL CLIENT_WEAPONS`, and `SIC1` in SiC 1's configuration |
| Per file | SiC 1's configuration: `agrunt.cpp` without `/YX`, SiC 2's four new files excluded | `overview.cpp` excluded, as in Spirit 1.2's project |
| Link | `/subsystem:windows /dll /incremental:no /map /debug /machine:I386 /def:".\hl.def"`, `/out:".\Releasehl/spirit.dll"` (SiC 1) or `bloodbath.dll` (SiC 2) | `/subsystem:windows /dll /map /machine:I386`, with `winmm.lib`, `wsock32.lib` and the branch's `utils/vgui/lib/win32_vc6/vgui.lib` |
| Sources | SiC 1: 101 `.cpp`, 3 `.c`; SiC 2: 105 `.cpp`, 3 `.c` | 83 `.cpp`, 3 `.c` |

`build_dsp.py` leaves out the IDE-only `/FD`. With `--browse-info` it keeps
`/Fr` and `/FR`, as Scientist Slaughterhouse's client build does; the
other mods' builds leave them out.

### If your build differs

| Symptom | Cause |
|---|---|
| Only the time stamps differ (COFF header and export directory; in the servers the debug directory too) | LINK didn't run at retail's time |
| Server: the debug record's signature and age differ | the 6 or 8 links weren't replayed into a fresh PDB |
| Server: the PDB path and the size of the debug record differ | the tree wasn't at `F:\SicEngineCodes` or `C:\bloody\bloodyEnginecodes` |
| SiC 1 server: 8,646 bytes differ, starting at four inline functions at the ends of `barney.obj`, `h_cycler.obj` and `handgrenade.obj` | only a full build, without the last session |
| SiC 1 server: 857 bytes differ: `client.obj` from `CBasePlayer::IsAlive` on, `FireBulletsPlayer`'s two random-seed calls, 4 bytes of `.data` | built without browse information (`/Fr`) |
| SiC 1 server: 185,775 bytes differ | `agrunt.cpp` compiled with `/YX` in SiC 1's configuration |
| SiC 1 client: 1,812 bytes differ: `CClassMenuPanel`'s constructor and four functions of `hl_weapons.obj` | built without browse information (`/FR`) |
| SiC 1 client: 299,634 bytes differ | LINK ran on Wine's builtin `msvcrt` |
| SiC 2 server: 15 bytes differ: `CreateDirectoryA` and `LCMapStringA` trade places among KERNEL32's thunks | LINK ran on the CD's `MSVCRT.DLL` |
| SiC 2 server: 148,066 bytes differ | only a full build, without the last session |
| SiC 2 server: 24,008 bytes differ, in `alias.obj`, `func_break.obj`, `nodes.obj` and the calls into them | `agrunt.cpp` without its `#pragma hdrstop` |
| SiC 2 client: 298,770 bytes differ (every DLL's thunk order, export addresses, the copies of identical functions LINK keeps) | LINK ran on the CD's `MSVCRT.DLL` |

## What the image holds besides code

A DLL also records how it was built. [reproducing.md](reproducing.md)
explains each part in general. For these mods:

| Item | Servers | Clients | How the build gets it |
|---|---|---|---|
| Browse information (`/Fr`, `/FR`: the compiler writes `.sbr` files for the IDE's source browser) | It adds to the compiler's intermediate code and so changes code. Without it SiC 1's server differs in 857 bytes (with the last session) | SiC 1's client differs in 1,812 bytes without it | The projects' own flags, kept |
| PDB record: a debug directory entry in CodeView "NB10" format that names the PDB file | SiC 1: 60 bytes at file offset 0x128000, after the last section: `F:\SicEngineCodes\DLLS\Releasehl\spirit.pdb`, signature 0x40a76645 (2004-05-16 13:01:57 UTC), age 6. SiC 2: 73 bytes at 0x127000: `C:\bloody\bloodyEnginecodes\DLLS\Releasehl\bloodbath.pdb`, signature 0x41c2bd09 (2004-12-17 11:03:37 UTC), age 8 | none (no `/debug`) | The tree at that path in the prefix copy; 6 or 8 links into a fresh PDB, the first at its creation time |
| Time stamps | COFF header, export directory and debug directory: SiC 1 0x40abd276, SiC 2 0x41c5c80c | COFF header and export directory: SiC 1 0x40abafac, SiC 2 0x41c2b587 | LINK runs with the clock frozen at retail's link time |
| Export name | `spirit.dll` in both (`hl.def`: `LIBRARY spirit`), although SiC 1 shipped as `sic.dll` and SiC 2 was linked as `bloodbath.dll` | `client.dll` | `hl.def` as Spirit wrote it; the output name per configuration |
| Rich header: a block LINK writes before the PE header that counts the objects each tool made ([compilers.md](compilers.md#survive-in-catacombs)) | SiC 1: C 8168 ×127, C++ 8168 ×104, MASM 7299 ×32, AliasObj 7291 ×2, Linker 5.12 8034 ×3, Import0 ×75, Linker 6.00 8168 ×1. SiC 2: the same with C++ ×108 | Both: C 8168 ×135, C++ 8168 ×94, MASM ×33, AliasObj ×3, Linker 5.12 ×9, Import0 ×645, Linker 6.00 8447 ×2 (`vgui.lib`'s import objects), Linker 6.00 8168 ×1 | The CD's compiler and libraries, and the file lists: Spirit 1.2's `hl.dsp` (101 sources; SiC 2 adds its four new files), Spirit 1.2's `cl_dll.dsp` with `mp3.cpp`. No placeholder object is needed |
| PE header position (`e_lfanew`) | SiC 1 0xe8, SiC 2 0xd8 | 0xe0 | Follows from the number of objects |
| Imports | KERNEL32, 75 functions. Their order inside the list differs between the two games (see below) | KERNEL32 68, USER32 3, WINMM 3, `vgui.dll` 550, WSOCK32 1, in that order (625) | The `.dsp` library lists, and LINK on the game's C runtime |
| Exports | SiC 1 589: `GetEntityAPI`, `GetEntityAPI2`, `GiveFnptrsToDll`, 305 entity factories, 281 decorated member names. SiC 2 590 (306 factories) | 542: 72 undecorated (the client interface, weapon and ammo factories), 470 decorated | `hl.def` and the `EXPORT` macro; the client's `dllexport` declarations |
| Dead code and identical-code folding (ICF: LINK keeps one copy of functions with identical bytes and relocations) | none: `/debug` turns `/OPT:REF` and `/OPT:ICF` off, so every function compiled stays in the image | on; which copy LINK keeps depends on its C runtime's `qsort` | LINK on the game's runtime; no dead-function or section-order stand-in was needed |
| Function order inside an object | header inline functions placed by `/YX`: the last session's (`barney.obj`, `h_cycler.obj`, `handgrenade.obj`), and in SiC 2 the empty `.pch` after `agrunt.cpp` (`alias.obj`, `func_break.obj`, `nodes.obj`) | one full build's | The full build and the replayed session |
| `.reloc` virtual size | SiC 1 0x19d8e, SiC 2 0x1a0f8 | SiC 1 0xc300, SiC 2 0xc310 | Follows from the code |
| `.bss`: uninitialised globals, which take no file space but are addressed by the code | ordered by a hash of their names ([reproducing.md](reproducing.md#bss-order)) | the same | Names that give retail's order ([below](#names-chosen-for-the-bss-layout)) |
| Link order | `hl.dsp` order: every file in name order, `aflock` to `zombie`, SiC 2's four new files in their sorted places | Spirit 1.2's `cl_dll.dsp` order, with `mp3.cpp` between `interface.cpp` and `mp5.cpp` | `build_dsp.py` links in project order |
| C runtime | `/MT`: the CD's static `LIBCMT.LIB` | the same, and the client compiles with `/GX` | The CD's `VC98/LIB` |

### Browse information

Spirit 1.2's projects compile with browse information, and Laurie Cheers'
build logs show the IDE passing it. The flag adds to the compiler's
intermediate code, and with it some code comes out differently. It settles
three things the source alone could not:

- SiC 1's client: `CClassMenuPanel`'s constructor (3,400 bytes) has
  retail's register and reload order only with `/FR`, and so do the header
  inline functions in `hl_weapons.obj`. With them every copy of identical
  functions that LINK keeps is retail's.
- SiC 1's server: `CBasePlayer::IsAlive` in `client.obj` lands after
  `RegisterEncoders`, as in retail.
- SiC 1's server: in `CBaseEntity::FireBulletsPlayer`, retail calls
  `UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) )` before
  `shared_rand + iShot`. Spirit's unchanged text compiles to that order with
  `/Fr` and to the other without it.

### The last build session

Neither server's last build was a full one. In SiC 1, `barney.obj`
(`CTalkMonster::FriendNumber`), `h_cycler.obj` (an inline `UseDecrement`)
and `handgrenade.obj` (`CHandGrenade::iItemSlot`, `UseDecrement`) have their
header inline functions where VC6 puts them when the file is compiled with
its whole include list in the `.pch`. A full build doesn't give that. A session that
recompiles `weapons.h`'s 51 dependents in project order, the first
(`agrunt.cpp`) finding the `.pch` that a build ending with `zombie.cpp`
leaves, puts those three objects right and leaves the other 48 as retail
has them. SiC 2's server shows the same three placements, and the same
session with its 49 dependents gives them.
[vc6-notes.md](vc6-notes.md#where-header-inline-functions-land) has the rule
for where header inline functions land.

### SiC 2's empty precompiled header

Retail SiC 2's `alias.obj`, `func_break.obj` and `nodes.obj` have header
inline functions (`CPointEntity::ObjectCaps`, `CBaseAlias::IsAlias`,
`CMultiAlias::FlushChanges`, `CBreakable::Classify` and `ObjectCaps`,
`CNodeEnt::ObjectCaps`) where no `.pch` left by the other sources puts
them. They fit only when the header that declares them is outside the
`.pch` the file compiles against. A source with no `#include` leaves an
empty `.pch`, which every later file reuses and leaves as it is. `agrunt.cpp`,
the project's second source, was emptied that way, so in a full build every
file from `AI_BaseNPC_Schedule.cpp` to `player.cpp` gets the empty `.pch`,
until `pm_debug.c`, a C file, starts a new one (`2d74976`). The empty `.pch`
also moves the `weapons.h` dependents in that range; the last session puts
them back (148,066 bytes differ without it).

### LINK's C runtime

LINK sorts the import thunks with the C runtime's `qsort`, and equal keys
come out in whatever order that `qsort` leaves them; the same sort chooses
which copy of identical functions LINK keeps
([reproducing.md](reproducing.md#import-table-order)). The two games were
linked on different runtimes:

- **SiC 1** gives retail's files with LINK on the VC6 CD's `MSVCRT.DLL`
  (1998), as most mods here do. On Wine's builtin `msvcrt`, 299,634 bytes of
  its client differ.
- **SiC 2** needs a `qsort` that leaves equal keys where they are. The
  server's 75 KERNEL32 thunks all have equal keys: LINK finds
  `CreateDirectoryA` first, for `nodes.obj`, and the C runtime's 74 after.
  Retail keeps that order. The CD's `qsort` swaps the first element with the
  middle one (slot 37, `LCMapStringA`) and stops, which is SiC 1's order and
  15 bytes off for SiC 2. The client on the CD's runtime differs in 298,770
  bytes: the thunk order of all five DLLs, 492 export addresses, and 22
  functions whose kept copy is another one (15 in `hl_baseentity.obj`, 3 in
  `voice_vgui_tweakdlg.obj`, 2 in `cdll_int.obj`, one each in `egon.obj` and
  `vgui_TeamFortressViewport.obj`). On Wine 11.0's builtin `msvcrt` both
  files are retail's.

Which runtime SiC 2's linker had is not known. The tree had moved from `F:`
to `C:` as well, which fits a new machine; Windows XP's own `msvcrt.dll`
would be one candidate, not checked.

## What in the source is not known from retail

The image gives every function body that the compiler emitted. It gives
names only through exports, save tables and strings, and nothing at all for
text that compiled to no code. So the source text is one spelling that gives
retail's bytes, not necessarily the mods' own. This section lists:

- source that exists to steer the compiler, or only for the one tree;
- names chosen for the `.bss` layout;
- file names and file contents the image doesn't show;
- other choices, and names we made up;
- the build history the script stands in for.

Commit messages give the full reasoning.

### Source that steers the compiler

These lines are needed for retail's bytes, or for both games from one tree,
but their text is ours.

| Where | What | Why | Evidence and status |
|---|---|---|---|
| [`dlls/agrunt.cpp:17`](../../../blob/sic/dlls/agrunt.cpp#L17) | `#pragma hdrstop` before `#ifdef SIC1` (`8f70aae`) | Gives SiC 2's build the empty `.pch` its empty `agrunt.cpp` left | Ours, only for the one tree; `21fc2ea`'s file is empty. Measured: [`agrunt.cpp`](#one-tree-for-both-games) |
| [`dlls/hl.dsp:204-206`](../../../blob/sic/dlls/hl.dsp#L204-L206) | `# SUBTRACT CPP /YX` for `agrunt.cpp` in `hl - Win32 Release SIC1` (`8f70aae`) | SiC 1's last session compiled the file with four headers precompiled. With the pragma `/YX` precompiles none, without it all ten; compiled without `/YX` the object is retail's | Ours, only for the one tree; `/YX"monsters.h"` works as well. `ec3024d`'s project has no such setting |
| [`dlls/util.h:290-292`](../../../blob/sic/dlls/util.h#L290-L292) | SiC 2's `UTIL_ExplosionSprite` declared in `util.h`, after `UTIL_Sparks` (`41f0116`) | With this declaration `CTestHull::BuildNodeGraph` in `nodes.cpp` has retail's register order (`1825f49`) | The function's place in `util.cpp`, after `UTIL_Sparks`, is retail's; its name and the place of its declaration are ours. Probably the effect [vc6-notes.md](vc6-notes.md#reloads-at-a-loop-end-symbol-numbers) describes for `nodes.cpp`'s long functions: their reloads follow the number of symbols declared before them (Scientist Slaughterhouse's `BuildNodeGraph` needed two of its mod's declarations in `util.h` the same way) |
| `SIC1` throughout | 217 `#ifdef SIC1`/`#ifndef SIC1` lines in 45 files, and the `... Release SIC1` configurations | One tree for both games | Ours. The preprocessor check above shows that each configuration sees exactly its historical tree's text |

### Names chosen for the `.bss` layout

VC6 lays out a file's uninitialised globals in the walk order of a
1024-bucket hash of their names
([reproducing.md](reproducing.md#bss-order)). Names reach the image this way
even though no symbol does, so these names were chosen to give retail's
layout:

| Where | What | Evidence |
|---|---|---|
| [`dlls/weapons.cpp:49`](../../../blob/sic/dlls/weapons.cpp#L49), [`weapons.h:412`](../../../blob/sic/dlls/weapons.h#L412) | `g_sModelIndexShock`, SiC 1's index of `sprites/white.spr` for the explosions' shock ring (`b96add0`) | It lies after `gMultiDamage` in `weapons.obj`'s `.bss`, where retail has it; the name was chosen to hash there |
| [`dlls/weapons.cpp:51-61`](../../../blob/sic/dlls/weapons.cpp#L51-L61), [`weapons.h:414-424`](../../../blob/sic/dlls/weapons.h#L414-L424) | SiC 2's ten sprite indexes: `g_sModelIndexWaterSplash`, `Expansion`, `Fog`, `Balloon`, `GreySmoke`, `Glare`, `Zerog`, `FireExplo`, `SmallPuff`, `BloodCloud` (`afecfc0`) | The binary has only their places among `weapons.cpp`'s uninitialised globals. These names give retail's places; `g_sModelIndexExpansion` and `g_sModelIndexBloodCloud` fitted as first named |

### Files

| Where | What | Evidence and status |
|---|---|---|
| `dlls/barneyzom.cpp`, `cremator.cpp`, `scutspider.cpp`, `z_ghoul.cpp` | SiC 2's four new files (`fde2b87`) | Their places in the project's alphabetical order are retail's, from where their code and data lie; the names are ours, chosen to sort there (the IDE sorts by lower-case name: `z_ghoul` sorts between `xen` and `zombie`, `ghoul` would sort after `ggrenade`). The data can't tell `cremator.cpp` and `scutspider.cpp` from code at the head of `crossbow.cpp` and `shotgun.cpp`, which have no initialised globals; those weapon files are compiled into the client too, and monster code there is implausible |
| `dlls/agrunt.cpp`, `bullsquid.cpp`, `hassassin.cpp`, `islave.cpp` in SiC 2 | Empty (`2d74976`) | The image shows that `agrunt.cpp` has no `#include` ([above](#sic-2s-empty-precompiled-header)); in the `#if 0` form of `ca80744`, or with its `#include` lines kept, it leaves another `.pch`. The other three don't reach the image and are emptied the same way. Whether comments stayed in them, the image can't show |
| `dlls/effects.cpp` | `explo_debris` and `explode_ball` (`CExploDebris`, `CExplodeBall`) at the end of the file (`c4f96c9`) | They have no initialised globals, so only the file count places them: with four new objects, none is left for them. The start of `egon.cpp` fits the same evidence |
| `cl_dll/mp3.cpp`, `mp3.h` | SiC 1's music player (`0958d87`) | The file name and its place in `cl_dll.dsp` come from Spirit 1.4's project, which has the same tutorial code at the same sort position; nothing in SiC 1's binary names its files. `fmod.h` and `fmod_errors.h` are the FMOD 3.71 headers of 1.4's copy; SiC 1 ships `fmod.dll` 3.6 |

### Other choices and guesses

| Where | What | Why, and what is open |
|---|---|---|
| [`dlls/shotgun.cpp:148-157`](../../../blob/sic/dlls/shotgun.cpp#L148-L157), [`236-245`](../../../blob/sic/dlls/shotgun.cpp#L236-L245) | SiC 2's gun flash as `#ifdef CLIENT_DLL`: `NORMAL_GUN_FLASH` in the client, `BRIGHT_GUN_FLASH` in the server (`3bba16c`) | The file is compiled into both DLLs, and this is the only difference between the two compilations. One commit has to build both DLLs, so the branch needs the conditional. Whether the author wrote one, or built the client (linked 2004-12-17) before changing the text for the server (2004-12-19), the image can't tell |
| [`dlls/hl.dsp`](../../../blob/sic/dlls/hl.dsp), Release link line | `/incremental:no` (`6a28b30`) | The IDE adds it for a Release configuration; Laurie Cheers' `dlls/hl.plg` shows it. Written into the project so that a command-line build links as the IDE did |
| [`dlls/skill.h`](../../../blob/sic/dlls/skill.h#L33) | The new `skilldata_t` fields' names (`13a03ed`) | The field order is retail's, from the order `RefreshSkillData` stores in; the names are ours, after the skill cvars |
| Throughout | Names of everything the binaries don't name | See [Names](#names) |

### Names

The servers name a class only where a decorated export or a save table
carries the name. So SiC 2's `CBarneyzom` and `CBaloon` are retail's (save
tables), and so is `CScutSpider` (its exported `LeapTouch`). These names are
ours, and their comments say so:

- classes: `CCremator`, `CGhoul`, `CBabySpider`, `CExploDebris`,
  `CExplodeBall` (no save table, no exported member);
- `skilldata_t` fields: `barneyzomHealth`, `crematorHealth`,
  `crematorDmgOneSlash`, `crematorDmgBothSlash`, `ghoulHealth`,
  `ghoulDmgOneSlash`, `ghoulDmgBothSlash`, `plrDmgFork`, `plrDmgMP18`,
  `plrDmgFlareClient`, `plrDmgFlareMonster`, `plrDmgTNT`;
- the function `UTIL_ExplosionSprite`;
- the eleven sprite indexes above;
- the four new files;
- the schedule arrays of `barneyzom.cpp` and `scutspider.cpp` (their
  schedule names are retail's strings), and the local `pBaloon` in
  `ggrenade.cpp`.

Everything else the binaries don't name (locals, constants, the mods' new
members) has names in Spirit's style.
`git grep -n -i -E 'names? (are )?ours' sic` lists the 30 lines that mark
them.

### `// sic` lines that keep retail's behaviour

These lines keep the mods' own oddities. Retail's code or data proves each
of them, and they steer nothing.

| Where | Game | Retail behaviour |
|---|---|---|
| [`cl_dll/ev_hldm.cpp:591`](../../../blob/sic/cl_dll/ev_hldm.cpp#L591) | SiC 2 | The shotgun's double shot sets trace hull 5 for its pellets; there is no hull 5 (`529446c`) |
| [`dlls/barneyzom.cpp:488`](../../../blob/sic/dlls/barneyzom.cpp#L488) | SiC 2 | The barney zombie's `TalkInit` keeps `if`/`else` pairs whose two arms set the same sentence group (`e056784`) |
| [`dlls/crowbar.cpp:362`](../../../blob/sic/dlls/crowbar.cpp#L362) | SiC 2 | The fork's hit on the world doesn't keep its trace, so it leaves no mark (`26e65ca`) |
| [`dlls/effects.cpp:2059`](../../../blob/sic/dlls/effects.cpp#L2059) | SiC 2 | A yellow-blooded gib is spawned as `models/agibs2.mdl`, then again as `agibs.mdl`, so `agibs2.mdl` is never seen (`c4f96c9`) |
| [`dlls/explode.cpp:144`](../../../blob/sic/dlls/explode.cpp#L144) | SiC 2 | A spark shower gives what it touches 15 health (`TakeHealth` with `DMG_BURN`) (`6166dca`) |
| [`dlls/ggrenade.cpp:229`](../../../blob/sic/dlls/ggrenade.cpp#L229) | both | The scorch decal is always drawn: `RANDOM_FLOAT( 0, 1 ) < 1.0` (`b96add0`) |
| [`dlls/ggrenade.cpp:281`](../../../blob/sic/dlls/ggrenade.cpp#L281) | SiC 2 | The debris sound draws `RANDOM_LONG( 1, 2 )` for three cases, so `debris1.wav` never plays (`19cd2a7`) |
| [`dlls/ggrenade.cpp:288`](../../../blob/sic/dlls/ggrenade.cpp#L288) | SiC 2 | The explosion's effects-and-think block is written seven times; only the last next think counts (`19cd2a7`) |
| [`dlls/h_cycler.cpp:476`](../../../blob/sic/dlls/h_cycler.cpp#L476), [`562`](../../../blob/sic/dlls/h_cycler.cpp#L562) | SiC 2 | `cycler_wreckage` and `cycler_baloon` pick a random point in their bounds and don't use it (`93f4f6c`) |
| [`dlls/monstermaker.cpp:156`](../../../blob/sic/dlls/monstermaker.cpp#L156) | SiC 2 | Both arms set `m_fFadeChildren`, so a monster maker's dead monsters always fade (`16ca2c4`) |
| [`dlls/scientist.cpp:211`](../../../blob/sic/dlls/scientist.cpp#L211) | both | The heal schedule is a single task, so a scientist only walks back and never heals (`37f58ff`) |
| [`dlls/scientist.cpp:724`](../../../blob/sic/dlls/scientist.cpp#L724) | both | `TalkInit` writes a fourth friend one past the end of the three-entry `m_szFriends` (`f3cc938`) |
| [`dlls/sound.cpp:1898`](../../../blob/sic/dlls/sound.cpp#L1898) | SiC 2 | Tiles struck on the server sound like graves; the client keeps the tile sounds (`282b555`) |
| [`dlls/util.cpp:1543`](../../../blob/sic/dlls/util.cpp#L1543) | SiC 2 | `UTIL_BloodDrips`' cloud has brightness written as -200, sent as 56 (`41f0116`) |

That makes 15 lines that carry `sic` (`git grep -n -w sic sic -- '*.cpp'
'*.h' '*.c'`).

### Preprocessor directives

Compiled code keeps no preprocessor directives, so every conditional in the
branch needs a source of its own.

- `SIC1` is ours ([One tree for both games](#one-tree-for-both-games)); it
  is the only define added to either project.
- SiC 1's only new conditionals are in the files it took from Spirit 1.4's
  copy of the MP3 tutorial: `mp3.h`'s include guard and FMOD's own headers.
- SiC 2's only new conditional is the shotgun's `#ifdef CLIENT_DLL`, twice
  ([Other choices and guesses](#other-choices-and-guesses)). The `#if 0`
  blocks in `cremator.cpp`, `z_ghoul.cpp` and `scutspider.cpp` come with the
  text they copy (`zombie.cpp`, `headcrab.cpp`). One of Spirit's
  conditionals went with its file's code: `hassassin.cpp`'s
  `#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )`.
- The only `#pragma` added is `agrunt.cpp`'s `hdrstop`, ours.

### Build stand-ins

The builds reproduce history that the images record only in part.

| Where | What | Why | Evidence and status |
|---|---|---|---|
| Servers: the last build session | `weapons.h` gets a new time stamp, and its 51 (SiC 1) or 49 (SiC 2) dependents are compiled again in project order after the full build | `barney.obj`, `h_cycler.obj` and `handgrenade.obj` have their header inline functions where a full build doesn't put them; in SiC 2 also every `weapons.h` dependent from `AI_BaseNPC_Schedule.cpp` to `player.cpp` | **Known:** those objects were last compiled in a session after a full build; in SiC 1 its first file found a `.pch` cut back to `extdll.h util.h cbase.h monsters.h`, which the `.pch` of a build ending with `zombie.cpp` gives. **Stand-in:** which header was edited. Both mods changed `weapons.h` (SiC 1 its ammo and clip sizes, SiC 2 its sprite indexes and renamed weapons). An `effects.h` edit, which neither mod changed, gives the same images. Other single headers don't: in SiC 1 `monsters.h` and `skill.h` leave objects out of place; in SiC 2 `skill.h`, `monsters.h`, `gamerules.h` or `explode.h` leave 56,324 to 155,250 bytes different. Only the objects' final states are evidence |
| Servers: 1 + 5 links (SiC 1), 1 + 7 (SiC 2) | The PDB's link history | Retail gives only the PDB's creation time (signature) and the number of links (age) | The middle links at retail's link time are a stand-in: their times, and what was compiled between them, are unknown. A link without incremental linking doesn't depend on the earlier links into the PDB. The PDB itself is not part of the image |
| Servers: the output names | SiC 1 linked as `spirit.dll`, SiC 2 as `bloodbath.dll` | The PDB names are `spirit.pdb` and `bloodbath.pdb` | Inferred: LINK names the PDB after the output. SiC 1's is Spirit's own output name, shipped renamed `sic.dll` |
| SiC 2: LINK's C runtime | Wine 11.0's builtin `msvcrt`, for the servers' links and the whole client build | SiC 2's linker broke `qsort` ties like it | Which runtime it was is not known ([LINK's C runtime](#links-c-runtime)) |
| The clients' build history | One full build each | The clients have no debug record, so nothing dates their builds | A full build gives every client object as retail has it. SiC 1's client was linked 2 h 28 min before its server; whether a `weapons.h` edit came before or after it is not measured |
| The machine | Wine 11.0 and, for SiC 1, the CD's `MSVCRT.DLL` | Stand in for the Windows systems the mods were built on | The identical clients show that the mods' linkers made the same choices |

**Known from retail, not stand-ins:** the trees' paths and drives (from the
PDB records), the link times, the PDBs' creation times and ages, the
compiler and linker builds and the object counts (from the Rich headers).

## How identity was reached

The work went in historical order: SiC 1 first, then SiC 2 on top of SiC
1's tree, then the one tree. The branch's commits are all of 2026-11-19.
Each part went the same way: the base, then the mod's functions, written
from the disassembly and decompiled code and compared with retail one
function at a time, then everything else the image records.

**SiC 1.** Of the game functions, exact:

| Commit | Server, of 3,755 | Client, of 1,550 | What it added |
|---|---|---|---|
| `6a28b30` (the base) | 3,675 | 1,465 | Spirit 1.2 as retail's projects |
| `ec3024d` | 3,749 | 1,549 | the mod's changes, merged in `c6f0864`, `887a9e2` and `ec3024d` |
| `ec3024d`, with browse information | 3,751 | 1,550 | the projects' `/Fr` and `/FR`, which the counts above leave out |

At `ec3024d` there was no reference, data, vtable, export or string
difference left. The image then needed no source change. Bytes that differ
in the whole file, release builds of `ec3024d`:

| Build | Server | Client |
|---|---|---|
| full build without browse information | 9,503 | 1,812 |
| with the projects' browse information | 8,646 | **0** |
| the last session, without browse information | 857 | |
| browse information and the last session | **0** | |

The server's four functions left by a full build with browse information
(22 bytes of code) were the three objects' header inline functions that
only the session places.

**SiC 2.** Of the game functions, exact:

| Commit | Server, of 3,744 | Client, of 1,550 | What it added |
|---|---|---|---|
| `fde2b87` | 3,552 | 1,486 | SiC 1's tree with SiC 2's projects: output name, alien files out, four new files |
| `49b53e4` | 3,554 | 1,486 | the skill data |
| `afecfc0` | 3,734 | 1,528 | weapons, monsters, effects and world (`023ff22`, `ea1e3d9`, `1825f49`), the sprite names |
| `2d74976` | 3,700 | 1,528, or 1,550 with LINK on Wine's runtime | the music player's third message (`4ed7ecc`), the empty alien files |

From `afecfc0` to `2d74976` a plain full build gains six exact server
functions (the inline functions of `alias.obj`, `func_break.obj` and
`nodes.obj`) and loses 40: header inline functions of `weapons.h`
dependents that the empty `.pch` moves and the last session puts back.
Bytes that differ in the whole file:

| State | Server | Client |
|---|---|---|
| `afecfc0`, a full build, LINK on the CD's runtime | 82,657 | |
| `afecfc0`, with the last session and LINK on Wine's runtime | 24,008 (`alias.obj`, `func_break.obj`, `nodes.obj` and the calls into them) | 4,053 of `.data` and 357 of `.text`: one Czech message still named the old music player, and every string after it sat 4 bytes off |
| `4ed7ecc` | | **0** |
| `2d74976` | **0** | **0** |

**One tree.** `8f70aae` to `8d34187` put SiC 1's text under `SIC1`, file by
file, each checked with the preprocessor against `ec3024d` and `21fc2ea`.
`agrunt.cpp` took the measurements in
[One tree for both games](#one-tree-for-both-games). All four files built
from one fresh clone at `8d34187` are retail's.

## Check it yourself

1. Get the retail files, build and compare as
   [rebuild/survive-in-catacombs.md](rebuild/survive-in-catacombs.md)
   says: it downloads the two archives, checks their SHA-256, unpacks the
   DLLs and runs `sic.sh` for each game with `RETAIL` set. Expected:
   `identical to retail` for both DLLs of each game, `cmp` reporting no
   difference, exit code 0.

2. Look at the parts besides code with GNU binutils, on retail's files and
   on yours:

   ```sh
   TZ=UTC objdump -p sic.dll | grep -E 'Time/Date|format NB10'
   ```

   Expected for `sic.dll` (yours is `spirit.dll`; objdump prints the
   signature's bytes in file order):

   ```
   Time/Date		Wed May 19 21:32:38 2004
   Time/Date stamp 		40abd276
   (format NB10 signature 4566a740 age 6 pdb F:\SicEngineCodes\DLLS\Releasehl\spirit.pdb)
   ```

   for SiC 1's `client.dll` (no PDB line):

   ```
   Time/Date		Wed May 19 19:04:12 2004
   Time/Date stamp 		40abafac
   ```

   for `bloodbath.dll`:

   ```
   Time/Date		Sun Dec 19 18:27:24 2004
   Time/Date stamp 		41c5c80c
   (format NB10 signature 09bdc241 age 8 pdb C:\bloody\bloodyEnginecodes\DLLS\Releasehl\bloodbath.pdb)
   ```

   and for SiC 2's `client.dll`:

   ```
   Time/Date		Fri Dec 17 10:31:35 2004
   Time/Date stamp 		41c2b587
   ```

3. See what the pragma does: delete the `#pragma hdrstop` line from
   `dlls/agrunt.cpp` in your checkout and build SiC 2's server again, with
   `RETAIL` set (`tools/release/sic.sh 2 ../sic ../bloodbath-build server`).
   It then `differs from retail`, and `cmp` reports `24008 of 1208393 bytes
   differ`: header inline functions of `alias.obj`, `func_break.obj` and
   `nodes.obj` sit in other places, and the calls into them differ. SiC 1's
   server is still identical after that edit.

The tools we used to compare partial matches are not in this repository: a
function matcher, a per-section image comparison, the reference checkers,
and the preprocessor comparison. For identical files, `cmp` is the whole
test.
