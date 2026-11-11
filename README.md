# Half-Life mods, rebuilt byte for byte

This repository holds C++ source for the game DLLs of five Half-Life mods,
reconstructed from the shipped binaries. Each tree compiles with the original
compiler, Visual C++ 6.0, back into the retail files. For eight of the ten
DLLs, the rebuilt file is identical to the retail one, byte for byte. That
includes the headers, time stamps, debug record and padding, so the SHA-256
matches.

Each mod lives on its own branch. `main` holds this summary, the
documentation and the build tools.

## Status

Verified with fresh release builds, by SHA-256 and `cmp` against the retail
files: They Hunger on 2026-11-11, the other four mods on 2026-11-01.

| Mod | Branch | Server DLL | Client DLL | Details |
|---|---|---|---|---|
| Point of View 1.1 | [`point-of-view`](../../tree/point-of-view) | `hl.dll`: **identical** | `client.dll`: **identical** | [docs/point-of-view.md](docs/point-of-view.md) |
| T2DEF (Todesangst 2) 1.1 | [`t2def`](../../tree/t2def) | `t2def.dll`: **identical** | `client.dll`: **identical** | [docs/t2def.md](docs/t2def.md) |
| Wanted! 1.5 | [`wanted`](../../tree/wanted) | `wanted.dll`: 810 of 1,122,380 bytes differ | `client.dll`: **identical** | [docs/wanted.md](docs/wanted.md) |
| Gunman Chronicles | [`gunman-chronicles`](../../tree/gunman-chronicles) | `gunman.dll`: **identical** | `client.dll`: 152,034 of 565,248 bytes differ | [docs/gunman-chronicles.md](docs/gunman-chronicles.md) |
| They Hunger (episodes 1 to 3) | [`they-hunger`](../../tree/they-hunger) | `einar.dll`: **identical** | `client.dll`: **identical** | [docs/they-hunger.md](docs/they-hunger.md) |

SHA-256 of the retail files, which the identical builds reproduce:

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

What the two remaining DLLs still lack:

- **Wanted! `wanted.dll`:** every function but one is identical at retail's
  address. In `CBot::Wander` the compiler emits one floating-point multiply
  in a 4-byte-shorter form than retail's, and the rest of that function
  shifts behind it.
- **Gunman Chronicles `client.dll`:** all but three functions compile to
  retail's code. About a third of the code sits 16 to 128 bytes away from
  retail's addresses. The linker's identical-code folding keeps a different
  copy of some identical functions. Which copy it keeps depends on dead code
  that the retail binary doesn't record.

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

Where the binary doesn't say what the source was, the trees contain a marked
stand-in. Examples: a function nothing calls, an empty object file, a
`// sic` line that steers the compiler. So "identical" is a statement about
the built file, not about every line of the source. The per-mod documents
list every stand-in with the evidence for it.

## Rebuilding

You need:
- the Visual C++ discs ([docs/compilers.md](docs/compilers.md));
- Wine, libfaketime and Python 3;
- the retail game files, if you want to compare them yourself.

[docs/setup.md](docs/setup.md) builds the toolchain folder. Then, for example:

```sh
git worktree add ../pov point-of-view
VC_TOOLCHAINS=/path/to/toolchains tools/release/point-of-view.sh ../pov ../pov-build
```

The script prints each built file's SHA-256 next to retail's.

## Documentation

- [docs/compilers.md](docs/compilers.md): which compiler and linker built each DLL, and where they come from.
- [docs/setup.md](docs/setup.md): setting up the compilers under Wine.
- [docs/reproducing.md](docs/reproducing.md): what a DLL holds besides its code, and how each part is rebuilt.
- [docs/vc6-notes.md](docs/vc6-notes.md): what we learned about Visual C++ 6.0's compiler and linker while matching.
- Per mod: [Point of View](docs/point-of-view.md), [T2DEF](docs/t2def.md), [Wanted!](docs/wanted.md), [Gunman Chronicles](docs/gunman-chronicles.md), [They Hunger](docs/they-hunger.md).

## Not included

The compilers and the retail game files are copyrighted, so they are not
here. The documents name where to get them and their hashes. The mod trees
are based on Valve's Half-Life SDK, and their files keep their original
copyright headers.
