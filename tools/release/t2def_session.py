#!/usr/bin/env python3
"""t2def_session.py PROJECT.dsp OUTDIR [--drive D:=PATH] FILE.cpp...: replay a
build session after a full build, for t2def.sh.

Recompiles FILEs in the order given, as the IDE does when a few edited files
are rebuilt: plain `CL /c` with the project's own flags for each file (its
per-file ADD/SUBTRACT CPP applied, IDE-only flags dropped, as build_dsp.py
does), sharing the real .pch the previous compile left in OUTDIR (the full
build's vc60-std.pch). No stub: VC6's own /YX decides, from the .pch on disk
and the file's include list, whether to reuse, shrink or rebuild it. Objects
and the compile PDB (vc60-std.pdb) are the full build's, in OUTDIR; LINK is
left to the caller (build_dsp.py relinks, every object being up to date).

Uses build_dsp.py's functions (from the folder above this one). The compiler
is VC6 RTM with the Standard edition's back end (vc6std), as for the release
build; VC_TOOLCHAINS, VC_WINEPREFIX and VC6_CRT come from the environment, as
for build_dsp.py. The same kind of session is in wanted_server.py.
"""
import argparse
import json
import os
import sys

sys.dont_write_bytecode = True  # no __pycache__ in the checkout
sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")))
import build_dsp  # noqa: E402

CONFIG = "hl - Win32 Release"
TOOL = "vc6std"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dsp")
    ap.add_argument("outdir")
    ap.add_argument("files", nargs="+")
    ap.add_argument("--drive", action="append", default=[], metavar="D:=PATH")
    a = ap.parse_args()
    for d in a.drive:
        letter, _, root = d.partition("=")
        build_dsp.DRIVES.append((os.path.abspath(root), letter.upper()))
    build_dsp.DRIVES.sort(key=lambda d: -len(d[0]))
    w = build_dsp.winpath
    out = os.path.abspath(a.outdir)
    dspdir = os.path.dirname(os.path.abspath(a.dsp))
    cpp, _, sources = build_dsp.parse_dsp(a.dsp, CONFIG)
    cpp = [f for f in cpp if not build_dsp.IDE_ONLY.match(f) and f.lower() != "/c"]
    by_name = {os.path.basename(s["path"].replace("\\", "/")).lower(): s for s in sources if not s["exclude"]}
    base = build_dsp.TOOLS[TOOL][1]
    pch, pdb = os.path.join(out, base + ".pch"), os.path.join(out, base + ".pdb")
    if not os.path.exists(pch):
        sys.exit(f"no {pch}: the session follows a full build")
    log = open(os.path.join(out, "session.log"), "w")
    for name in a.files:
        s = by_name.get(name.lower())
        if s is None:
            sys.exit(f"{name}: not a source of {a.dsp}")
        flags = [f for f in build_dsp.apply(cpp, s["add"], s["sub"]) if not build_dsp.IDE_ONLY.match(f)]
        obj = os.path.join(out, os.path.splitext(name)[0] + ".obj")
        rc, text = build_dsp.run(["CL", "/c"] + flags + [f"/Fp{w(pch)}", f"/Fo{w(obj)}", f"/Fd{w(pdb)}", s["path"]],
                                 dspdir, log, TOOL)
        if rc or not os.path.exists(obj):
            sys.exit(f"session compile of {name} failed:\n{text}")
    log.close()
    # build_dsp.py's record of the .pch no longer holds: mark it unknown, as it
    # does after a failed compile, so a later compile rebuilds it from a stub
    state = os.path.join(out, "pchstate.json")
    try:
        saved = json.load(open(state))
    except (OSError, ValueError):
        saved = {}
    if isinstance(saved, dict):
        saved[TOOL] = None
        json.dump(saved, open(state, "w"))
    print(f"session: recompiled {' '.join(a.files)}")


if __name__ == "__main__":
    main()
