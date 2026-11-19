# Rebuilding Scientist Slaughterhouse 1.0

Both DLLs rebuild identical to retail. Needs the [shared steps](README.md)
with the tree `vc6`. Background:
[scientist-slaughterhouse.md](../scientist-slaughterhouse.md).

1. Get the retail files (optional, for `cmp`; the script checks the
   SHA-256 without them): the mod's installer `ssh10.exe` (where to get
   it is not given here yet) into `$DL`. It is a self-extracting zip;
   7-Zip unpacks it without running it (and warns about data after the end
   of the archive, which is harmless):

   ```sh
   cd "$DL"
   sha256sum -c <<'EOF'
   98497a52522c5e7bb06f2c9a6a473c9cb8c80de5015a2efb4c23a74315466aeb  ssh10.exe
   EOF
   7z x -o"$WORK/retail/scientist-slaughterhouse" "$DL/ssh10.exe" ssh/DLLS/hl.dll ssh/cl_dlls/client.dll
   cd "$WORK/retail/scientist-slaughterhouse"
   sha256sum ssh/DLLS/hl.dll ssh/cl_dlls/client.dll
   ```

   ```
   8e4c150e8d8d57f13bbf7f6b5a5e4ebc37258838ccb7708c760bb7b3c139d4ae  ssh/DLLS/hl.dll
   1e4f2d5c2a6d92ac84f8bdf1fb46abbffabcb78107909aa44c2a75f84baf1365  ssh/cl_dlls/client.dll
   ```

2. Check out the branch:

   ```sh
   git -C "$MODS" worktree add "$WORK/src/scientist-slaughterhouse" scientist-slaughterhouse
   ```

3. Build and compare (about a quarter of an hour: the server's 1,433
   links take most of it, the client two minutes):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/scientist-slaughterhouse/ssh tools/release/scientist-slaughterhouse.sh "$WORK/src/scientist-slaughterhouse" "$WORK/build/scientist-slaughterhouse"
   ```

   The sources sit at `C:\Microsoft Visual Studio\MyProjects\Source Code`,
   retail's path, and the compiler at `C:\Microsoft Visual Studio\VC98`:
   drive `C:` of the prefix copy in the build folder.

4. Success: exit code 0, and for each DLL `identical to retail` with the
   SHA-256 of step 1 and `cmp …: no difference`.
