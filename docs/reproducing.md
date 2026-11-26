# Reproducing a retail DLL byte for byte

A DLL built with Visual C++ 6 (VC6) holds more than its compiled code. It
also records where and when it was linked, which PDB file it wrote, how many
objects went into it, and the order in which the linker met things. Two
builds of the same source on two machines can differ in every one of these.

This page lists each part of the image that is not code, says how it was
reproduced for the first five mods in this repository, and what to look at
when a rebuild "differs from retail". Counter-Strike adds Visual C++ 5.0,
PDBs written by hundreds of links, releases that continue an earlier
release's build, and a packed client format; its page,
[counter-strike.md](counter-strike.md), describes those. Scientist
Slaughterhouse adds a client linked incrementally from the Debug
configuration, whose image records a history of links, the compile times
of its anonymous namespaces, the full source paths the IDE passed, browse
information, and an order of RTTI records that follows the lengths of the
compiler's own paths; its page,
[scientist-slaughterhouse.md](scientist-slaughterhouse.md), describes
those. Survive in Catacombs 1 and 2 need browse information in all four
DLLs ([below](#browse-information-fr-fr)) and add one tree that builds two
games ([survive-in-catacombs.md](survive-in-catacombs.md)). How the code itself was matched is in
[vc6-notes.md](vc6-notes.md). The compilers are described in
[compilers.md](compilers.md), the toolchain folder and Wine prefix in
[setup.md](setup.md), and each mod's own details in
[point-of-view.md](point-of-view.md), [t2def.md](t2def.md),
[wanted.md](wanted.md), [gunman-chronicles.md](gunman-chronicles.md),
[they-hunger.md](they-hunger.md), [counter-strike.md](counter-strike.md),
[scientist-slaughterhouse.md](scientist-slaughterhouse.md) and
[survive-in-catacombs.md](survive-in-catacombs.md).

Terms used below:

- **Object**: one compiled source file (`.obj`), or one member the linker
  loads from a library such as `LIBCMT.LIB`.
- **COMDAT**: a section that holds one function or one piece of data, so
  that the linker can drop it or keep one copy of it. VC6 puts every inline
  function, and with `/Gy` every function, into its own COMDAT.
- **PDB**: the program database, the file that holds the debug information.
  The DLL only names it.
- **Precompiled header** (`/YX`): a snapshot of the compiler's state after a
  run of `#include` lines, saved to `vc60.pch` and reused by the next file
  that starts with the same includes.
- **`/OPT:REF`** drops functions nothing references. **`/OPT:ICF`**
  (identical-code folding) keeps one copy of each set of functions with the
  same bytes. LINK turns both on by default, unless the link line has
  `/debug`.

## Contents

- [What the image holds](#what-the-image-holds)
- [The PDB path](#the-pdb-path)
- [PDB signature and age](#pdb-signature-and-age)
- [Time stamps](#time-stamps)
- [The Rich header and the object count](#the-rich-header-and-the-object-count)
- [Import table order](#import-table-order)
- [Precompiled header state: full builds and build sessions](#precompiled-header-state-full-builds-and-build-sessions)
- [Browse information (`/Fr`, `/FR`)](#browse-information-fr-fr)
- [Export name, output name and PDB name](#export-name-output-name-and-pdb-name)
- [`.bss` order](#bss-order)
- [The `.reloc` size](#the-reloc-size)
- [Other header fields and padding](#other-header-fields-and-padding)
- [How to verify a build](#how-to-verify-a-build)
- [Every stand-in in the build environment](#every-stand-in-in-the-build-environment)

## What the image holds

| Part | Where | Decided by | Section |
|---|---|---|---|
| COFF time stamp | PE file header | the clock when LINK ran | [Time stamps](#time-stamps) |
| Export directory: DLL name and time stamp | `.rdata` | the `.def` file's `LIBRARY`; the clock | [Export name](#export-name-output-name-and-pdb-name), [Time stamps](#time-stamps) |
| Debug directory and CodeView record (time stamp, PDB signature, PDB age, PDB path) | after the last section (servers only) | the build folder, the PDB's history, the clock | [PDB path](#the-pdb-path), [PDB signature and age](#pdb-signature-and-age) |
| Rich header | between the DOS stub and the PE header | every object LINK loaded, and which compiler made it | [Rich header](#the-rich-header-and-the-object-count) |
| PE header position (`e_lfanew`) | DOS header | the number of objects | [Rich header](#the-rich-header-and-the-object-count) |
| Import directory and thunks | `.rdata` | the link line's library order; LINK's sort and the C runtime LINK ran on | [Import table order](#import-table-order) |
| Order of functions inside each object | `.text` | the precompiled header each file found; the text of each function | [Precompiled header state](#precompiled-header-state-full-builds-and-build-sessions), [vc6-notes.md](vc6-notes.md) |
| Which copy of identical functions is kept | `.text` (clients) | LINK's sort of every foldable COMDAT, dead ones included | [vc6-notes.md](vc6-notes.md#links-identical-code-folding) |
| Order of uninitialised globals | `.bss` (no bytes in the file, but every reference to them is) | a hash of their names | [`.bss` order](#bss-order) |
| `.reloc` virtual size | section header | live and dead absolute fixups | [The `.reloc` size](#the-reloc-size) |
| Image version, base, alignment | PE optional header | the link line and LINK's defaults | [Other header fields](#other-header-fields-and-padding) |
| Padding | between and after sections | whatever was in LINK's memory | [Other header fields](#other-header-fields-and-padding) |

Retail's values for the ten DLLs of the first five mods:

| Server | Point of View `hl.dll` | T2DEF `t2def.dll` | Wanted! `wanted.dll` | Gunman Chronicles `gunman.dll` | They Hunger `einar.dll` |
|---|---|---|---|---|---|
| Compiler | VC6 RTM Professional | VC6 RTM Standard | VC6 RTM Professional | VC6 RTM Professional | VC6 RTM Professional |
| Link time (UTC) | 2003-05-06 07:56:13 | 2002-12-16 01:56:59 | 2001-06-05 13:13:19; export directory 13:13:17 | 2000-11-10 22:46:15 | 2001-03-26 05:43:20 |
| PDB path | `D:\Editors\Levels\POVSourceCode\dlls\Releasehl\hl.pdb` | `C:\hledit todesangst2\code\dlls\Releasehl\hl.pdb` | `K:\Wantedhl\Source Code\New Code\Code\dlls\Releasehl\hl.pdb` | `C:\Gunman\Source\dlls\Releasehl\gunman.pdb` | `C:\hlsrc11\dlls\Releasehl\hl.pdb` |
| PDB created | at the final link | 2002-12-10 04:33:33 | 2001-04-02 10:56:04 | at the final link | 2001-03-10 06:25:12 |
| PDB age | 1 | 15 | 18 | 1 | 73 |
| Export name | `hl.dll` | `hl.dll` | `hl.dll` | `hl.dll` | `hl.dll` |
| C++ objects in the Rich header | 142 | 113 (Standard edition) | 146 | 124 | 100 |
| Placeholder objects needed | 1 | 0 | 8 | 2 | 0 |
| Last build | full build | 3 files recompiled after it | 3 files recompiled after it | full build; 5 objects need per-file `.pch` stand-ins | the 51 files that include `weapons.h` recompiled after it |
| Identical-code folding | no (`/debug`) | no | no | no | no |
| Rebuild | identical | identical | 810 bytes differ, all in `CBot::Wander` | identical | identical |

| Client (`client.dll`) | Point of View | T2DEF | Wanted! | Gunman Chronicles | They Hunger |
|---|---|---|---|---|---|
| Compiler | VC6 RTM Professional | VC6 SP4 Standard | VC6 RTM Professional | VC6 RTM Professional | VC6 RTM Professional |
| Link time (UTC) | 2003-05-02 01:24:25 | 2002-12-06 05:06:23 | 2000-12-02 15:44:22 | 2000-11-12 19:45:26 | 2001-03-26 05:49:33 |
| Debug directory | none | none | none | none | none |
| C++ objects in the Rich header | 62 | 80 (Standard edition) | 61 | 78 | 61 |
| Placeholder objects needed | 1 | 0 | 0 | 1 | 0 |
| Identical-code folding | yes | yes | yes | yes | yes |
| Dead functions added to the source (dropped by LINK) | 1 | 0 | 1 | 57 placeholders, fitted by search (and an unused enum for the symbol count) | 0 invented; two dead stubs of SDK 2.2b's stub file are needed |
| Rebuild | identical | identical | identical | identical | identical |

Fix these parts after the code and data match, not before. Most of them are
cheap and independent of each other, but a time stamp or a PDB path is
worth nothing while a function before it is still one byte too long.

## The PDB path

The five server projects link with `/debug`. LINK then writes a debug
directory and a CodeView record: the tag `NB10`, the PDB's signature and age,
and the full path of the PDB as LINK saw it. The record sits after the last
section, so the length of the path also sets the size of the file. The
clients link without `/debug` and have no debug directory, so their build
path reaches no byte of the image.

To reproduce the path:

1. **Lay the source tree out at that path.** The release scripts build a
   tree of symbolic links into your checkout under the build folder (`OUT`).
   `dlls/` and `cl_dll/` are real folders of per-file links, so the objects,
   the DLL and the PDB land in the build folder and never in the checkout.
2. **Give the drive letter to Wine.** Wine maps each drive to a Unix folder
   through a link in the prefix's `dosdevices` folder. For `C:` the tree
   goes inside the prefix's own `drive_c` (T2DEF, Gunman Chronicles, They
   Hunger). For another letter (`D:` for
   Point of View, `K:` for Wanted) the script adds the link
   (`dosdevices/d:` pointing at the tree's root). This is done in a copy of
   the shared prefix made for each build (`cp -a --reflink=auto`), so the
   shared prefix stays untouched.
3. **Build in the project's own output folder.** `dlls\Releasehl` is the
   Release folder of the SDK's `hl.dsp`; all five mods kept it.
   `build_dsp.py --drive D:=/path/to/root` makes the tool write paths under
   that root as `D:\...`.

Only the part of the path that the record shows is evidence: the folders
down to `dlls\Releasehl`. The rest of the tree under the root (`engine\`,
`common\`, `pm_shared\`, ...) follows the SDK's layout because the projects'
`/I..\engine` options expect it. It doesn't reach the image.

## PDB signature and age

The CodeView record carries two numbers from the PDB itself:

- the **signature**: the time the PDB file was created;
- the **age**: how many links have written to it.

A fresh PDB has age 1 and the link time as its signature. Each later link
into the same PDB adds one to the age and keeps the signature. So the two
numbers are a short history of the build folder.

| Server | Retail record | How it is reproduced |
|---|---|---|
| Point of View | age 1, signature = link time | Delete the output folder, then a full build: the final link creates the PDB. An existing PDB gave age 2. |
| Gunman Chronicles | age 1, signature = link time | The same. |
| T2DEF | age 15, created 2002-12-10 04:33:33, six days before the final link | **1 + 13 + 1 links.** A full build with LINK's clock frozen at the creation time (age 1). Then 13 relinks at the same time: every object is up to date, so `build_dsp.py` only links (ages 2 to 14). Then the last build session (below). Then the final link at retail's link time (age 15). |
| Wanted! | age 18, created 2001-04-02 10:56:04, two months before the final link | **1 + 17 links.** After the full build and the last build session, the PDB and DLL are deleted. One link at the creation time starts a fresh `hl.pdb`, then 17 more links follow, the last at retail's link time (with the two-second export stamp below). |
| They Hunger | age 73, created 2001-03-10 06:25:12, about sixteen days before the final link | **1 + 72 links.** After the full build and the last build sessions, the PDB and DLL are deleted. One link at the creation time starts a fresh `hl.pdb`, then 72 more at retail's link time. |

What is known and what isn't: retail gives only the creation time, the
count of links and the time of the last one. The times of the links in
between are stand-ins; they don't reach the image. Where the last build
session falls among the links is not known either (links don't touch the
precompiled header, so the image is the same). The PDB's contents are not
part of the image and are not compared.

## Time stamps

LINK writes the current time into:

- the COFF file header (`TimeDateStamp`, at file offset `e_lfanew + 8`);
- the export directory;
- the debug directory (servers);
- the CodeView signature, when the final link creates the PDB (Point of
  View and Gunman).

The import descriptors' time stamps are 0 in all ten files (not bound).

**How:** only LINK runs under libfaketime, with the clock frozen at
retail's own `TimeDateStamp`:

    TZ=UTC FAKETIME_DONT_FAKE_MONOTONIC=1 faketime -f '2000-11-10 22:46:15' wine LINK.EXE ...

`build_dsp.py --link-time 'YYYY-MM-DD hh:mm:ss'` does exactly this. A time
without a leading `@` freezes the clock; `FAKETIME_DONT_FAKE_MONOTONIC=1`
leaves the monotonic clock alone, so only the wall clock is faked. The
compiler runs on the real clock: the objects' own time stamps don't reach the
image. To read retail's value, take the COFF header's `TimeDateStamp` and
convert it, for example `date -u -d @$((0x3a0c7ab7))`.

**Wanted's export directory is two seconds early.** Its export directory
says 13:13:17, the rest of the image 13:13:19. LINK writes the export file
(`hl.exp`, whose export directory goes into the image) early in a link, and
the image's header at the end; Wanted's final link took about two seconds
between the two. The release script reproduces it with a clock that moves:
libfaketime reads its time from a file (`FAKETIME_TIMESTAMP_FILE`, with
`FAKETIME_NO_CACHE=1`). The file says 13:13:17 when LINK starts, and a
watcher rewrites it to 13:13:19 as soon as `hl.exp` appears in the output
folder. Only the two stamps are evidence; "the link took two seconds" is our
reading of them. In the other nine files both stamps are equal.

## The Rich header and the object count

Microsoft's linkers since VC5 Service Pack 3 write an undocumented block,
the **Rich header**, between the DOS stub and the PE header. It has one
entry per (product id, build number) pair with a count, masked by a key
that is computed from the DOS header and the entries (so the key comes out
right when the entries do). The counts are:

- every object the compiler made, by compiler build and edition. Each
  object carries a `@comp.id` record; the product id comes from the
  compiler's back end, `C2.DLL`, so the Standard and Professional editions
  stamp different ids (C/C++ 0x15/0x16 against 0x0a/0x0b). Objects from VC4
  and VC5 carry no record and are counted as unmarked;
- every library member LINK loads: the C runtime's objects, assembler
  objects, and the alias objects of `OLDNAMES.LIB`. `stricmp` and `strcmpi`
  call the same function but load two different alias objects, so the
  spelling shows in the header (Point of View `2fbd2e5`: retail has 3 alias
  objects, `strcmpi` made 4);
- one entry per imported function (Import0), and the linker itself.

So the object inventory must match exactly, including objects that
contribute no code.

**The object count also moves the PE header.** LINK reserves room for the
Rich header from the number of objects. One object too few or too many
moves `e_lfanew` by 16 bytes, and with it every header byte after it. Seen:
Point of View's server 0xd8 with 141 C++ objects and 0xe8 with 142; its
client 0xe8 with 61 and 0xd8 with 62; Gunman's server 0xe8 with 122 and 0xe0
with 124. How the count maps to the size was not worked out; it is not
monotonic. The edition matters in the same way: built with Professional
stamps, T2DEF's server objects merged into the C runtime's entries, the
header had 7 entries instead of retail's 9, and only 92.4% of the header
bytes were right.

**Placeholder objects.** When retail counts more C++ objects than the code
shows, some source files compiled to nothing the image keeps: no code, no
data, no export. Their names, contents and places can't be recovered. An
empty source file stands in for each:

| DLL | Retail / from the code | Placeholders | Where | Commit |
|---|---|---|---|---|
| Point of View server | 142 / 141 | `dlls/placeholder_object.cpp` | last in `hl.dsp` | `a7a4d18` |
| Point of View client | 62 / 61 | `cl_dll/placeholder_object.cpp` | last in `cl_dll.dsp` | `a1f9d66` |
| Gunman server | 124 / 122 | `dlls/placeholder_object1.cpp`, `placeholder_object2.cpp` | last in `hl.dsp` | `f9caa39` |
| Gunman client | 78 / 77 | `cl_dll/placeholder_object.cpp` | before `vgui_HudID.cpp` | `0c1e33e` |
| Wanted server | 146 / 138 | `dlls/untraced1.cpp` ... `untraced8.cpp` | last in `hl.dsp` | `e66989c` |

Relinking the same objects with an empty one added changes only the Rich
header and `e_lfanew`; every section stays as it was (tested for Point of
View and Gunman). Where a placeholder goes still matters, through the
precompiled header: a source with no `#include` writes an empty `.pch`, and
every later file reuses it, which changes where their header inline
functions land. So:

- Point of View and Gunman's server put them last, where nothing comes
  after them.
- Gunman's client puts it before `vgui_HudID.cpp`, because retail's objects
  from `vgui_HudID` on show exactly that empty `.pch`.
- Wanted's eight include the same headers as `wtownb.cpp`, the file before
  them, so each reuses its `.pch`. Wanted's last build session starts from
  the `.pch` the full build leaves; an empty one would undo it.

T2DEF, Wanted's client and both They Hunger DLLs need none: their code
accounts for every object. For They Hunger the counts are exactly those of
Valve's own file lists, which is how we know the mod added no source file.

## Import table order

The import tables have two orders, and they come from different places.

**1. The order of the DLLs** comes from the link line. LINK resolves
externals against the import libraries in link-line order, pass by pass.
The game's own objects are resolved first; the C runtime's references
(`kernel32` through `LIBCMT`) come in a later pass. Gunman's client imports
winmm, vgui, wsock32, user32, kernel32. The objects reference the first four
directly, so `user32.lib` came after `wsock32.lib` on Gunman's link line;
`cl_dll.dsp` now says so (`790ca3e`). Relinking there with either C runtime
under LINK gave the same table: it is the link line, not the sort.

**2. The order of the functions within one DLL** comes from a sort. LINK
orders them with the C runtime's `qsort`, and equal keys come out in
whatever order that `qsort` leaves them. The VC6 CD's `MSVCRT.DLL`
(6.00.8168, 1998) and Wine's builtin `msvcrt.dll` break ties differently.
Wine always loads its builtin `msvcrt.dll`, even with a native copy next to
the program, so the toolchain runs LINK from a copy whose import is renamed
to `MSVCRX.dll` and installs the CD's runtime under that name
([setup.md](setup.md)). The retail files disagree on which runtime their
linker had:

| DLL | With the CD's runtime | With Wine's builtin | Retail's linker ran on |
|---|---|---|---|
| Point of View server | retail's order | `TlsGetValue` and `CreateDirectoryA` swapped (7 bytes of `.text`, 4 of the import tables) | a 1998-style `qsort` |
| T2DEF server | `TlsAlloc` and `CreateDirectoryA` swapped (17 bytes) | retail's order | some later `msvcrt`, which breaks ties like Wine's |
| Gunman server | retail's order | 17 bytes of `.rdata` and 7 of `.text` differ | a 1998-style `qsort` |
| Gunman client | retail's order (after `790ca3e`) | the same table | either |
| Wanted server and client, Point of View client, They Hunger server | retail's order | not recorded | |
| They Hunger client | retail's order | functions change places inside two DLLs' lists: 14 of `vgui.dll`'s 542 thunks, 11 of KERNEL32's 65 | a 1998-style `qsort` |
| T2DEF client | links on Service Pack 4's own `MSVCRT.DLL`: retail's order | not recorded | |
| Survive in Catacombs server | retail's order | not recorded | a 1998-style `qsort` |
| Survive in Catacombs client | retail's order | the import thunks in another order: 299,634 bytes differ | a 1998-style `qsort` |
| Survive in Catacombs 2 server | `CreateDirectoryA` (slot 0) and `LCMapStringA` (slot 37) of KERNEL32's 75 swapped (15 bytes) | retail's order | some later `msvcrt`, which breaks ties like Wine's |
| Survive in Catacombs 2 client | 298,770 bytes differ: the order inside all five DLLs' lists, 492 export addresses, the kept copies | retail's order | the same |

So T2DEF's server and Survive in Catacombs 2's two DLLs build with their
tools' LINK on Wine's builtin runtime, T2DEF's client on Service Pack 4's
`MSVCRT.DLL` (6.00.8797), and the other ten DLLs on the VC6 CD's.
Survive in Catacombs 2's tree had moved to another drive since the first
game, which fits a new machine. The same `qsort` also
decides which copy of identical functions LINK keeps
([vc6-notes.md](vc6-notes.md#links-identical-code-folding)). Only Wine 11.0
was tested; another Wine version could break ties in its builtin
differently, which would change those three DLLs.

## Precompiled header state: full builds and build sessions

All the mods' projects use `/YX`, the VC6 IDE's automatic precompiled
header. All files of a project share one `vc60.pch`. Before each file the
compiler compares the `.pch` with the file's list of `#include`s and reuses,
shrinks or rebuilds it. So each file sees whatever `.pch` the previous
compile left.

**Why it matters for the bytes.** Whether a header is inside the `.pch` the
file found, and whether the file created that `.pch` or reused it, decides
where the header's inline functions (vtable-only virtuals, implicit
constructors and assignment operators, template members) land inside the
file's object: right after the first function that needs them, spread
between the file's own functions, or at the end. That decides the order of
`.text`, which moves every address after it, and in the clients which copy
of identical functions LINK keeps. The exact rule is in
[vc6-notes.md](vc6-notes.md#where-header-inline-functions-land).

What VC6 does with the shared `.pch`, measured with real compiles:

- A file whose include list extends the current `.pch` reuses it; the
  headers after it are compiled normally.
- Otherwise the `.pch` shrinks to the common prefix if that is at least 70%
  of the old list (7/10, 3/4 and 10/13 shrink; 2/3, 5/8 and 9/13 don't), and
  is rebuilt from the file's whole list if not.
- Code before an `#include` ends the `.pch` region, and so does an `#if`
  there: Valve's `#if !defined( OEM_BUILD )` before `squeakgrenade.cpp`'s
  includes makes VC6 rebuild the `.pch`.
- A file with no `#include` leaves an empty `.pch` that every later file
  reuses (Survive in Catacombs 2's emptied `agrunt.cpp`, the project's second
  source: every source up to `player.cpp` reuses it, which places three
  objects' inline functions as retail has them). A file whose first
  directive is `#pragma hdrstop` and whose code is all inside a conditional
  that is off does the same: the pragma ends the `.pch` region before the
  conditional (Survive in Catacombs' one tree for both games).
- A `.pch` is reused only with the same `/Fd` PDB (`vc60.pdb`); otherwise
  VC6 rebuilds it without a word.
- A `.pch` is stale once a header inside it has a newer time stamp, and the
  next file rebuilds it. Measured on They Hunger's `combat.cpp`: compiled
  twice, the second compile reuses the first one's `.pch`; with a new time
  stamp on `weapons.h`, which is in that `.pch`, it rebuilds it; with a new
  time stamp on `doors.h`, which is not, it still reuses it. When the stale
  `.pch`'s list is a prefix of the file's own, the rebuilt `.pch` keeps that
  shorter list (They Hunger's client `hl_weapons.cpp`).

**Full build.** The compiler runs over the sources in project order, which
is also the link order. Each `.pch` is VC6's own decision, so nothing needs
to be modelled. Every release build here starts with a full build into an
empty output folder (`build_dsp.py --full`).

**Incremental build.** After an edit, the IDE recompiles only the changed
files, and each of them sees the `.pch` the previous compile left, often
the one from the end of the last full build. Its objects can carry
`.pch` states that no full build gives. Two things in the image show it:
a PDB age above 1 with a creation time before the final link, and objects
whose inline placement no full build reproduces. `build_dsp.py`'s own
incremental builds replay `/YX` from a model (it rebuilds the `.pch` from
the include list the IDE would have had). The model is close but not exact:
an incremental build after a header edit once lost 6 of Point of View's
server functions that a full build of the same tree matched. Use full
builds for anything you compare.

**Replayed build sessions** (T2DEF, Wanted, They Hunger and Survive in Catacombs servers). After the full
build, the release script recompiles a short list of files with plain
`CL /c`, the project's flags, and the same `/Fp` and `/Fd` as the build, so
the real compiler decides each `.pch` from the one the previous compile
left. No model is involved. Then come the links (previous section).

- **T2DEF:** `sound.cpp`, `teamplay_gamerules.cpp`, `weapons.cpp`, in that
  order. Retail's `teamplay_gamerules.obj` ("HL Teamplay" first among its
  strings) and `weapons.obj` are what VC6 makes when those files find a
  `.pch` of `extdll.h`, `util.h`, `cbase.h`. A full build never gives them
  that: `squeakgrenade.cpp`'s `#if` and `voice_gamemgr.cpp`'s own header
  first make VC6 rebuild the `.pch` earlier. The full build ends with
  `zombie.cpp`'s `.pch`; `sound.cpp` rebuilds it from its three headers
  (`sound.obj` comes out as in the full build), and the other two reuse it.
  Three compiles are the fewest. `alias`, `arcade`, `h_export`, `lights`,
  `trigger_music` or `securitycam` in `sound.cpp`'s place give the same
  image; which file T2DEF's author recompiled, and before which link, is
  not known. This replaced a project-file stand-in (`8314fc9`, reverted by
  `0833c0e`).
- **Wanted:** `bot.cpp`, `colonel.cpp`, `combat.cpp`. Retail's
  `colonel.obj` (`CTalkMonster::DeclineFollowing` late), `combat.obj`
  (`CGib::ObjectCaps` between the two `FInViewCone`s) and `bot.obj` (the
  player's inline virtuals spread over the file) only come out with `.pch`
  states no full build gives. This is one consistent history, not the only
  one. `bot.obj` also needs IL that compiled to no code (an unused inline
  function in `bot.cpp`, `ca08901`; see
  [vc6-notes.md](vc6-notes.md#text-that-compiles-to-nothing-still-counts)).
- **They Hunger:** a header edit, not a list of files. `weapons.h` gets a
  new time stamp, and the 51 sources that include it are recompiled in
  project order, `agrunt.cpp` first; then the same again. Retail's
  `agrunt.obj`, `barney.obj`, `client.obj`, `combat.obj` and `effects.obj`
  have their header inlines where VC6 puts them when the `.pch` holds more
  of the file's include list than a full build gives it: `agrunt.cpp` needs
  at least its first 7 headers in it and gets 4, `barney.cpp` at least 5 and
  gets 4, and `client.cpp`, `combat.cpp` and `effects.cpp` create the `.pch`
  from their own lists where a full build has them reuse one of 3 headers.
  We compiled every source in every `.pch` state it can meet (1,430
  compiles): 75 objects are the same in every state, a full build gives 15
  of the other 21 as retail has them, and a rebuild of `weapons.h`'s
  dependents that starts without a usable `.pch` gives those five and keeps
  the 15. `weapons.h` is the only single header that does. The second
  rebuild is there so that its first file finds a stale `.pch`: the full
  build leaves `zombie.cpp`'s, which doesn't hold `weapons.h`, and the first
  rebuild's `agrunt.cpp` would reuse it. Known is only that the last build's
  first file found no usable `.pch`. One more object, `hl_wpn_glock.obj`,
  fits in no state and needs IL that compiled to no code (an unused inline
  function in `weapons.h`, `0aab512`).
- **Survive in Catacombs 1 and 2:** the same kind of header edit, once.
  `weapons.h` gets a new time stamp and its dependents are recompiled in
  project order (51 sources in the first game, 49 in the second), the first
  of them finding the `.pch` `zombie.cpp` left, which doesn't hold
  `weapons.h`. That gives `barney.obj`, `h_cycler.obj` and
  `handgrenade.obj` their inline functions where retail has them, and keeps
  every other recompiled object as retail has it. An edit of `effects.h`
  gives the same images; both mods changed `weapons.h`, neither `effects.h`
  ([survive-in-catacombs.md](survive-in-catacombs.md)).

**Per-file `.pch` stand-ins** (Gunman). Five server objects show other
states in retail: `ggrenade`, `demoman`, `banditgrunt` and `friendlygunman`
were compiled with a `.pch` of their whole include list, and `microraptor`
with one that lacks `squadmonster.h`. No history of up to three incremental
builds fits all five; a real three-step build fixed four of them and moved
three other objects. So `hl.dsp` gives those files their own `.pch`
(`# ADD CPP /Fp".\Releasehl\<file>.pch"`, and `/YX"schedule.h"` for
`microraptor.cpp`, `bd5439f`). `build_dsp.py` gives a file with its own
`/Fp` a fresh `.pch` at each compile and leaves the shared one alone.
Gunman's client does the same for four weapon objects (`gausspistol`,
`minigun`, `shotgun`, `spchemicalgun`; `f8ffba2`). These lines are stand-ins
for build history: Gunman's own project didn't have them, and retail's PDB
(age 1) doesn't show which builds came before.

Point of View needs neither: a full build gives every object, including
`ammo.obj`, whose file creates the client's `.pch`.

## Browse information (`/Fr`, `/FR`)

Spirit of Half-Life's projects, on which both Survive in Catacombs games
are built, ask for browse information: `/Fr` in the server's Release
configuration, `/FR` in the client's. Its author's build logs, shipped with
its source, show the IDE passing them with the output folder
(`/Fr".\Releasehl/"`, `/FR".\Release/"`). Scientist Slaughterhouse's
client needs its project's `/FR` too
([scientist-slaughterhouse.md](scientist-slaughterhouse.md)). Other mods'
projects have the option as well, and they rebuild identical without it;
their scripts leave it out. But it is not only bookkeeping for the IDE: it adds to the compiler
front end's intermediate code, and so it can move where VC6 emits the
header inline functions it places lazily
([vc6-notes.md](vc6-notes.md#where-header-inline-functions-land)), and
change register choice and instruction order inside functions.

Survive in Catacombs' four DLLs are retail's only with it. Without it, in
the first game's server, `CBasePlayer::IsAlive` lands elsewhere in
`client.obj`, `FireBulletsPlayer` makes its two random-seed calls in the
other order and 4 bytes of `.data` differ: 857 bytes in all, with the build
session above; in its client, `CClassMenuPanel`'s constructor gets other
registers and four functions of `hl_weapons.obj` differ: 1,812 bytes.
`build_dsp.py --browse-info` keeps the project's option and writes the
`.sbr` files to the output folder, as the IDE does; without it the option
is dropped.

## Export name, output name and PDB name

The export directory names the DLL. That name comes from the `.def` file's
`LIBRARY` line, not from the file LINK writes. All five servers export as
`hl.dll` (`LIBRARY hl`), although they ship as `hl.dll`, `t2def.dll`,
`wanted.dll`, `gunman.dll` and `einar.dll`. When `/out` names another file, LINK warns
`LNK4070: /OUT:hl.dll directive in .EXP differs from output filename`; that
is expected. T2DEF's base, Spirit of Half-Life, said `LIBRARY spirit`;
retail's export name gave `LIBRARY hl` (`3c83e84`). Survive in Catacombs'
servers, also built on Spirit of Half-Life, kept it: both export as
`spirit.dll`.

The output file name itself is not written into the image, except through
the PDB's name: LINK names the PDB after the output unless the link line has
`/pdb:`.

- **Gunman:** `/out:".\Releasehl\gunman.dll"`, so `gunman.pdb`, as in
  retail.
- **Point of View:** `hl.dll` and `hl.pdb`.
- **T2DEF:** retail's DLL is `t2def.dll` (the name in `liblist.gam`) but its
  PDB is `hl.pdb`. Either the project set `/pdb`, or its author built
  `hl.dll` and renamed it; which one is not known. The branch keeps
  `/out:".\Releasehl\t2def.dll"` and adds `/pdb:".\Releasehl\hl.pdb"`
  (`d2e6f35`). `build_dsp.py` honours a project's own `/pdb`, so the project
  as written gives retail's file.
- **Wanted:** linked as `hl.dll` (so `hl.pdb`) and renamed to `wanted.dll`
  afterwards.
- **They Hunger:** the same: linked as `hl.dll`, the project's own output
  name, and renamed to `einar.dll`.
- **Survive in Catacombs:** linked as `spirit.dll` (so `spirit.pdb`), the
  project's own output name, and shipped renamed `sic.dll`. Survive in
  Catacombs 2's project links `bloodbath.dll`, as shipped, so
  `bloodbath.pdb`.

The export list itself comes from the source (`EXPORT` members, the `.def`
file). One detail that surprises: VC6 exports a string literal (`??_C@...`)
when its first use is inside an `EXPORT` member defined in the class body
(Gunman `320d5ba`, 17 exported literals).

## `.bss` order

Uninitialised globals take no bytes in the file, but every instruction that
uses one holds its address, so their order shows. VC6 doesn't lay them out
in declaration order. It walks a 1024-bucket hash table of their names:

```python
def bss_bucket(name):
    h = 0
    for c in name.encode():
        h = (h * 4 + (h >> 4) + c) & 0xffffffff
    return (h ^ (h >> 16)) % 1024
```

- Uninitialised data comes first, in ascending bucket order; inside a
  bucket the latest declaration comes first.
- Then zero-initialised data (`= 0`, `= NULL`), in definition order.
- The name hashed is the plain identifier for file-scope data, `static` or
  not, and the full decorated name for function statics and static class
  members, for example `?last_time@?1??Think@CHalfLifeTeamplay@@UAEXXZ@4HA`.
- Class objects of 8 bytes or more are 8-aligned. An unreferenced `static`,
  or a `const` object with an empty constructor, gets storage only if the
  object has other `.bss`.

This rule is exact: it predicted a ladder of 3,072 test names, 42 random
sets of 30 names, and every object's `.bss` in Wanted's server (worked out
from the hash loop in the compiler's front end and black-box tests).

So every global whose name the reconstruction made up must have a name that
lands where retail has it. Examples:

- T2DEF `489cd88`: `g_iT2DefMusicOn` lands in retail's place,
  `g_iT2DEFMusicOn` doesn't.
- Point of View `d05f31b`: three globals in `dlls/globals.cpp`.
- Wanted `77d79d0`: zero initialisers where retail keeps definition order,
  and a class renamed (`CHalfLifeCTC`), because the decorated names of the
  function statics `last_frags` and `last_time` contain the class name.
  With `CCaptureTheChicken` they hash to buckets 95 and 299 (`last_frags`
  first); with `CHalfLifeCTC` to 529 and 11.
- Gunman `ccaf4db`: holes of 4 bytes that nothing in retail reads are
  `static int` placeholders (names ours, marked `// sic`).
- They Hunger `e90c20f`: the client's five gun objects are globals whose
  names retail doesn't give. Their addresses give each one a window of
  buckets between the SDK's globals (below 60; 60 to 171; 172 to 519; 738 to
  818; above the previous one, up to 819), and the names were chosen to hash
  into them (`g_Chain` 4, `g_Ap9` 137, `g_Hkg36` 354, `g_ClTaurus` 753,
  `g_TfcSniper` 817). No naming scheme we tried fits all five, so retail's
  names are not known. In the server (`1e4cdca`), the gun counters of the
  two helicopters are function statics; named `cnt`, in `CApache::FireGun`
  and in the `FireGun` of a class named `CEinarBoss`, they come in retail's
  order.

To test candidate names, compute the buckets, or compile the file with the
candidates and read the order of the symbols in the object. One compile can
test many candidates, since the order is pairwise.

## The `.reloc` size

`.reloc` lists every absolute address in the image (dir32 fixups) so that
Windows can move the DLL. Its contents and the data directory's size count
only the code that is in the image. The section header's virtual size does
not: LINK sizes the section before it drops unreferenced COMDATs. It is a
constant plus 2 bytes for each dir32 fixup of every COMDAT, live or dead.

This only matters for the clients: the servers link with `/debug`, which
turns `/OPT:REF` off, so their dead code stays in the image anyway.

So when the live code is identical and the virtual size is 2n bytes short,
retail's dead code had n more absolute fixups than yours. It is a measure of
code that is not in the image:

- **Point of View client:** 0x7e6a with a fixup-free dead function, 0x7e70
  with an earlier guess that had three, retail 0x7e72: the missing dead
  function had four (`469339a`, the placeholder `__pov_unknown_stub` in
  `cl_dll/hl/hl_weapons.cpp`).
- **Wanted client:** equal, so its dead placeholder (a 16-byte empty
  function, `HUD_WeaponsUnknown`, `5f7d773`) needs none.
- **Gunman client:** 160 bytes short with the live fixups equal: 80 dir32
  fixups in dead code, now in three of the client's 57 placeholder
  functions (`6b5c736`).
- **They Hunger client:** equal (0x8824), with our guess for the dead
  weapon-prediction code of the five guns in place: 15 absolute fixups, two
  per `HUD_PrepEntity` line and one per `case`. So retail's dead code had
  that many, and the text its `hl_weapons.cpp` had beyond ours had none: a
  probe function with three made the size 0x882a.

Check the live code first. Wanted's server was 2 bytes short too; the cause
was a live function's missing third read of `gpGlobals` (block padding hid
it in the written directory), not dead code (`3ae2513`).

A related tell: a floating-point constant (`__real@...`) or a string
literal that a dead function uses stays in the image if live code uses the
same value, at the dead function's object, because LINK keeps the first
definition. Gunman's client has the double 3.0 among `ev_hldm.obj`'s
constants although only a later object uses it, which places a dead
function there.

## Other header fields and padding

- **Image version** comes from the link line's `/version`: Wanted's server
  says 1.1 (`25f618f`).
- The rest of the optional header (image base 0x10000000, 4 KB file and
  section alignment, checksum 0, OS and subsystem 4.0) is LINK 6.00's
  default for these link lines.
- **Padding.** LINK doesn't write every byte between and after the
  sections; those keep whatever was in memory. Wine's memory is zeroed, and
  all ten retail files have zero padding, so this needed nothing. It is
  not a given: Sweet Half-Life's server, another VC6 DLL, keeps one byte of
  leftover memory after its debug directory.

## How to verify a build

Each release script builds and checks in one go:

    VC_TOOLCHAINS=/path/to/toolchains tools/release/<mod>.sh SRC OUT [server|client|both]

For each DLL it prints the path of the built file, its SHA-256, retail's
SHA-256 (built into the script), and `identical to retail` or `differs from
retail`. With `RETAIL=/dir` set it also runs `cmp` against the retail file
there; `RETAIL` is a per-mod folder with the mod's own `dlls/` and
`cl_dlls/`, as in the game folder. The exit code is 0 only if every built
DLL is identical. The per-mod pages list the commands and hashes.

By hand:

    sha256sum ours.dll retail.dll
    cmp ours.dll retail.dll && echo identical
    cmp -l ours.dll retail.dll | wc -l     # how many bytes differ
    cmp -l ours.dll retail.dll | head      # where: 1-based decimal offsets, octal values

Map the offsets to sections with the section table (`objdump -h` from a
binutils with PE support, or any PE viewer), then look up the usual cause:

| What differs | Usual cause |
|---|---|
| 4 bytes at `e_lfanew + 8` and 4 in the export directory | link time not set |
| the export directory's time stamp only | a final link that took longer (Wanted's two seconds) |
| debug directory and CodeView record; the file size by a few bytes | PDB path (its length), signature or age |
| `e_lfanew` and every header byte after it | object count (placeholders), or a compiler edition that merges Rich entries |
| the Rich header alone | object inventory: an alias object (`stricmp`/`strcmpi`), the C runtime's build, the compiler's build or edition |
| a few bytes of import thunks, plus one byte in every call through them | function order in an import DLL: the C runtime LINK ran on; or DLL order: the link line |
| `.text` from one point on, everything shifted | a function before it has the wrong size, or an object's section order differs (`.pch` state, lazy inline emission), or (clients) LINK kept another copy of identical functions |
| relocated operands that point into `.bss` | `.bss` order: names |
| `.rdata`/`.data` pointers only | they point at code that moved; fix the code |
| the `.reloc` virtual size only | the number of absolute fixups in dead code |
| one byte of padding | memory left over on the original machine; Wine can't reproduce it |

A small difference is not always a small cause: one byte of `.text` too
many in an early function shifts everything after it, and the count of
differing bytes jumps from a few to hundreds of thousands. Find the first
differing offset in `.text`, then the function that holds it.

## Every stand-in in the build environment

What the release builds assume beyond the source, per DLL. The per-mod
pages also list the stand-ins inside the source (placeholders and `// sic`
lines).

| DLL | Stand-in | What is known |
|---|---|---|
| all servers | the tree under the root, except the folders the PDB path names | only the PDB path is attested |
| all | Wine 11.0, a fresh win64 prefix | other versions not tested; T2DEF's server and Survive in Catacombs 2's DLLs depend on Wine's builtin `qsort` |
| Point of View, Gunman servers | a fresh PDB at the final link | follows from age 1 and the signature |
| T2DEF server | 13 relinks at the PDB's creation time; the session before the final link; `sound.cpp` as the session's first file | only the count, the creation time and the final time are known; six other files work in `sound.cpp`'s place |
| T2DEF server | tools on Wine's builtin `msvcrt.dll` | T2DEF's linker broke `qsort` ties like it; which runtime it was is unknown |
| Wanted server | 16 links at the final time between the first and the last; `bot.cpp`, `colonel.cpp`, `combat.cpp` as the session | only the count and the two end times are known; the session is one consistent history |
| Wanted server | the export stamp two seconds before the image | the two stamps are retail's; the moving clock is our way to get them |
| They Hunger server | 71 links at the final time between the first and the last | only the count and the two end times are known |
| They Hunger server | the last builds: `weapons.h`'s 51 dependents recompiled twice | known: five objects were compiled against fuller `.pch` states than a full build gives, and the last build's first file found no usable `.pch`; the first of the two rebuilds only makes the `.pch` stale for the second |
| Gunman server and client | per-file `/Fp` (and one `/YX"schedule.h"`) in the project files | five server and four client objects show other `.pch` states; the history that gave them is not found |
| Survive in Catacombs servers | 4 (first game) and 6 (second) links at the final time between the first and the last | only the count and the two end times are known |
| Survive in Catacombs servers | the last build: `weapons.h`'s dependents recompiled once | known: `barney.obj`, `h_cycler.obj` and `handgrenade.obj` (in the second game also the other dependents the empty `.pch` had moved) were last compiled in `.pch` states a full build doesn't give; an `effects.h` edit gives the same image |
| Survive in Catacombs 2 server and client | LINK on Wine's builtin `msvcrt.dll` | its `qsort` breaks ties as the original linker's did; which runtime that was is unknown |
| Point of View, Gunman, Wanted | placeholder objects | the count is retail's; names, contents and places are not |
| all other DLLs | LINK on the VC6 CD's `MSVCRT.DLL` (T2DEF's client: Service Pack 4's) | gives retail's import order and kept copies; which runtime the original linkers ran on is not known |
| Scientist Slaughterhouse server | 1,431 links at the final time between the first and the last; the last session (`flyingobject.h` and `game.h` edited, six sources recompiled once or twice) | only the count and the two end times are known; the session is one history that gives each object retail's `.pch` state |
| Scientist Slaughterhouse client | 456 relinks that only raise the PDB's age; the session before the full link; an earlier text of `sword.cpp` until the full link (shipped in `tools/release/scientist-slaughterhouse/`) | the PDB's creation, the full link's and the last link's times and the age are retail's; the July `sword.cpp` is a guess that gives its two data contributions of retail's sizes |
| Scientist Slaughterhouse, both | the compiler at `C:\Microsoft Visual Studio\VC98` and `TMP` at `C:\DOCUME~1\ADMINI~1\LOCALS~1\Temp` | the install root fits the source tree's path (`...\MyProjects\Source Code`), the `TMP` path is a guess (Windows 2000's and XP's for the Administrator); only the lengths reach the image, through the client's RTTI order, and the source's stand-in pads are tuned to them |
