#!/usr/bin/env python3
"""Build one configuration of a Visual C++ .dsp project with the VC6 (or VC5) toolchain.

Compiles every source in project-file order (which is the order the IDE hands
objects to the linker, and so decides code layout), honouring per-file
Exclude_From_Build and ADD/SUBTRACT CPP overrides, then links with a .map file.

    VC_TOOLCHAINS=/path/to/toolchains build_dsp.py PROJECT.dsp "hl - Win32 Release" OUTDIR
                 [--cpp-add FLAGS] [--cpp-sub FLAGS] ...

The compilers run through vc6.sh, vc5.sh and vc4.sh, found next to this file.
VC_TOOLCHAINS (required) is the toolchain folder of docs/setup.md, with the
compiler trees and the Wine prefix. VC_WINEPREFIX selects another prefix, for
example a per-build copy with an extra drive letter (see --drive). VC6_CRT and
VC5_CRT are passed on to vc6.sh and vc5.sh. Needs only Python 3's standard
library, and libfaketime's faketime for --link-time.

Flags that only affect IDE bookkeeping (/FD minimal rebuild, /Fr /FR browse
info, /Gm) are dropped; they don't change generated code. /YX (automatic
precompiled headers) is kept, with one .pch in OUTDIR: it changes the code
layout. With VC6, inline functions that only a vtable or a compiler-made
constructor needs, from headers inside the .pch, go to the end of the object
instead of after the first function that needs them. Which headers are inside
depends on the file compiled before (only the .cpp's own #include lines
count): if the old .pch's list is a prefix of this file's, it is reused; if
not, it is rebuilt from the common prefix when that covers at least 70% of the
old list (measured: 7/10 and 10/13 shrink, 9/13 rebuilds), else from this
file's whole list; a file with an #if before its first #include rebuilds it
from its own list even when that extends the old one (measured with T2DEF's
squeakgrenade.cpp). So a full build in project order
is replayed: before an incremental compile the .pch is rebuilt from a stub
with the include list the IDE would have had in it at that point.
--no-pch drops /YX. A /Fp in a source's own "# ADD CPP" gives that file a
.pch of its own, created afresh each time it compiles (a stand-in for build
history: the file found no usable .pch); the project's .pch is untouched.

--compiler picks the compiler for every source (vc6, the default, VC6
Professional as on the CD; vc6std, the same with the Standard edition's back
end; vc6sp4 and vc6sp4std, VC6 with Service Pack 4; vc6sp5, with Service Pack
5; vc6sp5pp, SP5 with the Processor Pack; vc5, which is Visual C++ 5.0 with
Service Pack 3; vc5-rtm, without service packs; vc5-n and vc5-rtm-n, those two
running on their own release's MSVCRT.DLL whatever VC5_CRT says, which a VC5
link needs to give a retail file; vc42, vc41, vc40 for Visual C++ 4.x), and
--compiler-file overrides it per source (a.cpp=vc5,b.cpp=vc6), for a DLL
whose objects come from two compilers. --linker picks the linker and its
libraries (default: --compiler's). Each compiler gets its own .pch (the
IDE's vc60.pch, vc50.pch), replayed over its own files in project order.
"""
import argparse
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = {  # compiler name -> wrapper command, .pch/.pdb base name
    "vc6": ([os.path.join(HERE, "vc6.sh")], "vc60"),
    "vc6sp4": ([os.path.join(HERE, "vc6.sh"), "-sp4"], "vc60-sp4"),
    "vc6sp4std": ([os.path.join(HERE, "vc6.sh"), "-sp4std"], "vc60-sp4std"),
    "vc6std": ([os.path.join(HERE, "vc6.sh"), "-std"], "vc60-std"),
    "vc6sp5": ([os.path.join(HERE, "vc6.sh"), "-sp5"], "vc60-sp5"),
    "vc6sp5pp": ([os.path.join(HERE, "vc6.sh"), "-sp5pp"], "vc60-sp5pp"),
    "vc5": ([os.path.join(HERE, "vc5.sh"), "-sp3"], "vc50"),
    "vc5-rtm": ([os.path.join(HERE, "vc5.sh"), "-rtm"], "vc50-rtm"),
    # the same two from vc5/<tree>/nativecrt (the release's own MSVCRT.DLL), whatever VC5_CRT says;
    # a .pch of their own: this script keeps one .pch state per compiler name
    "vc5-n": (["env", "VC5_CRT=native", os.path.join(HERE, "vc5.sh"), "-sp3"], "vc50-n"),
    "vc5-rtm-n": (["env", "VC5_CRT=native", os.path.join(HERE, "vc5.sh"), "-rtm"], "vc50-rtm-n"),
    "vc42": ([os.path.join(HERE, "vc4.sh"), "-42"], "vc40"),
    "vc41": ([os.path.join(HERE, "vc4.sh"), "-41"], "vc40-41"),
    "vc40": ([os.path.join(HERE, "vc4.sh"), "-40"], "vc40-40"),
}


def wineprefix():
    """The Wine prefix the wrappers use: VC_WINEPREFIX, else $VC_TOOLCHAINS/wineprefix."""
    tc = os.environ.get("VC_TOOLCHAINS")
    if not tc:
        sys.exit("build_dsp.py: set VC_TOOLCHAINS to the toolchain folder (docs/setup.md)")
    return os.environ.get("VC_WINEPREFIX") or os.path.join(os.path.abspath(tc), "wineprefix")


IDE_ONLY = re.compile(r"^/(FD|Fr|FR|Gm)(\S*)$", re.I)
PCH = re.compile(r"^/(YX|Yc|Yu|Fp)(\S*)$", re.I)


DRIVES = []  # (unix root, "D:") pairs from --drive, longest root first


def winpath(path):
    path = os.path.abspath(path)
    for root, letter in DRIVES:
        if path == root or path.startswith(root + "/"):
            return letter + "\\" + path[len(root):].lstrip("/").replace("/", "\\")
    return "Z:" + path.replace("/", "\\")


def split_flags(text):
    """Split a .dsp flag line, joining `/D "X"` and `/I "dir"` into `/DX`, `/Idir`."""
    # .dsp flag lines are Windows-quoted; shlex copes with the plain quoting used.
    tokens = shlex.split(text.replace("\\", "\\\\"))
    out = []
    i = 0
    while i < len(tokens):
        t = tokens[i]
        if t in ("/D", "/I", "/U") and i + 1 < len(tokens):
            out.append(t + tokens[i + 1])
            i += 2
            continue
        out.append(t)
        i += 1
    return out


def apply(flags, add, sub):
    """Apply ADD/SUBTRACT CPP semantics: subtract by option name, then append."""
    def key(f):
        if f[:2] in ("/D", "/I", "/U"):  # one define/include dir, not the whole option
            return f.lower()
        m = re.match(r"^(/[A-Za-z]+)", f)
        return (m.group(1) if m else f).lower()
    sub_keys = {key(f) for f in sub}
    sub_exact = set(sub)
    out = [f for f in flags if f not in sub_exact and key(f) not in sub_keys]
    return out + [f for f in add if f not in out]


def parse_dsp(path, config):
    lines = open(path, encoding="latin-1").read().replace("\r", "").split("\n")
    cfg_re = re.compile(r'^!(?:IF|ELSEIF)\s+"\$\(CFG\)" == "(.*)"$')
    project_cpp, link, sources = None, None, []
    in_source, cur, cur_cfg = False, None, None
    for line in lines:
        m = cfg_re.match(line)
        if m:
            cur_cfg = m.group(1)
            continue
        if line.startswith("!ENDIF"):
            cur_cfg = None
            continue
        if line.startswith("# Begin Source File"):
            in_source, cur = True, {"path": None, "exclude": False, "add": [], "sub": []}
            continue
        if line.startswith("# End Source File"):
            if cur["path"] and re.search(r"\.(c|cpp)$", cur["path"], re.I):
                sources.append(cur)
            in_source, cur = False, None
            continue
        if in_source:
            if line.startswith("SOURCE="):
                cur["path"] = line[len("SOURCE="):].strip().strip('"')
            elif cur_cfg in (None, config):
                if re.match(r"^# PROP Exclude_From_Build 1", line):
                    cur["exclude"] = True
                elif line.startswith("# ADD CPP "):
                    cur["add"] += split_flags(line[len("# ADD CPP "):])
                elif line.startswith("# SUBTRACT CPP "):
                    cur["sub"] += split_flags(line[len("# SUBTRACT CPP "):])
            continue
        if cur_cfg == config:
            if line.startswith("# ADD CPP "):
                project_cpp = split_flags(line[len("# ADD CPP "):])
            elif line.startswith("# SUBTRACT CPP ") and project_cpp is not None:
                project_cpp = apply(project_cpp, [], split_flags(line[len("# SUBTRACT CPP "):]))
            elif line.startswith("# ADD LINK32 "):
                link = split_flags(line[len("# ADD LINK32 "):])
            elif line.startswith("# SUBTRACT LINK32 ") and link is not None:
                link = apply(link, [], split_flags(line[len("# SUBTRACT LINK32 "):]))
    if project_cpp is None or link is None:
        raise SystemExit(f"configuration {config!r} not found in {path}")
    return project_cpp, link, sources


INCLUDE_RE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)


def find_ci(directory, name):
    """Case-insensitive lookup of a relative path, as the Windows compiler sees it."""
    cur = directory
    for part in name.replace("\\", "/").split("/"):
        if part in ("", "."):
            continue
        if part == "..":
            cur = os.path.dirname(cur)
            continue
        try:
            entries = {e.lower(): e for e in os.listdir(cur)}
        except OSError:
            return None
        hit = entries.get(part.lower())
        if hit is None:
            return None
        cur = os.path.join(cur, hit)
    return cur if os.path.isfile(cur) else None


def dependencies(src, incdirs, cache):
    """Every project header src includes, directly or not ("..." includes only)."""
    seen, todo = set(), [src]
    while todo:
        f = todo.pop()
        if f in seen:
            continue
        seen.add(f)
        if f not in cache:
            try:
                text = open(f, encoding="latin-1").read()
            except OSError:
                text = ""
            found = []
            for inc in INCLUDE_RE.findall(text):
                for d in [os.path.dirname(f)] + incdirs:
                    hit = find_ci(d, inc)
                    if hit:
                        found.append(hit)
                        break
            cache[f] = found
        todo += cache[f]
    return seen


class Region(list):
    """header_region's result; .conditional: an #if came before the first #include."""
    conditional = False


def header_region(path):
    """The source's own #include lines before its first line of code, as
    (spelling, lowercase name) pairs: what /YX keys a .pch on."""
    try:
        text = open(path, encoding="latin-1").read()
    except OSError:
        return Region()
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    region, skip = Region(), []  # skip: one flag per open #if, True inside "#if 0"
    for line in text.split("\n"):
        line = line.strip()
        if not line:
            continue
        if not line.startswith("#"):
            if any(skip):
                continue
            break
        d = re.match(r"#\s*(\w+)\s*(.*)", line)
        word, rest = (d.group(1), d.group(2).strip()) if d else ("", "")
        if word in ("if", "ifdef", "ifndef"):
            if not region:
                region.conditional = True
            skip.append(word == "if" and rest == "0")  # other conditions: taken
        elif word in ("else", "elif") and skip:
            skip[-1] = False if skip[-1] else skip[-1]
        elif word == "endif" and skip:
            skip.pop()
        elif word == "include" and not any(skip):
            m = re.match(r'(["<][^">]+[">])', rest)
            if m:
                region.append((m.group(1), m.group(1)[1:-1].replace("/", "\\").lower()))
    return region


def pch_states(entries):
    """Replay /YX over the build: entries are (options key, header region) in
    build order; returns the .pch each one finds, as (key, names, index of the
    entry that made it), or None for no usable .pch."""
    state, found = None, []
    for i, (key, region) in enumerate(entries):
        found.append(state)
        names = tuple(n for _, n in region)
        if key is None:
            continue
        if not names:
            state = None  # it writes an empty .pch: the next file starts afresh
            continue
        if state is None or state[0] != key:
            state = (key, names, i)
            continue
        if getattr(region, "conditional", False):
            state = (key, names, i)  # an #if before the includes: rebuilt from this file's list
            continue
        have = state[1]
        if names[:len(have)] == have:
            continue  # used as it is
        common = 0
        while common < min(len(have), len(names)) and have[common] == names[common]:
            common += 1
        # VC6 keeps the common part when it is at least 70% of the old list
        state = (key, names[:common] if common * 10 >= len(have) * 7 else names, i)
    return found


LINK_TIME = None  # --link-time: LINK runs under libfaketime, frozen at this UTC time


def run(args, cwd, log, tool="vc6"):
    log.write("$ " + ("" if tool == "vc6" else f"[{tool}] ") + " ".join(args) + "\n")
    log.flush()
    # Capture through a file, not a pipe: Wine's background services inherit
    # the handle and keep a pipe open long after the tool exits.
    with tempfile.TemporaryFile() as tmp:
        cmd, env = TOOLS[tool][0] + args, None
        if LINK_TIME and args and args[0] == "LINK":
            cmd = ["faketime", "-f", LINK_TIME] + cmd  # absolute: the clock stays frozen
            env = dict(os.environ, TZ="UTC", FAKETIME_DONT_FAKE_MONOTONIC="1")
        p = subprocess.run(cmd, cwd=cwd, stdin=subprocess.DEVNULL, stdout=tmp, stderr=subprocess.STDOUT, env=env)
        tmp.seek(0)
        text = tmp.read().decode("latin-1")
    log.write(text)
    return p.returncode, text


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dsp")
    ap.add_argument("config")
    ap.add_argument("outdir")
    ap.add_argument("--cpp-add", default="", help="extra compiler flags")
    ap.add_argument("--cpp-sub", default="", help="compiler flags to remove")
    ap.add_argument("--name", help="output DLL base name (default: .def LIBRARY or project)")
    ap.add_argument("--exclude", default="", help="comma-separated source basenames to leave out")
    ap.add_argument("--full", action="store_true", help="recompile everything, even up-to-date objects")
    ap.add_argument("--no-pch", action="store_true", help="drop /YX (precompiled headers)")
    ap.add_argument("--compiler", default="vc6", choices=sorted(TOOLS), help="compiler for every source")
    ap.add_argument("--compiler-file", default="",
                    help="comma-separated per-source compilers, as source.cpp=vc5 (basenames)")
    ap.add_argument("--linker", choices=sorted(TOOLS), help="linker and libraries (default: --compiler)")
    ap.add_argument("--drive", action="append", default=[], metavar="D:=PATH",
                    help="write paths under PATH as D:\\... (the Wine prefix, VC_WINEPREFIX, must map D: to PATH)")
    ap.add_argument("--link-time", metavar="'YYYY-MM-DD hh:mm:ss'",
                    help="run LINK with the clock frozen at this UTC time (libfaketime), for retail's time stamps")
    a = ap.parse_args()
    # the wrappers run in the project's folder: pass them absolute paths
    for var in ("VC_TOOLCHAINS", "VC_WINEPREFIX"):
        if os.environ.get(var):
            os.environ[var] = os.path.abspath(os.environ[var])
    wineprefix()  # stops here when VC_TOOLCHAINS is not set
    global LINK_TIME
    LINK_TIME = a.link_time
    for d in a.drive:
        letter, _, root = d.partition("=")
        if len(letter) != 2 or letter[1] != ":" or not root:
            ap.error(f"--drive {d}: expected D:=PATH")
        DRIVES.append((os.path.abspath(root), letter.upper()))
    DRIVES.sort(key=lambda d: -len(d[0]))
    excluded = {e.strip().lower() for e in a.exclude.split(",") if e.strip()}
    per_file = {}
    for item in filter(None, (e.strip() for e in a.compiler_file.split(","))):
        name, _, tool = item.partition("=")
        if tool not in TOOLS:
            ap.error(f"--compiler-file {item}: compiler must be one of {', '.join(sorted(TOOLS))}")
        per_file[name.strip().lower()] = tool

    dspdir = os.path.dirname(os.path.abspath(a.dsp))
    out = os.path.abspath(a.outdir)
    os.makedirs(out, exist_ok=True)
    cpp, link, sources = parse_dsp(a.dsp, a.config)
    cpp = [f for f in cpp if not IDE_ONLY.match(f) and f.lower() != "/c"]
    cpp = apply(cpp, split_flags(a.cpp_add), split_flags(a.cpp_sub))
    if a.no_pch:
        cpp = [f for f in cpp if not PCH.match(f)]

    # Keep one wineserver alive across the many CL invocations; starting it for
    # every source file dominates build time otherwise.
    subprocess.run(["wineserver", "-p", "120"], stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL,
                   env=dict(os.environ, WINEPREFIX=wineprefix()))

    log = open(os.path.join(out, "build.log"), "w")
    stamps_path = os.path.join(out, "objstamps.json")
    try:
        stamps = {} if a.full else json.load(open(stamps_path))
    except (OSError, ValueError):
        stamps = {}
    dep_cache = {}
    todo = []
    for src in sources:
        if src["exclude"] or os.path.basename(src["path"].replace("\\", "/")).lower() in excluded:
            log.write(f"# excluded: {src['path']}\n")
            continue
        flags = [f for f in apply(cpp, src["add"], src["sub"]) if not IDE_ONLY.match(f)]
        if a.no_pch:
            flags = [f for f in flags if not PCH.match(f)]
        src_path = find_ci(dspdir, src["path"]) or os.path.join(dspdir, src["path"].replace("\\", "/"))
        yx = "/YX" in (f.upper() for f in flags)
        # A /Fp in the source's own "# ADD CPP" gives it a .pch of its own: it
        # doesn't touch the project's, and a full build starts it afresh, so
        # it is compiled creating a new one (a stand-in for build history).
        own = [f for f in src["add"] if f.upper().startswith("/FP")]
        if own and yx:
            flags = [f for f in flags if not f.upper().startswith("/FP")]
            src["own_pch"] = os.path.join(out, os.path.splitext(os.path.basename(src_path))[0] + ".own.pch")
            yx = False  # not part of the shared .pch's history
        lang = "c" if src_path.lower().endswith(".c") else "cpp"
        tool = per_file.pop(os.path.basename(src_path).lower(), a.compiler)
        todo.append((src, flags, src_path, (lang + " " + " ".join(flags)) if yx else None,
                     header_region(src_path) if yx else [], tool))
    if per_file:
        ap.error(f"--compiler-file names sources not in the build: {', '.join(sorted(per_file))}")

    # The one .pch per compiler, as the IDE has it, and what is in it now.
    tools = sorted({t for *_, t in todo})
    pch_file = {t: os.path.join(out, TOOLS[t][1] + ".pch") for t in tools}
    pch_state_path = os.path.join(out, "pchstate.json")
    states = {t: pch_states([(k if tt == t else None, r) for _, _, _, k, r, tt in todo] + [(None, [])])
              for t in tools}
    try:
        saved = json.load(open(pch_state_path)) if not a.full else {}
    except (OSError, ValueError):
        saved = {}
    if not isinstance(saved, dict):  # written before --compiler: VC6 only
        saved = {"vc6": saved}
    have = {t: saved.get(t) if os.path.exists(pch_file[t]) else None for t in tools}
    for t in tools:
        if a.full and os.path.exists(pch_file[t]):
            os.remove(pch_file[t])

    def make_pch(state, tool):
        """Rebuild tool's .pch so it holds exactly state's include list."""
        if os.path.exists(pch_file[tool]):
            os.remove(pch_file[tool])
        if state is None:
            return None
        maker_src, maker_flags, maker_path = todo[state[2]][:3]
        region = todo[state[2]][4][:len(state[1])]
        ext = os.path.splitext(maker_src["path"])[1]
        stub_rel = os.path.join(os.path.dirname(maker_src["path"].replace("\\", "/")), "_yx_stub" + ext)
        stub = os.path.join(dspdir, stub_rel)
        with open(stub, "w", newline="\r\n") as f:
            f.write("".join(f"#include {spelled}\n" for spelled, _ in region) + "\nint _yx_stub;\n")
        try:
            run(["CL", "/c"] + maker_flags + [f"/Fp{winpath(pch_file[tool])}",
                 f"/Fo{winpath(os.path.join(out, '_yx_stub.obj'))}",
                 f"/Fd{winpath(os.path.join(out, TOOLS[tool][1] + '.pdb'))}", stub_rel.replace("/", "\\")],
                dspdir, log, tool)
        finally:
            os.remove(stub)
            if os.path.exists(os.path.join(out, "_yx_stub.obj")):
                os.remove(os.path.join(out, "_yx_stub.obj"))
        return [state[0], list(state[1])]

    objs, failed, compiled = [], [], 0
    for i, (src, flags, src_path, pkey, region, tool) in enumerate(todo):
        obj_name = os.path.splitext(os.path.basename(src["path"].replace("\\", "/")))[0] + ".obj"
        obj = os.path.join(out, obj_name)
        incdirs = [os.path.normpath(os.path.join(dspdir, f[2:].replace("\\", "/"))) for f in flags if f.startswith("/I")]
        found = states[tool][i]
        need = [found[0], list(found[1])] if found else None
        # what the .pch holds when this file compiles is part of its build
        key = ("" if tool == "vc6" else f"[{tool}] ") + " ".join(flags) + \
            (f" [pch {' '.join(need[1])}]" if pkey and need else "") + (" [own pch]" if src.get("own_pch") else "")
        deps = dependencies(src_path, incdirs, dep_cache)
        newest = max(os.path.getmtime(d) for d in deps)
        if os.path.exists(obj) and stamps.get(obj_name) == key and os.path.getmtime(obj) >= newest:
            objs.append(obj)
            continue
        compiled += 1
        stamps.pop(obj_name, None)
        pch = []
        if pkey:
            if have[tool] != need:
                have[tool] = make_pch(found, tool)
            pch = [f"/Fp{winpath(pch_file[tool])}"]
        elif src.get("own_pch"):
            if os.path.exists(src["own_pch"]):
                os.remove(src["own_pch"])
            pch = [f"/Fp{winpath(src['own_pch'])}"]
        rc, text = run(["CL", "/c"] + flags + pch + [f"/Fo{winpath(obj)}",
                       f"/Fd{winpath(os.path.join(out, TOOLS[tool][1] + '.pdb'))}", src["path"]], dspdir, log, tool)
        if pkey:
            after = states[tool][i + 1]
            have[tool] = [after[0], list(after[1])] if after else None
        if rc != 0 or not os.path.exists(obj):
            failed.append(src["path"])
            sys.stderr.write(f"FAILED {src['path']}\n")
            have[tool] = None  # unknown now
        else:
            objs.append(obj)
            stamps[obj_name] = key
    json.dump(stamps, open(stamps_path, "w"), indent=0, sort_keys=True)
    json.dump(have, open(pch_state_path, "w"))

    defs = [f for f in link if f.lower().startswith("/def:")]
    name = a.name
    if not name and defs:
        deffile = os.path.join(dspdir, defs[0].split(":", 1)[1].strip('"').replace("\\", "/"))
        m = re.search(r"^LIBRARY\s+(\S+)", open(deffile, encoding="latin-1").read(), re.M)
        name = m.group(1) if m else None
    name = name or os.path.splitext(os.path.basename(a.dsp))[0]
    # the project's own /pdb names the PDB (its path is in the image's CodeView record), as in the IDE
    pdbs = [f for f in link if re.match(r"^/pdb:", f, re.I) and f.lower() != "/pdb:none"]
    pdb = os.path.splitext(os.path.basename(pdbs[0].split(":", 1)[1].strip('"').replace("\\", "/")))[0] if pdbs else name
    link = [f for f in link if not re.match(r"^/(out|map|pdb|implib):", f, re.I) and f.lower() != "/map"]
    link += [f"/out:{winpath(os.path.join(out, name + '.dll'))}", f"/map:{winpath(os.path.join(out, name + '.map'))}",
             f"/pdb:{winpath(os.path.join(out, pdb + '.pdb'))}", f"/implib:{winpath(os.path.join(out, name + '.lib'))}"]
    rc, text = run(["LINK"] + link + [winpath(o) for o in objs], dspdir, log, a.linker or a.compiler)
    log.close()
    print(f"compiled {compiled} of {len(objs) + len(failed)} sources ({len(failed)} failed); link rc={rc}; "
          f"output {out}/{name}.dll")
    if failed:
        print("failed:", " ".join(failed))
    tail = [l for l in text.splitlines() if "error" in l.lower()][:10]
    if tail:
        print("\n".join(tail))
    return 1 if failed or rc else 0


if __name__ == "__main__":
    sys.exit(main())
