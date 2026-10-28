#!/usr/bin/env python3
"""Lays WebKit's runtime out as the root of a Tessera release, on the build host.

    stage-payload.py --build <webkit build dir> --sysroot <sysroot> \
        --prefix <private dependency prefix> --out <payload root>

Reads ELF files with readelf and never runs them. Everything the engine and its
two processes load goes beside the C library, named by its SONAME; the C
library, its loader and the Vulkan loader are the platform's and are left out.
A library that is not aarch64, or that names GL, EGL, GBM, DRM, Wayland or X,
fails the staging.
"""

import argparse
import collections
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

MULTIARCH = "aarch64-linux-gnu"
PLATFORM = {
    "libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0", "librt.so.1",
    "libgcc_s.so.1", "ld-linux-aarch64.so.1", "libvulkan.so.1",
}
FORBIDDEN = re.compile(r"^lib(?:EGL|GLESv2|GLESv1_CM|OpenGL|GLX|GL|epoxy|gbm|drm|wayland|X|xcb)[-.0-9]")
# Plugins whose closure is clean and that a page's media and text need.
GST_WANTED = None  # every plugin whose closure passes


def readelf(path, *args):
    return subprocess.run(["readelf", *args, str(path)], capture_output=True, text=True,
                          env={**os.environ, "LC_ALL": "C"}).stdout


def dynamic(path):
    needed, soname = [], None
    for line in readelf(path, "-d").splitlines():
        m = re.search(r"\(NEEDED\)\s+Shared library: \[(.+)\]", line)
        if m:
            needed.append(m.group(1))
        m = re.search(r"\(SONAME\)\s+Library soname: \[(.+)\]", line)
        if m:
            soname = m.group(1)
    return needed, soname


def is_aarch64(path):
    return "AArch64" in readelf(path, "-h")


class Staging:
    def __init__(self, sysroot, search):
        self.sysroot = sysroot
        self.search = search
        self.libs = {}  # name -> real path
        self.problems = []

    def resolve(self, path):
        for _ in range(40):
            if not path.is_symlink():
                return path
            target = os.readlink(path)
            path = (self.sysroot / target.lstrip("/")) if target.startswith("/") else path.parent / target
        raise RuntimeError(f"symlink loop at {path}")

    def find(self, name):
        for directory in self.search:
            candidate = directory / name
            if candidate.exists() or candidate.is_symlink():
                try:
                    real = self.resolve(candidate)
                except RuntimeError:
                    continue
                if real.exists():
                    return real
        return None

    def add(self, roots):
        queue = collections.deque(roots)
        while queue:
            path = queue.popleft()
            needed, _ = dynamic(path)
            if not is_aarch64(path):
                self.problems.append(f"{path}: not aarch64")
            for name in needed:
                if name in PLATFORM or name in self.libs:
                    continue
                if FORBIDDEN.match(name):
                    self.problems.append(f"{path} needs {name}")
                    continue
                real = self.find(name)
                if real is None:
                    self.problems.append(f"{path} needs {name}, which is nowhere")
                    continue
                self.libs[name] = real
                queue.append(real)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--sysroot", type=Path, required=True)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--host-certificates", type=Path, default=Path("/etc/ssl/certs/ca-certificates.crt"))
    args = parser.parse_args()
    sysroot = args.sysroot.absolute()
    sys_lib = sysroot / "usr/lib" / MULTIARCH
    search = [args.prefix / "lib", args.build / "lib", sys_lib, sysroot / "lib" / MULTIARCH]
    out = args.out.absolute()
    if out.exists():
        sys.exit(f"{out} exists; stage into a new directory")
    libdir = out / "lib" / MULTIARCH
    libdir.mkdir(parents=True)

    engine = args.build / "lib/libWPEWebKit-2.0.so.1.12.0"
    programs = [args.build / "bin/WPEWebProcess", args.build / "bin/WPENetworkProcess"]
    staging = Staging(sysroot, search)

    # What the engine and the processes load at start.
    staging.add([engine, *programs])
    _, engine_soname = dynamic(engine)
    staging.libs[engine_soname] = engine

    # GIO's modules: TLS for https, and the proxy resolvers.
    gio_src = sys_lib / "gio/modules"
    gio_out = out / "usr/lib" / MULTIARCH / "gio/modules"
    gio_out.mkdir(parents=True)
    for module in sorted(gio_src.glob("*.so")):
        trial = Staging(sysroot, search)
        trial.add([module])
        if trial.problems:
            print(f"left out {module.name}: {trial.problems[0]}")
            continue
        shutil.copy2(module, gio_out / module.name)
        staging.libs.update(trial.libs)

    # GStreamer's plugins and the scanner that reads them.
    gst_src = sys_lib / "gstreamer-1.0"
    gst_out = out / "usr/lib" / MULTIARCH / "gstreamer-1.0"
    gst_out.mkdir(parents=True)
    kept = 0
    for plugin in sorted(gst_src.glob("libgst*.so")):
        trial = Staging(sysroot, search)
        trial.add([plugin])
        if trial.problems:
            print(f"left out {plugin.name}: {trial.problems[0]}")
            continue
        shutil.copy2(plugin, gst_out / plugin.name)
        staging.libs.update(trial.libs)
        kept += 1
    scanner = sysroot / "usr/libexec/gstreamer-1.0/gst-plugin-scanner"
    if scanner.exists():
        scanner_out = out / "usr/libexec/gstreamer-1.0"
        scanner_out.mkdir(parents=True)
        shutil.copy2(scanner, scanner_out / scanner.name)
        staging.add([scanner])

    if staging.problems:
        for problem in staging.problems:
            print("PROBLEM:", problem)
        sys.exit(1)

    for name, real in sorted(staging.libs.items()):
        shutil.copy2(real, libdir / name)
    # The name a link asks for.
    (libdir / "libWPEWebKit-2.0.so").symlink_to(engine_soname)
    for stem in ("libglib-2.0", "libgobject-2.0"):
        for name in staging.libs:
            if name.startswith(stem + ".so."):
                (libdir / (stem + ".so")).symlink_to(name)

    bundle = out / "System/Applications/Browser.app/Contents/MacOS"
    bundle.mkdir(parents=True)
    for program in programs:
        shutil.copy2(program, bundle / program.name)

    # Fonts and their configuration, for text.
    shutil.copytree(sysroot / "etc/fonts", out / "etc/fonts", symlinks=True)
    shutil.copytree(sysroot / "usr/share/fonts", out / "usr/share/fonts", symlinks=True)
    # The certificates https is checked against.
    certs = out / "etc/ssl/certs"
    certs.mkdir(parents=True)
    shutil.copy2(args.host_certificates, certs / "ca-certificates.crt")

    # No shared object may carry a debug section into the image.
    strip = shutil.which("llvm-strip") or shutil.which("strip")
    if strip:
        for path in list(libdir.iterdir()) + list(bundle.iterdir()) + list(gst_out.iterdir()) + list(gio_out.iterdir()):
            if path.is_file() and not path.is_symlink():
                subprocess.run([strip, "--strip-unneeded", str(path)], capture_output=True)
    # The Browser links the Tessera view and the display-taking web view; a
    # payload whose engine lacks them is not one the build may link against.
    exported = readelf(engine, "--dyn-syms", "--wide")
    needed = ["wpe_display_tessera_new", "wpe_view_tessera_pointer_move", "webkit_web_view_new_with_display"]
    if all(name in exported for name in needed):
        (out / "tessera-engine").write_text("\n".join(needed) + "\n")
    else:
        print("WARNING: the engine lacks the Tessera view; the build will leave the Browser out")
    print(f"staged {len(staging.libs)} libraries, {kept} GStreamer plugins into {out}")


main()
