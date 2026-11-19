# Compilers

The ten DLLs of the first five mods and Scientist Slaughterhouse's two were
built with Microsoft Visual C++ 6.0 (VC6), in three variants: the Professional edition as first released, the
Standard edition as first released, and the Standard edition with Service
Pack 4. Counter-Strike's 40 DLLs, from 1999 to 2002, span five compilers:
Visual C++ 5.0 as first released, then VC6 as first released, with Service
Pack 4, with Service Pack 5, and with Service Pack 5 and the Processor Pack;
[Counter-Strike](#counter-strike) has them. This page says which variant
built which DLL, how that is known, where each variant comes from, and which
files of it matter. [setup.md](setup.md) turns the discs into the toolchain
folder.

A VC6 compile runs four programs. `CL.EXE` is the driver: it reads the
options and calls the passes. `C1.DLL` (C) or `C1XX.DLL` (C++) is the front
end, which parses the source. `C2.DLL` is the back end, which generates and
optimises the code. `LINK.EXE` then links the objects and libraries into the
DLL. The edition shows in the back end: the Standard edition's `C2.DLL`
ignores optimisation options, so `/O2` gives the same code as `/Od`, and it
stamps its objects as Standard edition objects.

## Summary

| DLL | VC6 variant | `CL` | `C1`, `C1XX` | `C2` | `LINK` | C runtime library | Runtime LINK ran on | Tree |
|---|---|---|---|---|---|---|---|---|
| Point of View `hl.dll` | Professional, no service pack | 12.00.8168 | 12.00.8168 | 12.00.8168 | 6.00.8168 | `LIBCMT.LIB` of the VC6 CD | the CD's `MSVCRT.DLL` 6.00.8168 | `vc6` |
| Point of View `client.dll` | the same | | | | | the same | the same | `vc6` |
| T2DEF `t2def.dll` | Standard, no service pack | 11.99.8168 | 12.00.8168 | 11.99.8168 | 6.00.8168 | the CD's `LIBCMT.LIB` | Wine 11.0's builtin `msvcrt.dll`, standing in for the unknown one T2DEF's linker ran on | `vc6std`, `VC6_CRT=wine` |
| T2DEF `client.dll` | Standard, Service Pack 4 | 12.00.8804 | 12.00.8867 | 11.99.8799 | 6.00.8447 | SP4's `LIBCMT.LIB` | SP4's `MSVCRT.DLL` 6.00.8797 | `vc6sp4std` |
| Wanted! `wanted.dll` | Professional, no service pack | 12.00.8168 | 12.00.8168 | 12.00.8168 | 6.00.8168 | the CD's `LIBCMT.LIB` and `LIBCIMT.LIB` | the CD's `MSVCRT.DLL` | `vc6` |
| Wanted! `client.dll` | the same | | | | | the CD's `LIBCMT.LIB` | the same | `vc6` |
| Gunman Chronicles `gunman.dll` | the same | | | | | the CD's `LIBCMT.LIB` | the same | `vc6` |
| Gunman Chronicles `client.dll` | the same | | | | | the same | the CD's `MSVCRT.DLL` (not pinned) | `vc6` |
| They Hunger `einar.dll` | the same | | | | | the CD's `LIBCMT.LIB` | the CD's `MSVCRT.DLL` | `vc6` |
| They Hunger `client.dll` | the same | | | | | the same | the same | `vc6` |
| Scientist Slaughterhouse `hl.dll` | the same | | | | | the CD's `LIBCMT.LIB` | the CD's `MSVCRT.DLL` | `vc6`, run as `C:\Microsoft Visual Studio\VC98` (`VC6_ROOT`) |
| Scientist Slaughterhouse `client.dll` | the same | | | | | the CD's `LIBCMTD.LIB` (a Debug build) | the same | the same |

All but two of these DLLs rebuild byte for byte with the tree named. The
Wanted! server and the Gunman Chronicles client don't yet; their remaining
differences are in the code, not in the toolchain ([wanted](wanted.md),
[gunman-chronicles](gunman-chronicles.md)).

What the evidence pins and what it doesn't:
- The Rich header (next section) records the back end's edition and build,
  and the linker's build. It doesn't record the front ends or `CL.EXE`. The
  versions in those columns are the files of the tree that rebuilds the
  DLL.
- Build 8168 is VC6 as first released (RTM, "release to manufacturing").
  Published lists give 8168 for Service Packs 1 and 2 as well, but those
  two don't change the compiler. SP2, which includes SP1 (archive.org item
  `msvs-6.0`), ships no `CL.EXE`, `C1.DLL`, `C1XX.DLL`, `C2.DLL` or
  `LINK.EXE`. SP3's back end writes 8447 (item `msvs6sp3`), which rules
  out SP3 and later as a whole. SP3's front ends (12.00.8472) with RTM's
  back end still write 8168. For Gunman Chronicles they build both DLLs
  byte for byte the same as RTM's front ends do.
- For the T2DEF client, SP4's back end (8799), linker (8447) and C runtime
  (8797) are pinned, its front ends are not. With SP4's `C1XX.DLL`
  12.00.8867, `cl_dll.dsp`'s `/O2` expands inline functions that retail
  doesn't have; `/Ob0` (no inline expansion) added to the project's flags
  gives retail's code. Whether T2DEF's project had `/Ob0`, or the front end
  T2DEF used didn't inline there, is not known.

## How the compiler is identified

### The Rich header

LINK since Visual C++ 5.0 Service Pack 3 writes an undocumented block, the
Rich header, between the DOS stub and the PE header. Each object file that
VC6's compiler writes carries a stamp (the `@comp.id` symbol) with a
product ID and a build number. LINK counts the objects it loads per stamp,
including the objects it pulls from libraries, and lists the counts, plus
an entry for itself. So the Rich header is an inventory of the tools that
made the DLL.

Product IDs that occur here:

| ID | Name | Meaning |
|---|---|---|
| 0x0a, 0x0b | Utc12 C, Utc12 C++ | an object from VC6's C or C++ compiler, Professional or Enterprise back end |
| 0x15, 0x16 | Utc12 C Std, Utc12 C++ Std | the same with the Standard edition's back end |
| 0x04 | Linker 6.00 | LINK 6.00 itself; also objects made by LIB 6.00, such as the import objects of a library built with it |
| 0x13 | Linker 5.12 | import objects made by the librarian of LINK 5.12 (build 8034): the system import libraries on the VC6 CD (`KERNEL32.LIB`, ...) |
| 0x0e | MASM 6.13 | assembler objects (build 7299) from the C runtime library |
| 0x0c | AliasObj 6.0 | alias objects (build 7291) from `OLDNAMES.LIB`, which maps old names such as `stricmp` to `_stricmp` |
| 0x01 | Import0 | one per imported function |

The build number of a compiler object is the back end's build (8168 for
RTM, 8799 for SP4), which is why `C2.DLL` decides the stamp. The objects
inside a C runtime library carry the stamp of the compiler Microsoft built
that library with: 8168 in the VC6 CD's `LIBCMT.LIB`, 8797 in SP4's. That is
how the C runtime library is identified.

The twelve retail files (read from the files themselves; product ID,
build, count):

| DLL | Compiler objects | C runtime and libraries | Linker |
|---|---|---|---|
| POV `hl.dll` | C 8168 ×122, C++ 8168 ×142 | MASM 7299 ×30, AliasObj 7291 ×3, Linker 5.12 8034 ×3, Import0 ×74 | 6.00 8168 ×1 |
| POV `client.dll` | C 8168 ×128, C++ 8168 ×62 | MASM ×30, AliasObj ×2, Linker 5.12 ×9, Import0 ×636 | 6.00 8168 ×3 |
| T2DEF `t2def.dll` | **C Std 8168 ×3, C++ Std 8168 ×113**; C 8168 ×127, C++ 8168 ×3 | MASM ×39, AliasObj ×2, Linker 5.12 ×3, Import0 ×75 | 6.00 8168 ×1 |
| T2DEF `client.dll` | **C Std 8799 ×3, C++ Std 8799 ×80**; C 8797 ×133, C++ 8797 ×11 | MASM ×38, AliasObj ×3, Linker 5.12 ×9, Import0 ×645 | 6.00 **8447** ×3 |
| Wanted! `wanted.dll` | C 8168 ×127, C++ 8168 ×146 | MASM ×32, AliasObj ×3, Linker 5.12 ×3, Import0 ×81 | 6.00 8168 ×1 |
| Wanted! `client.dll` | C 8168 ×128, C++ 8168 ×61 | MASM ×30, AliasObj ×2, Linker 5.12 ×9, Import0 ×636 | 6.00 8168 ×3 |
| Gunman `gunman.dll` | C 8168 ×123, C++ 8168 ×124 | MASM ×30, AliasObj ×3, Linker 5.12 ×3, Import0 ×74 | 6.00 8168 ×1 |
| Gunman `client.dll` | C 8168 ×128, C++ 8168 ×78 | MASM ×32, AliasObj ×2, Linker 5.12 ×9, Import0 ×636 | 6.00 8168 ×3 |
| They Hunger `einar.dll` | C 8168 ×122, C++ 8168 ×100 | MASM ×30, AliasObj ×3, Linker 5.12 ×3, Import0 ×74 | 6.00 8168 ×1 |
| They Hunger `client.dll` | C 8168 ×128, C++ 8168 ×61 | MASM ×30, AliasObj ×2, Linker 5.12 ×9, Import0 ×636 | 6.00 8168 ×3 |
| Scientist Slaughterhouse `hl.dll` | C 8168 ×125, C++ 8168 ×131 | MASM ×31, AliasObj ×3, Linker 5.12 ×3, Import0 ×75 | 6.00 8168 ×1 |
| Scientist Slaughterhouse `client.dll` | C 8168 ×139, C++ 8168 ×105 | MASM ×33, AliasObj ×3, Linker 6.00 **8447** ×2, Linker 5.12 ×9, Import0 ×646 | 6.00 8168 ×1 |

How to read it:
- **Edition.** Ten DLLs have only Professional stamps (0x0a, 0x0b). In the
  T2DEF DLLs the mod's own sources carry Standard stamps (0x15, 0x16); the
  Professional-stamped objects there (C 8168 ×127 and C++ 8168 ×3 in the
  server) are the C runtime library's, which Microsoft compiled with the
  Professional compiler.
- **Service pack.** Build 8168 everywhere except the T2DEF client: 8799 is
  the build SP4's back end writes (SP3 writes 8447, SP5 8966, SP6 9782,
  according to the published lists; the SP4 disc's `C2.DLL` files are
  12.00.8799 and 11.99.8799). Its C runtime objects are 8797, the build of
  SP4's C runtime (SP4's `MSVCRT.DLL` is 6.00.8797), and about a quarter of
  its C runtime code differs from the CD's `LIBCMT.LIB`. The T2DEF server's
  C runtime matches the CD's almost entirely (63.5 of 63.7 KB). The Gunman
  client's 128 C objects of build 8168 show the CD's `LIBCMT.LIB` too.
- **Linker.** 6.00.8168 is the CD's LINK; 8447 is the LINK of Service
  Packs 3 to 6, and SP4's `LINK.EXE` is 6.00.8447. In the clients, two of
  the three Linker 6.00 objects are `vgui.lib`'s import objects: SDK 2.0's
  `vgui.lib` (which SDK 2.1 ships unchanged) was made with build 8168, SDK
  2.2's (T2DEF, Scientist Slaughterhouse) with 8447; Scientist
  Slaughterhouse's client, linked by the CD's LINK, shows the two as their
  own entry.
- **Object counts.** Every object counts, including empty ones and alias
  objects. LINK also sizes the header from the number of objects, so one
  object too few moves the PE header. The mods' pages list the placeholder
  objects that make the counts match. They Hunger needs none: its counts are
  those of Valve's own file lists (2.2b's `hl.dsp` without
  `voice_gamemgr.cpp`, and 2.1's `cl_dll.dsp` without the three studio
  renderer files), which shows that the mod added no source file.

### The code

- **Professional, `/O2`.** Functions address their locals relative to
  `esp`, without a frame pointer. In Gunman Chronicles 4% (server) and 6%
  (client) of the functions set up a frame pointer, in the Wanted! server 2
  of 2,978 user functions of 16 bytes or more. A Standard edition compiler
  can't produce that code.
- **Standard edition.** The T2DEF server's code is `/Od` code (every
  function has a frame pointer, no optimisation) although `hl.dsp` says
  `/O2`. With the Standard back end the project's own flags give it.
- **Rebuilding.** Each variant was confirmed by rebuilding: ten of the
  twelve DLLs are byte-identical. The Wanted! server differs inside one
  function only. The Gunman Chronicles client differs in which copies of
  identical functions LINK keeps, which moves much of its code, and in the
  code of three functions.

### The runtime LINK ran on

LINK sorts with the C runtime's `qsort` in two places that reach the image:
the order of the import table, and the copy it keeps when it folds
identical functions into one (`/OPT:ICF`, identical-code folding, on by
default in a link without `/debug`). `qsort` doesn't keep elements that
compare equal in their original order, and each runtime's `qsort` orders
them its own way. Wine always loads its builtin `msvcrt.dll`; the
`nativecrt` copies in [setup.md](setup.md) make LINK load a disc's own
`MSVCRT.DLL` instead. The compiler doesn't depend on it: SDK 2.0's server
objects come out identical with either runtime.

| DLL | Runtime | Evidence |
|---|---|---|
| POV `hl.dll` | VC6 CD's `MSVCRT.DLL` | On Wine's builtin, two imports (`TlsGetValue`, `CreateDirectoryA`) change places: 4 bytes of the import tables and 7 of `.text` differ. |
| POV `client.dll` | VC6 CD's | Kept copies of folded functions: 851 of the 861 game functions at retail's address on the CD's runtime, 845 on the builtin, before the last fixes; identical now. |
| T2DEF `t2def.dll` | Wine 11.0's builtin | On the CD's runtime, `TlsAlloc` and `CreateDirectoryA` change places (17 bytes); retail has the builtin's order. T2DEF's linker probably ran on a later Windows `msvcrt.dll` whose `qsort` orders like Wine's; which one is not known. The server links with `/debug`, so no folding. Only Wine 11.0 was tested. |
| T2DEF `client.dll` | SP4's `MSVCRT.DLL` 6.00.8797 | Rebuilds identical on it (the `vc6sp4std` default). Whether Wine's builtin gives the same file is not recorded. |
| Wanted! `wanted.dll` | VC6 CD's | Builds on it give retail's import table (`/debug`, so no folding). Whether the builtin gives the same is not recorded. |
| Wanted! `client.dll` | VC6 CD's | On the builtin, seven folded stubs keep another object's copy. |
| Gunman `gunman.dll` | VC6 CD's | On the builtin, the import table changes (17 bytes of `.rdata`, 7 of `.text`). |
| Gunman `client.dll` | not pinned | Most of the client's remaining differences are the copies LINK keeps of folded functions, and with the current sources neither runtime's `qsort` gives retail's copies. Its build uses the CD's runtime, as the server's does. |
| They Hunger `einar.dll` | VC6 CD's | Rebuilds identical on it (`/debug`, so no folding; one import DLL). Whether the builtin gives the same import order is not recorded. |
| They Hunger `client.dll` | VC6 CD's | Rebuilds identical on it. On the builtin, functions change places inside two import lists (14 of `vgui.dll`'s 542 thunks, 11 of KERNEL32's 65); the build we tried it on differed from retail in 216,849 bytes. |
| Scientist Slaughterhouse `hl.dll` | VC6 CD's | Rebuilds identical on it (`/debug`, so no folding; one import DLL). Whether the builtin gives the same is not recorded. |
| Scientist Slaughterhouse `client.dll` | VC6 CD's | Rebuilds identical on it (linked incrementally, so no folding). Whether the builtin gives the same is not recorded. |

## Counter-Strike

Counter-Strike's game DLLs went through five compilers in three years. The
23 distinct servers (`mp.dll`) and 17 distinct clients (`client.dll`) from
Beta 1.0 to 1.5 group like this ([counter-strike.md](counter-strike.md) has
the releases):

| Releases | DLLs | Compiler | `CL` | Back end | `LINK` | C runtime library | Runtime LINK ran on | Tree |
|---|---|---|---|---|---|---|---|---|
| Betas 1.0 to 4.1 | 8 servers, 4 clients | Visual C++ 5.0 Professional as on the CD | 11.00.7022 | `C2.EXE` 11.00 | 5.00.7022 | VC5's `LIBCMT.LIB` | VC5's `MSVCRT.DLL` 5.00.7022 | `vc5/rtm`, `VC5_CRT=native` |
| Beta 5.0 (leak) to 7.1 | 9 servers, 8 clients | VC6 Professional as on the CD | 12.00.8168 | `C2.DLL` 12.00.8168 | 6.00.8168 | the CD's `LIBCMT.LIB` | the CD's `MSVCRT.DLL` 6.00.8168 | `vc6` |
| 1.0, 1.0b, 1.1 | 3 servers, 2 clients | VC6 Professional, Service Pack 4 | 12.00.8804 | `C2.DLL` 12.00.8799 | 6.00.8447 | SP4's `LIBCMT.LIB` (8797) | SP4's `MSVCRT.DLL` 6.00.8797 | `vc6sp4` |
| 1.3, server | 1 server | VC6 Professional, Service Pack 5 | 12.00.8804 | `C2.DLL` 12.00.8966 | 6.00.8447 | SP5's `LIBCMT.LIB` (8047) | SP5's `MSVCRT.DLL` (SP4's file) | `vc6sp5` |
| 1.3 client, 1.4, 1.5 | 2 servers, 3 clients | VC6 SP5 with the Processor Pack | 12.00.8804 | `C2.DLL` 13.00.9044 | 6.00.8447 | SP5's `LIBCMT.LIB` (8047) | SP5's `MSVCRT.DLL` | `vc6sp5pp` |

The front ends are the trees' own: `C1.DLL` and `C1XX.DLL` "11.00" in VC5,
12.00.8168 on the VC6 CD, 12.00.8867 in SP4, and in SP5 `C1.DLL` 12.00.8867
with `C1XX.DLL` 12.00.8964. One source of the 1.4 and 1.5 clients,
`cl_dll/steam_wrap.c`, is compiled by `vc6sp5` without the Processor Pack:
retail's code of it is VC6 code without the pack's instruction scheduling
(the CD's, SP4's and SP5's back ends give the same bytes for it; SP5 is our
choice).

All 38 PE files among the 40 rebuild byte for byte with the tree named. The
1.4 and 1.5 clients are not PE files but the engine's packed form of one;
they do not rebuild byte for byte: 18 of their functions come out in
another order of instructions than retail's, below.

### How each compiler is known

**Visual C++ 5.0 (Betas 1.0 to 4.1).** The PE header says linker 5.00, and
there is no Rich header. Visual Studio 97 Service Pack 3's linker is 5.10
and writes one; Service Packs 1 and 2 ship linker 5.02.7132 (by the list in
[oxiKKK's MSVC toolchain catalog](https://github.com/oxiKKK/msvc-toolchain-catalog)).
So linker 5.00 means the compiler as on the CD. VC5's objects carry no
`@comp.id` stamp, so nothing else in the file names the compiler; the
rebuilds confirm it. With Service Pack 3's compiler passes, five functions
of Beta 1.0's client come out differently.

**VC6 and its service packs (Beta 5.0 leak to 1.5).** Every PE file from
the leaked Beta 5.0 on has a Rich header. The betas' show C and C++ objects
of build 8168 and LINK 6.00.8168, as in the first five mods; from 1.0 on the
builds are later ones. For example, product ID, build and count:

| DLL | Compiler objects | C runtime and libraries | Linker |
|---|---|---|---|
| Beta 5.0 leak `mp.dll` | C 8168 ×110, C++ 8168 ×78 | MASM 7299 ×31, AliasObj 7291 ×2, Linker 5.12 8034 ×3, Import0 ×75 | 6.00 8168 ×1 |
| Beta 6.5 `client.dll` | C 8168 ×131, C++ 8168 ×85 | MASM ×30, AliasObj ×2, Linker 5.12 ×9, Import0 ×636 | 6.00 8168 ×3 |
| 1.0 `mp.dll` | **C 8799 ×3, C++ 8799 ×76**; C 8797 ×109, C++ 8797 ×8 | MASM ×31, AliasObj ×3, Linker 5.12 ×3, Import0 ×75 | 6.00 **8447** ×1 |
| 1.0 `client.dll` | **C 8799 ×3, C++ 8799 ×91**; C 8797 ×129, C++ 8797 ×11 | MASM ×33, AliasObj ×2, Linker 5.12 ×9, Linker 6.00 8168 ×2, Import0 ×640 | 6.00 8447 ×1 |
| 1.3 `mp.dll` | **C 8966 ×3, C++ 8966 ×77**; C 8047 ×112, C++ 8047 ×8 | MASM ×32, AliasObj ×3, Linker 5.12 ×3, Import0 ×76 | 6.00 8447 ×1 |
| 1.3 `client.dll` | **ID 48 9044 ×3, ID 49 9044 ×102**; C 8047 ×133, C++ 8047 ×11 | MASM ×33, AliasObj ×3, Linker 5.12 ×9, Import0 ×647 | 6.00 8447 ×3 |
| 1.4 `mp.dll` (1.5 the same) | **ID 48 9044 ×3, ID 49 9044 ×77**; C 8047 ×112, C++ 8047 ×3 | MASM ×31, AliasObj ×3, Linker 5.12 ×3, Import0 ×73 | 6.00 8447 ×1 |

How to read it:
- **8799, 8966, 9044.** The back end's build: Service Pack 4's `C2.DLL` is
  12.00.8799, Service Pack 5's 12.00.8966. The Processor Pack's back end
  (13.00.9044) stamps its objects with product IDs 48 (C) and 49 (C++)
  instead of VC6's 10 and 11. All three were measured by compiling a test
  file with each tree.
- **8797 and 8047.** The C runtime library's objects carry the build
  Microsoft compiled it with: 8797 in SP4's `LIBCMT.LIB`, 8047 in SP5's
  (503 C objects and 22 C++ objects in each of SP5's `LIBC.LIB` and
  `LIBCMT.LIB`). The Processor Pack ships no runtime library.
- **SP5 without the pack.** The 1.3 server's objects are 8966, its client's
  9044: the two DLLs of 1.3 were compiled by two compilers.

**The 1.4 and 1.5 clients** keep no PE header and so no Rich header. Their
code is the Processor Pack's in most objects; they contain SP5's
`LIBCMT.LIB` (479 of their 482 runtime functions are its code) and only
exception-handling tables of VC6's format. But 18 functions
(`ev_hldm.cpp`'s fire events, both renderers' `StudioFxTransform`,
`CHud::Init`, `CHudAmmo::Draw`, `CHudHealth::DrawRadar` and seven more)
have an order of instructions that none of the compilers tried gives:
- every VC6 back end above, with single compiler switches, every pair and
  the likely triples of them;
- the first Processor Pack, for SP4 (2000; `C2.DLL` 13.00.8943, found in
  the "Visual Studio Processor Pack" folder of the Xbox XDK 4361 package
  on archive.org, item `xbox-sdks`): byte for byte the same code as 9044;
- Visual C++ .NET 2002 (13.00.9466; archive.org item `X08-65717`) with
  VC6's headers and libraries: its code calls `__ftol2`, which the client
  never does, and it matches far fewer functions;
- the 2001 builds of the 13.00 compiler in the Windows XP DDK (13.00.9176)
  and the Xbox XDKs 3424, 3521, 3729 and 3911 (13.00.9175, 9176, 9210,
  9254).

None of these is needed to rebuild anything here.
[counter-strike.md](counter-strike.md) has the measurements.

### The runtime LINK ran on

As with VC6 above, LINK 5.00 sorts with the C runtime's `qsort`, and the
order it gives equal elements reaches the image. On Wine's builtin
`msvcrt.dll`, Beta 1.0's client links into a file that differs from retail
in 24,717 bytes (other kept copies of identical functions, another import
order), and Beta 1.0's server orders two imports differently and leaves
stray bytes in the reserved field of its MISC debug record. On VC5's own
`MSVCRT.DLL` (5.00.7022) both are retail's files. Only LINK needs it: the
compiler's objects are the same on either runtime. LINK 5.00 gives retail's
client on 5.00.7022, on Service Pack 3's 5.00.7303 and on the VC6 CD's
6.00.8168 alike. `tools/vc5.sh` runs on Wine's runtime unless `VC5_CRT` is
`native`; the release scripts use `build_dsp.py --compiler vc5-rtm-n`,
which always runs the `nativecrt` copy.

The VC6 builds of Counter-Strike run on each tree's own `MSVCRT.DLL`
(`vc6.sh`'s default), and all rebuild byte for byte that way. Whether Wine's
builtin gives the same files was not recorded for them.

## Where the compilers come from

Three discs from the Internet Archive. Both editions of VC6 as first
released are on two CDs of February 1999; Service Pack 4 is on a disc of
August 2000. Counter-Strike needs three more downloads: Service Pack 5 and
the Processor Pack (2001), and the Visual C++ 5.0 disc of Visual Studio 97
Professional (1997). [setup.md](setup.md) has the steps.

| Disc | archive.org item, file | SHA-256 of the file | SHA-1 (archive.org) |
|---|---|---|---|
| Visual C++ 6.0 Professional Edition, English retail CD (part no. X04-58170, matrix BR11577-103) | [`X04-58170`](https://archive.org/details/X04-58170), `1999-02_X04-58170_X04-58170_VCP600ENU1.7z` | `4e3fa95110e263fe4a42fe151b3f5208d414db72c006beee999a76c6c0ed4261` | `7d476db1e8eaebe5733fbe95cb7a6bd01132c479` |
| Visual C++ 6.0 Learning Edition, English (X04-58202, matrix BR11678-353) | [`X04-58202`](https://archive.org/details/X04-58202), `1999-02_X04-58202_X04-58202_VCL600ENU1.7z` | `7ed8ad7ebdbc29e8c1512c6530d267dd48804ac1860344191c99cb7234c3d9c3` | `cec6f224387bb9121f4d91c10818847738647973` |
| Visual Studio 6.0 Service Pack 4 (X05-86594, matrix X05-86829) | [`X05-86594`](https://archive.org/details/X05-86594), `2000-08_X05-86594_X05-86829_vs6sp4.7z` | `d67c8795c59fcbf0d67c468cb747bc28a9f15cb88a532896c1449e1e6da7251a` | `7ba4001a8e6a3af8f9807d85471b5ea83000eae8` |
| Visual Studio 6.0 Service Pack 5 (2001), a self-extracting cabinet | [`vs6sp5`](https://archive.org/details/vs6sp5), `vs6sp5.exe` | `10a834041223ad21181c0e87c0a90930d976498556af2e8f959a044101e6a604` | `6af12fb3e2c7c271be67c9c9833c63b82bb7b9bf` |
| Visual C++ 6.0 Processor Pack for Service Pack 5, a self-extracting cabinet | [`vcpp5`](https://archive.org/details/vcpp5), `vcpp5.exe` | `96817df1bb0cb58e5b1d0fbcc5b993a63548182add1fbff66365401203ecfb6c` | `d62465c0e3215086f0f4745253402378ad823d54` |
| Visual Studio 97 Professional, disc 3 (Visual C++ 5.0) | [`microsoft-visual-studio-97-professional-edition-disc-3`](https://archive.org/details/microsoft-visual-studio-97-professional-edition-disc-3), `Microsoft Visual Studio 97 Professional Edition - Disc 3.iso` | `99101fb01f564ba9cc50cbcbf862c334cd1be16a2e8c277cb81f9a500cb6413f` | `a3ade9b0681ffab34f058a7991f6b7aa25d342e1` |

What each tree takes from them:

| Tree | Contents |
|---|---|
| `vc6` | The Professional CD's `VC98/BIN`, `VC98/INCLUDE`, `VC98/LIB`, plus `MSPDB60.DLL`, `MSOBJ10.DLL` and `MSDIS110.DLL` from its `COMMON/MSDEV98/BIN`. `nativecrt` uses the CD's `OS/SYSTEM/MSVCRT.DLL`. |
| `vc6std` | `vc6` with `C2.DLL` and `CL.EXE` from the Learning Edition CD (11.99.8168), the only two files in which its `VC98/BIN` differs from the Professional CD's. That `C2.DLL` is the Standard edition's back end: its objects carry the Standard stamps. `nativecrt` uses the Professional CD's `MSVCRT.DLL` (the two CDs' copies are identical). |
| `vc6sp4std` | `vc6` with SP4's English VC files (`enu/vc98/bin`, `include`, `lib`, and `enu/common/msdev98/bin/msdis110.dll`), and SP4's Standard back end `enu/os/system/msse.dll` as `C2.DLL`. `nativecrt` uses SP4's `enu/os/system/msvcrt.dll`. |
| `vc6sp4` | The same with the Professional back end `enu/os/system/msvcep.dll` as `C2.DLL`. Counter-Strike 1.0 to 1.1. |
| `vc6sp5` | `vc6` with SP5's VC files (`vc98/bin`, `include`, `lib` of its cabinets, English only, so without an `enu/` level), its Professional back end `os/system/msvcep.dll` as `C2.DLL` (12.00.8966) and `VB98/msdis110.dll` as `MSDIS110.DLL`. SP5 is cumulative: it carries every file `vc6sp4` takes from SP4. `nativecrt` uses SP5's `os/system/msvcrt.dll`, which is SP4's file. The Counter-Strike 1.3 server, and `steam_wrap.c` in the 1.4 and 1.5 clients. |
| `vc6sp5pp` | `vc6sp5` with the files the Processor Pack's `vcpp.inf` puts into `VC98`: `C2.DLL` 13.00.9044 and MASM 6.15 (`ML.EXE`, `ML.ERR`, `H2INC.EXE`, `H2INC.ERR`) in `BIN`; the intrinsic headers, `MALLOC.H` and `FPIEEE.H` in `INCLUDE`. SP5 carries the same `C2.DLL` as `os/system/vcpp.dll`. The Counter-Strike 1.3 client, 1.4 and 1.5. |
| `vc5/rtm` | The disc's `DEVSTUDIO/VC/BIN`, `INCLUDE` and `LIB` (without `BIN/WIN95` and `BIN/WINNT`), plus `MSPDB50.DLL`, `MSDIS100.DLL`, `RC.EXE`, `RCDLL.DLL` from `DEVSTUDIO/SHAREDIDE/BIN` and `MSVCP50.DLL` from `DEVSTUDIO/VC/REDIST` in `BIN`. `nativecrt` uses `DEVSTUDIO/VC/REDIST/MSVCRT.DLL`. Counter-Strike Betas 1.0 to 4.1. |

The C runtime DLLs the `nativecrt` copies run on:

| File | Version | Size | SHA-1 | SHA-256 |
|---|---|---|---|---|
| VC6 CD `OS/SYSTEM/MSVCRT.DLL` (1998-06-17) | 6.00.8168.0 | 254,005 | `11179154ec7375e7dc50f2b349c571d01a8856b9` | `fb6260b07c05bdbcb6928156da514be5b6b69f095b335692ba46c561d85bd5f8` |
| SP4 `enu/os/system/msvcrt.dll`, SP5 `os/system/msvcrt.dll` | 6.00.8797.0 | 278,581 | `63a4fcd64ecea975c1b91de04702c68a9f2a3c7d` | `887eb5ce93edb7192ca3e9220f07f9ca0f94db02af5862ebcbdfcb852db99fd1` |
| VC5 disc `DEVSTUDIO/VC/REDIST/MSVCRT.DLL` (1997-01-23) | 5.00.7022 | 271,632 | `56c58e43281a14356bc038280bc4ac4267a9a581` | `d398de0da85572ae6e40254f93ffe0ab0bc4bfcb77681b4a0c358493552707bb` |

## The files that matter

The trees are copies of the discs' files; nothing in them is patched except
the import names in `nativecrt`. `tools/check_toolchains.sh` checks each
whole tree (every header and library included) against these digests, the
SHA-256 of the sorted list of the SHA-256 sums of all its files:

| Tree | Digest of all files (`VC98` and `nativecrt`) |
|---|---|
| `vc6` | `0a5cba66bdf0ed7bdf98a797c90f7954cd5c089aa9f668cead7e52a06ebad391` |
| `vc6std` | `807d9e2fe7f1b179b4670d8298b3a326b8e97c16ccd3422e57d50bb472b134d2` |
| `vc6sp4std` | `359ffb6276ad76d1fa0bd32ecba3f876a3d12a626d03ea953cc91b5be3eb587d` |
| `vc6sp4` | `f2461043c1ec4f62fd775bd81911ca8a6e69d240ed834b79002f9869794d00ff` |
| `vc6sp5` | `a9c0ecd8b48d124d592c58e979fee23a03033d1f7abedc2f7a07d0a6766a30ac` |
| `vc6sp5pp` | `6a9e4308740d03c55a4b751c6f184a892cc2d3e78d16a532d35dfb9c15b627b8` |
| `vc5/rtm` (`BIN`, `INCLUDE`, `LIB` and `nativecrt`) | `a129b7cd4bba11a6c3fced02f684c00f5267fc5579c7599adcfb5956746c9699` |

The files that make the code and the image, as on the discs (`VC98/BIN`,
`VC98/LIB`):

| File | `vc6` | `vc6std` | `vc6sp4std` |
|---|---|---|---|
| `CL.EXE` | 12.00.8168 `d9220fdfaf355c81f26e7b03c5dc628ec72ff81c99dc38ed98b1ad4bdbbb309c` | 11.99.8168 `bdc8b313c2b594536c825cf02c6692d6263cc65c08b0f9048e54d07fb04acf44` | 12.00.8804 `1bf99f206271ecdbd13da2829192ea2c02e2a44c740b8d72935d5d9cb753b156` |
| `C1.DLL` | 12.00.8168 `ee14a2603a1339f95db2e396751f7e51b50e421731229fb2416a01f974689d80` | as `vc6` | 12.00.8867 `0792033fc3d2fec262ff95478c6a9e70a06e836ed55f0a9710200fa06c204754` |
| `C1XX.DLL` | 12.00.8168 `4095e9e8de5ac4b13dbfe310abb8de711475b1a92c5f94375919947937f3237e` | as `vc6` | 12.00.8867 `71718c2ba17a0e8c030ce4e29c7904661b513e3937dd701de79fc2af50a28a52` |
| `C2.DLL` | 12.00.8168 `45187b0b6288240f73272a7c61e6329c50048a76e57db3bb87b6f0229e09e27d` | 11.99.8168 `f849b976db7a4377c361ab9ab4eebe3903bd21cb24db6b265542593fc8e1ee62` | 11.99.8799 `d92bacaa72176f078d6f49b2f271b88d82517bc1d9da28779f377c195872c8ec` |
| `LINK.EXE` | 6.00.8168 `08d3a4c9701c8f6c871b2c48bd74d07422aa1f86e5d9a4ddb2c9bd6a88b857e7` | as `vc6` | 6.00.8447 `9672e578fdfaa43bdb8e9c16071682988665cb90bba2904bf02f6a5576d8ffbc` |
| `MSPDB60.DLL` (writes the PDB) | 6.00.8168 `7c90b80d42914562332e28c253415e72a80c6651003d08a9ffdc4ba6e27fc37c` | as `vc6` | as `vc6` |
| `LIBCMT.LIB` | `1ef9c27b4f76031e40fcbdf7d518adc4969f90d6a00dd1ea88697907268c1b2b` | as `vc6` | `5dc8e4bc5377ca3d385b747e937f1cba4519431776d7c8f28d9cb852d79b0160` |
| `OLDNAMES.LIB` | `348a220e94f90bd9f530514ae59ff9b044b76e2056f52b45df406c12d2156afb` | as `vc6` | as `vc6` |
| `KERNEL32.LIB` | `cd4a546f71481319aeee455aa0fb51698a094f9d3e6d8339acf445489a263c68` | as `vc6` | as `vc6` |
| `LIBCIMT.LIB` (old iostream library, Wanted! server) | `a78e4d21e853b385b043a1dfd0d65e8a3030eb0d776f4d66c2f76afeee6c115f` | | |
| `USER32.LIB` (clients) | `0a3c34c4daec09c1b9fd163d5d0b2ee9a6cbe0b3e863ae81a1fae3b9f936fd19` | | as `vc6` |
| `WINMM.LIB` (clients) | `bb098679a802afdbc6fdaac03dfe1ea81453a36f6d4768b1c1cd092b1dbe9342` | | as `vc6` |
| `WSOCK32.LIB` (clients) | `3ac0ae32d0c9ea77813db7f4d2ea8eba23ae420b40a24845cbe36f6702fca4ba` | | as `vc6` |

Counter-Strike's trees, the same files (`vc6sp4` differs from `vc6sp4std`
only in `C2.DLL`; `OLDNAMES.LIB`, `KERNEL32.LIB`, `USER32.LIB`,
`WINMM.LIB` and `WSOCK32.LIB` of the three VC6 trees are the CD's, as above;
`MSPDB60.DLL` is the CD's in every VC6 tree):

| File | `vc6sp4` | `vc6sp5` | `vc6sp5pp` |
|---|---|---|---|
| `CL.EXE` | 12.00.8804, as `vc6sp4std` | as `vc6sp4` | as `vc6sp4` |
| `C1.DLL` | 12.00.8867, as `vc6sp4std` | as `vc6sp4` | as `vc6sp4` |
| `C1XX.DLL` | 12.00.8867, as `vc6sp4std` | 12.00.8964 `f014b3bee650224adf6cb44e51f0eeac5abbf5da666fa299d5250b2f3d1937c6` | as `vc6sp5` |
| `C2.DLL` | 12.00.8799 `5649bd68ed833fcf4396c4a0a834e94ab1ec162316b3cbc3d8a9beabdc54e517` | 12.00.8966 `d50100ac2380d58f3f6f756961fb1319d35f5248e5fa6cafb866ca657e5dda4a` | 13.00.9044 `6c8e3988a5ccd69a9f9829b5c7512848d71c018b8d24be3928d7b00b5f79f231` |
| `LINK.EXE` | 6.00.8447, as `vc6sp4std` | as `vc6sp4` | as `vc6sp4` |
| `LIBCMT.LIB` | as `vc6sp4std` | `28b9f04962378ec4668072f37d7fd2835cd6cacc17b40cf22002c57bd8e76714` | as `vc6sp5` |

| File | `vc5/rtm` |
|---|---|
| `CL.EXE` | 11.00.7022 `bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb` |
| `C1.DLL` | 11.00 `e519dfe6f45cc0503b7aac43d125b2f156305269b89f0dc765c40d29627c0943` |
| `C1XX.DLL` | 11.00 `fea1e9fbbc2d204202f1a5ea61e5a938b17a92acda57e16dd53d4ec01c9d186b` |
| `C2.EXE` (the back end is a program in VC5) | 11.00 `6d1406af5f62647347cfa7175cd7e108b838d6841a546396f29252198a14d73d` |
| `LINK.EXE` | 5.00.7022 `279544809e6353c8e622eb88aea94e86b0258f3c4b8a976373bf4d68a41b63e1` |
| `MSPDB50.DLL` | 5.00.7022 `06aaaca9c928bdef790259c9c30328529e10d1ec2f77da09c4ff74de7734fe69` |
| `LIBCMT.LIB` | `7e161e461fd97f6fd5a8e6c1fdb5c7ec1b408e75bfacbc1c226f7556ccd1995b` |
| `OLDNAMES.LIB` | `56190b253c7c5f127569cd1dc037e27600f42e4952f558e7ab1966cbb98cb1d6` |
| `KERNEL32.LIB` | `095f231e13cb3e6a6c4913d33ca8048506011034c89774b6ba0857466abba3cf` |
| `USER32.LIB`, `WINMM.LIB`, `WSOCK32.LIB` (clients) | `5badb752dbcd6355a666d5b88aa9fd5e27291c16edc76f1b5258468d994f1809`, `a996cf8e569b6f2dd0a478119a57c325733af545ffee2e6c6fa7dba40bfba260`, `0f94040eb6fba0408841b9d5e12d307deca926dbbb251b4eaad4fcafeb1719c7` |

The copies that actually run (`nativecrt`, `msvcrt.dll` imports renamed to
`MSVCRX.dll`). The T2DEF server runs `vc6std/VC98/BIN` unpatched instead
(`VC6_CRT=wine`).

| File | `vc6/nativecrt` | `vc6sp4std/nativecrt` |
|---|---|---|
| `CL.EXE` | `daac6e527edc7ae487b1f98e4333ea1cbf12fedc8e6c85f3f1aa1de7cc0ab84d` | `2d493c8328b245d377cea9704e32d69d0bb652c931c3615aaca70fb4b53f2229` |
| `C1.DLL` | `b42f24c2e4bc5e2024ac19c4edf1ed25985968d95237e79bf774d2a448ab30b8` | `1cccc856ab15652401eb5033e25013b140df515972ced7d5484c45e279fa4ad5` |
| `C1XX.DLL` | `f5c8d552fe9fcbbe3a627631cb21752694b650bddc89efb13df95cce8366a069` | `ac4aecee64c3b95f0135aea56c4ecc370b1838660e09b32b33cbe12cc7778be6` |
| `C2.DLL` | `495b9f07c57273896edfb40792c8eafd7eabaee6477a820d53cc93e759031f6e` | `c9f62cac175d6efcc8158657996c8d012df074f3ef0339c7c8916ebfb9d6f8f6` |
| `LINK.EXE` | `a48e47ff5c7410276e1fc78c5e7d7107d5202386439b0b60543147d719997336` | `941f2e5dbbdd3e3c4914d989e6be923d205c09afbf231a360314b358ae306185` |
| `MSPDB60.DLL` | `bd21b8f3cd5e3a552cf610797297bffaf367dfb16f315d004fab7e916f898824` | as `vc6/nativecrt` |
| `MSVCRX.DLL` | `fb6260b07c05bdbcb6928156da514be5b6b69f095b335692ba46c561d85bd5f8` (the CD's `MSVCRT.DLL`) | `887eb5ce93edb7192ca3e9220f07f9ca0f94db02af5862ebcbdfcb852db99fd1` (SP4's) |

The clients also link `utils/vgui/lib/win32_vc6/vgui.lib` from their own
branch: SDK 2.0's in Point of View, Wanted!, Gunman Chronicles and They
Hunger (`df1d49f874c87ba1fbcfe0d164bec90d7940e81341c01c001a701b85ef46677a`;
SDK 2.1, They Hunger's base, ships the same file), SDK 2.2's in T2DEF
(`2f3a070def04fb0d542c1a10c2cc005d18d15fdd32d547a91e0c8e43bfb99785`).
Counter-Strike's VGUI clients link SDK 2.0's from Beta 6.5 to 1.1, SDK
2.2's in 1.3 and SDK 2.3's in 1.4 and 1.5
(`5124e919ad7a5c0583753d4e41a834026596da940c7b6879c04b56afb76cdf49`).

Besides the toolchain, the build reproduces what the image records about
the build itself: the PDB path, the link time, the PDB's age and the build
history. [reproducing.md](reproducing.md) explains those.
