# Compilers

All eight DLLs were built with Microsoft Visual C++ 6.0 (VC6), in three
variants: the Professional edition as first released, the Standard edition
as first released, and the Standard edition with Service Pack 4. This page
says which variant built which DLL, how that is known, where each variant
comes from, and which files of it matter. [setup.md](setup.md) turns the
discs into the toolchain folder.

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
  Published lists of these build numbers give 8168 for Service Packs 1 and
  2 as well, so the stamps alone don't exclude those two. They were not
  tried; the RTM files rebuild the DLLs exactly.
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

The eight retail files (read from the files themselves; product ID, build,
count):

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

How to read it:
- **Edition.** Six DLLs have only Professional stamps (0x0a, 0x0b). In the
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
  `vgui.lib` was made with build 8168, SDK 2.2's (T2DEF) with 8447.
- **Object counts.** Every object counts, including empty ones and alias
  objects. LINK also sizes the header from the number of objects, so one
  object too few moves the PE header. The mods' pages list the placeholder
  objects that make the counts match.

### The code

- **Professional, `/O2`.** Functions address their locals relative to
  `esp`, without a frame pointer. In Gunman Chronicles 4% (server) and 6%
  (client) of the functions set up a frame pointer, in the Wanted! server 2
  of 2,978 user functions of 16 bytes or more. A Standard edition compiler
  can't produce that code.
- **Standard edition.** The T2DEF server's code is `/Od` code (every
  function has a frame pointer, no optimisation) although `hl.dsp` says
  `/O2`. With the Standard back end the project's own flags give it.
- **Rebuilding.** Each variant was confirmed by rebuilding: six of the
  eight DLLs are byte-identical. The Wanted! server differs inside one
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

## Where the compilers come from

Three discs from the Internet Archive. Both editions of VC6 as first
released are on two CDs of February 1999; Service Pack 4 is on a disc of
August 2000. [setup.md](setup.md) has the steps.

| Disc | archive.org item, file | SHA-256 of the file | SHA-1 (archive.org) |
|---|---|---|---|
| Visual C++ 6.0 Professional Edition, English retail CD (part no. X04-58170, matrix BR11577-103) | [`X04-58170`](https://archive.org/details/X04-58170), `1999-02_X04-58170_X04-58170_VCP600ENU1.7z` | `4e3fa95110e263fe4a42fe151b3f5208d414db72c006beee999a76c6c0ed4261` | `7d476db1e8eaebe5733fbe95cb7a6bd01132c479` |
| Visual C++ 6.0 Learning Edition, English (X04-58202, matrix BR11678-353) | [`X04-58202`](https://archive.org/details/X04-58202), `1999-02_X04-58202_X04-58202_VCL600ENU1.7z` | `7ed8ad7ebdbc29e8c1512c6530d267dd48804ac1860344191c99cb7234c3d9c3` | `cec6f224387bb9121f4d91c10818847738647973` |
| Visual Studio 6.0 Service Pack 4 (X05-86594, matrix X05-86829) | [`X05-86594`](https://archive.org/details/X05-86594), `2000-08_X05-86594_X05-86829_vs6sp4.7z` | `d67c8795c59fcbf0d67c468cb747bc28a9f15cb88a532896c1449e1e6da7251a` | `7ba4001a8e6a3af8f9807d85471b5ea83000eae8` |

What each tree takes from them:

| Tree | Contents |
|---|---|
| `vc6` | The Professional CD's `VC98/BIN`, `VC98/INCLUDE`, `VC98/LIB`, plus `MSPDB60.DLL`, `MSOBJ10.DLL` and `MSDIS110.DLL` from its `COMMON/MSDEV98/BIN`. `nativecrt` uses the CD's `OS/SYSTEM/MSVCRT.DLL`. |
| `vc6std` | `vc6` with `C2.DLL` and `CL.EXE` from the Learning Edition CD (11.99.8168), the only two files in which its `VC98/BIN` differs from the Professional CD's. That `C2.DLL` is the Standard edition's back end: its objects carry the Standard stamps. `nativecrt` uses the Professional CD's `MSVCRT.DLL` (the two CDs' copies are identical). |
| `vc6sp4std` | `vc6` with SP4's English VC files (`enu/vc98/bin`, `include`, `lib`, and `enu/common/msdev98/bin/msdis110.dll`), and SP4's Standard back end `enu/os/system/msse.dll` as `C2.DLL`. `nativecrt` uses SP4's `enu/os/system/msvcrt.dll`. |
| `vc6sp4` | The same with the Professional back end `enu/os/system/msvcep.dll` as `C2.DLL`. Not used by these four mods. |

The C runtime DLLs the `nativecrt` copies run on:

| File | Version | Size | SHA-1 | SHA-256 |
|---|---|---|---|---|
| VC6 CD `OS/SYSTEM/MSVCRT.DLL` (1998-06-17) | 6.00.8168.0 | 254,005 | `11179154ec7375e7dc50f2b349c571d01a8856b9` | `fb6260b07c05bdbcb6928156da514be5b6b69f095b335692ba46c561d85bd5f8` |
| SP4 `enu/os/system/msvcrt.dll` | 6.00.8797.0 | 278,581 | `63a4fcd64ecea975c1b91de04702c68a9f2a3c7d` | `887eb5ce93edb7192ca3e9220f07f9ca0f94db02af5862ebcbdfcb852db99fd1` |

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
branch: SDK 2.0's in Point of View, Wanted! and Gunman Chronicles
(`df1d49f874c87ba1fbcfe0d164bec90d7940e81341c01c001a701b85ef46677a`), SDK
2.2's in T2DEF (`2f3a070def04fb0d542c1a10c2cc005d18d15fdd32d547a91e0c8e43bfb99785`).

Besides the toolchain, the build reproduces what the image records about
the build itself: the PDB path, the link time, the PDB's age and the build
history. [reproducing.md](reproducing.md) explains those.
