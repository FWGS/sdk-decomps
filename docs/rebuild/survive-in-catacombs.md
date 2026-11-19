# Rebuilding Survive in Catacombs 1 and 2

All four DLLs, two per game, rebuild identical to retail from one checkout
of branch `sic`. Needs the [shared steps](README.md) with the tree `vc6`,
and `bsdtar` (libarchive) to unpack the mods' RAR archives. Background:
[survive-in-catacombs.md](../survive-in-catacombs.md).

1. Get the retail files (optional, for `cmp`; the script checks the
   SHA-256 without them): the mods' release archives, mirrored on the
   Internet Archive (about 230 MB together):

   ```sh
   cd "$DL"
   curl -L -o survive-in-catacombs.rar 'https://archive.org/download/half-lifeseriesmods_20191215_1013/Survive%20in%20Catacombs/Survive%20in%20Catacombs.rar'
   curl -L -o survive-in-catacombs-2.rar 'https://archive.org/download/half-lifeseriesmods_20191215_1013/Survive%20in%20Catacombs%202.rar'
   sha256sum -c <<'EOF'
   995d3e9dabadaf0031031653fe2972f8dd4e7f6db970745c2da3075510f03c97  survive-in-catacombs.rar
   f045e41022610f77c5f1d56bee6d1f5d92715e8c9657fe99388af36029fb222c  survive-in-catacombs-2.rar
   EOF
   mkdir -p "$WORK/retail/sic1" "$WORK/retail/sic2"
   bsdtar -xf survive-in-catacombs.rar -C "$WORK/retail/sic1" Catacombs/Dlls Catacombs/Cl_dlls
   bsdtar -xf survive-in-catacombs-2.rar -C "$WORK/retail/sic2" Bloodbath/Dlls Bloodbath/Cl_dlls
   cd "$WORK/retail"
   sha256sum sic1/Catacombs/Dlls/sic.dll sic1/Catacombs/Cl_dlls/client.dll sic2/Bloodbath/Dlls/bloodbath.dll sic2/Bloodbath/Cl_dlls/client.dll
   ```

   Both archives must print `OK`, then:

   ```
   eab217809f7c656acb7678b2035a4dc7f82a08db276febaceef72dfa55509359  sic1/Catacombs/Dlls/sic.dll
   0cb9fd2beed7c6ed327545a280826ecaaa5269ae3539c9a0602522cca3e6527a  sic1/Catacombs/Cl_dlls/client.dll
   673f7e4ef7c75fdf551c87470f6e4f8a5a66f16fe98bfb44ca6436d14c88ba57  sic2/Bloodbath/Dlls/bloodbath.dll
   76cc187b538b7fa0ae6bea5842e1484369fb64c6d7184851fb77de96cedbe21a  sic2/Bloodbath/Cl_dlls/client.dll
   ```

2. Check out the branch:

   ```sh
   git -C "$MODS" worktree add "$WORK/src/sic" sic
   ```

3. Build and compare, one game per command (about two minutes each; the
   two can run at the same time):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/sic1/Catacombs tools/release/sic.sh 1 "$WORK/src/sic" "$WORK/build/sic1"
   RETAIL=$WORK/retail/sic2/Bloodbath tools/release/sic.sh 2 "$WORK/src/sic" "$WORK/build/sic2"
   ```

   The first argument is the game. Survive in Catacombs' sources sit at
   `F:\SicEngineCodes`, retail's path: drive `F:` of the prefix copy in
   the build folder is `$WORK/build/sic1/root`. Survive in Catacombs 2's
   sit at `C:\bloody\bloodyEnginecodes`, in the prefix copy's drive `C:`.

4. Success: exit code 0 for each game, and for each of the four DLLs
   `identical to retail` with the SHA-256 of step 1 and `cmp …: no
   difference`.

5. Optional: the historical trees. The branch's history has each game's
   tree as it was before the two were joined: `ec3024d` (Survive in
   Catacombs) and `21fc2ea` (Survive in Catacombs 2). The same script
   builds them into the same files:

   ```sh
   git -C "$MODS" worktree add --detach "$WORK/src/sic-ec3024d" ec3024d
   git -C "$MODS" worktree add --detach "$WORK/src/sic-21fc2ea" 21fc2ea
   cd "$MODS"
   RETAIL=$WORK/retail/sic1/Catacombs tools/release/sic.sh 1 "$WORK/src/sic-ec3024d" "$WORK/build/sic1-ec3024d"
   RETAIL=$WORK/retail/sic2/Bloodbath tools/release/sic.sh 2 "$WORK/src/sic-21fc2ea" "$WORK/build/sic2-21fc2ea"
   ```

   Success as in step 4.
