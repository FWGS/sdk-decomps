#!/usr/bin/env python3
"""scientist-slaughterhouse_server.py PROJECT.dsp OUTDIR DRIVE_C: build
Scientist Slaughterhouse's server (hl.dll) as the retail file's build was
made, for scientist-slaughterhouse.sh.

PROJECT.dsp is dlls\\hl.dsp in the source tree that the script lays out at
retail's path, C:\\Microsoft Visual Studio\\MyProjects\\Source Code; DRIVE_C is
drive_c of the Wine prefix copy (VC_WINEPREFIX); OUTDIR is dlls\\Profilehl,
deleted first. The steps:

- a full build of hl.dsp, "hl - Win32 Profile" (/G5 /MT /W3 /Zi /O2 /YX and
  CLIENT_WEAPONS; LINK /profile /debug, not incremental), with each source
  handed to CL by its full path, as the IDE does (build_dsp.py
  --full-source-paths; for the server it changes nothing in the image).
  CL creates and reuses one vc60.pch in project order, as the IDE does;
- the last build session. Where VC6 puts the header inline functions an
  object needs depends on the .pch its last compile found, and a full build
  in project order leaves several objects with functions out of retail's
  order. SESSION replays edits and recompiles that give each object the
  .pch state retail's shows: flyingobject.h edited (dd_sphere.cpp then
  reuses the three-header .pch the full build left), then game.h (each of
  its dependents rebuilds the .pch from its own list: bullsquid.obj,
  client.obj), then islave.cpp, beams.cpp, aflock.cpp and bigmomma.cpp
  compiled twice each (the second compile reuses the file's own .pch) and
  freezethrower.cpp and func_break.cpp once. Each item is compiled with
  plain CL /c, the project's and the file's own flags and the same /Fp and
  /Fd, so VC6's /YX decides each .pch from the one the previous compile
  left. Only the objects' final states are evidence; the edits and their
  order are a stand-in that produces them;
- the PDB's history. Retail's CodeView record names a PDB created
  2002-04-12 23:45:57 UTC (its signature) and written by 1,433 links (its
  age). hl.pdb is deleted, the first link runs with the clock frozen at the
  creation time, links 2 to 1,433 at retail's link time, 2002-08-17
  07:56:57 UTC (libfaketime; only LINK runs under it), which sets the COFF
  header's, the export directory's and the debug directory's time stamps.
  Each full link into an existing PDB keeps its signature and raises its age
  by one. Only the first and the last link's times and the count are known;
  the 1,431 links in between stand in for the PDB's unknown history (only
  their number reaches the image). They take most of the build's time,
  about half a second each.

Uses build_dsp.py (from the folder above this one); VC_TOOLCHAINS,
VC_WINEPREFIX and VC6_ROOT come from the environment.
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

LINK_TIME = "2002-08-17 07:56:57"    # retail's TimeDateStamp, 0x3d5e01c9
PDB_CREATED = "2002-04-12 23:45:57"  # the CodeView signature, 0x3cb771b5
PDB_LINKS = 1433                     # the CodeView age
CONFIG = "hl - Win32 Profile"
# The last build session: "edit:HEADER" gives HEADER a new time stamp and
# recompiles every source that includes it, in project order; a plain name
# recompiles that source.
SESSION = ["edit:flyingobject.h", "edit:game.h", "islave.cpp", "islave.cpp", "beams.cpp", "beams.cpp",
           "freezethrower.cpp", "func_break.cpp", "aflock.cpp", "aflock.cpp", "bigmomma.cpp", "bigmomma.cpp"]


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
        sys.exit("scientist-slaughterhouse_server.py: set VC_WINEPREFIX to the prefix copy that holds the tree")
    shutil.rmtree(out, ignore_errors=True)
    print(f"server: full build of \"{CONFIG}\"", flush=True)
    # output through a file, not a pipe (Wine's background services could keep a pipe open)
    with tempfile.TemporaryFile() as tmp:
        r = subprocess.run([sys.executable, os.path.join(TOOLS, "build_dsp.py"), dsp, CONFIG, out, "--name", "hl",
                            "--drive", f"C:={drive_c}", "--link-time", LINK_TIME, "--full", "--full-source-paths"],
                           stdin=subprocess.DEVNULL, stdout=tmp, stderr=subprocess.STDOUT)
        tmp.seek(0)
        text = tmp.read().decode("latin-1")
    if r.returncode:
        sys.exit(f"full build failed ({r.returncode}):\n{text}")
    print("".join("  " + line + "\n" for line in text.splitlines() if line.startswith("compiled ")), end="", flush=True)

    build_dsp.DRIVES.append((drive_c, "C:"))
    build_dsp.FULL_SOURCE_PATHS = True
    cpp, link, sources = build_dsp.parse_dsp(dsp, CONFIG)
    cpp = [f for f in cpp if not build_dsp.IDE_ONLY.match(f) and f.lower() != "/c"]
    by_name = {os.path.basename(s["path"].replace("\\", "/")).lower(): s for s in sources if not s["exclude"]}
    dspdir = os.path.dirname(dsp)
    w = build_dsp.winpath
    log = open(os.path.join(out, "session.log"), "w")
    todo, said = [], []
    for item in SESSION:
        if item.startswith("edit:"):
            header = item[len("edit:"):]
            names = dependents(header, sources, cpp, dspdir)
            todo += [("touch", header)] + [("cl", n) for n in names]
            said.append(f"{header} edited ({len(names)} sources recompiled)")
        else:
            todo.append(("cl", item))
            said.append(item)
    print("server: last session: " + ", ".join(said), flush=True)
    for kind, name in todo:
        if kind == "touch":
            touch(name, dspdir)
            log.write(f"# new time stamp for {name}\n")
            continue
        s = by_name.get(name.lower())
        if s is None:
            sys.exit(f"{name}: not a source of hl.dsp")
        flags = [f for f in build_dsp.apply(cpp, s["add"], s["sub"]) if not build_dsp.IDE_ONLY.match(f)]
        obj = os.path.join(out, os.path.splitext(name)[0] + ".obj")
        rc, text = build_dsp.run(["CL", "/c"] + flags + [f"/Fp{w(os.path.join(out, 'vc60.pch'))}", f"/Fo{w(obj)}",
                                 f"/Fd{w(os.path.join(out, 'vc60.pdb'))}", build_dsp.cl_source(dspdir, s["path"])],
                                 dspdir, log)
        if rc or not os.path.exists(obj):
            sys.exit(f"session compile of {name} failed:\n{text}")
    # relink exactly as build_dsp.py did, into a new PDB
    objs = [os.path.join(out, os.path.splitext(os.path.basename(s["path"].replace("\\", "/")))[0] + ".obj")
            for s in sources if not s["exclude"]]
    for ext in (".pdb", ".dll", ".exp", ".lib", ".map"):
        p = os.path.join(out, "hl" + ext)
        if os.path.exists(p):
            os.remove(p)
    link = [f for f in link if not re.match(r"^/(out|map|pdb|implib):", f, re.I) and f.lower() != "/map"]
    link += [f"/out:{w(os.path.join(out, 'hl.dll'))}", f"/map:{w(os.path.join(out, 'hl.map'))}",
             f"/pdb:{w(os.path.join(out, 'hl.pdb'))}", f"/implib:{w(os.path.join(out, 'hl.lib'))}"]
    print(f"server: link 1 of {PDB_LINKS} at {PDB_CREATED} UTC (creates hl.pdb), links 2 to {PDB_LINKS} "
          f"at {LINK_TIME} UTC", flush=True)
    t0 = time.time()
    for k in range(PDB_LINKS):
        build_dsp.LINK_TIME = PDB_CREATED if k == 0 else LINK_TIME
        rc, text = build_dsp.run(["LINK"] + link + [w(o) for o in objs], dspdir, log)
        if rc:
            log.close()
            sys.exit(f"link {k + 1} failed:\n{text}")
    log.close()
    print(f"  {PDB_LINKS} links: {time.time() - t0:.0f} s", flush=True)


if __name__ == "__main__":
    main()
