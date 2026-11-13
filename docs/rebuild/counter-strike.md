# Rebuilding Counter-Strike, Beta 1.0 to 1.5

Every release is a tag on branch `counter-strike` (`cstrike-beta1.0` …
`cstrike-1.5`). All 23 servers and 15 of the 17 clients rebuild identical
to retail; the clients of 1.4 and 1.5 do not. Needs the
[shared steps](README.md) with the trees `vc5/rtm`, `vc6`, `vc6sp4`,
`vc6sp5` and `vc6sp5pp`. Background:
[counter-strike.md](../counter-strike.md).

1. Get the retail files (optional, for `cmp`; the script checks the
   SHA-256 without them). They are in Ch0wW's archive, one branch per
   release; [counter-strike.md](../counter-strike.md#where-the-retail-files-come-from)
   lists branch and folder, [its status table](../counter-strike.md#status)
   the SHA-256. For Beta 1.0:

   ```sh
   git clone --depth 1 --branch csbeta10 https://github.com/Ch0wW/counterstrike-betas "$WORK/retail/cstrike-beta1.0"
   sha256sum "$WORK/retail/cstrike-beta1.0/cstrk10/dlls/mp.dll" "$WORK/retail/cstrike-beta1.0/cstrk10/cl_dlls/client.dll"
   ```

   ```
   f4155737d5f190a385da49308a1ef38446f5dd50aaae2362eccff48addac3bf6  .../dlls/mp.dll
   07e181325c522cd612c1059d28c3c173c7200b1c9953f7d9bec418a4d6055564  .../cl_dlls/client.dll
   ```

2. Check out a release by its tag:

   ```sh
   git -C "$MODS" worktree add --detach "$WORK/src/cstrike-beta1.0" cstrike-beta1.0
   ```

3. Build and compare (Beta 1.0 takes about seven minutes: its server
   replays 1,215 links; releases from Beta 6.5 on take about one):

   ```sh
   cd "$MODS"
   RETAIL=$WORK/retail/cstrike-beta1.0/cstrk10 tools/release/counter-strike.sh cstrike-beta1.0 "$WORK/src/cstrike-beta1.0" "$WORK/build/cstrike-beta1.0"
   ```

   The server's sources sit at the path retail's debug record names, on a
   drive of the prefix copy in the build folder: `D:` for Betas 1.0 to 4.1
   and 5.2 to 7.1 (`D:\half-life\code2` and others), `C:` for Beta 5.0 and
   1.0b, `F:` for 1.0, `U:` for 1.1 to 1.5. Betas 2.1, 3.0, 3.1 and 6.1
   continue an earlier release's build; the script takes its source from
   the repository by tag.

4. Success: exit code 0, and for each DLL `identical to retail` with the
   SHA-256 of step 1 and `cmp …: no difference`.

5. All releases, one after another (about 45 minutes):

   ```sh
   cd "$MODS"
   mkdir -p "$WORK/build"
   for t in $(git tag -l 'cstrike-*'); do
       [ -d "$WORK/src/$t" ] || git worktree add --detach "$WORK/src/$t" "$t"
       tools/release/counter-strike.sh "$t" "$WORK/src/$t" "$WORK/build/$t" > "$WORK/build/$t.log" 2>&1
       echo "$t: exit code $?"
   done
   cat "$WORK"/build/cstrike-*.log | grep -c 'identical to retail'
   ```

   Expected: exit code 0 for every tag but `cstrike-1.4` and `cstrike-1.5`,
   and 51 `identical to retail` lines (two per tag; one for
   `cstrike-beta6.5d`, which ships Beta 6.5's DLLs and whose tree builds
   Beta 6.6's client). For 1.4 and 1.5 the server is identical and the
   client log ends:

   ```
   differs from retail
   …
   not identical, as known: this is the SHA-256 the release's commit message records for this tree
   ```

   with SHA-256 `c2c6e9b895eba6d17ea9d206e36786893524fe0768e4299be7f6bcaf1f674d56`
   (1.4) and `e8b1191cfa8dc959f900d37fefe8de34c0dd65b84309becf63cd3937286d0009`
   (1.5): those clients ship in the engine's packed format, and 18 of their
   functions differ from retail's in instruction order
   ([counter-strike.md](../counter-strike.md#what-is-not-identical-in-the-14-and-15-clients)).
