# Rebuilding T2DEF (Todesangst 2) 1.1

Both DLLs rebuild identical to retail. Needs the [shared steps](README.md)
with the trees `vc6`, `vc6std` and `vc6sp4std`. Background:
[t2def.md](../t2def.md).

1. Get the retail files (optional, for `cmp`): the mod folder of an
   installed T2DEF 1.1 (sources in [t2def.md](../t2def.md#the-retail-files)),
   copied to `$WORK/retail/t2def`. Check it:

   ```sh
   sha256sum "$WORK/retail/t2def/dlls/t2def.dll" "$WORK/retail/t2def/cl_dlls/client.dll"
   ```

   ```
   ba8fea632d5eca98b9e68078e57a3aa37e07acd872063746d4c597c2ea076135  .../dlls/t2def.dll
   cbd18f9318fe5a626bb35318f70db6a340300d8c6874f8b64e7bdb53f5e91762  .../cl_dlls/client.dll
   ```

2. Check out the branch:

   ```sh
   git -C "$MODS" worktree add "$WORK/src/t2def" t2def
   ```

3. Build and compare (about two minutes):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/t2def tools/release/t2def.sh "$WORK/src/t2def" "$WORK/build/t2def"
   ```

   The server's sources sit at `C:\hledit todesangst2\code`, retail's path:
   drive `C:` of the prefix copy in the build folder.

4. Success: exit code 0, and for each DLL `identical to retail` with the
   SHA-256 of step 1 and `cmp …: no difference`. The server's log
   (`server.log`) shows `warning LNK4070`; that is expected.
