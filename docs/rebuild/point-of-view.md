# Rebuilding Point of View 1.1

Both DLLs rebuild identical to retail. Needs the [shared steps](README.md)
with the tree `vc6`. Background: [point-of-view.md](../point-of-view.md).

1. Get the retail files (optional, for `cmp`): install Point of View 1.1
   (`pov.exe` from <https://www.moddb.com/mods/point-of-view/downloads/point-of-view-11>;
   7-Zip can't open that installer) and copy its mod folder `pov` to
   `$WORK/retail/pov`. Check it:

   ```sh
   sha256sum "$WORK/retail/pov/dlls/hl.dll" "$WORK/retail/pov/cl_dlls/client.dll"
   ```

   ```
   0f42bad4664a87cf3793efd213088b9c2c5dd0ac230b45db3544fd7c344f7ccd  .../dlls/hl.dll
   c0a40333d725c4a8d66e2640e78929ebc69bd118c28d3b79e0aab35d69608ea1  .../cl_dlls/client.dll
   ```

2. Check out the branch:

   ```sh
   git -C "$MODS" worktree add "$WORK/src/point-of-view" point-of-view
   ```

3. Build and compare (about two minutes):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/pov tools/release/point-of-view.sh "$WORK/src/point-of-view" "$WORK/build/point-of-view"
   ```

   The server's sources sit at `D:\Editors\Levels\POVSourceCode`, retail's
   path: drive `D:` of the prefix copy in the build folder.

4. Success: exit code 0, and for each DLL `identical to retail` with the
   SHA-256 of step 1 and `cmp …: no difference`. Without `RETAIL` the
   `cmp` lines are left out.
