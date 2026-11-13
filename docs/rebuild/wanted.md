# Rebuilding Wanted! 1.5

The client rebuilds identical to retail; the server does not yet (810 of
its 1,122,380 bytes differ, inside one function). Needs the
[shared steps](README.md) with the tree `vc6`. Background:
[wanted.md](../wanted.md).

1. Get the retail files (optional, for `cmp`): `wantedsp.zip` from
   <https://www.moddb.com/mods/wantedhl/downloads/wanted-for-half-life-original-won-version>
   into `$DL`, then:

   ```sh
   7z x -o"$WORK/retail/wanted" "$DL/wantedsp.zip"
   sha256sum "$WORK/retail/wanted/wantedsp/dlls/wanted.dll" "$WORK/retail/wanted/wantedsp/cl_dlls/client.dll"
   ```

   ```
   2e3233515c6e5cc7b77a84bb514212583997b10497b6ca96a1ca57c53ac25875  .../dlls/wanted.dll
   584813e7ec904ec6fa596fa1360d3da37bfc87cfb1b1785886b8d868d793e479  .../cl_dlls/client.dll
   ```

2. Check out the branch:

   ```sh
   git -C "$MODS" worktree add "$WORK/src/wanted" wanted
   ```

3. Build and compare (about two minutes):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/wanted/wantedsp tools/release/wanted.sh "$WORK/src/wanted" "$WORK/build/wanted"
   ```

   The server's sources sit at `K:\Wantedhl\Source Code\New Code\Code`,
   retail's path: drive `K:` of the prefix copy in the build folder.

4. Expected: the client `identical to retail` and `cmp …: no difference`;
   the server `differs from retail` with SHA-256
   `e4dfb4369731a2a36c2ef71b4680361da7912f932bc78c5bb2a9cc4df696c99f` and
   `810 of 1122380 bytes differ`. Exit code 1 because of the server.
