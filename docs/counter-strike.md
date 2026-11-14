# Counter-Strike (Beta 1.0 to 1.5)

Counter-Strike is a team-based multiplayer modification of Half-Life:
terrorists against counter-terrorists, in rounds, with money, bought weapons,
hostages and, from Beta 4.0 on, a bomb. Its first public Beta came out on 19
June 1999. Version 1.0, the first after the Betas, followed on 8 November
2000, and updates continued to 1.5 on 12 June 2002. Every release ships two
game DLLs, built with Microsoft Visual C++:

- `dlls/mp.dll`, the server: the game rules, players, weapons, hostages;
- `cl_dlls/client.dll`, the client: the HUD and menus, and from Beta 6.5 on
  the client side of the weapons.

The branch [`counter-strike`](../../../tree/counter-strike) holds C++ source
that compiles back into these files for every release whose files survive,
from Beta 1.0 to 1.5, one commit per release in historical order. It is
Valve's Half-Life SDK 1.0 (`b443b44`, tag `initial`, as released on
1999-04-07) plus 24 commits, with 27 tags named `cstrike-*` for the
versions. Each commit message says what the release changed, how its DLLs
were built and which parts of its text are stand-ins; each tag message
gives the retail files' SHA-256. File references below are to the head of
the branch, `5e4dafa` (1.5), unless a tag is named.

## Contents

- [Status](#status)
- [Where the retail files come from](#where-the-retail-files-come-from)
- [What the code is](#what-the-code-is)
- [Compilers](#compilers)
- [Rebuilding](#rebuilding)
- [What identity needed](#what-identity-needed)
- [Stand-ins](#stand-ins)
- [Beta 6.5d and the shared tags](#beta-65d-and-the-shared-tags)
- [The packed clients of 1.4 and 1.5](#the-packed-clients-of-14-and-15)
- [What is not identical in the 1.4 and 1.5 clients](#what-is-not-identical-in-the-14-and-15-clients)
- [Sources](#sources)
- [Check it yourself](#check-it-yourself)

## Status

The releases ship 40 distinct DLLs: 23 servers and 17 clients (15 PE files
and two in the engine's packed format). **38 of the 40 rebuild byte for
byte.** The two that don't are the clients of 1.4 and 1.5
([below](#what-is-not-identical-in-the-14-and-15-clients)).

Verified on 2026-11-14 with fresh release builds of every tag, from a clone
of the repository, by SHA-256 and `cmp` against the retail files.

| Release | Released | Tag (commit) | Server | Client | Retail `mp.dll` SHA-256 | Retail `client.dll` SHA-256 |
|---|---|---|---|---|---|---|
| Beta 1.0 | 1999-06-19 | [`cstrike-beta1.0`](../../../tree/cstrike-beta1.0) (`a29757a`) | **identical** | **identical** | `f4155737d5f190a385da49308a1ef38446f5dd50aaae2362eccff48addac3bf6` | `07e181325c522cd612c1059d28c3c173c7200b1c9953f7d9bec418a4d6055564` |
| Beta 1.1 | 1999-06-27 | [`cstrike-beta1.1`](../../../tree/cstrike-beta1.1) (`26b28e7`) | **identical** | Beta 1.0's file, **identical** | `577517c643e09c7f9426ff8cde659254df496a79bd18d075a340110bb0a5ecec` | `07e181325c522cd612c1059d28c3c173c7200b1c9953f7d9bec418a4d6055564` |
| Beta 2.0 | 1999-08-13 | [`cstrike-beta2.0`](../../../tree/cstrike-beta2.0) (`06d374c`) | **identical** | **identical** | `258e016a4b257b759bc42dc71769455508309eb86de188c1dcd6ad3cd4c6debe` | `791242cc4a109773c940962dcd7fbccb79955c8adffc36773d3eff0acfafaf73` |
| Beta 2.1 | 1999-08-17 | [`cstrike-beta2.1`](../../../tree/cstrike-beta2.1) (`2d195a8`) | **identical** | Beta 2.0's file, **identical** | `57f9dd8b2cae2072a77e053eb174fd3842d9ef8253031cb4c55475a38f40d844` | `791242cc4a109773c940962dcd7fbccb79955c8adffc36773d3eff0acfafaf73` |
| Beta 3.0 | 1999-09-14 | [`cstrike-beta3.0`](../../../tree/cstrike-beta3.0) (`b0635b4`) | **identical** | **identical** | `a1c31191a6176be679ee5fa2e6b1e7feaf59f83e26117b346ed3f2263ce4382b` | `a3298a3a5a2a0369340d63d30beb6a271653f17310892e7bc70b1d40c5600c50` |
| Beta 3.1 | 1999-09-16 | [`cstrike-beta3.1`](../../../tree/cstrike-beta3.1) (`69f37fc`) | **identical** | Beta 3.0's file, **identical** | `7dac5feda8c0137a7bcdb3855ab3e94ce3c5bcf84e3cb47f9319113b03f0b209` | `a3298a3a5a2a0369340d63d30beb6a271653f17310892e7bc70b1d40c5600c50` |
| Beta 4.0 | 1999-11-05 | [`cstrike-beta4.0`](../../../tree/cstrike-beta4.0) (`8f90f1a`) | **identical** | **identical** | `95312d4de5ca88c387b072f97563911b84a8e0c2c046b850a097bfab8c4dcca6` | `bd1b1c76bfd79a7614f36508518637bf78716e1414e8592b19bd7bbd44f3f3c7` |
| Beta 4.1 | 1999-12-01 | [`cstrike-beta4.1`](../../../tree/cstrike-beta4.1) (`059541f`) | **identical** | Beta 4.0's file, **identical** | `f14aed6b873e9a1b31786782d5e33f3d57656f25d72f2a45a0a76bcddb4dc08f` | `bd1b1c76bfd79a7614f36508518637bf78716e1414e8592b19bd7bbd44f3f3c7` |
| Beta 5.0, leaked build | files of 1999-12-09 | [`cstrike-beta5.0-leak`](../../../tree/cstrike-beta5.0-leak) (`2683e38`) | **identical** | **identical** | `2ba7b797f9e9fdad3a88dbbeab61125e664a8777153c27ef4251ad69e8fcc816` | `62e8489388fa153da1fbdf0d348b33d749a50acb5cbcc4cbda7540e956aefa74` |
| Beta 5.0 | 1999-12-23 | [`cstrike-beta5.0`](../../../tree/cstrike-beta5.0) (`1cbc344`) | **identical** | **identical** | `87ea6dade4035c65338fb8cd95cce5b489b53b3951b8c34323fcd214c50927d7` | `b3ebbbe9290812cd01b44a19d7b989aba90e8f068051340a33f8dc564ed4c6f0` |
| Beta 5.2 | 2000-01-10 | [`cstrike-beta5.2`](../../../tree/cstrike-beta5.2) (`f8c1f94`) | **identical** | **identical** | `af119154607b38b6d0bae8a10efc702c4354d7088166895b47c73e3a75938b07` | `79730770a318066771ee2299af9266bd99aebb153bda0fa586d0d9baf6cce123` |
| Beta 6.0 | 2000-03-10 | [`cstrike-beta6.0`](../../../tree/cstrike-beta6.0) (`9760190`) | **identical** | **identical** | `23796074115c00f308170bdd68c8a40ad0d1ec7e769bf3d9549650d0c3d09f7c` | `bc7412fcbbb20f8f4c1a37a8e5c83b36926d54b86450b0bf52319fe83a7fd80b` |
| Beta 6.1 | 2000-03-24 | [`cstrike-beta6.1`](../../../tree/cstrike-beta6.1) (`9876677`) | **identical** | Beta 6.0's file, **identical** | `d7f05c09d3d9bd1ee44ed2c438ba6830819a57ec532ee20c5b1e860f8debac96` | `bc7412fcbbb20f8f4c1a37a8e5c83b36926d54b86450b0bf52319fe83a7fd80b` |
| Beta 6.5 | 2000-06-08 | [`cstrike-beta6.5`](../../../tree/cstrike-beta6.5) (`f8291ba`) | **identical** | **identical** | `abc1fb0da96654ea3e8a05cedf0ad4ca1d553a44b4b19691ec822c815e2c89cf` | `db55dcbd4316f73412fa6f73828476985f490b1872ddfebfa5f57497a8e1f95a` |
| Beta 6.5d | not dated | [`cstrike-beta6.5d`](../../../tree/cstrike-beta6.5d) (`18911c6`) | Beta 6.5's file | Beta 6.5's file | `abc1fb0da96654ea3e8a05cedf0ad4ca1d553a44b4b19691ec822c815e2c89cf` | `db55dcbd4316f73412fa6f73828476985f490b1872ddfebfa5f57497a8e1f95a` |
| Beta 6.6 | 2000-06-22 | [`cstrike-beta6.6`](../../../tree/cstrike-beta6.6) (`a43d0b4`) | **identical** | **identical** | `84fb002a67a9f8dd7e48ec60494c5078d598d2151ddc34409234ee98d87d90da` | `1db237bb08759f1a7323cf1d1c3a24d912a0893e50638a5a684cd40994bd757d` |
| Beta 7.0 | 2000-08-26 | [`cstrike-beta7.0`](../../../tree/cstrike-beta7.0) (`654725d`) | **identical** | **identical** | `210d739c26423f48693a800d536a9a2e838ebd2c24afa1bbebf97fc8870b56e3` | `3f16ba1141c172802ed6f32b08bc3de4d1576cb14a2d3d7a3d79a0e20968e496` |
| Beta 7.1 | September 2000 | [`cstrike-beta7.1`](../../../tree/cstrike-beta7.1) (`15d5377`) | **identical** | **identical** | `41f728a70c0778dff227d88635e10b2e4710fb5136e8545bb199c869ba7ce9b3` | `62e647eda8ce22b574a52e5f9040a9acc1c449602cc05b125e1f62cafe13178d` |
| 1.0 | 2000-11-08 | [`cstrike-1.0`](../../../tree/cstrike-1.0) (`e943424`) | **identical** | **identical** | `21073277e6e94d947075b6c958a61df2b840d3cd9db394b3349c887457332fea` | `0dab270405a0cd9bf68ebfd2137f013dddf2f7128bd496530af8d82ad76501a2` |
| 1.0b | 2001-01-25 | [`cstrike-1.0b`](../../../tree/cstrike-1.0b) (`9c64f22`) | **identical** | 1.0's file, **identical** | `0fc676a35a0c84a4a5cf50c392d2ca18768397ecb4ebeb939e9bcf73e06dcbdc` | `0dab270405a0cd9bf68ebfd2137f013dddf2f7128bd496530af8d82ad76501a2` |
| 1.1 | 2001-03-13 | [`cstrike-1.1`](../../../tree/cstrike-1.1) (`08c7456`) | **identical** | **identical** | `bea6118681f7a9d05b32fa71a2a6f828faee1151eca887f53d96544c96774881` | `47e8c2ed15a15e199ae7fadb58c4637fb7893fdb6c3a347ddd8fd2631dbc078b` |
| 1.1b | not dated | [`cstrike-1.1b`](../../../tree/cstrike-1.1b) (`08c7456`) | 1.1's file | 1.1's file | `bea6118681f7a9d05b32fa71a2a6f828faee1151eca887f53d96544c96774881` | `47e8c2ed15a15e199ae7fadb58c4637fb7893fdb6c3a347ddd8fd2631dbc078b` |
| 1.1c | 2001-04-06 | [`cstrike-1.1c`](../../../tree/cstrike-1.1c) (`08c7456`) | 1.1's file | 1.1's file | `bea6118681f7a9d05b32fa71a2a6f828faee1151eca887f53d96544c96774881` | `47e8c2ed15a15e199ae7fadb58c4637fb7893fdb6c3a347ddd8fd2631dbc078b` |
| 1.2 | 2001-06-30 (the archive's list) | [`cstrike-1.2`](../../../tree/cstrike-1.2) (`08c7456`) | 1.1's file (no package to compare) | 1.1's file (no package to compare) | `bea6118681f7a9d05b32fa71a2a6f828faee1151eca887f53d96544c96774881` | `47e8c2ed15a15e199ae7fadb58c4637fb7893fdb6c3a347ddd8fd2631dbc078b` |
| 1.3 | 2001-09-12 | [`cstrike-1.3`](../../../tree/cstrike-1.3) (`ffb6e62`) | **identical** | **identical** | `598c9473cc16386a99bc01bba8f4bb31ffb10473f111434b2926fa3be81c30e1` | `71fdbe140df3598a74738537b924141f4fb4b0b828737bd99a7223d9e4549460` |
| 1.4 | 2002-04-24 | [`cstrike-1.4`](../../../tree/cstrike-1.4) (`3916799`) | **identical** | packed; **not identical** | `f11016c098bf874f3b8ea9936a0c6760097d177412895de11faa0efbddeaac98` | `4b8af3606cb440fc40f066abe96b5959fe2036cae6809b7ffdbeeb6c50d3471d` |
| 1.5 | 2002-06-12 | [`cstrike-1.5`](../../../tree/cstrike-1.5) (`5e4dafa`) | **identical** | packed; **not identical** | `f10cb95979afd46c677bbf21d40be7ec5a9fc179306a03b3b1ce76744ca4d2e8` | `61228d9c9664c0c67af3fb7717d7fa05b0fee88728a97030e89b45b12b5167ac` |

Release dates are the commit messages'. Beta 7.1's DLLs are linked on 15
September 2000 (UTC); the archive's README dates the release 13 September.

- **Server-only releases.** Betas 1.1, 2.1, 3.1, 4.1, 6.1 and 1.0b ship a
  new server and the previous release's client. Each has a commit of its
  own, and its tree builds both files: the new server and, unchanged, the
  earlier client.
- **Beta 6.5d** is a Linux release. Its package carries Beta 6.5's two
  Windows DLLs and a new Linux library. Its commit is the source of that
  library, a state between 6.5 and 6.6 ([below](#beta-65d-and-the-shared-tags)).
  Beta 6.5's tag builds the two DLLs of the 6.5d package.
- **1.1b, 1.1c and 1.2** have 1.1's two DLLs. Their tags point at 1.1's
  commit. Only 1.1c has a package of its own, and its DLLs were compared:
  byte for byte 1.1's. 1.1b and 1.2 have no package in the archive; see
  [the shared tags](#beta-65d-and-the-shared-tags).
- **1.4 and 1.5** ship their clients in the engine's packed format. Their
  servers are identical; the clients are compared in packed form and are
  not identical. Our builds give
  `c2c6e9b895eba6d17ea9d206e36786893524fe0768e4299be7f6bcaf1f674d56` (1.4)
  and `e8b1191cfa8dc959f900d37fefe8de34c0dd65b84309becf63cd3937286d0009`
  (1.5).

**What "identical" means here.** It means the files: every byte of a DLL,
with its headers, time stamps, debug record and padding. It does not mean
that every line of the source is Counter-Strike's own text. A compiled DLL
doesn't record names, comments, or text that compiled to no code, and in
places the cause in retail's source left no other trace. There the branch
has a marked stand-in: an unused declaration or enum, an unused inline
function of empty statements, a dead store, a dead function the linker
counts. The build also stands in for part of the build history (which files
were edited before the last builds, the links between a PDB's creation and
the release). [Stand-ins](#stand-ins) summarises them per release; each
commit message lists its own.

## Where the retail files come from

The retail DLLs are not in this repository. They come from **Ch0wW's archive
of the historical Counter-Strike releases**,
<https://github.com/Ch0wW/counterstrike-betas>, which keeps the files of each
release's installer, unpacked and unmodified, on a branch per version. This
work would not have been possible without it.

Where each version is in the archive, as fetched on 2026-11-11. Some branches
were later moved on to a sub-release, so for those the commit is given. The
SHA-256 values in [Status](#status) are what counts.

| Version | Branch or commit | Folder |
|---|---|---|
| Beta 1.0 | `csbeta10` | `cstrk10` |
| Beta 1.1 | `csbeta11` | `cstrk11` (upper-case names: `DLLS/MP.DLL`, `CL_DLLS/CLIENT.DLL`) |
| Betas 2.0 to 4.1 | `csbeta20`, `csbeta21`, `csbeta30`, `csbeta31`, `csbeta40`, `csbeta41` | `cstrk20` ... `cstrk41` |
| Beta 5.0, leaked build | `csbeta50_leak_20DEC99` | `cstrk50leak` |
| Betas 5.0 to 6.1 | `csbeta50`, `csbeta52`, `csbeta60`, `csbeta61` | `cstrk50` ... `cstrk61` |
| Beta 6.5 | commit `0ce5e4e8c1fdbc69fc1720dafa5a5e3ce1eef16d` | `cstrk65` |
| Beta 6.5d | `csbeta65` | `cstrk65` |
| Betas 6.6 to 7.1 | `csbeta66`, `csbeta70`, `csbeta71` | `cstrk66`, `cstrk70`, `cstrk71` |
| 1.0 | commit `ab6a359832dac535b4e4f25c7d15e724ea514cc1` | `cstrk10r` |
| 1.0b | `cs10_retail` | `cstrk10r` |
| 1.1 | commit `a98571c1aa4ca3f3a6f42c8848727b34db618bf0` | `cstrk11r` |
| 1.1c | `cs11_retail` | `cstrk11r` |
| 1.3, 1.4, 1.5 | `cs13_retail`, `cs14_retail`, `cs15_retail` | `cstrk13`, `cstrk14`, `cstrike` |

Each folder is the mod folder of the release: `dlls/mp.dll`,
`cl_dlls/client.dll`, and for most versions from Beta 3.1 on the Linux
server library (`mp_i386.so`, from Beta 5.0 `cs_i386.so`).

- **The Beta 5.0 leak.** The archive dates the leak 20 December 1999; its
  files are of 9 December, an earlier build of Beta 5.0 than the release of
  the 23rd. Both of its DLLs are targets and are identical.
- **Missing versions.** Betas 1.2, 6.2, 6.7 and 6.8 are not in the archive:
  there are no files to rebuild.
- **1.1b and 1.2.** The archive lists 1.1b as included in 1.1's package and
  1.2 as an engine update, with a link to 1.1c's package. Neither has a
  package of its own.
- **The earlier Beta 1.1 file.** Until its fix of 2024-12-30, the archive
  carried another `MP.DLL` as Beta 1.1 (commit `4f9681b`, SHA-256
  `7407164fdc4c112ca51a30a16f9d633991c68960d54b2f6ed642865d5d3ee3b4`). It
  differs from the released file in 4 bytes, two each at file offsets
  0x7BF94 and 0x7C0DC, and has the same link time and PDB record. It is not
  a target.

## What the code is

The branch keeps one tree per release, each the one before it with the
release's changes. The base changes three times:

- **Betas 1.0 to 6.1: SDK 1.0's game code**, as the Counter-Strike team
  changed it. Beta 1.0's server is SDK 1.0's 59 files plus 12 of
  Counter-Strike's own (`ammo`, `awp`, `flashbang`, `g3sg1`, `glock18`,
  `hostage`, `m249`, `m3`, `m4a1`, `mp5navy`, `tmp`, `usp`; the names are
  from Beta 3.1's Linux library). Of its 2,694 game functions (functions
  outside the C runtime), 2,226 are SDK 1.0's unchanged, 255 changed and 213
  new. Half-Life's own weapons stay compiled in, with their `Precache`
  emptied, until Beta 4.0 drops their 13 files. The client is SDK 1.0's,
  with one function changed in Beta 1.0 (the scoreboard draws players with
  the model `terror` in red); the later Betas add Counter-Strike's HUD to
  it.
- **Betas 6.5 to 1.1: SDK 2.0.** Movement is `pm_shared`'s, shared with the
  client; the weapons are in `dlls/wpn_shared` and compiled into both DLLs,
  so the client predicts them; the client is SDK 2.0's VGUI client with
  Counter-Strike's HUD on top. 1.0 adds Counter-Strike's player renderer on
  SDK 2.1's `StudioModelRenderer.cpp`; 1.1 takes SDK 2.2's log format.
- **1.3 to 1.5: SDK 2.2 and 2.3 pieces.** 1.3 has SDK 2.2's voice manager and
  spectator HUD and SDK 2.2b's voice status. 1.4 has SDK 2.3's engine
  interface (`GetNewDLLFunctions`), spectator panel and score board. Its
  client replaces the 40 interface functions that 1.3's exported with one
  export, `F`, through which the engine and the client exchange tables of
  functions (`engine/APIProxy.h`).

The commit messages describe each release's game changes in plain words:
new guns, money, the round rules, the bomb (Beta 4.0, a weapon from the 5.0
leak on), observers, VIP and escape maps (Beta 6.0), vehicles (Beta 7.0),
the training course (1.0), spectators as a team (1.1), HLTV (1.3), hostages
that find their way (1.4). Retail's bugs and oddities are kept and marked
`// sic`.

**Names.** A name is retail's where the DLL (exports, save tables,
strings), the release's `titles.txt` or a Linux library's symbol table gives
it. The Linux libraries are not stripped and name every function with its
parameter types; Beta 3.1's is the first, and its names were carried back to
Beta 1.0 through the matched functions (2,681 of Beta 1.0's 2,694 game
functions are named). Valve's Counter-Strike 1.6 Linux libraries, which
carry debug information, gave names, members, locals and the order of
functions in a file where the code was found through them. Every other name
is ours, marked `// name ours` where it could be taken for retail's
(`git grep -n -i -E 'names? ours' cstrike-1.5` lists 440 lines in 86
files). Names that are ours stay ours: they were not renamed later to 1.6's.

## Compilers

Five toolchains built these DLLs. [compilers.md](compilers.md#counter-strike)
says where each comes from and which files matter.

| Releases | Compiler | How it is known | Tree |
|---|---|---|---|
| Betas 1.0 to 4.1 | Visual C++ 5.0 as on the CD: CL 11.00.7022, LINK 5.00.7022, the CD's `LIBCMT.LIB` | LINK 5.00 in the PE header and no Rich header (Service Pack 3's LINK is 5.10 and writes one; Service Packs 1 and 2 ship LINK 5.02.7132). Five functions of Beta 1.0's client that SP3's compiler builds differently have the CD compiler's form. | `vc5/rtm` |
| Beta 5.0 leak to Beta 7.1 | Visual C++ 6.0 as on the CD (12.00.8168, LINK 6.00.8168) | Rich header: C and C++ objects of build 8168, LINK 6.00.8168 | `vc6` |
| 1.0, 1.0b, 1.1 | VC6 Service Pack 4 (back end 12.00.8799, LINK 6.00.8447, C runtime 8797) | Rich header: 8799, LINK 8447, runtime objects 8797 | `vc6sp4` |
| 1.3 server | VC6 Service Pack 5 (back end 12.00.8966) | Rich header: 8966, LINK 8447, runtime objects 8047 | `vc6sp5` |
| 1.3 client, 1.4, 1.5 | VC6 SP5 with the Processor Pack (back end 13.00.9044) | Rich header: product ids 48 and 49, build 9044, LINK 8447. The packed 1.4 and 1.5 clients have no Rich header; their code, SP5's `LIBCMT` and VC6's exception tables say the same. | `vc6sp5pp` (and `vc6sp5` for one file of the 1.4 and 1.5 clients) |

Every tool runs on its own release's `MSVCRT.DLL`, not Wine's builtin
([LINK on its own C runtime](#link-on-its-own-c-runtime)).

## Rebuilding

### What you need

- The toolchain folder from [setup.md](setup.md), with the trees `vc5/rtm`,
  `vc6`, `vc6sp4`, `vc6sp5` and `vc6sp5pp`, each with its `nativecrt` copy,
  and `wineprefix`. `tools/check_toolchains.sh` compares each tree with the
  one that built the identical DLLs.
- Wine (we used 11.0 with a win64 prefix), libfaketime's `faketime` (we used
  0.9.12), Python 3 (standard library only), git, `sha256sum` and `cmp`.
- A clone of this repository that has the `counter-strike` branch **and its
  tags**: Betas 2.1, 3.0, 3.1 and 6.1 continue an earlier release's build,
  and the script takes that release's source from the repository by its tag.

The branch stores its sources with CRLF line ends and has no
`.gitattributes`. Leave `core.autocrlf` off (the default on Linux).

### Commands

From a checkout of `main`, for one release:

```sh
git worktree add --detach ../cs-b10 cstrike-beta1.0
VC_TOOLCHAINS=/path/to/toolchains \
    tools/release/counter-strike.sh cstrike-beta1.0 ../cs-b10 ../cs-b10-build
```

- The first argument is the tag, `cstrike-beta1.0` to `cstrike-1.5`. The
  second is a checkout of that tag; if it is at another commit, the script
  says so and builds what is there.
- `../cs-b10-build` is the build folder, created if needed. The server is
  built in `OUT/wineprefix`, a copy of the toolchain's Wine prefix with the
  source tree laid out at retail's path inside it; the built file is copied
  to `OUT/server/mp.dll`. The client is `OUT/client/client.dll`, and for 1.4
  and 1.5 also `OUT/client/client.packed.dll`, the file that is compared.
  `OUT/server.log` and `OUT/client.log` hold the output, and
  `OUT/server/build.log` every CL and LINK command line.
- A fourth argument picks `server`, `client` or `both` (the default).
- To compare with your own retail copy as well, set `RETAIL` to the
  release's mod folder (the archive's `cstrk10`, for example). For
  `cstrike-beta6.5d`, whose tree builds Beta 6.6's client, give Beta 6.6's
  folder.

For each DLL the script prints the built file, its SHA-256, retail's and
`identical to retail` or `differs from retail`; with `RETAIL` set it also
runs `cmp`. It exits with 0 only if every DLL it built is identical, so the
1.4 and 1.5 tags exit with 1 when the client is built. For those clients it
also says whether the packed file has the SHA-256 the commit message
records for the tree.

Run times on our machine, with five builds running at once: Beta 1.0's
server, 1,215 links, 472 seconds; Beta 3.1's server, which rebuilds Betas
2.0, 2.1 and 3.0 on the way (729 links), 466 seconds; a server of 1.0 to
1.5, about 20 seconds; a client, under a minute; all 27 tags, both DLLs
each, about ten minutes.

### What the script does for the server

Every server links with `/debug` up to 1.3, and its CodeView record names
the PDB (program database) LINK wrote: its path, its creation time (the
signature) and how many links wrote it (the age). The script rebuilds what
that record and the objects show, with `tools/release/counter-strike_server.py`:

1. **The tree.** It lays out the checkout at the path the record names, for
   example `D:\half-life\code2` for Betas 1.0 to 4.0, on that drive of the
   prefix copy (a drive other than C: gets its own folder and drive
   letter), as symbolic links. `dlls` is a folder of links to single files,
   so an edit can replace one file with a copy that has a new time stamp;
   the checkout is never written.
2. **A Rebuild All** with the IDE's command lines: CL from `dlls` with
   `mp.dsp`'s Release flags and one precompiled header through `/YX`, the
   sources in project order, then LINK with the project's link line and the
   objects in project order. Its link runs with the clock at the PDB's
   creation time and creates the PDB (not for the Beta 5.0 leak:
   [below](#the-build-history-in-the-objects)).
3. **Relinks.** Links of the same objects with the clock at retail's link
   time, as many as the age needs. They stand for the builds between the
   PDB's creation and the release, which the image doesn't record.
4. **The last builds.** Each is one or more edits (a header or a source
   given a new time stamp) and an IDE build: the out-of-date sources compiled
   in project order against the `.pch` the previous compile left, then a
   link. The last link runs at retail's link time.
5. **Earlier releases first** (Betas 2.1, 3.0, 3.1, 6.1). Their PDB is the
   previous release's, still growing. The script runs the earlier releases'
   recipes in the same tree, on their own source taken from the repository
   by tag (`git archive`), prints whether each of their `mp.dll` has its
   retail SHA-256, and then the release's own builds bring in its text: every file
   the release changed is edited, which turns it into the release's copy.
6. **The clock.** Only LINK runs under libfaketime, with `TZ=UTC`. A link
   whose stamps are a second apart gets a clock that starts at the earlier
   second and advances 1 ms per read
   ([Two time stamps a second apart](#two-time-stamps-a-second-apart)).

1.4 and 1.5 link without `/debug`: no debug record, one Rebuild All and one
link at the link time, in 1.3's tree path, which no longer reaches the image.

### The servers' build records

What the debug records give (path, creation, age), and the history the
script builds. LINK 5.00 (to Beta 4.1) starts a new PDB at age 0 and LINK
6.00 at 1; both add 1 per link.

| Release | Tree (from the PDB path) | PDB created (UTC) | Links (age) | Builds after the Rebuild All | Link time (UTC) |
|---|---|---|---|---|---|
| Beta 1.0 | `D:\half-life\code2` | 1999-05-08 22:08:13 | 1,215 (1214) | 1,210 relinks; edits of `weapons.h`, of `gamerules.h`, and two of `weapons.cpp` | 1999-06-18 21:24:28 |
| Beta 1.1 | the same | 1999-06-26 20:35:07 | 17 (16) | 15 relinks; an edit of `weapons.h` | 1999-06-27 02:40:32 |
| Beta 2.0 | the same | 1999-07-04 17:07:56 | 293 (292) | 289 relinks; `weapons.h`, then `util.cpp` twice | 1999-08-13 05:07:06 |
| Beta 2.1 | the same, 2.0's PDB | the same | 309 (308) | 14 relinks; two builds of the five files 2.1 changed | 1999-08-16 20:22:43 |
| Beta 3.0 | the same, 2.1's PDB | the same | 705 (704) | 394 relinks; a build of 3.0's edits and new files; an edit of `cbase.h` | 1999-09-14 01:39:25 |
| Beta 3.1 | the same, 3.0's PDB | the same | 729 (728) | 23 relinks; a build of 3.1's edits | 1999-09-16 02:14:56 |
| Beta 4.0 | the same | 1999-10-20 06:22:27 | 255 (254) | 249 relinks; five builds | 1999-11-02 19:29:18 |
| Beta 4.1 | `D:\half-life\beta41` | 1999-11-24 21:31:07 | 3 (2) | two builds of the three files 4.1 changed | 1999-11-24 21:42:30 and :31 |
| Beta 5.0 leak | `C:\SIERRA\Half-Life\cstrike5` | the link time | 1 (1) | a build after edits of five files; the Rebuild All's link wrote another folder's PDB | 1999-12-09 00:42:32 |
| Beta 5.0 | the same | 1999-12-10 20:20:19 | 53 (53) | 50 relinks; two builds | 1999-12-20 23:50:41 |
| Beta 5.2 | `D:\half-life\code` | 2000-01-05 00:44:46 | 138 (138) | 133 relinks; `game.cpp`, `player.h`, `multiplay_gamerules.cpp` twice | 2000-01-10 21:45:20 |
| Beta 6.0 | the same | 2000-02-10 07:16:44 | 242 (242) | 238 relinks; three builds | 2000-03-06 07:06:18 |
| Beta 6.1 | the same, 6.0's PDB | the same | 295 (295) | 51 relinks; a build of the 27 changed files; one of two more | 2000-03-21 02:50:13 |
| Beta 6.5 | `D:\half-life\code\beta7` | 2000-06-05 21:55:09 | 2 (2) | one relink | 2000-06-05 21:58:32 |
| Beta 6.6 | the same | 2000-06-16 22:23:20 | 29 (29) | 26 relinks; `weapons.h`; the six files with new log lines | 2000-06-21 20:00:54 and :55 |
| Beta 7.0 | `D:\half-life\code\TEST` | 2000-08-23 02:29:27 | 35 (35) | 31 relinks; three builds | 2000-08-25 00:16:47 |
| Beta 7.1 | `D:\half-life\code\Beta71` | 2000-09-12 03:59:07 | 56 (56) | 55 relinks | 2000-09-15 02:36:05 and :06 |
| 1.0 | `F:\counter-strike\source` | the link time | 1 (1) | none | 2000-11-03 03:20:15 |
| 1.0b | `C:\Counter-Strike\source` | the link time | 1 (1) | none | 2001-01-08 19:14:07 |
| 1.1 | `U:\Counter-Strike\source` | the link time | 1 (1) | none | 2001-03-07 02:19:38 |
| 1.3 | `U:\hl1\release\dev\goldsrc\cstrike` | the link time | 1 (1) | none | 2001-09-12 19:12:48 |
| 1.4 | no debug record | | 1 | none | 2002-04-19 12:04:21 |
| 1.5 | no debug record | | 1 | none | 2002-06-10 07:57:11 |

The PDB is in `dlls\Releasemp` but for the 5.0 leak to 6.1, where LINK's
outputs went to `dlls` itself. Known from retail: the paths, the creation
times, the ages and the link times. Stand-ins: the relinks (their times and
what was compiled between them are unknown) and, where several histories
give the same objects, which edits the last builds followed: the objects
fix the state each file was last compiled in, not the edits.

### What the script does for the client

The clients have no debug record, so no path or history reaches the image
but what the objects show. `tools/release/counter-strike_client.py` builds
`cl_dll - Win32 Release` of `cl_dll/cl_dll.dsp` into an empty folder: every
source in project order with the project's flags, one `.pch` as the IDE's
`/YX` uses it, one link with LINK's clock at retail's link time. On top of
that, per release:

- **1.0 and 1.1:** `wpn_c4.cpp` is compiled once more after the full build,
  with the `.pch` the build left, and the DLL linked again. Retail's
  `wpn_c4.obj` has the form of a source that wrote its own precompiled
  header; a second compile gives it, and so would an include order that
  makes the file write its own (the same image).
- **Beta 6.6:** the client was linked on 2000-06-19, two days before the
  server, and one line of the shared `dlls/wpn_shared/wpn_c4.cpp` (the log
  line "%s planted the bomb") was written in between. The tree has the line;
  the client is built from a copy of the tree without it.
- **Beta 6.5d:** its tree is that earlier state, and builds Beta 6.6's
  `client.dll` as it is.
- **1.4 and 1.5:** `cl_dll/steam_wrap.c` is compiled by VC6 SP5 without the
  Processor Pack and without `/Ob2` (retail's code of it has that form), and
  the built PE file is packed with `tools/packed_dll.py`
  ([below](#the-packed-clients-of-14-and-15)).

### Project flags

The flags are the `.dsp` files' in each tag; the scripts leave out the
IDE-only `/FD`.

| Releases | Server compile (`dlls/mp.dsp`) | Server link | Client compile (`cl_dll/cl_dll.dsp`) |
|---|---|---|---|
| Betas 1.0 to 4.1 | `/G5 /MT /W3 /GX /Zi /O2 /Ob0 /YX` | `/map /debug /def:".\mp.def"` | `/MT /W3 /GX /Zi /O2 /Ob0 /YX` |
| Beta 5.0 leak to 6.1 | the same without `/Zi` | the same with `/incremental:no`, without `kernel32.lib` | the same, `kernel32.lib` off the link line |
| Betas 6.5 to 1.3 | SDK 2.0's: `/G5 /MT /W3 /GX /O2 /YX`, `/D CLIENT_WEAPONS` | `/map /debug /incremental:no` | SDK 2.0's VGUI client: `/MT /W3 /GX /Zi /O2 /YX`; `/D CLIENT_DLL` from Beta 7.0, `/D CLIENT_WEAPONS` from 1.3 |
| 1.4, 1.5 | `/G5 /MT /W3 /O2 /Ob2 /YX`: `/Ob2` new, `/GX` gone | without `/debug` | `/O2 /Ob2`, `/GX` kept |

`/Ob0` (no inline expansion) is in every project up to Beta 6.1. `/Zi`
matters even without a PDB in the image: it writes more intermediate code,
which moves where header inline functions land.

### If your build differs

| Symptom | Cause |
|---|---|
| Only the time stamps differ | LINK didn't run at retail's time. |
| Betas 1.0 to 4.1: two KERNEL32 imports trade places, the servers' MISC debug record has non-zero reserved bytes, the clients keep other copies of identical functions (measured on Beta 1.0: 24,717 bytes of the client differ) | LINK ran on Wine's builtin `msvcrt.dll`: the `vc5/rtm/nativecrt` copy is missing. |
| Beta 1.0's server: 2 bytes of the debug record differ | Fewer links than 1,215: the age (measured with 6 links). |
| The debug record's path and size differ | The tree was not at retail's path. |
| Betas 2.1, 3.0, 3.1, 6.1: "no tag ... in its repository" | The clone doesn't have the earlier releases' tags. |
| Beta 1.1 with `RETAIL`: `cmp: no .../dlls/mp.dll` | The archive spells Beta 1.1's names in capitals; give `RETAIL` a folder with lower-case names (links will do). |
| 1.4 and 1.5: the client differs | As known; the script says whether the packed file has the SHA-256 the commit message records. |

## What identity needed

Most of what a DLL records besides its code, and how a release build gets
it, is in [reproducing.md](reproducing.md). This section is about what
Counter-Strike needed beyond the earlier mods.

### LINK on its own C runtime

LINK sorts with the C runtime's `qsort` in two places that reach the image:
the order of the imported functions in each DLL's list, and which copy of
identical functions it keeps when it folds them. `qsort` is not stable, and
Wine's builtin `msvcrt.dll` orders equal keys differently from the 1997
runtime. LINK 5.00 (Betas 1.0 to 4.1) folds identical functions in a link
without `/debug`, as LINK 6.00 does. Measured on Beta 1.0's client: on
Wine's runtime 24,717 bytes differ, because 6 of the 9 folded groups keep
another copy and two of KERNEL32's 60 imports trade places; on Visual C++
5.0's own `MSVCRT.DLL` (5.00.7022) the file is retail's. Service Pack 3's
runtime and the VC6 CD's give retail's file too. The servers, linked with
`/debug`, show it in their imports (`GetFileType` in slot 0 and `RtlUnwind`
in slot 36 of KERNEL32's 72) and in the MISC debug record, whose three
reserved bytes LINK 5.00 leaves uninitialised: zero on the old runtime, as
in retail, `01 97 0c` on Wine's. The compiler doesn't care; only LINK needs
the old runtime. The VC6 trees likewise run on their release's own
`MSVCRT.DLL` ([compilers.md](compilers.md#the-runtime-link-ran-on)).

### PDB ages count links

LINK 5.00 writes age 0 into a PDB it creates and adds 1 per link; `/map`
turns incremental linking off, so every link is a full one. Beta 1.0's age,
1214, is therefore 1,215 links into a PDB created 41 days before the
release. The build does them for real (one link takes about 0.37 s here):
one at the creation time, 1,210 relinks, and the four links of the last
builds. With 6 links instead, the file differs from retail in exactly the
two bytes of the age. Betas 2.0 to 3.1 share one PDB, created on
1999-07-04, with ages 292, 308, 704 and 728; Betas 6.0 and 6.1 share one
too. That is why those releases continue the earlier build instead of
starting their own.

### The build history in the objects

With `/YX` (the automatic precompiled header) CL saves the state after a
file's leading headers in one `.pch`, and a later file whose include list
starts the same way reuses it. Whether a file wrote the `.pch` or reused
one, and which, decides where the headers' inline functions land in its
object ([vc6-notes.md](vc6-notes.md#where-header-inline-functions-land);
VC5 behaves alike). A Rebuild All gives every object one state; files
compiled again after an edit can be in another. So the order of functions
in the objects shows which files were compiled last, and in what state.

- **Beta 1.0.** No history of one, two or three builds after the Rebuild All
  gives every object retail's state. Seven histories of four builds do, and
  the script's is one of the two that edit only files Counter-Strike changed:
  `weapons.h`, `gamerules.h`, then `weapons.cpp` twice (its object needs
  the `.pch` it wrote itself). The image fixes each object's state, not the
  edits.
- **Beta 3.0** adds `debris.cpp`, which Beta 3.1's Linux library lists with
  nothing in it. If its first line is a declaration, `/YX` stops there and
  leaves an empty `.pch`, which every later file reuses: in effect the rest
  of the project, up to Beta 3.1's release, compiles without a precompiled
  header, and that is what every object of both images shows. An empty or
  comment-only file is not the same: its reuse counts one declaration more
  (next section).
- **Beta 5.0 leak.** The PDB is new at the release link (age 1). LINK's
  outputs moved from `dlls\Releasemp` to `dlls` in this version, so the
  Rebuild All's link wrote the old folder's PDB; the script doesn't link
  after the Rebuild All, and the one link after the last build creates the
  PDB.
- **From 1.0 on** each server is a plain Rebuild All with one link (age 1,
  the signature equal to the link time).
- **`hostage.cpp`'s include order.** In every server from Beta 6.5 to 1.3,
  `hostage.obj` has the form of a source that writes its own `.pch`. A file
  whose include list begins `extdll.h`, `util.h`, `cbase.h` doesn't do that
  in a Rebuild All: it reuses the three-header `.pch`. 1.0's PDB has one
  link, so no later build can explain it there: the text must. So
  [`hostage.cpp`](../../../blob/counter-strike/dlls/hostage.cpp#L35-L46)
  includes `../engine/studio.h` ahead of `cbase.h`, and the same line gives
  retail's file in every Beta from 6.5 on. The exact place is ours.

### VC5 orders operands by the declaration count

Visual C++ 5.0 numbers everything a source file declares, headers
included, with one counter, and orders some pairs of operands by those
numbers: the two operands of an x87 instruction, or two calls on either
side of `==`. Unchanged SDK code changes its operand order when
declarations are added anywhere before it, so the stock functions pin how
many declarations Counter-Strike's headers had. Over every source of Beta
1.0's server, seven functions depend on the count. We measured what each
kind of declaration costs: an enumerator, a variable or a typedef is 1, a
function declaration 1 plus one per parameter, a class 7 plus its members.
A compile that reuses a `.pch` another file wrote counts one more, so the
count depends on the build history too.

That made **`weapons.h` includes `effects.h`** visible, the line SDK 2.2's
`weapons.h` has and SDK 1.0 to 2.1's don't. Beta 1.1's image shows it
twice: its intermediate code (5,071 bytes, which place `weapons.obj`'s
header inlines) and its declarations (about 294, which move operands in
the files that include `weapons.h`). It was found on Beta 4.1, where
`func_tank.cpp`, which includes `effects.h` itself before `weapons.h`,
needed fewer declarations in `weapons.h` than every other file. Before
that, Beta 1.1's tree had an unused enum of 303 numbers and an unused
inline function of 159 lines in its place, about the same amount of both.
The include is in
[`dlls/weapons.h`](../../../blob/counter-strike/dlls/weapons.h#L18) from
Beta 1.1 on.

VC6 doesn't order operands that way, and the VC5 declaration stand-ins end
with Beta 4.1. A few VC6 functions do follow a count of declared names
modulo 64 ([vc6-notes.md](vc6-notes.md#reloads-at-a-loop-end-symbol-numbers)):
two constructors and `CRifleMenuPanel::Update` in the VGUI clients, hence
the names enum at the end of
[`cl_dll/hud.h`](../../../blob/counter-strike/cl_dll/hud.h#L948).

### Text that compiles to nothing

Where a file writes its own `.pch`, VC6 (and VC5) emits the headers' inline
functions lazily, as far as the compiler's intermediate code for the file's
own text has advanced
([vc6-notes.md](vc6-notes.md#text-that-compiles-to-nothing-still-counts)).
So text that makes no code still moves functions: an unused inline
function, statements behind a `return`, comments in VC5 (its intermediate
code spends two more bytes on a statement from line 128 on), the line
layout in VC6 (it keeps a record per source line with code). Where retail's
text had more of it than ours, the branch has an unused inline function of
empty statements, sized to the window that gives retail's order. That is
the commonest stand-in of the branch.

**`RotateVectorZ`.** Every retail client from Beta 6.5 on rotates a vector
in `EV_CreateSmoke` with pi written as 3.141517 (the double
`15 * ( 3.141517 / 180 )`, loaded twice). Valve's 1.6 client has that as an
inline function, `RotateVectorZ`, in `util_vector.h`. With the function in
[`cl_dll/util_vector.h`](../../../blob/counter-strike/cl_dll/util_vector.h#L125-L132),
at 1.6's line and with 1.6's parameters and locals, the large filler that
the VGUI clients had in `hud.h` (141 empty statements in the Betas, 145 in
1.0 and 1.1) shrank to 60 and 58, and 1.3 needs none. Nothing calls the
function (a call in `EV_CreateSmoke` gives other code); its statements are
ours, 402 bytes of intermediate code with the CD's compiler and 435 with
SP4's and later. That it was already a header inline in 2000 is an
inference.

### Dead code that only LINK counts

Without `/debug` LINK drops functions nothing calls and folds identical ones
([vc6-notes.md](vc6-notes.md#links-identical-code-folding)). To find
identical functions it sorts all of them, dead ones included, by size and
relocation count, and the copy it keeps follows from the order the sort
leaves. LINK also reserves two bytes of `.reloc` for every absolute fixup
in its input, dead functions included. So dead functions leave nothing in
the image but still decide which copies are kept and the size of `.reloc`.
Examples:

- **The VGUI clients (Betas 6.5 to 7.1).** With every function right, 27 of
  58 fold groups in Beta 6.5 kept another copy than retail, and retail's
  input had exactly one absolute fixup more than ours in all four versions.
  What fixed it, with names and places from Valve's 1.6 client: a dead
  `UTIL_TextureHit` in `ev_hldm.cpp` (256 bytes, three fixups), two dead
  stubs `UTIL_FindEntityByClassname` and `UTIL_PlayerByIndex` at the top of
  `hl_baseentity.cpp`, `UTIL_MakeVectors` in `hl_weapons.cpp`, and in Beta
  7.0 the class `CELITE` last in `weapons.h` (its class-body functions are
  folded away; only the kept copies show where it stood).
- **1.0's client:** 46 of 69 groups were wrong until two dead stubs,
  `CGrenade::Save` and `Restore`, stood in `hl_baseentity.cpp`. The 1.0
  server has the two functions; 1.6's client has the stubs at that place.
- **1.4's server**, the first linked without `/debug`: `CLocalNav::GetNode`
  declared `inline` (one dead copy fewer in the sort; 1.6's debug
  information marks it declared inline), and the dead `CLocalNav::LadderHit`
  written as the Linux code has it, 1,280 bytes with 25 absolute fixups,
  which made `.reloc` retail's size.
- **1.5's server:** the dead SDK function `NextLevel` must be 337 to 384
  bytes in the sort; the SDK's text is 325. Its last statement is a stand-in
  (seven inline calls that make no code).

### The number of declared names

From 1.0 on, the number of declared names matters: the compiler's intermediate code refers to every declared name by
its index, and an index from 0x8000 on (the 32,768th name) takes two bytes
more. A file that includes `windows.h` is near that border.
So the number of names declared before a file's own code changes the size
of its intermediate code, and with it where `hl_weapons.obj` puts the
headers' inline functions, without changing any machine code. 1.0's client
needs about 8,000 more names than this tree's text and VC6's own
`windows.h` declare: 7,888 to 7,902 give retail's layout. The branch
declares 7,896 in an enum in
[`dlls/extdll.h`](../../../blob/counter-strike/dlls/extdll.h#L44-L68). A
newer Platform SDK's `windows.h` on the build machine is the likely
source; that was not tested. The 1.3 server depends on the count too (about
7,830 to 13,000 give its file); the 1.4 server is retail's with 1 to 8,400.

### Two time stamps a second apart

LINK stamps the export directory early in a link and the COFF header (and
the debug directory) at its end. In five links retail's two differ by a
second: the servers of Beta 4.1 (21:42:30 and :31), 6.6 and 7.1, and the
clients of Beta 3.0 and 1.1. The script runs those links with a clock that
starts at the earlier second and advances 1 ms per read. Measured on Beta
4.1's link, LINK 5.00 reads the clock 500 to 999 times before it stamps the
exports and 1,500 to 1,999 times before the header, so the stepping clock
gives retail's two stamps. That the second changed during the link is our
reading of the two stamps.

### One file in two states

Beta 6.6's two DLLs come from two states of one shared file. The server,
linked on 2000-06-21, logs "%s planted the bomb" in `CC4::PrimaryAttack`
(`dlls/wpn_shared/wpn_c4.cpp`); the client, linked two days earlier, was
compiled before that line existed. The tree has the line, and the client's
build leaves it out. Beta 6.5d's commit is the earlier state, and Beta 7.0's
client has the line.

## Stand-ins

The trees mark stand-ins and retail's kept bugs with `// sic`
(`git grep -n -w sic cstrike-1.5` lists 73 lines in 28 files). Every
stand-in has a size, place or form that gives retail's bytes, measured with
real builds; the commit messages give the windows that work where there is
one. The kinds:

- **Declarations** (VC5, to Beta 4.1): unused typedefs and enums that stand
  for declarations retail's headers had.
- **Intermediate code**: unused inline functions of empty statements,
  comments and line layout, which place header inline functions.
- **Declared names**: `extdll.h`'s 7,896 names (from 1.0 on), and the
  enums of names at the end of `hud.h` in the VGUI clients (a count modulo
  64 orders some operands).
- **Steering code**: dead stores, dead locals or a dead block that use up
  the inliner's budget or change an operand order, where retail's text did
  something the code doesn't show.
- **Dead functions** the linker counts, of a given size and number of
  fixups.
- **Build history**: the edits and relinks of the server builds, the
  second compile of `wpn_c4.cpp` in 1.0 and 1.1.

Per release, from the commit messages (see each commit for the windows and
the evidence):

| Release (commit) | Server | Client |
|---|---|---|
| Beta 1.0 (`a29757a`) | Two unused typedefs, in `animation.h` and `multiplay_gamerules.cpp`: one declaration each, before `SetSequenceBox` and `GetNextBestWeapon` | none |
| Beta 1.1 (`26b28e7`) | 1.0's, and an unused enum of 11 in `weapons.h` ahead of `CBasePlayerItem` | Beta 1.0's file |
| Beta 2.0 (`06d374c`) | the earlier ones; an unused enum at the end of `cbase.h` (6 declarations every file sees); an unused inline in `client.cpp` ahead of `IsCloseToBase` (8 lines) | an unused prototype and a dead function in `health.h` and `health.cpp` |
| Beta 2.1 (`2d195a8`) | the inline in `client.cpp` 5 lines instead of 8 | Beta 2.0's file |
| Beta 3.0 (`b0635b4`) | `animation.h`'s typedef, the enums in `weapons.h` and `cbase.h`; `debris.cpp`'s one declaration (the file is in the Linux object list, its text is ours); the P90's and knife's armour ratios in `TakeDamage` written as multiples of `ARMOR_RATIO` (constant pooling) | unused declarations in `health.h` and `health.cpp`; two globals of `ammo.cpp` ahead of an `#include`; braces in `DrawPlayers` (the `.bss` order) |
| Beta 3.1 (`69f37fc`) | 3.0's | Beta 3.0's file |
| Beta 4.0 (`8f90f1a`) | declarations in `weapons.h` (11), `cbase.h` (9) and `animation.h`; intermediate code in `multiplay_gamerules.cpp` (a 42-line function), `client.cpp` (7 lines) and `combat.cpp` (a comment) | declarations in `health.h`, `health.cpp` and `message.cpp`; braces in `DrawPlayers` |
| Beta 4.1 (`059541f`) | 4.0's, with 8 declarations before `GetNextBestWeapon` and a 25-line comment in `weapons.cpp` (line numbers) | Beta 4.0's file |
| Beta 5.0 leak (`2683e38`) | the declaration stand-ins gone; unused inlines: 24 empty statements in `weapons.h` ahead of `CBasePlayerItem`, 3 in `multiplay_gamerules.cpp` | an unused inline in the client's headers |
| Beta 5.0 (`1cbc344`) | `weapons.h`'s as the leak; `multiplay_gamerules.cpp`'s gone | the inline, 15 statements |
| Beta 5.2 (`f8c1f94`) | `weapons.h`'s 24 statements | the client's unused inline |
| Beta 6.0 (`9760190`) | `weapons.h`'s 24. Kept as text, not stand-ins: eight unreferenced globals in `client.cpp` and an unused local in `FireBullets`, which the Linux library and 1.6 have | the inline; `MAX_MOTD_LENGTH` 245 and two 8-byte gaps in `CHudNVG` that nothing touches |
| Beta 6.1 (`9876677`) | 6.0's (6.1's objects don't show `weapons.h`'s) | Beta 6.0's file |
| Beta 6.5 (`f8291ba`) | 14 dead stores in `FireBullets3`, a dead block in `CHostage::IdleThink` (the inliner's budget); the place of `hostage.cpp`'s include | empty statements in `hud.h` (60), at the end of `CHudMoney::Draw` (85) and in `hl_weapons.cpp` (8); `RotateVectorZ`'s statements; 18 unused names at the end of `hud.h`; the dead functions LINK counts (names and places from 1.6) |
| Beta 6.5d (`18911c6`) | 6.6's; no Windows server of this state exists, so they mean nothing here | 6.6's |
| Beta 6.6 (`a43d0b4`) | 6.5's, and 8 empty statements at the end of `CHalfLifeMultiplay::InitHUD` | 6.5's, with 6 names at the end of `hud.h` |
| Beta 7.0 (`654725d`) | 26 empty statements at the end of `CHalfLifeMultiplay::Think`; in the new `CFuncVehicle::Next` 94 dead stores to 20 dead locals and a `goto`; `FireBullets3` and `IdleThink` as before | 6.5's, with 48 names at the end of `hud.h` |
| Beta 7.1 (`15d5377`) | 116 statements at the end of `Think` (with `trains.h` included ahead of `gamerules.h`, the only reading built); 42 dead stores in `CFuncVehicle::CollisionDetection`, a few in `Next` | 7.0's; `ammo.cpp`'s line layout in two functions as 1.6's |
| 1.0 (`e943424`) | `extdll.h`'s 7,896 names (the server doesn't depend on them); 154 statements at the end of `Think`; `IdleThink`'s dead block with one arm; the vehicle's as 7.1; `FireBullets3` needs none | `extdll.h`'s names; 58 statements in `hud.h`, 8 in `hl_weapons.cpp`, 85 in `CHudMoney::Draw`; one unused name at the end of `hud.h` |
| 1.0b (`9c64f22`) | 1.0's; a pointer local in `CBasePlayer::TakeDamage` (operand order) | 1.0's file |
| 1.1 (`08c7456`) | 122 statements at the end of `Think`; `IdleThink`'s block; parentheses in `Host_Say`'s flood test | 1.0's; 7 names at the end of `hud.h`; SDK 2.2's unused `GetScoreBoard` accessor in the viewport's header |
| 1.3 (`ffb6e62`) | gone: `Think`'s filler, `CollisionDetection`'s dead stores. New: `float *pdt = &dt;` in `StudioProcessGait` | `hud.h`'s statements gone; 26 statements in `hl_weapons.cpp`, 85 in `CHudMoney::Draw`, 46 names in `hud.h`, a 22-line filler in the viewport's header; two empty statements in `wpn_knife.cpp`'s `Swing`; a second spread loop in `EV_HLDM_FireBullets` that the optimiser removes |
| 1.4 (`3916799`) | six small ones: `Next`'s two dead locals, `PickNextVIP`'s `iCount = iCount + 1`, an empty test in `CHostage::MoveToward`, early returns in `CONSOLE_ECHO` and `CONSOLE_ECHO_LOGGED`, `GiveCTTouchBonus`'s local, a final `return` in `ShowMenu`. Gone: `IdleThink`'s, `pdt`, `Host_Say`'s | 1.3's; fillers in `hl_weapons.cpp` (68 statements) and `ev_hldm.cpp` (60), which only set the order of header inlines and prove nothing while the client is not identical |
| 1.5 (`5e4dafa`) | 1.4's, and `NextLevel`'s last statement | 1.4's |

The Beta 4.1 server entry and the Beta 6.0 client entry come from the
trees: those two commit messages don't list them. Retail's own bugs and
oddities, which the trees also mark `// sic`, are in the commit messages
too (for
example `FireBullets3`'s switch without breaks in Beta 3.0, which turns
bullet penetration off, and Beta 2.0's `mp_limitteams`, read under a
misspelt name).

## Beta 6.5d and the shared tags

**Beta 6.5d** (`18911c6`) is a Linux release: its package has Beta 6.5's two
Windows DLLs unchanged and a new `cs_i386.so` (SHA-256
`3e8a6dd8ea13f8d611774ef6985ef6cf22a534a8226bd1abe8576ffce1e5d16e`). That
library was built from a later source than 6.5: the tree on its way to 6.6.
The commit is that source: everything 6.6 changed (`CBasePlayer::VerifyModel`,
the reworked map votes, the larger score message, the weapons' changes, chat
every 0.66 seconds) but the changes of eight functions, which come with 6.6's
commit: seven functions' log lines and `ClientUserInfoChanged`'s name
filter.

The library itself was not rebuilt (no Linux compiler of the time is set
up). How the state is known:

- Valve's 6.5d and 6.6 Linux libraries, built by the same compiler with the
  same options, have 2,772 of 2,790 game functions byte for byte alike after
  masking addresses and 10 more instruction for instruction, and no function
  more or less. The other eight are 6.5d's functions with the new statements
  inserted, instruction by instruction.
- The tree against the Windows servers: every function but those eight is
  retail Beta 6.6's code, and six of the eight compile to retail Beta 6.5's
  bytes exactly. The other two carry more of 6.6's changes, so no Windows
  file has their 6.5d form.
- Beta 6.6's `client.dll`, linked on 2000-06-19, was built from this state:
  the tag `cstrike-beta6.5d` builds it byte for byte, as it is.

Not known: the day of the Linux build (the library has no time stamp), and
what the client's own files were on that day; `cl_dll` here is Beta 6.6's.
Beta 6.5's tag stays on Beta 6.5's commit, which builds the two DLLs of the
6.5d package.

**1.1b, 1.1c and 1.2** have their tags on 1.1's commit (`08c7456`):

- **1.1c** (2001-04-06): the package's `mp.dll`, `client.dll`,
  `cs_i386.so`, `liblist.gam` and `titles.txt` are 1.1's files byte for byte;
  only `readme.txt` differs (the version's hitbox fix is in the player
  models).
- **1.1b**: the archive lists it as included in 1.1's package.
- **1.2**: no binary of it was compared, because the archive has no package
  for it. Its list dates it 30 June 2001, calls it an engine update with no
  change from 1.1, and links 1.1c's package, so the tag is on the commit of
  the release those files come from. The archive's history goes from 1.1 to
  1.1c to 1.3 with no 1.2 state, and the change logs Valve shipped with 1.3,
  1.4 and 1.5 go from 1.1c (dated 4.6.01) to 1.3 (9.12.01) without a 1.2
  entry. If a 1.2 package turns up, compare its DLLs with 1.1's.

## The packed clients of 1.4 and 1.5

The `client.dll` of 1.4 and 1.5 is not a PE file but the engine's packed
form of one. Its layout follows **oxiKKK's decryptor**,
<https://github.com/oxiKKK/goldsource-dll-decryptor>, whose field names
`tools/packed_dll.py` uses; we checked it against the two retail files.

- 68 plain bytes shaped like a DOS header: 60 bytes of text (all zero in the
  retail files), the offset of the signature where `e_lfanew` would be
  (0x40), and the signature 0x12345678.
- Then everything else under a running XOR: the key starts at `W` (0x57),
  and after each byte the decoded byte plus `W` is added to it (modulo 256).
- Decoded, a 24-byte header: a number called `nRandom`, the number of
  sections minus one, the address of the export `F`, the image base, the
  entry point and the import directory, each but the first two disguised by
  a constant.
- One 20-byte record per section (address, size in memory, size in the file,
  offset in the file, a flag), then the sections' bytes.

What a packed file keeps of its PE file: every section but the relocations,
each cut to its real length, with all addresses moved to the header's image
base by the PE file's own relocation table. The retail files have three
sections (code, read-only data, data) at image base 0x1900000. The PE
headers and the relocation table are gone, and with them the Rich header
and the COFF time stamp: the link time survives only in the export
directory (1.4: 2002-04-19 06:25:51 UTC; 1.5: 2002-06-06 00:54:18 UTC).

**How identity is tested.** The tree builds a plain PE file;
`packed_dll.py pack` packs it at image base 0x1900000, and the packed file
is compared with retail's (SHA-256 and `cmp`). One value has to come from
the shipped file because nothing else determines it: `nRandom` (0x6035 in
1.4, 0x08D5 in 1.5; both below 0x8000, as a `rand()` result would be). The
packer moves the image by our relocation table, so a wrong table would show
as a difference.

**The round trip.** `packed_dll.py pe` writes a PE file around the decoded
sections (with headers of our own), and `pack` of that file with retail's
number gives both retail files back byte for byte. So `pack` writes the
format exactly as the retail files have it.

Not decidable from the files, and so our choice: whether Valve linked at
0x1900000 or their packer moved the image (1.3's client is at 0x10000000,
and the project keeps that default), and every PE header field.

Because the XOR key carries each byte into the next, one different byte
changes everything after it: `cmp` finds 612,222 of the 614,557 bytes the
two 1.4 files have in common different. Comparisons of the decoded sections say more.

## What is not identical in the 1.4 and 1.5 clients

Measured from fresh clones of the trees that the tags hold:

| | 1.4 | 1.5 |
|---|---|---|
| Packed file, ours | 614,605 bytes, `c2c6e9b895eba6d17ea9d206e36786893524fe0768e4299be7f6bcaf1f674d56` | 614,573 bytes, `e8b1191cfa8dc959f900d37fefe8de34c0dd65b84309becf63cd3937286d0009` |
| Packed file, retail | 614,557 bytes | 614,525 bytes |
| `.text`, ours and retail's | 0x707fe, 0x707de | 0x707de, 0x707be |
| `.rdata`, ours and retail's | 0x1b837, 0x1b827 | 0x1b837, 0x1b827 |
| Functions by name | 1,779 equal, 79 differ: the 18 below and 61 artefacts of the comparison, each checked against the linked image | 1,788 equal, 81 differ: the 18 and artefacts |

Already retail's in 1.4: every function of the C runtime (each of retail's
is in the build and each of the build's in retail), the import sets of all
five DLLs, the 47 static initialisers, the 493 exports and the export
table's size.

**The 18 functions** differ from retail only in the order of their
instructions:

- in `ev_hldm.cpp`: `EV_FireGlock1`, `EV_FireGlock2`, `EV_FireShotGunDouble`,
  `EV_FireShotGunSingle`, `EV_FireMP5`, `EV_FirePython`, `EV_FireGauss`,
  `EV_FireUSP`, `EV_HLDM_DecalGunshot`;
- `StudioFxTransform` in both renderers;
- `CHud::Init`, `CHudAmmo::Draw`, `GetWeaponAccuracyFlags`,
  `CHudHealth::DrawRadar`, `CHudSpectator::DeathMessage`,
  `EV_GetDefaultShellInfo`, and `LoadSteam` in the Steam wrapper.

Their sign: a `float` that a call returns and the code passes on as an
argument. The Processor Pack's back end stores it at once
(`fstp [esp+4]; add esp, 4`), and so do retail's 1.1 and 1.3 clients and the
1.4 and 1.5 servers (9, 9, 3 and 3 times). Retail's 1.4 and 1.5 clients
never do; they have `add esp, 4; ...; fstp [esp]` 9 times, all in
`ev_hldm.obj` and the two renderers. Functions equal to retail in other
objects keep the Processor Pack's order, so the cause is per object, not
the whole client. The client is still VC6's otherwise: SP5's C runtime
(479 of 482 functions), only VC6's exception tables, 865 calls of `__ftol`.

**What was tried** (on 1.4's text, which is 1.5's in every one of the 18):

- every VC6 back end we have: the CD's (12.00.8168), SP4's (8799), SP5's
  (8966), the Processor Pack's (13.00.9044);
- the first Processor Pack, of 2000 (back end 13.00.8943, found in the Xbox
  XDK 4361 package): code byte for byte 9044's;
- Visual C++ .NET 2002 (13.00.9466): its code calls `__ftol2`, which the
  client never does, 977 functions match by name against 1,747 with the
  Processor Pack, and it stores the value the old way;
- the 13.00 compilers of 2001 in the Windows XP DDK (9176) and the Xbox
  XDKs (9175, 9176, 9210, 9254): 13 to 29 functions equal in two probe files
  against 66 and 78, the old store order, and 9210 and 9254 call `__ftol2`;
- every single option and every pair of 25 code options, every triple of 17
  of them, and the back end's pass-through switches;
- 0 to 256 extra declared names, and counts up to 30,000;
- Valve's public `engine/APIProxy.h` and `cdll_int.h`, and 1.3's headers, in
  place of ours: no function's code changed.

What fits is that those objects were compiled by a back end close to
13.00.9044 that we don't have, on another machine or as a hotfix; an
incremental build across a compiler change would give the per-object
picture. That is a reading, not a finding.

Also open: the order of header inlines in three objects
(`hud_spectator.obj`, the viewport, `hl_weapons.obj`), 31 of 53 fold groups
in 1.4 that keep another copy than retail, 16 bytes of `.rdata`, and the
order of `vgui.dll`'s and KERNEL32's import entries.

The two releases are committed and tagged as usual, with the server
identical and the client not, and the measured distance in their commit and
tag messages. The 18 functions rule identity out with the compilers we
have, so no further stand-ins were written whose only use would be to come
closer to it. **The work on the image stopped there, by the maintainer's
decision.** The two client fillers of 1.4 (`hl_weapons.cpp`,
`ev_hldm.cpp`) set only the order of header inlines and prove nothing while
the client is not identical.

## Sources

Used:

- the retail binaries: the Windows DLLs of each release, and the Linux
  server libraries from Beta 3.1 on, for their symbols (names, parameter
  types, which functions and objects exist) and, where Windows dropped a
  function, its code;
- the releases' own data files (`titles.txt`, `liblist.gam`, the readme
  texts);
- Valve's Half-Life SDK text, from 1.0 (the base of the branch) to 2.3;
- Valve's later public repository, <https://github.com/ValveSoftware/halflife>,
  for headers a version's binary shows (tested on 1.4; nothing was taken
  into the tree);
- Valve's Counter-Strike 1.6 Linux libraries `client.so` and `cs.so`, which
  carry debug information: names, class members, locals, and the file and
  line layout. They are two to three years later than these versions, so
  each use was checked against the version's own binary.

Not used, at any point: ReGameDLL_CS, cs16-client, leaked source code, or any
other community recreation or decompilation of Counter-Strike. So the source
can be checked against the game itself: what it says about a release's
behaviour is what that release's binary does.

## Check it yourself

1. Get the retail files from
   [Ch0wW's archive](https://github.com/Ch0wW/counterstrike-betas) (the
   branch or commit in [the table above](#where-the-retail-files-come-from))
   and check them against [Status](#status), for example Beta 1.0's:

   ```sh
   git clone --depth 1 --branch csbeta10 https://github.com/Ch0wW/counterstrike-betas b10-retail
   sha256sum b10-retail/cstrk10/dlls/mp.dll b10-retail/cstrk10/cl_dlls/client.dll
   ```

2. Build and compare, from a checkout of `main`:

   ```sh
   git worktree add --detach ../cs-b10 cstrike-beta1.0
   RETAIL=/path/to/b10-retail/cstrk10 VC_TOOLCHAINS=/path/to/toolchains \
       tools/release/counter-strike.sh cstrike-beta1.0 ../cs-b10 ../cs-b10-build
   echo $?
   ```

   Expected: `identical to retail` and `cmp ...: no difference` for both
   DLLs, exit code 0. The server part prints a Rebuild All of 71 sources at
   `D:\half-life\code2`, link 1 at 1999-05-08 22:08:13 UTC, links 2 to 1211
   as relinks, four builds, and 1215 links.

3. Look at a server's debug record with GNU binutils, on retail's file and
   on yours:

   ```sh
   TZ=UTC objdump -p mp.dll | grep -E 'Time/Date|format NB10'
   ```

   Expected for Beta 1.0 (objdump prints the signature's bytes in file
   order):

   ```
   Time/Date		Fri Jun 18 21:24:28 1999
   Time/Date stamp 		376ab90c
   (format NB10 signature cdb53437 age 1214 pdb D:\half-life\code2\dlls\Releasemp\mp.pdb)
   ```

   For Beta 4.1 the header says 21:42:31 and the export directory's stamp is
   `383c5bc6`, a second earlier.

4. The packed clients, without building anything:

   ```sh
   python3 tools/packed_dll.py info cl_dlls/client.dll
   python3 tools/packed_dll.py pe cl_dlls/client.dll client-pe.dll
   python3 tools/packed_dll.py pack client-pe.dll client-again.dll 0x6035 0x1900000
   cmp cl_dlls/client.dll client-again.dll && echo identical
   ```

   For 1.4 (0x08D5 for 1.5). `info` prints the number 0x6035, three sections
   at 0x1901000, 0x1972000 and 0x198e000, and the export directory: 493
   functions, time stamp 0x3cbfb86f (2002-04-19 06:25:51 UTC). The round trip
   gives the retail file back.

5. See what a stand-in does: in a checkout of `cstrike-beta6.5`, give the
   unused inline function `b65_hud_unknown` in
   [`cl_dll/hud.h`](../../../blob/cstrike-beta6.5/cl_dll/hud.h#L803-L865)
   four more empty statements (64 instead of 60) and build the client
   (`... cstrike-beta6.5 ../cs-b65 ../cs-b65-build client`). `cmp -l` then
   counts 30,134 differing bytes, and with 59 statements 177; 60 to 63 give
   retail's file (measured with full builds of the tag).

The tools we used to compare partial matches are not in this repository:
function matchers, per-section image comparisons, and the models of the
compilers' inline placement and of LINK's folding sort.
For identical files, `cmp` is the whole test.
