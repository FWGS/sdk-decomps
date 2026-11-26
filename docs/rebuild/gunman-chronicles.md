# Rebuilding Gunman Chronicles

Both DLLs rebuild identical to retail. Needs the [shared steps](README.md) with the
tree `vc6`. Background: [gunman-chronicles.md](../gunman-chronicles.md).

1. Get the retail files (optional, for `cmp`): the game's `rewolf` folder
   from an installation of the retail CD (version 1.0.0.0), copied to
   `$WORK/retail/rewolf`. Check it:

   ```sh
   sha256sum "$WORK/retail/rewolf/dlls/gunman.dll" "$WORK/retail/rewolf/cl_dlls/client.dll"
   ```

   ```
   e6e8d4497f11787889f12e7356157cbce98d380570805ca55faef3c07ce088f0  .../dlls/gunman.dll
   4d2918498fb0d3aa40ede643beabea235925c0eaa6742130a686968686a51aed  .../cl_dlls/client.dll
   ```

2. Check out the branch:

   ```sh
   git -C "$MODS" worktree add "$WORK/src/gunman-chronicles" gunman-chronicles
   ```

3. Build and compare (about two minutes):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/rewolf tools/release/gunman-chronicles.sh "$WORK/src/gunman-chronicles" "$WORK/build/gunman-chronicles"
   ```

   The server's sources sit at `C:\Gunman\Source`, retail's path: drive `C:`
   of the prefix copy in the build folder.

4. Expected: both DLLs `identical to retail` and `cmp …: no difference`.
   Exit code 0.Exit code 1 because of the client.
