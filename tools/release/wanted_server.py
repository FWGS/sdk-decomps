#!/usr/bin/env python3
"""wanted_server.py PROJECT.dsp OUTDIR ROOT: build Wanted!'s server as the
retail file's build was made, for wanted.sh.

PROJECT.dsp is dlls\\hl.dsp in the source tree that wanted.sh lays out at
retail's PDB path, K:\\Wantedhl\\Source Code\\New Code\\Code; ROOT is the
folder mapped to drive K: in the Wine prefix copy (VC_WINEPREFIX); OUTDIR is
dlls\\Releasehl, deleted first. The steps:

- a full build of hl.dsp, linked as hl.dll (retail's PDB is Releasehl\\hl.pdb
  and its export name hl.dll: the mod's author linked hl.dll and renamed it
  wanted.dll);
- the last build session. Retail's objects show that its last build was
  incremental. SESSION lists the files recompiled after the full build, in
  order, with VC6's own /YX deciding each .pch from the one the previous
  compile left, as the IDE does when a few edited files are rebuilt;
- the PDB's history. Retail's CodeView record names a PDB created 2001-04-02
  10:56:04 UTC (its signature, 0x3ac85ac4) and written by 18 links (its age,
  0x12). The session's relink starts a fresh hl.pdb with LINK at that time,
  then links 17 more times with the clock frozen at retail's link time,
  2001-06-05 13:13:19 UTC (libfaketime), which sets the time stamps of the
  image. In the last link the clock reads 13:13:17 until LINK has written
  hl.exp (retail's export directory is stamped two seconds before its image:
  its final link took that long), then 13:13:19. Only the two stamps are
  known; the 16 links in between stand in for the PDB's unknown history.

Uses build_dsp.py (from the folder above this one); VC_TOOLCHAINS and
VC_WINEPREFIX come from the environment. The final link loads libfaketime
directly (LD_PRELOAD) to read its time from a file: FAKETIME_LIB names the
library, else it is found from the faketime program.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time

TOOLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.dont_write_bytecode = True  # no __pycache__ in the checkout
sys.path.insert(0, TOOLS)
import build_dsp  # noqa: E402

LINK_TIME = "2001-06-05 13:13:19"
PDB_CREATED = "2001-04-02 10:56:04"
PDB_LINKS = 18
# retail's export directory is stamped EXP_TIME, two seconds before the image
# (LINK_TIME): its final link wrote hl.exp, then took two seconds to the image
EXP_TIME = "2001-06-05 13:13:17"
CONFIG = "hl - Win32 Release"
# The last session: bot.cpp (it rebuilds the .pch from its own list, so its
# header inlines are placed lazily with a cursor constant near 0; with the
# stand-in in bot.cpp that gives retail's bot.obj), then colonel.cpp and
# combat.cpp. Evidence per file: docs/wanted.md.
SESSION = ["bot.cpp", "colonel.cpp", "combat.cpp"]


def libfaketime():
    """The path of libfaketime.so.1, as the faketime program preloads it."""
    lib = os.environ.get("FAKETIME_LIB")
    if lib:
        return lib
    exe = shutil.which("faketime")
    if exe:
        m = re.search(rb"([\x21-\x7e]*/libfaketime\.so\.1)\x00", open(exe, "rb").read())
        if m and os.path.exists(m.group(1).decode()):
            return m.group(1).decode()
    for p in ("/usr/lib64/libfaketime.so.1", "/usr/lib/x86_64-linux-gnu/faketime/libfaketime.so.1",
              "/usr/lib/faketime/libfaketime.so.1", "/usr/local/lib/faketime/libfaketime.so.1"):
        if os.path.exists(p):
            return p
    sys.exit("wanted_server.py: libfaketime.so.1 not found; set FAKETIME_LIB to its path")


def final_link(args, cwd, log, exp):
    """The last link with a clock that reads EXP_TIME until LINK has written the
    .exp, then LINK_TIME: libfaketime reads its time from a file, and a watcher
    rewrites the file as soon as the new .exp appears."""
    lib = libfaketime()
    if os.path.exists(exp):
        os.remove(exp)
    stamp = tempfile.NamedTemporaryFile("w", suffix=".faketime", delete=False)
    stamp.write(EXP_TIME + "\n")
    stamp.close()
    done = threading.Event()

    def watch():
        while not done.is_set():
            if os.path.exists(exp):
                with open(stamp.name, "w") as f:
                    f.write(LINK_TIME + "\n")
                return
            time.sleep(0.001)

    t = threading.Thread(target=watch)
    t.start()
    saved = dict(os.environ)
    os.environ.pop("FAKETIME", None)
    os.environ.update(LD_PRELOAD=lib, FAKETIME_TIMESTAMP_FILE=stamp.name,
                      FAKETIME_NO_CACHE="1", TZ="UTC", FAKETIME_DONT_FAKE_MONOTONIC="1")
    build_dsp.LINK_TIME = None
    try:
        return build_dsp.run(args, cwd, log)
    finally:
        os.environ.clear()
        os.environ.update(saved)
        done.set()
        t.join()
        os.remove(stamp.name)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__.split("\n\n")[0])
    dsp, out, root = (os.path.abspath(p) for p in sys.argv[1:])
    if not os.environ.get("VC_WINEPREFIX"):
        sys.exit("wanted_server.py: set VC_WINEPREFIX to the prefix copy that maps K: to ROOT")
    shutil.rmtree(out, ignore_errors=True)
    print(f"server: full build, LINK at {LINK_TIME} UTC", flush=True)
    # output through a file, not a pipe (Wine's background services could keep a pipe open)
    with tempfile.TemporaryFile() as tmp:
        r = subprocess.run([sys.executable, os.path.join(TOOLS, "build_dsp.py"), dsp, CONFIG, out, "--name", "hl",
                            "--drive", f"K:={root}", "--link-time", LINK_TIME, "--full"],
                           stdin=subprocess.DEVNULL, stdout=tmp, stderr=subprocess.STDOUT)
        tmp.seek(0)
        text = tmp.read().decode("latin-1")
    if r.returncode:
        sys.exit(f"full build failed ({r.returncode}):\n{text}")
    print("".join("  " + line + "\n" for line in text.splitlines() if line.startswith("compiled ")), end="", flush=True)

    build_dsp.DRIVES.append((root, "K:"))
    cpp, link, sources = build_dsp.parse_dsp(dsp, CONFIG)
    cpp = [f for f in cpp if not build_dsp.IDE_ONLY.match(f) and f.lower() != "/c"]
    by_name = {os.path.basename(s["path"].replace("\\", "/")).lower(): s for s in sources}
    dspdir = os.path.dirname(dsp)
    w = build_dsp.winpath
    log = open(os.path.join(out, "session.log"), "w")
    print(f"server: last session: {', '.join(SESSION)}", flush=True)
    for name in SESSION:
        s = by_name[name.lower()]
        flags = [f for f in build_dsp.apply(cpp, s["add"], s["sub"]) if not build_dsp.IDE_ONLY.match(f)]
        obj = os.path.join(out, os.path.splitext(name)[0] + ".obj")
        rc, text = build_dsp.run(["CL", "/c"] + flags + [f"/Fp{w(os.path.join(out, 'vc60.pch'))}", f"/Fo{w(obj)}",
                                 f"/Fd{w(os.path.join(out, 'vc60.pdb'))}", s["path"]], dspdir, log)
        if rc:
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
    print(f"server: link 1 of {PDB_LINKS} at {PDB_CREATED} UTC (creates hl.pdb), links 2 to {PDB_LINKS - 1} "
          f"at {LINK_TIME} UTC", flush=True)
    for k in range(PDB_LINKS - 1):
        build_dsp.LINK_TIME = PDB_CREATED if k == 0 else LINK_TIME
        rc, text = build_dsp.run(["LINK"] + link + [w(o) for o in objs], dspdir, log)
        if rc:
            break
    else:
        print(f"server: link {PDB_LINKS} of {PDB_LINKS}: {EXP_TIME} UTC until hl.exp is written, then {LINK_TIME} UTC",
              flush=True)
        rc, text = final_link(["LINK"] + link + [w(o) for o in objs], dspdir, log, os.path.join(out, "hl.exp"))
    log.close()
    if rc:
        sys.exit(f"link failed:\n{text}")


if __name__ == "__main__":
    main()
