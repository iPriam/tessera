#!/usr/bin/env python3
"""Inspect aarch64 DT_NEEDED closure with readelf, never execute target code."""

import argparse
import collections
import json
import os
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sysroot", type=Path, required=True)
    parser.add_argument("--search", type=Path, action="append", default=[])
    parser.add_argument("--page-size", type=int, default=16384)
    parser.add_argument("libraries", type=Path, nargs="+")
    args = parser.parse_args()
    if args.page_size <= 0 or args.page_size & (args.page_size - 1):
        parser.error("--page-size must be a positive power of two")
    sysroot = args.sysroot.absolute()
    search = args.search + [
        sysroot / "usr/lib/aarch64-linux-gnu",
        sysroot / "lib/aarch64-linux-gnu",
        sysroot / "usr/lib",
        sysroot / "lib",
    ]
    forbidden = re.compile(r"^lib(?:EGL|GLES|OpenGL|GL|epoxy|gbm|drm|wayland|X|xcb)")
    queue = collections.deque((p.absolute(), [p.name]) for p in args.libraries)
    seen = set()
    report = {
        "libraries": [], "forbidden": [], "missing": [],
        "non_aarch64": [], "bad_alignment": [],
    }

    def resolve(path):
        # Rebase absolute target-side symlinks, rather than following them on ed.
        for _ in range(40):
            if not path.is_symlink():
                return path.resolve()
            target = Path(os.readlink(path))
            if target.is_absolute() and path.is_relative_to(sysroot):
                path = sysroot / target.relative_to("/")
            else:
                path = target if target.is_absolute() else path.parent / target
        raise RuntimeError(f"Symlink cycle: {path}")

    while queue:
        path, chain = queue.popleft()
        path = resolve(path)
        if path in seen:
            continue
        seen.add(path)
        if not path.is_file():
            report["missing"].append({"path": str(path), "chain": chain})
            continue
        with path.open("rb") as stream:
            header = stream.read(64)
            if len(header) < 64 or header[:6] != b"\x7fELF\x02\x01" or int.from_bytes(header[18:20], "little") != 183:
                report["non_aarch64"].append(str(path))
                continue
            phoff = int.from_bytes(header[32:40], "little")
            phsize = int.from_bytes(header[54:56], "little")
            phcount = int.from_bytes(header[56:58], "little")
            if phsize < 56 or not phcount or phcount == 65535:
                raise RuntimeError(f"Unsupported program-header table: {path}")
            for index in range(phcount):
                stream.seek(phoff + index * phsize)
                ph = stream.read(phsize)
                if len(ph) != phsize:
                    raise RuntimeError(f"Truncated program-header table: {path}")
                if int.from_bytes(ph[:4], "little") != 1:  # PT_LOAD
                    continue
                offset = int.from_bytes(ph[8:16], "little")
                address = int.from_bytes(ph[16:24], "little")
                alignment = int.from_bytes(ph[48:56], "little")
                if alignment < args.page_size or (address - offset) % args.page_size:
                    report["bad_alignment"].append({"path": str(path), "segment": index})
        dynamic = subprocess.run(
            ["readelf", "-d", str(path)], check=True, text=True,
            capture_output=True, env={**os.environ, "LC_ALL": "C"},
        ).stdout
        needed = re.findall(r"\(NEEDED\).*?\[(.*?)\]", dynamic)
        soname = re.search(r"\(SONAME\).*?\[(.*?)\]", dynamic)
        if forbidden.match(path.name) or (soname and forbidden.match(soname[1])):
            report["forbidden"].append(chain)
        report["libraries"].append({"path": str(path), "needed": needed})
        for name in needed:
            if forbidden.match(name):
                report["forbidden"].append(chain + [name])
            dependency = next((d / name for d in search if (d / name).exists()), None)
            if dependency is None:
                report["missing"].append({"needed": name, "chain": chain})
            else:
                queue.append((dependency, chain + [name]))

    report["pass"] = not any(report[k] for k in ("forbidden", "missing", "non_aarch64", "bad_alignment"))
    print(json.dumps(report, indent=2))
    return 0 if report["pass"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
