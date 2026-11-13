#!/usr/bin/env python3
"""counter-strike_client.py VER SRC OUT: build a Counter-Strike release's
client.dll as the retail file's build was made, for counter-strike.sh.

VER is a recipe below (counter-strike.sh maps the tags to them), SRC a
checkout of the release's commit, OUT the build folder (emptied). The built
file is OUT/client.dll; for 1.4 and 1.5 also OUT/client.packed.dll, the file
to compare with retail's. VC_TOOLCHAINS (and VC_WINEPREFIX, if set) come from
the environment.

Every client is a full build of cl_dll\\cl_dll.dsp, "cl_dll - Win32 Release",
by build_dsp.py: the sources in project order with the project's flags, one
shared .pch replayed as the IDE's /YX does, one link with LINK's clock
frozen at retail's link time (libfaketime). The clients have no debug
directory, so no path reaches the image. Per recipe:

* the compiler (build_dsp.py --compiler): Visual C++ 5.0 as on the CD for
  Betas 1.0 to 4.0, CL and LINK on the CD's own MSVCRT.DLL (vc5-rtm-n: on
  Wine's builtin msvcrt LINK keeps other copies of identical functions and
  orders the imports differently); Visual C++ 6.0 as on the CD for Beta 5.0
  (leak) to 7.1; Service Pack 4 for 1.0 and 1.1; SP5 with the Processor Pack
  for 1.3 to 1.5. Every VC6 tool runs on its release's own MSVCRT.DLL.
* the link time; "@T i0.001" is a clock that starts at T and steps 1 ms per
  read, for the links whose two stamps are a second apart (Beta 3.0, 1.1).
* 1.0 and 1.1: after the full build, wpn_c4.cpp is compiled once more (with
  the .pch the build left) and the DLL linked again: retail's wpn_c4.obj has
  the form of a source that wrote its own precompiled header.
* Beta 6.6: the client was linked two days before the server, and the tree
  holds the release's last text. The build runs on a copy of the tree
  (OUT-src) without the one line of dlls/wpn_shared/wpn_c4.cpp written after
  the client's link. Beta 6.5d's commit is that earlier state: its tree
  builds Beta 6.6's client.dll as it is (recipe b65d).
* 1.4 and 1.5: retail's client.dll is not a PE file but the engine's packed
  form of one (tools/packed_dll.py). The build makes a plain PE as before and
  packs it (image base 0x1900000, the header number copied from retail,
  because nothing else determines it); that file is the one compared.
  cl_dll\\steam_wrap.c is compiled by Visual C++ 6.0 SP5 without the
  Processor Pack (build_dsp.py --compiler-file): retail's code of it is VC6
  code without the pack's scheduling.
"""
import os
import shutil
import sys

TOOLS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
sys.dont_write_bytecode = True  # no __pycache__ in the checkout
sys.path.insert(0, TOOLS)
import build_dsp  # noqa: E402
import packed_dll  # noqa: E402

CONFIG = "cl_dll - Win32 Release"

# recipe: (link time UTC, build_dsp.py's compiler, sources compiled again after the full build)
VERSIONS = {
    "b10": ("1999-06-18 07:53:01", "vc5-rtm-n", ()),
    "b20": ("1999-08-11 03:05:45", "vc5-rtm-n", ()),
    "b30": ("@1999-09-13 21:35:33 i0.001", "vc5-rtm-n", ()),    # export directory 21:35:33, COFF header 21:35:34
    "b40": ("1999-10-20 22:00:16", "vc5-rtm-n", ()),
    "b50leak": ("1999-12-09 00:35:55", "vc6", ()),
    "b50": ("1999-12-16 00:55:02", "vc6", ()),
    "b52": ("2000-01-07 20:31:53", "vc6", ()),
    "b60": ("2000-03-07 21:43:56", "vc6", ()),
    "b65": ("2000-06-06 22:16:26", "vc6", ()),
    "b65d": ("2000-06-19 03:50:16", "vc6", ()),    # Beta 6.6's client.dll, from Beta 6.5d's commit as it is
    "b66": ("2000-06-19 03:50:16", "vc6", ()),
    "b70": ("2000-08-25 18:28:18", "vc6", ()),
    "b71": ("2000-09-15 02:58:29", "vc6", ()),
    "r10": ("2000-11-03 04:28:47", "vc6sp4", ("dlls\\wpn_shared\\wpn_c4.cpp",)),
    "r11": ("@2001-03-07 20:15:00 i0.001", "vc6sp4", ("dlls\\wpn_shared\\wpn_c4.cpp",)),  # 20:15:00 / 20:15:01
    "r13": ("2001-09-04 22:27:12", "vc6sp5pp", ()),
    "r14": ("2002-04-19 06:25:51", "vc6sp5pp", ()),    # the export directory's stamp, the only one a packed file keeps
    "r15": ("2002-06-06 00:54:18", "vc6sp5pp", ()),
}
# recipe: (the packed header's number nRandom, copied from retail; the image base of the packed file)
PACKED = {"r14": (0x6035, 0x1900000), "r15": (0x08D5, 0x1900000)}
# recipe: build_dsp.py's --compiler-file
COMPILER_FILE = {"r14": "steam_wrap.c=vc6sp5", "r15": "steam_wrap.c=vc6sp5"}
# recipe: (file, exact line) pairs written after the client's link: the build runs on a copy without them
AS_OF_LINK = {
    "b66": (("dlls/wpn_shared/wpn_c4.cpp",
             "\t\t\tUTIL_LogPrintf( \"%s planted the bomb\\n\", STRING( m_pPlayer->pev->netname ) );"
             "\t// Beta 6.6: in the server, linked 2000-06-21; the client, linked 2000-06-19, was built before"
             " this line existed (release_client.py builds it without the line)"),),
}


def as_of_link(ver, src, copy):
    """A copy of the tree SRC in COPY with AS_OF_LINK[ver]'s lines taken out. Each line must be in its file
    exactly once, as a whole line."""
    if os.path.isdir(copy):
        shutil.rmtree(copy)
    shutil.copytree(src, copy, symlinks=True, ignore=shutil.ignore_patterns(".git"))
    for rel, line in AS_OF_LINK[ver]:
        p = os.path.join(copy, rel)
        data = open(p, "rb").read()
        want = line.encode("latin-1") + b"\r\n"
        i = data.find(want)
        if data.count(want) != 1 or (i > 0 and data[i - 1:i] != b"\n"):
            sys.exit(f"{os.path.join(src, rel)}: the line the client's link predates is not there once:\n  {line}")
        open(p, "wb").write(data[:i] + data[i + len(want):])
        print(f"client: {rel} as of the client's link, without the line {line.strip()[:60]!r} ...", flush=True)
    return copy


def again(out, calls, files):
    """Compile FILES once more with the command build_dsp.py used for them, against the .pch the full build
    left, then link again with the build's own link command. CALLS: build_dsp.run's calls, recorded."""
    log = open(os.path.join(out, "build.log"), "a")
    log.write("# compiled again: %s\n" % " ".join(files))
    ok = True
    for f in files:
        args, cwd, tool = [c for c in calls if c[0][0] == "CL" and c[0][-1].lower().endswith("\\" + f.lower())][-1]
        rc, _ = build_dsp.run(args, cwd, log, tool)
        ok = ok and rc == 0
    args, cwd, tool = [c for c in calls if c[0][0] == "LINK"][-1]
    rc, _ = build_dsp.run(args, cwd, log, tool)
    log.close()
    print("compiled %s again (%s); link rc=%d" % (", ".join(files), "ok" if ok else "FAILED", rc), flush=True)
    return ok and rc == 0


def main(argv):
    if len(argv) != 3 or argv[0] not in VERSIONS:
        sys.exit("usage: counter-strike_client.py VER SRC OUT\n  VER: " + " ".join(VERSIONS))
    ver, src, out = argv[0], os.path.abspath(argv[1]), os.path.abspath(argv[2])
    link_time, compiler, incr = VERSIONS[ver]
    if ver in AS_OF_LINK:
        src = as_of_link(ver, src, out + "-src")
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    os.environ.pop("DISPLAY", None)
    os.environ.pop("WAYLAND_DISPLAY", None)
    args = [os.path.join(src, "cl_dll", "cl_dll.dsp"), CONFIG, out, "--name", "client", "--compiler", compiler,
            "--full", "--link-time", link_time]
    if ver in COMPILER_FILE:
        args += ["--compiler-file", COMPILER_FILE[ver]]
    # build_dsp.py in this process, its tool calls recorded for the compiles that follow the full build
    calls, run = [], build_dsp.run

    def record(args, cwd, log, tool="vc6"):
        calls.append((list(args), cwd, tool))
        return run(args, cwd, log, tool)
    build_dsp.run = record
    sys.argv = [os.path.join(TOOLS, "build_dsp.py")] + args
    try:
        rc = build_dsp.main()
    finally:
        build_dsp.run = run
    sys.stdout.flush()
    dll = os.path.join(out, "client.dll")
    if rc or not os.path.exists(dll):
        return 2
    if incr and not again(out, calls, incr):
        return 2
    if ver in PACKED:
        rnd, base = PACKED[ver]
        packed = os.path.join(out, "client.packed.dll")
        open(packed, "wb").write(packed_dll.pack(open(dll, "rb").read(), rnd, base))
        print("client: packed %s into %s (number %#06x, base %#x)" % (dll, packed, rnd, base), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
