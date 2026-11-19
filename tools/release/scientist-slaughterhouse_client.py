#!/usr/bin/env python3
"""scientist-slaughterhouse_client.py PROJECT.dsp OUTDIR DRIVE_C SWORD_0701:
build Scientist Slaughterhouse's client.dll as the retail file's build was
made, for scientist-slaughterhouse.sh.

PROJECT.dsp is cl_dll\\cl_dll.dsp in the source tree that the script lays out
at retail's path, C:\\Microsoft Visual Studio\\MyProjects\\Source Code; DRIVE_C
is drive_c of the Wine prefix copy (VC_WINEPREFIX); OUTDIR is cl_dll\\Debug,
deleted first. The compiler runs as C:\\Microsoft Visual Studio\\VC98\\BIN
(VC6_ROOT, which the script lays out) with TMP and TEMP pinned in the
prefix: the client's RTTI records (/GR) come out in an order that the
lengths of those paths move.

Retail's client.dll is "cl_dll - Win32 Debug" edited to optimise (/O2 /Zi
on LIBCMTD, _DEBUG), linked incrementally. Its time stamps and PDB age
record a history, which is replayed:

1. A full build in project order with the project's /YX (one vc60.pch, VC6
   deciding each state), each source handed to CL by its full path as the
   IDE does (__FILE__ and the anonymous namespaces carry it), and the
   project's browse information (/FR: it moves where VC6 emits the inline
   functions it places lazily). The five files with an anonymous namespace
   are compiled at the seconds their names give (COMPILE_TIMES). The first
   link creates the PDB at 2002-04-12 23:18:18 UTC, retail's PDB signature:
   age 1. hl_weapons.cpp builds the .pch itself (the file before it shares
   no header with it), which emits CBaseEntity::Respawn after
   HUD_InitClientWeapons: the order of the last full link.
2. 456 relinks that change nothing but the PDB's age: one object's file
   time and COFF time stamp move on by a second, LINK relinks it in place
   and the age grows by one. A stand-in for the builds of April to July,
   which left nothing else in the image.
3. vgui_ServerBrowser.cpp compiled twice (the second compile reuses the .pch
   it built itself, which emits its classes' RTTI type descriptors in
   retail's order) and hl_weapons.cpp once, then a full link (the .ilk
   deleted) at 2002-07-01 15:12:23 UTC, retail's export and debug directory
   time.
4. hl_weapons.cpp (now reusing its own .pch: Respawn before
   HUD_InitClientWeapons) and sword.cpp, then an incremental relink at
   2002-08-11 22:30:25 UTC, the COFF header's time. LINK keeps the old
   layout and moves what no longer fits: HUD_InitClientWeapons goes into
   the first gap, EV_FireGauss's padding (retail 0x10017420). Age 459.

Steps 1 to 3 compile SWORD_0701 in place of dlls\\sword.cpp: a text of
sword.cpp as it may have been at the last full link. Retail's .data keeps
two contributions of sword.obj from that link (21-24 and 18-20 bytes) and
puts p_sword.mdl and v_sword.mdl first-fit, as a later relink of a
sword.obj with two more strings does. The stand-in uses the crowbar's view
and player models (crowbar.obj owns those strings), so sword.obj's own are
yelflare1.spr and w_sword.mdl. Only the two sizes and their order are
recorded; the text is a guess. Step 4 compiles the tree's own sword.cpp.

Uses build_dsp.py (from the folder above this one); VC_TOOLCHAINS,
VC_WINEPREFIX and VC6_ROOT come from the environment.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

TOOLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.dont_write_bytecode = True  # no __pycache__ in the checkout
sys.path.insert(0, TOOLS)
import build_dsp  # noqa: E402

CONFIG = "cl_dll - Win32 Debug"
PDB_CREATED = "2002-04-12 23:18:18"  # the CodeView signature, 0x3cb76b3a
FULL_LINK = "2002-07-01 15:12:23"    # export and debug directories: the last full link, 0x3d207157
LAST_LINK = "2002-08-11 22:30:25"    # COFF header: the last link, 0x3d56e581
RELINKS = 456                        # age 459 = 1 (creation) + 456 + 1 (full link) + 1 (last relink)
# the files with an anonymous namespace, at the second their ?%C:\...\file.cpp<number> names give
COMPILE_TIMES = {
    "vgui_scrollbar2.cpp": "2002-04-19 01:37:55",
    "vgui_slider2.cpp": "2002-04-19 01:37:57",
    "vgui_ControlConfigPanel.cpp": "2002-04-19 01:40:13",
    "vgui_ConsolePanel.cpp": "2002-04-22 06:21:41",
    "vgui_ServerBrowser.cpp": "2002-04-22 06:56:10",
}
PRE_SESSION = ["vgui_ServerBrowser.cpp", "vgui_ServerBrowser.cpp", "hl_weapons.cpp"]
SESSION = ["hl_weapons.cpp", "sword.cpp"]
SWORD = os.path.join("..", "dlls", "sword.cpp")  # from cl_dll


class Build:
    def __init__(self, dsp, out, drive_c):
        self.dsp, self.out, self.drive_c = dsp, out, drive_c
        self.dspdir = os.path.dirname(dsp)
        self.dll = os.path.join(out, "client.dll")
        self.link = None

    def full_build(self):
        cmd = [sys.executable, os.path.join(TOOLS, "build_dsp.py"), self.dsp, CONFIG, self.out, "--name", "client",
               "--drive", f"C:={self.drive_c}", "--full", "--incremental", "--new-pdb", "--full-source-paths",
               "--browse-info", "--link-time", PDB_CREATED,
               "--compile-time", ",".join(f"{k}={v}" for k, v in COMPILE_TIMES.items())]
        # output through a file, not a pipe (Wine's background services could keep a pipe open)
        with tempfile.TemporaryFile() as tmp:
            r = subprocess.run(cmd, stdin=subprocess.DEVNULL, stdout=tmp, stderr=subprocess.STDOUT)
            tmp.seek(0)
            text = tmp.read().decode("latin-1")
        print("".join("  " + l + "\n" for l in text.splitlines() if l.startswith(("compiled", "LINK:", "failed"))),
              end="", flush=True)
        if r.returncode or not os.path.exists(self.dll):
            sys.exit(f"full build failed ({r.returncode}):\n{text}")
        self.link = json.load(open(os.path.join(self.out, "link.json")))

    def run_link(self, when, label):
        """LINK with step 1's arguments, the clock at WHEN, the files' times real."""
        build_dsp.LINK_TIME, build_dsp.INCREMENTAL = when, True
        with open(os.path.join(self.out, "build.log"), "a") as lg:
            lg.write(f"# {label}\n")
            rc, text = build_dsp.run(["LINK"] + self.link["args"], self.link["cwd"], lg, self.link["tool"])
        if rc:
            sys.exit(f"{label}: LINK failed ({rc}):\n{text}")
        why = [l.strip() for l in text.splitlines() if "LNK60" in l or "performing full link" in l]
        return ("full link" if why or "Creating library" in text else "incremental relink"), why

    def relinks(self, n):
        """Age-only relinks: one object moved on by a second per link, relinked in place."""
        objs = [x for x in self.link["args"] if x.lower().endswith(".obj")]
        first = os.path.basename(objs[0].replace("\\", "/"))
        p = os.path.join(self.out, first)
        for i in range(n):
            t = build_dsp.obj_times(p)
            build_dsp.set_obj_times(p, t[0] + 1, t[1] + 1)
            kind, why = self.run_link(PDB_CREATED, f"age-only relink {i + 1}/{n} ({first})")
            if kind != "incremental relink":
                sys.exit(f"age-only relink {i + 1} was a {kind} ({'; '.join(why)})")
        return first

    def compile(self, items, label):
        """Compile ITEMS in order with plain CL, sharing the .pch, as the IDE recompiles changed files."""
        cpp, _link, sources = build_dsp.parse_dsp(self.dsp, CONFIG)
        cpp = [f for f in cpp if not build_dsp.ide_only(f, True) and f.lower() != "/c"]
        cpp = [f"{f[:3]}{build_dsp.winpath(self.out)}\\" if build_dsp.BROWSE.match(f) else f for f in cpp]
        by_name = {os.path.basename(s["path"].replace("\\", "/")).lower(): s for s in sources if not s["exclude"]}
        times = {k.lower(): v for k, v in COMPILE_TIMES.items()}
        w = build_dsp.winpath
        with open(os.path.join(self.out, "build.log"), "a") as lg:
            lg.write(f"# {label}\n")
            for name in items:
                s = by_name.get(name.lower())
                if s is None:
                    sys.exit(f"{name}: not a source of cl_dll.dsp")
                flags = [f for f in build_dsp.apply(cpp, s["add"], s["sub"]) if not build_dsp.ide_only(f, True)]
                obj = os.path.join(self.out, os.path.splitext(name)[0] + ".obj")
                old = build_dsp.obj_times(obj)
                rc, text = build_dsp.run(["CL", "/c"] + flags + [f"/Fp{w(os.path.join(self.out, 'vc60.pch'))}",
                                         f"/Fo{w(obj)}", f"/Fd{w(os.path.join(self.out, 'vc60.pdb'))}",
                                         build_dsp.cl_source(self.dspdir, s["path"])], self.dspdir, lg, "vc6",
                                         times.get(name.lower()))
                if rc or not os.path.exists(obj):
                    sys.exit(f"{label}: compile of {name} failed:\n{text}")
                new = build_dsp.obj_times(obj)
                if old and (new[0] <= old[0] or new[1] <= old[1]):  # LINK must see both times change
                    build_dsp.set_obj_times(obj, max(new[0], old[0] + 1), max(new[1], old[1] + 1))


def main():
    if len(sys.argv) != 5:
        sys.exit(__doc__.split("\n\n")[0])
    dsp, out, drive_c, sword_0701 = (os.path.abspath(p) for p in sys.argv[1:])
    for var in ("VC_WINEPREFIX", "VC6_ROOT"):
        if not os.environ.get(var):
            sys.exit(f"scientist-slaughterhouse_client.py: {var} is not set (scientist-slaughterhouse.sh sets it)")
    build_dsp.DRIVES[:] = [(drive_c, "C:")]
    build_dsp.FULL_SOURCE_PATHS = True
    b = Build(dsp, out, drive_c)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    sword = os.path.normpath(os.path.join(b.dspdir, SWORD))
    target = os.readlink(sword) if os.path.islink(sword) else None
    if target is None:
        sys.exit(f"{sword}: expected the tree's symlink into the checkout")
    os.remove(sword)
    shutil.copyfile(sword_0701, sword)
    try:
        t0 = time.time()
        print(f"client: 1. full build of \"{CONFIG}\" (sword.cpp's earlier text), PDB created at {PDB_CREATED} UTC",
              flush=True)
        b.full_build()
        t1 = time.time()
        print(f"  {t1 - t0:.0f} s", flush=True)
        first = b.relinks(RELINKS)
        t2 = time.time()
        print(f"client: 2. {RELINKS} relinks that raise the PDB's age ({first}): {t2 - t1:.0f} s", flush=True)
        b.compile(PRE_SESSION, "session before the full link")
        ilk = os.path.join(out, "client.ilk")
        if os.path.exists(ilk):
            os.remove(ilk)
        kind, why = b.run_link(FULL_LINK, "full link (.ilk deleted)")
        t3 = time.time()
        print(f"client: 3. {' '.join(PRE_SESSION)} compiled, {kind} at {FULL_LINK} UTC: {t3 - t2:.0f} s", flush=True)
    finally:
        if os.path.lexists(sword):
            os.remove(sword)
        os.symlink(target, sword)
    b.compile(SESSION, "last session")
    kind, why = b.run_link(LAST_LINK, "last link")
    print(f"client: 4. {' '.join(SESSION)} compiled (sword.cpp's own text), {kind} at {LAST_LINK} UTC: "
          f"{time.time() - t3:.0f} s", flush=True)


if __name__ == "__main__":
    main()
