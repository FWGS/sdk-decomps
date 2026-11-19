#!/usr/bin/env python3
"""sic_server.py GAME PROJECT.dsp CONFIG OUTDIR D:=ROOT: build Survive in
Catacombs' server (GAME 1, sic.dll, linked as spirit.dll) or Survive in
Catacombs 2's (GAME 2, bloodbath.dll) as the retail file's build was made,
for sic.sh.

PROJECT.dsp is DLLS\\hl.dsp in the source tree that sic.sh lays out at
retail's PDB path; CONFIG is the project's configuration for the game ("hl -
Win32 Release SIC1" for SiC 1 on branch sic, "hl - Win32 Release" for SiC 2
and for the two historical trees); OUTDIR is DLLS\\Releasehl, deleted first;
D:=ROOT maps the tree's drive (SiC 1: F: = OUT/root, SiC 2: C: = drive_c of
the prefix copy, VC_WINEPREFIX). The steps, the same for both games with
each game's values (GAMES below):

- a full build of hl.dsp in project order, with the project's browse
  information (/Fr, build_dsp.py --browse-info): it adds to the compiler's
  intermediate code and so decides code (without it 857 bytes of SiC 1's
  server differ even with the steps below). CL creates and reuses one
  vc60.pch, as the IDE does. In SiC 2's tree agrunt.cpp, the project's
  second source, has no #include before its first line of code, so VC6's
  /YX leaves an empty .pch that every source up to player.cpp reuses; that
  places the header inline functions of alias.obj, func_break.obj and
  nodes.obj where retail has them;
- the last build session. weapons.h was edited and the sources that include
  it were recompiled in project order, each against the .pch the previous
  compile left, as the IDE does: the tree's weapons.h gets a new time stamp,
  then every source that depends on it (with the SIC1 conditionals resolved
  for the configuration: 51 sources for SiC 1, 49 for SiC 2) is compiled
  with plain CL /c, the project's and the file's own flags and the same /Fp
  and /Fd. The first finds the .pch zombie.cpp left, which doesn't hold
  weapons.h. That puts the header inline functions of barney.obj,
  h_cycler.obj and handgrenade.obj where retail has them. Known is only
  that those objects were last compiled in such .pch states; a weapons.h
  edit is the stand-in (both mods changed weapons.h);
- the PDB's history. Retail's CodeView record names a PDB created at
  PDB_CREATED (its signature) and written by PDB_LINKS links (its age). The
  PDB is deleted, link 1 runs with the clock frozen at the creation time,
  the others at retail's link time (libfaketime; only LINK runs under it),
  which sets the COFF header's, the export directory's and the debug
  directory's time stamps. Only the first and the last link's times and
  the count are known; the links in between stand in for the PDB's unknown
  history (they don't reach the image);
- the C runtime LINK runs on: SiC 1 the VC6 CD's MSVCRT.DLL (the tree's
  nativecrt copy, VC6_CRT=native), SiC 2 Wine's builtin msvcrt
  (VC6_CRT=wine). LINK orders the 75 KERNEL32 import thunks with qsort, and
  only Wine's gives SiC 2's order (on the CD's runtime CreateDirectoryA,
  slot 0, trades places with LCMapStringA, slot 37). Which runtime SiC 2's
  linker ran on is not known; Wine's builtin is a stand-in. The compiles
  run on the CD's runtime; their output doesn't depend on it.

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

GAMES = {
    1: dict(name="spirit",                       # hl.dsp's output; hl.def's LIBRARY spirit; shipped as sic.dll
            link_time="2004-05-19 21:32:38",     # retail's TimeDateStamp, 0x40abd276
            pdb_created="2004-05-16 13:01:57",   # the CodeView signature, 0x40a76645
            pdb_links=6,                         # the CodeView age
            crt="native"),
    2: dict(name="bloodbath",                    # hl.dsp's output (export name spirit.dll, from hl.def)
            link_time="2004-12-19 18:27:24",     # 0x41c5c80c
            pdb_created="2004-12-17 11:03:37",   # 0x41c2bd09
            pdb_links=8,
            crt="wine"),
}
# The last session: one edit of this header, its dependents recompiled.
SESSION_HEADER = "weapons.h"

SIC1_IF = re.compile(r"^\s*#\s*(ifdef|ifndef|if|else|elif|endif)\b\s*(\w*)")


def resolve_sic1(text, defined):
    """text with the SIC1 conditionals (#ifdef SIC1, #ifndef SIC1, their #else and #endif) resolved as the
    preprocessor would; every other line and conditional kept. A tree without SIC1 is returned unchanged."""
    if "SIC1" not in text:
        return text
    out, stack = [], []  # stack: None for another conditional, [taken] for a SIC1 one
    for line in text.split("\n"):
        m = SIC1_IF.match(line)
        w, arg = (m.group(1), m.group(2)) if m else (None, "")
        if w in ("ifdef", "ifndef") and arg == "SIC1":
            stack.append([defined == (w == "ifdef")])
            continue
        if w in ("if", "ifdef", "ifndef"):
            stack.append(None)
        elif w == "else" and stack and stack[-1] is not None:
            stack[-1][0] = not stack[-1][0]
            continue
        elif w == "endif" and stack:
            if stack.pop() is not None:
                continue
        if all(f[0] for f in stack if f is not None):
            out.append(line)
    return "\n".join(out)


def dependencies(src, incdirs, cache, sic1):
    """Every project header src includes, directly or not, as build_dsp.dependencies, with the SIC1
    conditionals resolved for the configuration: a source whose #include lines are all SiC 1's is not a
    SiC 2 dependent."""
    seen, todo = set(), [src]
    while todo:
        f = todo.pop()
        if f in seen:
            continue
        seen.add(f)
        if f not in cache:
            try:
                text = resolve_sic1(open(f, encoding="latin-1").read(), sic1)
            except OSError:
                text = ""
            found = []
            for inc in build_dsp.INCLUDE_RE.findall(text):
                for d in [os.path.dirname(f)] + incdirs:
                    hit = build_dsp.find_ci(d, inc)
                    if hit:
                        found.append(hit)
                        break
            cache[f] = found
        todo += cache[f]
    return seen


def ci_path(path):
    """path with each component spelled as on disk (the project's /I "..\\dlls" is the tree's DLLS)."""
    parts = os.path.normpath(path).split(os.sep)
    cur = os.sep if path.startswith(os.sep) else ""
    for p in parts:
        if not p:
            continue
        nxt = os.path.join(cur, p)
        if not os.path.exists(nxt) and os.path.isdir(cur or "."):
            hit = [e for e in os.listdir(cur or ".") if e.lower() == p.lower()]
            if hit:
                nxt = os.path.join(cur, hit[0])
        cur = nxt
    return cur


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
    if len(sys.argv) != 6 or sys.argv[1] not in ("1", "2"):
        sys.exit(__doc__.split("\n\n")[0])
    g = GAMES[int(sys.argv[1])]
    dsp, config, out = os.path.abspath(sys.argv[2]), sys.argv[3], os.path.abspath(sys.argv[4])
    letter, _, root = sys.argv[5].partition("=")
    if len(letter) != 2 or letter[1] != ":" or not root:
        sys.exit(f"sic_server.py: {sys.argv[5]}: expected D:=ROOT")
    if not os.environ.get("VC_WINEPREFIX"):
        sys.exit("sic_server.py: set VC_WINEPREFIX to the prefix copy that maps the tree's drive")
    name, link_time, pdb_created, links = g["name"], g["link_time"], g["pdb_created"], g["pdb_links"]
    shutil.rmtree(out, ignore_errors=True)
    os.environ["VC6_CRT"] = "native"  # the compiles: the CD's runtime (their output is the same on either)
    print(f'server: full build, "{config}", with browse information', flush=True)
    # output through a file, not a pipe (Wine's background services could keep a pipe open)
    with tempfile.TemporaryFile() as tmp:
        r = subprocess.run([sys.executable, os.path.join(TOOLS, "build_dsp.py"), dsp, config, out, "--name", name,
                            "--drive", f"{letter}={root}", "--link-time", link_time, "--full", "--browse-info"],
                           stdin=subprocess.DEVNULL, stdout=tmp, stderr=subprocess.STDOUT)
        tmp.seek(0)
        text = tmp.read().decode("latin-1")
    if r.returncode:
        sys.exit(f"full build failed ({r.returncode}):\n{text}")
    print("".join("  " + line + "\n" for line in text.splitlines() if line.startswith("compiled ")), end="", flush=True)

    build_dsp.DRIVES.append((os.path.abspath(root), letter.upper()))
    cpp, link, sources = build_dsp.parse_dsp(dsp, config)
    sic1 = any(re.fullmatch(r'/D"?SIC1"?', f) for f in cpp)  # the configuration selects SiC 1's code
    cpp = [f for f in cpp if not build_dsp.ide_only(f, True) and f.lower() != "/c"]
    by_name = {os.path.basename(s["path"].replace("\\", "/")).lower(): s for s in sources if not s["exclude"]}
    dspdir = os.path.dirname(dsp)
    w = build_dsp.winpath
    log = open(os.path.join(out, "session.log"), "w")

    cache, names = {}, []
    for s in sources:
        if s["exclude"]:
            continue
        path = build_dsp.find_ci(dspdir, s["path"])
        flags = build_dsp.apply(cpp, s["add"], s["sub"])
        incdirs = [ci_path(os.path.join(dspdir, f[2:].replace("\\", "/"))) for f in flags if f.startswith("/I")]
        if SESSION_HEADER in {os.path.basename(d).lower() for d in dependencies(path, incdirs, cache, sic1)}:
            names.append(os.path.basename(path))
    print(f"server: session: {SESSION_HEADER} edited, {len(names)} sources recompiled", flush=True)
    touch(SESSION_HEADER, dspdir)
    log.write(f"# new time stamp for {SESSION_HEADER}\n")
    for n in names:
        s = by_name[n.lower()]
        flags = [f for f in build_dsp.apply(cpp, s["add"], s["sub"]) if not build_dsp.ide_only(f, True)]
        # the .sbr files go to the output folder, as the IDE's /Fr".\Releasehl/"
        flags = [f"{f[:3]}{w(out)}\\" if build_dsp.BROWSE.match(f) else f for f in flags]
        obj = os.path.join(out, os.path.splitext(n)[0] + ".obj")
        rc, text = build_dsp.run(["CL", "/c"] + flags + [f"/Fp{w(os.path.join(out, 'vc60.pch'))}", f"/Fo{w(obj)}",
                                 f"/Fd{w(os.path.join(out, 'vc60.pdb'))}", s["path"]], dspdir, log)
        if rc or not os.path.exists(obj):
            sys.exit(f"session compile of {n} failed:\n{text}")

    # relink exactly as build_dsp.py did, into a fresh PDB
    objs = [os.path.join(out, os.path.splitext(os.path.basename(s["path"].replace("\\", "/")))[0] + ".obj")
            for s in sources if not s["exclude"]]
    for ext in (".pdb", ".dll", ".exp", ".lib", ".map"):
        p = os.path.join(out, name + ext)
        if os.path.exists(p):
            os.remove(p)
    link = [f for f in link if not re.match(r"^/(out|map|pdb|implib):", f, re.I) and f.lower() != "/map"]
    link += [f"/out:{w(os.path.join(out, name + '.dll'))}", f"/map:{w(os.path.join(out, name + '.map'))}",
             f"/pdb:{w(os.path.join(out, name + '.pdb'))}", f"/implib:{w(os.path.join(out, name + '.lib'))}"]
    os.environ["VC6_CRT"] = g["crt"]
    print(f"server: link 1 of {links} at {pdb_created} UTC (creates {name}.pdb), links 2 to {links} at "
          f"{link_time} UTC, LINK on " + ("the VC6 CD's MSVCRT.DLL" if g["crt"] == "native" else
                                          "Wine's builtin msvcrt"), flush=True)
    for k in range(links):
        build_dsp.LINK_TIME = pdb_created if k == 0 else link_time
        rc, text = build_dsp.run(["LINK"] + link + [w(o) for o in objs], dspdir, log)
        if rc:
            log.close()
            sys.exit(f"link {k + 1} failed:\n{text}")
    log.close()


if __name__ == "__main__":
    main()
