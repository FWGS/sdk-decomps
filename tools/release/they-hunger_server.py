#!/usr/bin/env python3
"""they-hunger_server.py PROJECT.dsp OUTDIR DRIVE_C: build They Hunger's server
as the retail file's build was made, for they-hunger.sh.

PROJECT.dsp is dlls\\hl.dsp in the source tree that they-hunger.sh lays out at
retail's PDB path, C:\\hlsrc11; DRIVE_C is drive_c of the Wine prefix copy
(VC_WINEPREFIX); OUTDIR is dlls\\Releasehl, deleted first. The steps:

- a full build of hl.dsp, linked as hl.dll (retail's PDB is Releasehl\\hl.pdb
  and its export name hl.dll: the mod's author linked hl.dll and renamed it
  einar.dll). CL creates and reuses one vc60.pch in project order, as the IDE
  does;
- the last build sessions. Retail's objects show that its last build was not
  a full one: agrunt.obj, barney.obj, client.obj, combat.obj and effects.obj
  have their header inline functions where VC6 puts them when each file's
  whole #include list is inside the .pch, which a full build in project order
  gives none of them. One cause gives all five and moves nothing else:
  weapons.h was edited and the build recompiled the sources that include it,
  in project order, the first of them (agrunt.cpp) finding a .pch that the
  edit had made stale. Replayed as the IDE does it: the tree's weapons.h gets
  a new time stamp, then every source that depends on it is compiled with
  plain `CL /c`, the project's flags and the same /Fp and /Fd, so VC6's own
  /YX decides each .pch from the one the previous compile left. This is done
  twice: the full build leaves zombie.cpp's .pch, which doesn't hold
  weapons.h, so the first rebuild's agrunt.cpp reuses it; the first rebuild
  leaves world.cpp's, which does, so the second one starts from a stale .pch.
  What is known is only that the last build's first file found no usable
  .pch; "the build before it also followed a weapons.h edit" stands in for
  that;
- the PDB's history. Retail's CodeView record names a PDB created 2001-03-10
  06:25:12 UTC (its signature, 0x3aa9c8c8) and written by 73 links (its age).
  hl.pdb is deleted, the first link runs with the clock frozen at the
  creation time, links 2 to 73 at retail's link time, 2001-03-26 05:43:20 UTC
  (libfaketime; only LINK runs under it), which sets the COFF header's, the
  export directory's and the debug directory's time stamps. Only the first
  and the last link's times and the count are known; the 71 links in between
  stand in for the PDB's unknown history (they don't reach the image).

Uses build_dsp.py (from the folder above this one); VC_TOOLCHAINS and
VC_WINEPREFIX come from the environment.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

TOOLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.dont_write_bytecode = True  # no __pycache__ in the checkout
sys.path.insert(0, TOOLS)
import build_dsp  # noqa: E402

LINK_TIME = "2001-03-26 05:43:20"    # retail's TimeDateStamp, 0x3abed6f8
PDB_CREATED = "2001-03-10 06:25:12"  # the CodeView signature, 0x3aa9c8c8
PDB_LINKS = 73                       # the CodeView age
CONFIG = "hl - Win32 Release"
# The last sessions: two rebuilds, each after an edit of this header.
SESSION = ["weapons.h", "weapons.h"]


def dependents(header, sources, cpp, dspdir):
    """The project's sources that include HEADER, directly or not, in project order."""
    cache = {}
    out = []
    for s in sources:
        if s["exclude"]:
            continue
        path = build_dsp.find_ci(dspdir, s["path"])
        flags = build_dsp.apply(cpp, s["add"], s["sub"])
        incdirs = [os.path.normpath(os.path.join(dspdir, f[2:].replace("\\", "/"))) for f in flags if f.startswith("/I")]
        if header.lower() in {os.path.basename(d).lower() for d in build_dsp.dependencies(path, incdirs, cache)}:
            out.append(os.path.basename(path))
    return out


def touch(header, dspdir):
    """Give HEADER a new time stamp. The tree is made of symlinks into the
    checkout: the link becomes a copy first, so the checkout keeps its own."""
    h = build_dsp.find_ci(dspdir, header)
    if h is None:
        sys.exit(f"{header}: no such header next to the project")
    if os.path.islink(h):
        target = os.path.realpath(h)
        os.remove(h)
        shutil.copyfile(target, h)
    time.sleep(2.1)  # past any two-second time stamp granularity
    os.utime(h, None)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__.split("\n\n")[0])
    dsp, out, drive_c = (os.path.abspath(p) for p in sys.argv[1:])
    if not os.environ.get("VC_WINEPREFIX"):
        sys.exit("they-hunger_server.py: set VC_WINEPREFIX to the prefix copy that holds the tree in drive_c")
    shutil.rmtree(out, ignore_errors=True)
    print(f"server: full build, LINK at {LINK_TIME} UTC", flush=True)
    # output through a file, not a pipe (Wine's background services could keep a pipe open)
    with tempfile.TemporaryFile() as tmp:
        r = subprocess.run([sys.executable, os.path.join(TOOLS, "build_dsp.py"), dsp, CONFIG, out, "--name", "hl",
                            "--drive", f"C:={drive_c}", "--link-time", LINK_TIME, "--full"],
                           stdin=subprocess.DEVNULL, stdout=tmp, stderr=subprocess.STDOUT)
        tmp.seek(0)
        text = tmp.read().decode("latin-1")
    if r.returncode:
        sys.exit(f"full build failed ({r.returncode}):\n{text}")
    print("".join("  " + line + "\n" for line in text.splitlines() if line.startswith("compiled ")), end="", flush=True)

    build_dsp.DRIVES.append((drive_c, "C:"))
    cpp, link, sources = build_dsp.parse_dsp(dsp, CONFIG)
    cpp = [f for f in cpp if not build_dsp.IDE_ONLY.match(f) and f.lower() != "/c"]
    by_name = {os.path.basename(s["path"].replace("\\", "/")).lower(): s for s in sources if not s["exclude"]}
    dspdir = os.path.dirname(dsp)
    w = build_dsp.winpath
    log = open(os.path.join(out, "session.log"), "w")
    for header in SESSION:
        names = dependents(header, sources, cpp, dspdir)
        print(f"server: session: {header} edited, {len(names)} sources recompiled", flush=True)
        touch(header, dspdir)
        log.write(f"# new time stamp for {header}\n")
        for name in names:
            s = by_name[name.lower()]
            flags = [f for f in build_dsp.apply(cpp, s["add"], s["sub"]) if not build_dsp.IDE_ONLY.match(f)]
            obj = os.path.join(out, os.path.splitext(name)[0] + ".obj")
            rc, text = build_dsp.run(["CL", "/c"] + flags + [f"/Fp{w(os.path.join(out, 'vc60.pch'))}", f"/Fo{w(obj)}",
                                     f"/Fd{w(os.path.join(out, 'vc60.pdb'))}", s["path"]], dspdir, log)
            if rc or not os.path.exists(obj):
                sys.exit(f"session compile of {name} failed:\n{text}")
    # relink exactly as build_dsp.py did, with a fresh PDB
    objs = []
    for s in sources:
        if s["exclude"]:
            continue
        objs.append(os.path.join(out, os.path.splitext(os.path.basename(s["path"].replace("\\", "/")))[0] + ".obj"))
    for ext in (".pdb", ".dll", ".exp", ".lib", ".map"):
        p = os.path.join(out, "hl" + ext)
        if os.path.exists(p):
            os.remove(p)
    link = [f for f in link if not re.match(r"^/(out|map|pdb|implib):", f, re.I) and f.lower() != "/map"]
    link += [f"/out:{w(os.path.join(out, 'hl.dll'))}", f"/map:{w(os.path.join(out, 'hl.map'))}",
             f"/pdb:{w(os.path.join(out, 'hl.pdb'))}", f"/implib:{w(os.path.join(out, 'hl.lib'))}"]
    print(f"server: link 1 of {PDB_LINKS} at {PDB_CREATED} UTC (creates hl.pdb), links 2 to {PDB_LINKS} "
          f"at {LINK_TIME} UTC", flush=True)
    for k in range(PDB_LINKS):
        build_dsp.LINK_TIME = PDB_CREATED if k == 0 else LINK_TIME
        rc, text = build_dsp.run(["LINK"] + link + [w(o) for o in objs], dspdir, log)
        if rc:
            log.close()
            sys.exit(f"link {k + 1} failed:\n{text}")
    log.close()


if __name__ == "__main__":
    main()
