# Rebuilding They Hunger (episodes 1 to 3)

Both DLLs rebuild identical to retail. Needs the [shared steps](README.md)
with the tree `vc6`. Background: [they-hunger.md](../they-hunger.md).

1. Get the retail files (optional, for `cmp`): `TheyHunger3.exe` from
   <https://www.moddb.com/mods/they-hunger/downloads/they-hunger-1-2-and-3>
   into `$DL` (a self-extracting zip; 7-Zip unpacks it without running it):

   ```sh
   7z x -o"$WORK/retail/they-hunger" "$DL/TheyHunger3.exe"
   sha256sum "$WORK/retail/they-hunger/Hunger/dlls/einar.dll" "$WORK/retail/they-hunger/Hunger/cl_dlls/client.dll"
   ```

   ```
   0b9c21c87dab0eec02b323eaab434cf77a2f69982bbf0a3f3f6a316fa95159d3  .../dlls/einar.dll
   09e8de1a62f4f1fae32b040678fc5b97fe5ba8f20ec44db14b29aedcf37db105  .../cl_dlls/client.dll
   ```

2. Check out the branch:

   ```sh
   git -C "$MODS" worktree add "$WORK/src/they-hunger" they-hunger
   ```

3. Build and compare (about two minutes):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/they-hunger/Hunger tools/release/they-hunger.sh "$WORK/src/they-hunger" "$WORK/build/they-hunger"
   ```

   The server's sources sit at `C:\hlsrc11`, retail's path: drive `C:` of
   the prefix copy in the build folder.

4. Success: exit code 0, and for each DLL `identical to retail` with the
   SHA-256 of step 1 and `cmp …: no difference`.
