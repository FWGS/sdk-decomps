#!/usr/bin/env python3
"""counter-strike_server.py VER SRC OUT: build a Counter-Strike release's
server, mp.dll, as the retail file's build was made, for counter-strike.sh.

VER is a recipe below (b10 ... r15; counter-strike.sh maps the tags to them),
SRC a checkout of the release's commit, OUT the build folder. VC_TOOLCHAINS
and VC_WINEPREFIX come from the environment; VC_WINEPREFIX must be a copy of
the toolchain's prefix (counter-strike.sh makes OUT/wineprefix), because the
build maps a drive letter in it. The built file is OUT/mp.dll.

The build reproduces what the image records about the build, not just the
code (docs/counter-strike.md explains each part; each release's commit
message says what its image showed):

* The tree at the path retail's CodeView record names, e.g.
  D:\\half-life\\code2 for Betas 1.0 to 4.0 (VERSIONS[VER]["tree"]): a tree
  of symbolic links into SRC on that drive of the prefix copy (drive_c, or
  drive_<x> with a dosdevices link). dlls is a real folder of per-file links,
  so an edit can turn one file into a copy with a new time stamp and the
  checkout is never written.
* The IDE's command lines, run from ...\\dlls: CL with mp.dsp's Release flags
  and /Fp, /YX, /Fo, /Fd into the project's Intermediate_Dir, then
  ...\\dlls\\X.cpp; LINK with mp.dsp's line plus /incremental:no, /pdb:,
  /map:, /out:, /implib: into the project's Output_Dir, and the objects in
  project order.
* The compiler: Visual C++ 5.0 as on the CD for Betas 1.0 to 4.1, with CL
  and LINK on the CD's own MSVCRT.DLL (build_dsp.py's vc5-rtm-n; on Wine's
  builtin msvcrt LINK orders two imports differently and leaves stray bytes
  in the MISC debug record); Visual C++ 6.0 as on the CD for Beta 5.0 (leak)
  to 7.1; Service Pack 4 for 1.0 to 1.1, Service Pack 5 for 1.3, SP5 with
  the Processor Pack for 1.4 and 1.5 (VERSIONS[VER]["compiler"]). Every VC6
  tool runs on its release's own MSVCRT.DLL (vc6.sh's default).
* The history that /YX (automatic precompiled headers) leaves in the
  objects: a Rebuild All in project order, then the release's last builds,
  SESSION: each item is one or more edits followed by an IDE Build, i.e. the
  out-of-date sources compiled in project order against the .pch the
  previous compile left, then a link. "h:HEADER": the header is edited (its
  link in the tree becomes a copy with a new time stamp) and every source
  that depends on it is compiled; "c:SOURCE": that source alone. Several
  edits before one Build are joined with "+".
* A release with a BASE (Beta 2.1 on 2.0, 3.0 on 2.1, 3.1 on 3.0, 6.1 on
  6.0) continues the base's build in the same tree, output folder and PDB:
  the base's whole recipe (and its base's, down to the first) runs first on
  the base's source, which this script takes from the same repository by
  tag (git archive into OUT/src-<tag>), then the release's session, whose
  edits bring in the release's text (an edited file becomes a copy of the
  release's file). Every file of dlls the two sources differ in, or that
  the release adds, must be edited in the session; "changed" in an item
  stands for an edit of each such file the session does not name. When the
  release's mp.dsp adds sources (nothing else may change in it), it replaces
  the tree's before the release's first build.
* The PDB: retail's CodeView (NB10) record names the PDB's creation time
  (its signature) and its age. LINK 5.00 creates a PDB at age 0 and adds 1
  per link (/map turns incremental linking off, so every link is a full
  one); LINK 6.00 with /incremental:no creates it at age 1. pdb_links is the
  number of links either way. The Rebuild All's link creates mp.pdb with the
  clock at the creation time; the links between it and the session's are a
  stand-in (relinks of the same objects, which only advance the age); the
  session's links follow, the last at retail's link time. Only LINK runs
  under libfaketime, which sets the COFF header's, the export directory's
  and the debug directory's time stamps. A link time starting with "@" is a
  libfaketime spec: "@T i0.001" is a clock that starts at T and advances
  1 ms per read, for the links whose stamps are a second apart. 1.4 and 1.5
  link without /debug: no PDB record, one link at the link time.
"""
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tarfile
import time

TOOLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.dont_write_bytecode = True  # no __pycache__ in the checkout
sys.path.insert(0, TOOLS)
import build_dsp  # noqa: E402

CONFIG = "mp - Win32 Release"
COMPILER = "vc5-rtm-n"         # Visual C++ 5.0 as on the CD, on its own MSVCRT.DLL; VERSIONS[v]["compiler"] overrides
TREE = r"D:\half-life\code2"   # Betas 1.0 to 4.0; VERSIONS[v]["tree"] overrides

# Per recipe: retail's SHA-256 (also checked for a base on the way), the time stamps (UTC) and the
# build history the image shows. tag: the release's tag, used to find a base's source.
VERSIONS = {
    "b10": dict(
        tag="cstrike-beta1.0",
        sha256="f4155737d5f190a385da49308a1ef38446f5dd50aaae2362eccff48addac3bf6",
        link_time="1999-06-18 21:24:28",    # TimeDateStamp 0x376ab90c (COFF header, exports, debug directory)
        pdb_created="1999-05-08 22:08:13",  # NB10 signature 0x3734b5cd
        pdb_links=1215,                     # NB10 age 1214
        # weapons.h and gamerules.h edited, weapons.cpp built twice: the shortest history that gives every
        # object retail's /YX state
        session=["h:weapons.h", "h:gamerules.h", "c:weapons.cpp", "c:weapons.cpp"],
    ),
    "b11": dict(
        tag="cstrike-beta1.1",
        sha256="577517c643e09c7f9426ff8cde659254df496a79bd18d075a340110bb0a5ecec",
        link_time="1999-06-27 02:40:32",    # 0x37758f20
        pdb_created="1999-06-26 20:35:07",  # 0x3775397b: a new PDB
        pdb_links=17,                       # age 16
        # a Rebuild All made the new PDB; the last build followed an edit of weapons.h: client, combat and
        # effects then write their own .pch, as retail's objects show
        session=["h:weapons.h"],
    ),
    "b20": dict(
        tag="cstrike-beta2.0",
        sha256="258e016a4b257b759bc42dc71769455508309eb86de188c1dcd6ad3cd4c6debe",
        link_time="1999-08-13 05:07:06",    # 0x37b3a7fa
        pdb_created="1999-07-04 17:07:56",  # 0x377f94ec: a new PDB, kept to Beta 3.1
        pdb_links=293,                      # age 292
        # an edit of weapons.h (client, combat, effects write their own .pch), two of util.cpp (util.obj
        # reuses the .pch it wrote)
        session=["h:weapons.h", "c:util.cpp", "c:util.cpp"],
    ),
    "b21": dict(
        tag="cstrike-beta2.1",
        sha256="57f9dd8b2cae2072a77e053eb174fd3842d9ef8253031cb4c55475a38f40d844",
        link_time="1999-08-16 20:22:43",    # 0x37b87313
        base="b20",                         # the same PDB, age 308: 16 links after Beta 2.0's
        pdb_links=16,
        # edits of ak47.cpp and client.cpp, then of client.cpp, g3sg1.cpp, hostage.cpp and player.cpp (the
        # five files Beta 2.1 changed): client.cpp's last compile reuses the .pch its compile before wrote
        session=["c:ak47.cpp+c:client.cpp", "c:client.cpp+c:g3sg1.cpp+c:hostage.cpp+c:player.cpp"],
    ),
    "b30": dict(
        tag="cstrike-beta3.0",
        sha256="a1c31191a6176be679ee5fa2e6b1e7feaf59f83e26117b346ed3f2263ce4382b",
        link_time="1999-09-14 01:39:25",    # 0x37dda74d
        base="b21",                         # the same PDB, age 704: 396 links after Beta 2.1's
        pdb_links=396,
        # a build after Beta 3.0's edits, with debris.cpp, knife.cpp and p90.cpp added to the project
        # (debris.cpp's first declaration precedes any #include, so it leaves the .pch empty), then a build
        # after an edit of cbase.h
        session=["changed", "h:cbase.h"],
    ),
    "b31": dict(
        tag="cstrike-beta3.1",
        sha256="7dac5feda8c0137a7bcdb3855ab3e94ce3c5bcf84e3cb47f9319113b03f0b209",
        link_time="1999-09-16 02:14:56",    # 0x37e052a0
        base="b30",                         # the same PDB, age 728: 24 links after Beta 3.0's
        pdb_links=24,
        session=["changed"],
    ),
    "b40": dict(
        tag="cstrike-beta4.0",
        sha256="95312d4de5ca88c387b072f97563911b84a8e0c2c046b850a097bfab8c4dcca6",
        link_time="1999-11-02 19:29:18",    # 0x381f3b8e
        pdb_created="1999-10-20 06:22:27",  # 0x380d5ea3: a new PDB
        pdb_links=255,                      # age 254
        # edits of game.cpp (it leaves a .pch no later source can reuse), gamerules.h, then combat.cpp,
        # func_tank.cpp and multiplay_gamerules.cpp (each writes its own), multiplay_gamerules.cpp again
        # (it reuses the .pch it wrote), doors.cpp, ggrenade.cpp and weapons.cpp
        session=["c:game.cpp", "h:gamerules.h", "c:combat.cpp+c:func_tank.cpp+c:multiplay_gamerules.cpp",
                 "c:multiplay_gamerules.cpp", "c:doors.cpp+c:ggrenade.cpp+c:weapons.cpp"],
    ),
    "b41": dict(
        tag="cstrike-beta4.1",
        sha256="f14aed6b873e9a1b31786782d5e33f3d57656f25d72f2a45a0a76bcddb4dc08f",
        # the link crossed a second: the export directory says 21:42:30, the COFF header and the debug
        # directory 21:42:31; a clock that steps 1 ms per read from 21:42:30 gives that
        link_time="@1999-11-24 21:42:30 i0.001",
        pdb_created="1999-11-24 21:31:07",  # 0x383c591b: a new PDB
        pdb_links=3,                        # age 2
        tree=r"D:\half-life\beta41",
        # a Rebuild All in a new tree, then builds after edits of game.cpp and of util.cpp and weapons.cpp
        session=["c:game.cpp", "c:util.cpp+c:weapons.cpp"],
    ),
    "b50leak": dict(
        tag="cstrike-beta5.0-leak",
        sha256="2ba7b797f9e9fdad3a88dbbeab61125e664a8777153c27ef4251ad69e8fcc816",
        link_time="1999-12-09 00:42:32",    # 0x384efaf8
        pdb_created="1999-12-09 00:42:32",  # = the link time: the link created the PDB
        pdb_links=1,                        # age 1 (LINK 6.00 starts at 1)
        compiler="vc6",
        tree=r"C:\SIERRA\Half-Life\cstrike5",  # the record names ...\dlls\mp.pdb (mp.dsp's Output_Dir is "")
        # a Rebuild All whose link wrote another PDB, then a build after edits of game.cpp, gamerules.cpp,
        # multiplay_gamerules.cpp, util.cpp and weapons.cpp, and the one link, which creates this PDB
        rebuild_link=False,
        session=["c:game.cpp+c:gamerules.cpp+c:multiplay_gamerules.cpp+c:util.cpp+c:weapons.cpp"],
    ),
    "b50": dict(
        tag="cstrike-beta5.0",
        sha256="87ea6dade4035c65338fb8cd95cce5b489b53b3951b8c34323fcd214c50927d7",
        link_time="1999-12-20 23:50:41",    # 0x385ec0d1
        pdb_created="1999-12-10 20:20:19",  # 0x38516083: a new PDB
        pdb_links=53,                       # age 53
        compiler="vc6",
        tree=r"C:\SIERRA\Half-Life\cstrike5",
        session=["c:game.cpp+c:multiplay_gamerules.cpp", "c:multiplay_gamerules.cpp+c:util.cpp+c:weapons.cpp"],
    ),
    "b52": dict(
        tag="cstrike-beta5.2",
        sha256="af119154607b38b6d0bae8a10efc702c4354d7088166895b47c73e3a75938b07",
        link_time="2000-01-10 21:45:20",    # 0x387a52f0
        pdb_created="2000-01-05 00:44:46",  # 0x387293fe: a new PDB in a new tree
        pdb_links=138,                      # age 138
        compiler="vc6",
        tree=r"D:\half-life\code",
        # game.cpp; an edit of player.h, the header Beta 5.2 changed (42 sources); multiplay_gamerules.cpp
        # twice (the second reuses the .pch the first wrote). The image fixes each object's last compile,
        # not the builds themselves: other histories give the same file
        session=["c:game.cpp", "h:player.h", "c:multiplay_gamerules.cpp", "c:multiplay_gamerules.cpp"],
    ),
    "b60": dict(
        tag="cstrike-beta6.0",
        sha256="23796074115c00f308170bdd68c8a40ad0d1ec7e769bf3d9549650d0c3d09f7c",
        link_time="2000-03-06 07:06:18",    # 0x38c358ea
        pdb_created="2000-02-10 07:16:44",  # 0x38a265dc: a new PDB in the same folder
        pdb_links=242,                      # age 242
        compiler="vc6",
        tree=r"D:\half-life\code",
        session=["c:game.cpp",
                 "c:ak47.cpp+c:c4.cpp+c:client.cpp+c:combat.cpp+c:func_tank.cpp+c:gamerules.cpp+c:multiplay_gamerules.cpp",
                 "c:multiplay_gamerules.cpp+c:util.cpp+c:weapons.cpp"],
    ),
    "b61": dict(
        tag="cstrike-beta6.1",
        sha256="d7f05c09d3d9bd1ee44ed2c438ba6830819a57ec532ee20c5b1e860f8debac96",
        link_time="2000-03-21 02:50:13",    # 0x38d6e365
        base="b60",                         # the same PDB, age 295: 53 links after Beta 6.0's
        pdb_links=53,
        compiler="vc6",
        tree=r"D:\half-life\code",
        # one build after the edits of the 27 files Beta 6.1 changed, then one of doors.cpp and effects.cpp
        # (doors.cpp stands for whatever left the .pch effects.obj shows)
        session=["changed", "c:doors.cpp+c:effects.cpp"],
    ),
    "b65": dict(
        tag="cstrike-beta6.5",
        sha256="abc1fb0da96654ea3e8a05cedf0ad4ca1d553a44b4b19691ec822c815e2c89cf",
        link_time="2000-06-05 21:58:32",    # 0x393c2518
        pdb_created="2000-06-05 21:55:09",  # a new PDB in a new tree, 3 min 23 s before the release link
        pdb_links=2,                        # age 2
        compiler="vc6",
        tree=r"D:\half-life\code\beta7",
        # SDK 2.0's project (sources in wpn_shared and ..\pm_shared); a Rebuild All and one more link
        session=[],
    ),
    "b66": dict(
        tag="cstrike-beta6.6",
        sha256="84fb002a67a9f8dd7e48ec60494c5078d598d2151ddc34409234ee98d87d90da",
        link_time="@2000-06-21 20:00:54 i0.001",  # export directory 20:00:54, COFF header 20:00:55
        pdb_created="2000-06-16 22:23:20",  # a new PDB in Beta 6.5's folder
        pdb_links=29,                       # age 29
        compiler="vc6",
        tree=r"D:\half-life\code\beta7",
        # an edit of weapons.h (45 sources), then the six sources that got server log lines after the
        # client's link, each of which writes its own .pch
        session=["h:weapons.h",
                 "c:client.cpp+c:ggrenade.cpp+c:hostage.cpp+c:multiplay_gamerules.cpp+c:triggers.cpp+c:wpn_c4.cpp"],
    ),
    "b70": dict(
        tag="cstrike-beta7.0",
        sha256="210d739c26423f48693a800d536a9a2e838ebd2c24afa1bbebf97fc8870b56e3",
        link_time="2000-08-25 00:16:47",
        pdb_created="2000-08-23 02:29:27",  # a new PDB in a new tree
        pdb_links=35,                       # age 35
        compiler="vc6",
        tree=r"D:\half-life\code\TEST",
        # an edit of weapons.h, a build of client.cpp and multiplay_gamerules.cpp, and
        # multiplay_gamerules.cpp once more on the .pch it wrote
        session=["h:weapons.h", "c:client.cpp+c:multiplay_gamerules.cpp", "c:multiplay_gamerules.cpp"],
    ),
    "b71": dict(
        tag="cstrike-beta7.1",
        sha256="41f728a70c0778dff227d88635e10b2e4710fb5136e8545bb199c869ba7ce9b3",
        link_time="@2000-09-15 02:36:05 i0.001",  # export directory 02:36:05, COFF header 02:36:06
        pdb_created="2000-09-12 03:59:07",  # a new PDB in a new tree
        pdb_links=56,                       # age 56
        compiler="vc6",
        tree=r"D:\half-life\code\Beta71",
        session=[],                         # a Rebuild All and 55 relinks
    ),
    "r10": dict(
        tag="cstrike-1.0",
        sha256="21073277e6e94d947075b6c958a61df2b840d3cd9db394b3349c887457332fea",
        link_time="2000-11-03 03:20:15",
        pdb_created="2000-11-03 03:20:15",  # = the link time: this link made the PDB
        pdb_links=1,
        compiler="vc6sp4",                  # C/C++ 12.00.8799, LINK 6.00.8447, SP4's LIBCMT
        tree=r"F:\counter-strike\source",
        session=[],                         # a Rebuild All and its link
    ),
    "r10b": dict(
        tag="cstrike-1.0b",
        sha256="0fc676a35a0c84a4a5cf50c392d2ca18768397ecb4ebeb939e9bcf73e06dcbdc",
        link_time="2001-01-08 19:14:07",
        pdb_created="2001-01-08 19:14:07",
        pdb_links=1,
        compiler="vc6sp4",
        tree=r"C:\Counter-Strike\source",
        session=[],
    ),
    "r11": dict(
        tag="cstrike-1.1",
        sha256="bea6118681f7a9d05b32fa71a2a6f828faee1151eca887f53d96544c96774881",
        link_time="2001-03-07 02:19:38",
        pdb_created="2001-03-07 02:19:38",
        pdb_links=1,
        compiler="vc6sp4",
        tree=r"U:\Counter-Strike\source",
        session=[],
    ),
    "r13": dict(
        tag="cstrike-1.3",
        sha256="598c9473cc16386a99bc01bba8f4bb31ffb10473f111434b2926fa3be81c30e1",
        link_time="2001-09-12 19:12:48",
        pdb_created="2001-09-12 19:12:48",
        pdb_links=1,
        compiler="vc6sp5",                  # C/C++ 12.00.8966, LINK 6.00.8447, LIBCMT 8047
        tree=r"U:\hl1\release\dev\goldsrc\cstrike",
        session=[],
    ),
    "r14": dict(
        tag="cstrike-1.4",
        sha256="f11016c098bf874f3b8ea9936a0c6760097d177412895de11faa0efbddeaac98",
        link_time="2002-04-19 12:04:21",    # COFF header, export directory
        pdb_created="2002-04-19 12:04:21",  # no debug directory: the one link runs at the link time
        pdb_links=1,
        compiler="vc6sp5pp",                # ids 48/49 build 9044, LINK 6.00.8447, LIBCMT 8047
        tree=r"U:\hl1\release\dev\goldsrc\cstrike",  # 1.3's: the image names no path (no /debug)
        session=[],
    ),
    "r15": dict(
        tag="cstrike-1.5",
        sha256="f10cb95979afd46c677bbf21d40be7ec5a9fc179306a03b3b1ce76744ca4d2e8",
        link_time="2002-06-10 07:57:11",
        pdb_created="2002-06-10 07:57:11",
        pdb_links=1,
        compiler="vc6sp5pp",
        tree=r"U:\hl1\release\dev\goldsrc\cstrike",
        session=[],
    ),
}


def project_dirs(dsp):
    """mp.dsp's Release Output_Dir and Intermediate_Dir, relative to dlls ("" for the project folder itself):
    the SDK's .\\Releasemp for both up to Beta 4.1; from Beta 5.0 (leak) on the outputs go to dlls"""
    text = open(dsp, encoding="latin-1").read().replace("\r", "")
    sec = text[text.index('"$(CFG)" == "' + CONFIG + '"'):]
    sec = sec[:sec.find("!ELSEIF")] if "!ELSEIF" in sec else sec
    found = {}
    for key in ("Output_Dir", "Intermediate_Dir"):
        m = re.search(r'^# PROP ' + key + r' "([^"]*)"', sec, re.M)
        v = m.group(1) if m else ""
        found[key] = "" if v in ("", ".", ".\\") else (v[2:] if v.startswith(".\\") else v)
    if not found["Intermediate_Dir"]:
        sys.exit(f"{dsp}: no intermediate folder: not handled")
    return found["Output_Dir"], found["Intermediate_Dir"]


def sha256(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def tag_tree(src, tag, dest):
    """The tree of TAG from SRC's repository, extracted into DEST (git archive: nothing in the repository
    is written)."""
    rev = subprocess.run(["git", "-C", src, "rev-parse", "--verify", "--quiet", f"refs/tags/{tag}^{{commit}}"],
                         stdin=subprocess.DEVNULL, capture_output=True, text=True)
    if rev.returncode:
        sys.exit(f"{src}: no tag {tag} in its repository; this release continues {tag}'s build and takes its "
                 f"source from there (fetch the tags)")
    if os.path.isdir(dest):
        shutil.rmtree(dest)
    os.makedirs(dest)
    p = subprocess.Popen(["git", "-c", "core.autocrlf=false", "-C", src, "archive", "--format=tar",
                          rev.stdout.strip()], stdin=subprocess.DEVNULL, stdout=subprocess.PIPE)
    with tarfile.open(fileobj=p.stdout, mode="r|") as t:
        t.extractall(dest, **({"filter": "data"} if hasattr(tarfile, "data_filter") else {}))
    if p.wait():
        sys.exit(f"git archive {tag} failed")
    return dest


def main(argv):
    if len(argv) != 3 or argv[0] not in VERSIONS:
        sys.exit("usage: counter-strike_server.py VER SRC OUT\n  VER: " + " ".join(VERSIONS))
    ver, src, B = argv[0], os.path.abspath(argv[1]), os.path.abspath(argv[2])
    tc = os.environ.get("VC_TOOLCHAINS")
    prefix = os.environ.get("VC_WINEPREFIX")
    if not tc or not prefix:
        sys.exit("set VC_TOOLCHAINS and VC_WINEPREFIX (counter-strike.sh does)")
    prefix = os.path.abspath(prefix)
    if os.path.realpath(prefix) == os.path.realpath(os.path.join(tc, "wineprefix")):
        sys.exit("the server build needs its own copy of the Wine prefix (VC_WINEPREFIX)")
    if not os.path.isfile(os.path.join(prefix, "system.reg")):
        sys.exit(f"no Wine prefix at {prefix}")
    os.environ["VC_WINEPREFIX"] = prefix

    # the parts of the build, oldest first: (version, source tree, session, links); a version's bases first
    chain = [ver]
    while VERSIONS[chain[0]].get("base"):
        chain.insert(0, VERSIONS[chain[0]]["base"])
    os.makedirs(B, exist_ok=True)
    parts = []
    for v in chain:
        W = VERSIONS[v]
        tree = src if v == ver else tag_tree(src, W["tag"], os.path.join(B, "src-" + W["tag"]))
        if v != ver:
            print(f"{W['tag']}'s source (a base of this build): {tree}", flush=True)
        parts.append((v, tree, list(W["session"]), W["pdb_links"]))
    for k, (v, tree, sess, nl) in enumerate(parts):
        if not os.path.isfile(os.path.join(tree, "dlls", "mp.dsp")):
            sys.exit(f"{tree}: no dlls/mp.dsp")
    pdb_created = VERSIONS[parts[0][0]]["pdb_created"]
    wtree = VERSIONS[parts[0][0]].get("tree", TREE)  # the tree's path (the CodeView record's)
    compiler = VERSIONS[parts[0][0]].get("compiler", COMPILER)
    if any(VERSIONS[v].get("tree", TREE) != wtree or VERSIONS[v].get("compiler", COMPILER) != compiler for v, *_ in parts):
        sys.exit("a version and its bases must share tree and compiler")
    outdir, intdir = project_dirs(os.path.join(parts[0][1], "dlls", "mp.dsp"))
    for v, tree, *_ in parts[1:]:
        if project_dirs(os.path.join(tree, "dlls", "mp.dsp")) != (outdir, intdir):
            sys.exit(f"{v}: mp.dsp's Release folders differ from its base's: not handled")
    m = re.match(r"^([C-Z]):((?:\\[\w.-]+)+)$", wtree)
    if not m:
        sys.exit(f"tree {wtree}: expected a drive's absolute path")

    # the prefix copy's drive for the tree (C: is the prefix's own drive_c; another letter gets drive_<x>)
    letter = m.group(1).lower()
    droot = os.path.join(prefix, "drive_" + letter)
    os.makedirs(droot, exist_ok=True)
    if letter != "c":
        dlink = os.path.join(prefix, "dosdevices", letter + ":")
        if os.path.islink(dlink) and os.readlink(dlink) != "../drive_" + letter:
            os.remove(dlink)
        if not os.path.islink(dlink):
            os.symlink("../drive_" + letter, dlink)
    os.environ.pop("DISPLAY", None)
    os.environ.pop("WAYLAND_DISPLAY", None)
    build_dsp.DRIVES.append((droot, letter.upper() + ":"))

    # the tree: symlinks into the first part's source; dlls a real folder of per-file symlinks
    src0 = parts[0][1]
    top = os.path.join(droot, *m.group(2).strip("\\").split("\\"))
    shutil.rmtree(top, ignore_errors=True)
    os.makedirs(top)
    for e in sorted(os.listdir(src0)):
        if e.startswith(".git"):
            continue
        if e == "dlls":
            os.makedirs(os.path.join(top, e))
            for f in sorted(os.listdir(os.path.join(src0, e))):
                os.symlink(os.path.join(src0, e, f), os.path.join(top, e, f))
        else:
            os.symlink(os.path.join(src0, e), os.path.join(top, e))
    dlls = os.path.join(top, "dlls")
    out = os.path.join(dlls, intdir)  # the objects
    os.makedirs(out)
    lout = os.path.join(dlls, outdir) if outdir else dlls  # LINK's outputs
    lpre = (".\\" + outdir + "/") if outdir else ""  # how the IDE names them on the link line
    inc = [dlls, os.path.normpath(os.path.join(dlls, "..", "engine"))]

    relpath = {}  # source name -> its path relative to dlls, for sources not directly in dlls

    def project():
        """the IDE's view of mp.dsp as the tree has it now: compiler flags, link line, source names in project order"""
        cpp, link, sources = build_dsp.parse_dsp(os.path.join(dlls, "mp.dsp"), CONFIG)
        cpp = [f for f in cpp if not build_dsp.IDE_ONLY.match(f) and f.lower() != "/c" and f.upper() != "/YX"]
        if any(s["add"] or s["sub"] for s in sources):
            sys.exit("per-file settings in mp.dsp: not handled")
        names = []
        for s in sources:
            if not s["exclude"]:  # the file's own spelling where the tree has it (the IDE passes its path)
                p = build_dsp.find_ci(dlls, s["path"])
                names.append(os.path.basename(p) if p else os.path.basename(s["path"].replace("\\", "/")))
                rel = s["path"].replace("/", "\\")
                while rel.startswith(".\\"):
                    rel = rel[2:]
                if "\\" in rel:  # Beta 6.5 on: a source in a subfolder of dlls or beside it
                    relpath[names[-1]] = rel.rsplit("\\", 1)[0] + "\\" + names[-1]
        return cpp, link, names

    cpp, link, names = project()
    for k, f in enumerate(cpp):  # the dependency scan's include folders: dlls, then the project's /I list
        d = f[2:] if f[:2].upper() == "/I" and len(f) > 2 else cpp[k + 1] if f.upper() == "/I" and k + 1 < len(cpp) else None
        if d:
            d = os.path.normpath(os.path.join(dlls, d.strip('"').replace("\\", "/")))
            if d not in inc:
                inc.append(d)

    # what a continued version changes: files of dlls only, each changed or added one edited in its session;
    # "changed" stands for an edit of every such file the session does not name (c: for a source, h: for a
    # header); mp.dsp (sources added to the project) is taken over when the part starts. cl_dll is the
    # client's: the server never compiles it.
    changed = {}
    for k in range(1, len(parts)):
        old, new = parts[k - 1][1], parts[k][1]
        diff, added = set(), set()
        for dirpath, dirnames, filenames in os.walk(new):
            dirnames[:] = [d for d in dirnames if not d.startswith(".git")
                           and not (dirpath == new and d == "cl_dll")]
            for f in filenames:
                rel = os.path.relpath(os.path.join(dirpath, f), new)
                if rel.startswith(".git"):
                    continue
                o = os.path.join(old, rel)
                if not os.path.isfile(o):
                    added.add(rel)
                elif open(o, "rb").read() != open(os.path.join(new, rel), "rb").read():
                    diff.add(rel)
        removed = set()
        for dirpath, dirnames, filenames in os.walk(old):
            dirnames[:] = [d for d in dirnames if not d.startswith(".git")
                           and not (dirpath == old and d == "cl_dll")]
            for f in filenames:
                rel = os.path.relpath(os.path.join(dirpath, f), old)
                if not rel.startswith(".git") and not os.path.isfile(os.path.join(new, rel)):
                    removed.add(rel)
        bad = sorted(x for x in diff | added | removed if os.path.dirname(x) != "dlls") + sorted(removed)
        if bad:
            sys.exit(f"{parts[k][0]}: files removed or outside dlls: not handled: {' '.join(bad)}")
        files = {os.path.basename(x).lower(): os.path.basename(x) for x in diff | added if os.path.basename(x).lower() != "mp.dsp"}
        named = {e.partition(":")[2].lower() for item in parts[k][2] for e in item.split("+") if e != "changed"}
        rest = sorted(f for f in files if f not in named)
        sess = []
        for item in parts[k][2]:
            ed = []
            for e in item.split("+"):
                ed += [("h:" if f.endswith(".h") else "c:") + files[f] for f in rest] if e == "changed" else [e]
            sess.append("+".join(ed))
        parts[k] = parts[k][:2] + (sess,) + parts[k][3:]
        edited = {e.partition(":")[2].lower() for item in sess for e in item.split("+")}
        missing = sorted(files[f] for f in files if f not in edited)
        if missing:
            sys.exit(f"{parts[k][0]}: changed or added but not edited in its session: {' '.join(missing)}")
        changed[k] = "dlls/mp.dsp" in diff

    log = open(os.path.join(B, "build.log"), "w")
    subprocess.run(["wineserver", "-p", "120"], env=dict(os.environ, WINEPREFIX=prefix),
                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def cl(name):
        ipre = ".\\" + intdir + "/"
        args = ["CL"] + cpp + ["/Fp" + ipre + "mp.pch", "/YX", "/Fo" + ipre, "/Fd" + ipre, "/c",
                              os.path.normpath((wtree + "\\dlls\\" + relpath.get(name, name)).replace("\\", "/")).replace("/", "\\")]
        rc, text = build_dsp.run(args, dlls, log, compiler)
        if rc or not os.path.exists(os.path.join(out, os.path.splitext(name)[0] + ".obj")):
            print(f"compile of {name} failed:\n{text[-2000:]}")
            sys.exit(2)

    nlink = [0]

    def do_link(when):
        largs = [f for f in link if not re.match(r"^/(out|map|pdb|implib|incremental):", f, re.I) and f.lower() != "/map"]
        # a project that links without /debug (1.4 and 1.5): the IDE's options stand ahead of /machine
        at = largs.index("/debug") if "/debug" in largs else next(i for i, f in enumerate(largs) if f.lower().startswith("/machine:"))
        largs = ["LINK"] + largs[:at] + ["/incremental:no", "/pdb:" + lpre + "mp.pdb",
                 "/map:" + lpre + "mp.map"] + largs[at:] + \
            ["/out:" + lpre + "mp.dll", "/implib:" + lpre + "mp.lib"] + \
            [".\\" + intdir + "\\" + os.path.splitext(n)[0] + ".obj" for n in names]
        build_dsp.LINK_TIME = when
        rc, text = build_dsp.run(largs, dlls, log, compiler)
        nlink[0] += 1
        if rc:
            print(f"link {nlink[0]} failed:\n{text[-2000:]}")
            sys.exit(2)

    built = os.path.join(lout, "mp.dll")
    t0 = time.time()
    try:
        for k, (v, tree, sess, nl) in enumerate(parts):
            W = VERSIONS[v]
            if k == 0:
                print(f"{W['tag']}: Rebuild All at {wtree}: {len(names)} sources", flush=True)
                log.write(f"# {v}: Rebuild All\n")
                for n in names:
                    cl(n)
                if W.get("rebuild_link", True):
                    print(f"link 1 at {pdb_created} UTC (creates mp.pdb)", flush=True)
                    do_link(pdb_created)
                    middle = nl - 1 - len(sess)
                else:  # its link wrote another folder's PDB: the session's first link creates this one
                    print("no link after the Rebuild All (its PDB is not this one)", flush=True)
                    log.write(f"# {v}: no link after the Rebuild All\n")
                    middle = nl - len(sess)
            else:
                print(f"{W['tag']}: continues the build of {VERSIONS[parts[k - 1][0]]['tag']}", flush=True)
                middle = nl - len(sess)
            if middle:
                print(f"links {nlink[0] + 1} to {nlink[0] + middle}: relinks (stand-in for the builds between)", flush=True)
                log.write(f"# {v}: {middle} relinks\n")
                for _ in range(middle):
                    do_link(pdb_created if nlink[0] == 0 else W["link_time"])
            if k and changed[k]:
                # sources added to the project: the version's mp.dsp, the same flags and link line
                path = os.path.join(dlls, "mp.dsp")
                os.remove(path)
                shutil.copyfile(os.path.join(tree, "dlls", "mp.dsp"), path)
                cpp2, link2, names2 = project()
                if cpp2 != cpp or link2 != link or [n for n in names if n not in names2]:
                    sys.exit(f"{v}: mp.dsp changes more than its source list: not handled")
                new = [n for n in names2 if n not in names]
                names = names2
                print(f"{W['tag']}: its mp.dsp adds {len(new)} sources ({' '.join(new)})", flush=True)
                log.write(f"# {v}: mp.dsp, sources added: {' '.join(new)}\n")
            for item in sess:
                todo = set()
                heads = []
                for edit in item.split("+"):
                    kind, _, arg = edit.partition(":")
                    if kind == "c" and os.path.basename(arg.replace("\\", "/")) in relpath:
                        # a source in a subfolder of dlls or beside it (Beta 6.5 on: wpn_shared, ..\pm_shared),
                        # which the tree reaches through a folder symlink: it is compiled again, its file left
                        # alone (a source's time stamp does not reach the compiler; a header's does, through the .pch)
                        todo.add(os.path.basename(arg.replace("\\", "/")))
                        continue
                    target = build_dsp.find_ci(os.path.join(tree, "dlls"), arg)
                    if kind not in ("h", "c") or target is None:
                        sys.exit(f"session item {item!r}: expected h:HEADER or c:SOURCE of dlls, joined by +")
                    if kind == "c" and os.path.basename(target) not in names:
                        sys.exit(f"session item {item!r}: {arg} is not a source of the project")
                    # the tree's own copy of the file as this version has it
                    path = build_dsp.find_ci(dlls, arg) or os.path.join(dlls, os.path.basename(target))
                    if os.path.lexists(path):
                        os.remove(path)
                    shutil.copyfile(target, path)
                    time.sleep(2.1)  # past any two-second time stamp granularity
                    os.utime(path, None)
                    if kind == "h":
                        heads.append(os.path.basename(path).lower())
                    else:
                        todo.add(os.path.basename(path))
                if heads:
                    cache = {}
                    for n in names:
                        p = os.path.normpath(os.path.join(dlls, relpath.get(n, n).replace("\\", "/")))
                        if {os.path.basename(d).lower() for d in build_dsp.dependencies(p, inc, cache) if d != p} & set(heads):
                            todo.add(n)
                todo = [n for n in names if n in todo]
                what = ", ".join(e.partition(":")[2] for e in item.split("+"))
                print(f"{W['tag']}: build after an edit of {what}: {len(todo)} source{'' if len(todo) == 1 else 's'}; link {nlink[0] + 1}",
                      flush=True)
                log.write(f"# {v}: edit {item}\n")
                for n in todo:
                    cl(n)
                do_link(pdb_created if nlink[0] == 0 else W["link_time"])
            if k < len(parts) - 1:
                have = sha256(built)
                print(f"{W['tag']}'s build done ({nlink[0]} links): mp.dll {have} "
                      + ("(retail's)" if have == W["sha256"] else f"(differs from retail's {W['sha256']})"), flush=True)
    finally:
        log.close()
        # the prefix copy's own wineserver; never the toolchain's prefix
        subprocess.run(["timeout", "60", "wineserver", "-k"], env=dict(os.environ, WINEPREFIX=prefix),
                       stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    shutil.copyfile(built, os.path.join(B, "mp.dll"))
    print(f"{nlink[0]} links in {time.time() - t0:.0f} s; {built}", flush=True)
    if sha256(built) != VERSIONS[ver]["sha256"]:
        compare(open(built, "rb").read(), VERSIONS[ver])
    return 0


def compare(A, W):
    """Where a built file differs from what retail's image records: its size and debug records."""
    import struct
    pe = struct.unpack_from("<I", A, 0x3c)[0]
    va, size = struct.unpack_from("<II", A, pe + 24 + 96 + 6 * 8)
    print(f"  built {len(A)} bytes, COFF time stamp {struct.unpack_from('<I', A, pe + 8)[0]:#x}")
    if not va:
        return
    n = struct.unpack_from("<H", A, pe + 6)[0]
    so = pe + 24 + struct.unpack_from("<H", A, pe + 20)[0]
    for i in range(n):
        vs, sva, rs, raw = struct.unpack_from("<4I", A, so + 40 * i + 8)
        if sva <= va < sva + rs:
            o = raw + va - sva
    for i in range(size // 28):
        r = struct.unpack_from("<IIHHIIII", A, o + 28 * i)
        if r[4] == 2:
            d = A[r[7]:r[7] + r[5]]
            print(f"  CodeView: signature {struct.unpack_from('<I', d, 8)[0]:#x} age {struct.unpack_from('<I', d, 12)[0]} "
                  f"{d[16:].rstrip(bytes(1)).decode('latin-1')}")


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
