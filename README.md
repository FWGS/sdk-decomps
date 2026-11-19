# Half-Life mods, rebuilt byte for byte

This repository holds C++ source for the game DLLs of nine Half-Life mods,
reconstructed from the shipped binaries. Each tree compiles with the original
compiler back into the retail files: Visual C++ 6.0 for eight mods, and for
Counter-Strike, from its first beta in 1999 to version 1.5 in 2002, the five
compilers it went through. Of the 56 distinct DLLs, 52 rebuild identical to
the retail ones, byte for byte. That includes the headers, time stamps,
debug record and padding, so the SHA-256 matches.

Each mod lives on its own branch; Survive in Catacombs and its sequel share
one tree, which builds both. `main` holds this summary, the documentation
and the build tools.

## Status

Verified with fresh release builds, by SHA-256 and `cmp` against the retail
files: Counter-Strike on 2026-11-14, every tag. The shared build tools
changed for it, so the other five mods' DLLs were rebuilt the same day from
fresh clones of their branches: the eight identical ones byte for byte
again, and the two others with their known hashes. Scientist
Slaughterhouse on 2026-11-19: both DLLs from a fresh clone of the
repository, following [its rebuild page](docs/rebuild/scientist-slaughterhouse.md)
as written. The shared build tools gained options for it that are off by
default; the other six mods' DLLs, rebuilt the same day with the changed
tools from fresh clones of their branches and tags, came out as before:
every identical one byte for byte, the four others with their known
hashes. Survive in Catacombs 1 and 2 on 2026-11-19 too: all four DLLs
from a fresh clone of the repository, following
[their rebuild page](docs/rebuild/survive-in-catacombs.md) as written.

| Mod | Branch | Server DLL | Client DLL | Details |
|---|---|---|---|---|
| Point of View 1.1 | [`point-of-view`](../../tree/point-of-view) | `hl.dll`: **identical** | `client.dll`: **identical** | [docs/point-of-view.md](docs/point-of-view.md) |
| T2DEF (Todesangst 2) 1.1 | [`t2def`](../../tree/t2def) | `t2def.dll`: **identical** | `client.dll`: **identical** | [docs/t2def.md](docs/t2def.md) |
| Wanted! 1.5 | [`wanted`](../../tree/wanted) | `wanted.dll`: 810 of 1,122,380 bytes differ | `client.dll`: **identical** | [docs/wanted.md](docs/wanted.md) |
| Gunman Chronicles | [`gunman-chronicles`](../../tree/gunman-chronicles) | `gunman.dll`: **identical** | `client.dll`: 152,034 of 565,248 bytes differ | [docs/gunman-chronicles.md](docs/gunman-chronicles.md) |
| They Hunger (episodes 1 to 3) | [`they-hunger`](../../tree/they-hunger) | `einar.dll`: **identical** | `client.dll`: **identical** | [docs/they-hunger.md](docs/they-hunger.md) |
| Counter-Strike, Beta 1.0 to 1.5 (27 tags) | [`counter-strike`](../../tree/counter-strike) | `mp.dll`: **all 23 identical** | `client.dll`: **15 of 17 identical**; 1.4 and 1.5 not | [docs/counter-strike.md](docs/counter-strike.md) |
| Scientist Slaughterhouse 1.0 | [`scientist-slaughterhouse`](../../tree/scientist-slaughterhouse) | `hl.dll`: **identical** | `client.dll`: **identical** | [docs/scientist-slaughterhouse.md](docs/scientist-slaughterhouse.md) |
| Survive in Catacombs 1 and 2 (one tree) | [`sic`](../../tree/sic) | `sic.dll`, `bloodbath.dll`: **both identical** | `client.dll`: **both identical** | [docs/survive-in-catacombs.md](docs/survive-in-catacombs.md) |

SHA-256 of the retail files of the first five mods, Scientist
Slaughterhouse and Survive in Catacombs, which the identical builds
reproduce (Counter-Strike's 40 are in [its page](docs/counter-strike.md#status)):

| File | SHA-256 |
|---|---|
| Point of View `hl.dll` | `0f42bad4664a87cf3793efd213088b9c2c5dd0ac230b45db3544fd7c344f7ccd` |
| Point of View `client.dll` | `c0a40333d725c4a8d66e2640e78929ebc69bd118c28d3b79e0aab35d69608ea1` |
| T2DEF `t2def.dll` | `ba8fea632d5eca98b9e68078e57a3aa37e07acd872063746d4c597c2ea076135` |
| T2DEF `client.dll` | `cbd18f9318fe5a626bb35318f70db6a340300d8c6874f8b64e7bdb53f5e91762` |
| Wanted! `client.dll` | `584813e7ec904ec6fa596fa1360d3da37bfc87cfb1b1785886b8d868d793e479` |
| Gunman Chronicles `gunman.dll` | `e6e8d4497f11787889f12e7356157cbce98d380570805ca55faef3c07ce088f0` |
| They Hunger `einar.dll` | `0b9c21c87dab0eec02b323eaab434cf77a2f69982bbf0a3f3f6a316fa95159d3` |
| They Hunger `client.dll` | `09e8de1a62f4f1fae32b040678fc5b97fe5ba8f20ec44db14b29aedcf37db105` |
| Scientist Slaughterhouse `hl.dll` | `8e4c150e8d8d57f13bbf7f6b5a5e4ebc37258838ccb7708c760bb7b3c139d4ae` |
| Scientist Slaughterhouse `client.dll` | `1e4f2d5c2a6d92ac84f8bdf1fb46abbffabcb78107909aa44c2a75f84baf1365` |
| Survive in Catacombs `sic.dll` | `eab217809f7c656acb7678b2035a4dc7f82a08db276febaceef72dfa55509359` |
| Survive in Catacombs `client.dll` | `0cb9fd2beed7c6ed327545a280826ecaaa5269ae3539c9a0602522cca3e6527a` |
| Survive in Catacombs 2 `bloodbath.dll` | `673f7e4ef7c75fdf551c87470f6e4f8a5a66f16fe98bfb44ca6436d14c88ba57` |
| Survive in Catacombs 2 `client.dll` | `76cc187b538b7fa0ae6bea5842e1484369fb64c6d7184851fb77de96cedbe21a` |

What the four remaining DLLs still lack:

- **Wanted! `wanted.dll`:** every function but one is identical at retail's
  address. In `CBot::Wander` the compiler emits one floating-point multiply
  in a 4-byte-shorter form than retail's, and the rest of that function
  shifts behind it.
- **Gunman Chronicles `client.dll`:** all but three functions compile to
  retail's code. About a third of the code sits 16 to 128 bytes away from
  retail's addresses. The linker's identical-code folding keeps a different
  copy of some identical functions. Which copy it keeps depends on dead code
  that the retail binary doesn't record.
- **Counter-Strike 1.4 and 1.5 `client.dll`:** these ship in the engine's
  packed format, not as PE files; the rebuilt DLL is packed the same way and
  compared. 18 functions differ from retail in the order of their
  instructions only, and no compiler tried gives that order, so the files
  are not identical. The work on them stopped there.

## What "identical" takes

Identical code is not enough. A DLL also records how it was built:
- the path of its PDB file;
- the link time;
- how often the PDB was written;
- how many object files went in;
- the order of its imports;
- the state of the precompiled header when each file was compiled.

The release scripts rebuild all of that. [docs/reproducing.md](docs/reproducing.md)
explains each part.

Counter-Strike's betas add Visual C++ 5.0, whose code depends on how many
declarations a file has seen, and releases built on top of an earlier
release's build folder; [docs/counter-strike.md](docs/counter-strike.md)
explains what that took. Scientist Slaughterhouse's client was linked
incrementally, so its layout records a history of links, and the order of
its RTTI records follows the lengths of the compiler's own paths;
[docs/scientist-slaughterhouse.md](docs/scientist-slaughterhouse.md)
explains those. Survive in Catacombs' builds need the projects' browse
information, which changes code, and one tree builds both games;
[docs/survive-in-catacombs.md](docs/survive-in-catacombs.md) explains
those.

Where the binary doesn't say what the source was, the trees contain a marked
stand-in. Examples: a function nothing calls, an empty object file, a
`// sic` line that steers the compiler. So "identical" is a statement about
the built file, not about every line of the source. The per-mod documents
list every stand-in with the evidence for it.

## Rebuilding

You need:
- the Visual C++ discs ([docs/compilers.md](docs/compilers.md)), and for
  Counter-Strike also Visual C++ 5.0 and VC6's Service Pack 5 and Processor
  Pack;
- Wine, libfaketime and Python 3;
- the retail game files, if you want to compare them yourself.

[docs/rebuild/](docs/rebuild/README.md) has the steps as exact commands,
per mod; [docs/setup.md](docs/setup.md) explains the toolchain folder they
build. Then, for example:

```sh
git worktree add ../pov point-of-view
VC_TOOLCHAINS=/path/to/toolchains tools/release/point-of-view.sh ../pov ../pov-build
```

The script prints each built file's SHA-256 next to retail's.
Counter-Strike's script takes the release's tag first, because several
releases continue an earlier one's build:

```sh
git worktree add ../cs-1.0 cstrike-1.0
VC_TOOLCHAINS=/path/to/toolchains tools/release/counter-strike.sh cstrike-1.0 ../cs-1.0 ../cs-1.0-build
```

Survive in Catacombs' script takes the game, `1` or `2`, first: one
checkout of its branch builds both games.

```sh
git worktree add ../sic sic
VC_TOOLCHAINS=/path/to/toolchains tools/release/sic.sh 2 ../sic ../sic2-build
```

## Documentation

- [docs/rebuild/](docs/rebuild/README.md): step by step, with exact commands, how to rebuild each mod's DLLs and check them.
- [docs/compilers.md](docs/compilers.md): which compiler and linker built each DLL, and where they come from.
- [docs/setup.md](docs/setup.md): setting up the compilers under Wine.
- [docs/reproducing.md](docs/reproducing.md): what a DLL holds besides its code, and how each part is rebuilt.
- [docs/vc6-notes.md](docs/vc6-notes.md): what we learned about Visual C++ 6.0's compiler and linker while matching.
- Per mod: [Point of View](docs/point-of-view.md), [T2DEF](docs/t2def.md), [Wanted!](docs/wanted.md), [Gunman Chronicles](docs/gunman-chronicles.md), [They Hunger](docs/they-hunger.md), [Counter-Strike](docs/counter-strike.md), [Scientist Slaughterhouse](docs/scientist-slaughterhouse.md), [Survive in Catacombs 1 and 2](docs/survive-in-catacombs.md).

## Thanks

- **Ch0wW**, for archiving the historical versions of Counter-Strike, from
  Beta 1.0 on: <https://github.com/Ch0wW/counterstrike-betas>. The
  Counter-Strike reconstruction starts from those files.
- **oxiKKK**, for the decryptor of the engine's packed DLL format, which
  documents how Counter-Strike 1.4's and 1.5's `client.dll` are stored:
  <https://github.com/oxiKKK/goldsource-dll-decryptor>.

## Not included

The compilers and the retail game files are copyrighted, so they are not
here. The documents name where to get them and their hashes. The mod trees
are based on Valve's Half-Life SDK, and their files keep their original
copyright headers.
