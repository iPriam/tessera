#!/usr/bin/env python3
"""Preserve real GCC header identities across Debian's /lib -> usr/lib alias.

Clang emits /sysroot/lib/gcc/.../../../../../include/C++ dependencies.
Filesystem lookup follows the symlink first, but Ninja collapses '..' lexically
and incorrectly records /sysroot/include instead of /sysroot/usr/include.
Only rewrite this verified directory alias; compiler arguments and code stay
unchanged. Do not create compatibility symlinks in the shared sysroot.
"""

import os
from pathlib import Path
import re
import sys
import tempfile


def main():
    path = Path(sys.argv[1])
    original = path.read_text()
    sysroot = Path("/root/Dev/webkit-sysroot/aarch64")
    pattern = re.compile(
        re.escape(str(sysroot))
        + r"/lib/gcc/aarch64-linux-gnu/[0-9.]+/(?:\.\./){4}include/"
    )

    def canonical(match):
        resolved = Path(match[0]).resolve(strict=True)
        expected = sysroot / "usr/include"
        if resolved != expected:
            raise RuntimeError(f"Unexpected GCC include alias: {match[0]} -> {resolved}")
        return str(resolved) + "/"

    normalized = pattern.sub(canonical, original)
    if normalized != original:
        with tempfile.NamedTemporaryFile(
            mode="w", dir=path.parent, prefix=path.name + ".", delete=False,
        ) as output:
            output.write(normalized)
            temporary = output.name
        os.replace(temporary, path)


if __name__ == "__main__":
    main()
