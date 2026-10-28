# 14 — Tooling and ports

How the work is built, checked, and kept. The least interesting section and the
one containing the items that can stop the project rather than delay it.

## The thing that can destroy the work

**This repository needs four things outside itself: an installed Kira compiler,
the Kira libraries beside it in `../../kira-projects`, the framework's LLVM
bundle, and — for every build, since a Tessera target is never the host — a
sysroot holding the target's C library.** None of them is in this repository,
and the compiler and the LLVM bundle are named nowhere in a manifest, which is
what makes them easy to lose.

The compiler is a driver built from the `.klfd` definitions in `../klf-kira` and
installed as `kira` under `~/.klf`:

```sh
klf build ../klf-kira/languages/kira -o kira
cd ../klf-kira && knvm binstall
```

`/root/.klf/bin/kira` is a launcher, not the compiler: it reads
`~/.klf/toolchains/kira/current.toml` and dispatches to the channel named there
(`dev/0.1.0` on this host). A machine that has the checkout and has never run
`binstall` has no compiler at all, and `kira --version` on a machine with the
launcher but nothing installed answers for a channel that is not there.

The libraries are checkouts in `../../kira-projects`, which the components'
manifests name by relative path (`../../kira-projects/kira-ui` and the like)
and `scripts/lib.sh` names as `KIRA_PROJECTS`: Ember, KCoreAnimation,
KiraLayout, KCoreUI and KiraUI. Every one is built from the checkout as it
stands, uncommitted changes included. No
component imports a kernel package: LibTessera's `app/Backend/Linux/` is the
only code that declares a system call.

The LLVM bundle is the one the framework links with: the newest version under
`~/.kira/toolchains/llvm/<version>/<host-key>` that carries clang. The kernel
and the guest GPU stack are built with the same bundle, so the whole system has
one compiler. `TESSERA_LLVM` names a different one.

The sysroot is the one an unattended runner is most likely to be missing, and it
is covered below.

This was once much worse and the history is worth keeping: the syscall table
existed only in `../kira`'s working tree, so one `git checkout` made Tessera
unbuildable, with a compile error naming a system call as though it were a typo:

    error[KSEM279]: `ioctl` is not a system call Kira has a number for

That was the 1.9.1 compiler, which this project no longer builds against. The
compiler Tessera does build against resolves `@FFI.Syscall` through a table in
the framework that already carries every Linux call by name, so the table is no
longer something this project owns and the error above cannot happen. The general
rule is the one the sibling checkouts section of `ROADMAP.md` states: anything
Tessera needs that Kira has not committed is one `git checkout` from being lost,
so commit it there rather than patching it from here.

### What a Tessera program is made of

The SDK is Kira and RIDL. A component is Kira source; a library is a shared
library and the `.ridl` contract beside it, and a dependent reaches it through
that contract alone ([18](18-components.md)). No component compiles C, ships a
C header or binds one, and no package's Tessera rows name a C library to build.

The one exception is **Vulkan**: Ember's Vulkan backend links the Vulkan loader
for the GPU, and its Tessera rows name `vulkan` and nothing else. The loader and
Mesa's venus driver are platform runtime, built by
`kira build sysroot` into the sysroot, not components. There is no
Dawn.

A Kira library that still carries C for other platforms has no Tessera row for
it. Its bindings are then excluded on Tessera and a call through one traps by
name, so the gap is loud. KCoreAnimation's text engine, `kiratext`, is the one
left ([09](09-text.md)).

### The host's part of the toolchain

Tessera is a target of its own: `aarch64-unknown-tessera` and
`x86_64-unknown-tessera`, with the Linux kernel behind them (KLF-RIDL 5.8 in
`../kira-language-framework`). `scripts/lib.sh`'s `arch_kira_target` names it,
and `arch_linux_target` names the Linux triple behind it for C compiled outside
Kira, because clang knows no Tessera triple. A Tessera target is never the host,
so every build of the userland is a cross build, on any host.

A cross target needs more than the compiler. The framework builds every target
with the clang in the LLVM bundle Kira provisions, and that bundle carries a C
compiler and no C library, so what a target needs is a sysroot holding the
target's C library. On a Linux host of the target's architecture that sysroot is
the host's own root, `/`. The framework's runtime is Rust, and a Tessera triple
is a custom Rust target, `kernel/targets/<triple>.json` in the framework, whose
standard library is built from source: `scripts/setup-host.sh` installs the
nightly toolchain with `rust-src` through `rustup`.

Where the C library comes from is three branches in the framework's
`CToolchain::for_target`, `crates/klf-frontend-rt/src/toolchain.rs` in
`../kira-language-framework`. The host's own target takes no sysroot at all,
because the host's C library is the target's C library. An Apple target takes
`-isysroot` from `xcrun --show-sdk-path`. Anything else takes `--sysroot` from
the environment, and reads it under two names in this order:

    KLF_SYSROOT_<TRIPLE>   that target's own, dashes as underscores
    KLF_SYSROOT            whichever target is being built

Two names because a machine that builds for two targets has two C libraries and
one of them is not the other's default, while a user with one cross target
should not have to spell the triple. `scripts/lib.sh`'s
`require_sysroot` resolves the path and exports the plain name. A host set
up by anything else fails in one of three ways that name the piece rather than
the fix:

    kira: error: building for `aarch64-unknown-tessera` on `x86_64-unknown-linux-gnu` needs a sysroot
    holding that target's C library; set KLF_SYSROOT_AARCH64_UNKNOWN_TESSERA or KLF_SYSROOT to it

    kira: error: the sysroot at <path> holds no `libgcc_s.so.1` or `libgcc_s.a`, which is the unwinder a
    `<target>` program links; install the target's GCC runtime into it

    error: can't find crate for `std`

The middle one is the framework checking, before the link, that the sysroot holds
what `-lgcc_s` names. Without the check it fails as `unable to find library
-lgcc_s`, which names the linker rather than the sysroot. `libgcc_eh.a` does not
count: it is what clang's static-link spec asks for, and this link is not a
static one. It could have been hidden instead — clang takes
`--unwindlib=none`, and a build that passed it would link — but the sysroot is a
package set the user installs, so an incomplete one is theirs to fix and the
message should say which file is missing. The flag yields a binary needing a
`libgcc_s.so.1` the guest does not have, and the user finds that out at boot.

Fedora's aarch64 sysroot is genuinely incomplete for this, which is why
`scripts/setup-host.sh` has `stage_shared_unwinder`. The distribution's cross
gcc ships the static half of the GCC runtime into the sysroot — `libgcc.a`,
`libgcc_eh.a` — and keeps the shared half out of it. The script copies in the one
file from Debian's `libgcc-s1` package and writes `libgcc_s.so` beside it, because
`-lgcc_s` resolves the unversioned name and not the SONAME: with only the `.so.1`
present the link still fails. On a Darwin host no arrangement is needed, because
`libgcc-s1-arm64-cross` is one of the Debian `-arm64-cross` packages the script
already stages.

## Build

`kira build` is the entry point: the build package in `build/` describes every
step, and `kira build` with no step lists them. `kira build boot -arch <arch>
-test` builds what a boot needs and boots it.

Every program is dynamically linked, because the framework links the target's C
library rather than folding it in and there is no flag that turns that into a
static link. What that costs is visible in a built binary:

```console
$ file /root/.tessera-build/out/aarch64/rootfs/init
init: ELF 64-bit LSB pie executable, ARM aarch64, dynamically linked,
      interpreter /lib/ld-linux-aarch64.so.1

$ readelf -d /root/.tessera-build/out/aarch64/rootfs/init | grep NEEDED
libm.so.6   libc.so.6   libgcc_s.so.1
```

So every image with Kira code in it carries the loader and the C library, at the
path the ELF header names: `/lib/ld-linux-aarch64.so.1` on aarch64,
`/lib64/ld-linux-x86-64.so.2` on x86_64. `scripts/lib.sh`'s `guest_lib_dir`,
`guest_loader` and `guest_runtime_libs` answer both questions and
`build-initramfs.sh` and `build-system.sh` stage them, and the Build graph does. A
missing loader reads as an absent file — "no working init found" — so this is
worth an assertion rather than a stage that copies what it finds.

Known gaps:

- **`scripts/tessera build` as one command has never been run end to end.** Every
  stage was driven individually. That is a documented unknown in `HANDOFF.md`
  §11 and it is exactly the kind of thing that is broken and unnoticed.
- **The staged loader is checked against the ELF header.** Once the
  initramfs, the system volume and the installer are assembled,
  `check_interpreters` (`scripts/components.sh`) reads every ELF file's
  `PT_INTERP` with llvm-readelf and stops the build, naming the program and the
  loader, unless the release carries that loader with its execute bit.
- **Build times are not measured.** A kernel build and a Mesa build are each
  large and nobody knows which dominates a cold start.
- **There is no clean.** Removing `~/.tessera-build` is the only reset, and it
  costs everything including the multi-gigabyte GPU source trees.

## CI

There is none. Nothing has ever built Tessera except one machine.

That is the second-most-serious item here, because every claim in `HANDOFF.md`
is a claim about one machine. The minimum useful CI:

- Install the Kira compiler from `../klf-kira`, or take it as a build artefact.
- Run `kira build setup`: the host packages and the sysroot for each cross
  target.
- Build the kernel, initramfs and system volume.
- Run `-test` and assert the boot banner.

The obstacle is acceleration, and only for one architecture. x86_64 reaches
power off in under two seconds under TCG on this host and asserts fine —
`~/.tessera-build/logs/boot-x86_64.log` ends `[    1.894848] reboot: Power
down`. aarch64 does not boot at all under TCG here: QEMU produces no console
output from the kernel, so a 300-second timeout ends in an empty log. An arm64
Linux runner with KVM would fix that half, and nothing about an x86_64 boot test
needs hardware acceleration, which makes it the cheap one to put in CI first.

## Testing

Beyond the two boot assertions there is nothing.

What is worth having, roughly in order of value per effort:

- **A screendump assertion.** The pixel check done by hand this session — that
  the splash is 1280×800 with exactly three grey levels and 3645 pixels of mark
  — is a script. It catches every regression in the display path and it needs no
  eyes.
- **Unit tests for the pure parts.** The geometry, the ioctl request encoding,
  the struct offsets.
- **A manifest test.** Every package's native libraries share one namespace
  program-wide, so declaring the same name in two packages of one program is
  error 4035 rather than a harmless repeat. `ui` pulls Ember, the display
  client, LibTessera and DeviceKit, so their labels have to stay distinct as
  the libraries move; `kira check ../ui` is the command that would say so, and
  it passes today.
- **A protocol conformance test** once there is a protocol
  ([08](08-display-server.md)): a client that sends every message and every
  malformed variant, and a server that must survive it.
- **A boot-to-platform assertion** once there is a platform, which is the real
  end-to-end test and subsumes most of the others.

## Debugging

Today: `printf` to the serial console, and it scrolls past.

What is missing and cheap:

- **A log.** Levels, a timestamp, the service name — which falls out of the
  per-service pipe in [03](03-processes.md). Written to the data volume once
  there is one, so that a machine can be asked what happened last time.
  PID 1 writes each line it relays to `/data/system/logs/system.log`
  (`SystemDirectory.systemLogs`) after the UTC time and the service's name,
  and its own last line before it powers off, then syncs the file and the
  volume. At boot it keeps the last boot's log as `system.log.0`, as a Mac's
  newsyslog keeps the last, and logs `read the last boot's log` with its
  path, its count of lines and its last line.
- **A way to see processes.** `/proc` is mounted and nothing reads it.
- **Panics that say something.** The kernel pipes a faulting process's core to
  `reportcrash` (`kernel.core_pattern`), which reads `/proc` while the process
  is held and writes a Mac-style report to `/data/system/DiagnosticReports`:
  the signal or the Kira trap (`Namespace KIRA, Code KLF-RT-…` and its reason;
  the runtime raises SIGTRAP with the trap's number in `si_int`), every
  thread's registers and symbolized frames from each image's `.symtab` or
  `.dynsym`, the binary images with their build ids, the region the fault fell
  in, a VM region summary, the open files and the last trail lines.
  `kira build devroot crashes` copies the reports to the host's
  `~/.tessera-build/logs/crashes`.
- **A debugger.** A `-dev` boot gives the VM a virtio console port,
  `tessera.debug`, joined to `127.0.0.1:7331` on the host, and init starts
  `debugserver` beside devroot. It speaks lldb's gdb-remote protocol on that
  port over libtessera's `Debug` calls (§20.2 of
  [16](16-libtessera.md)): `lldb` then `gdb-remote 127.0.0.1:7331`, with
  the host's copy of the system root for symbols.

Worth noting what already exists and is underused: QEMU's monitor writes the
guest's display to a PPM file, which is how drawing is verified without
photographing a desktop. That technique should be in a script, not in a
handoff document.

## The other two hosts

- **x86_64.** Builds and boots on a Fedora host, which it did not before: the
  x86_64 boot test prints `x86_64 booted to the initramfs userland`, and
  `-services-media` passes. Directory enumeration used to fail on it, because
  `O_DIRECTORY` is the one open flag the two architectures do not share —
  `0o200000` on x86-64, `0o40000` on aarch64 — and a program that hardcoded one
  got `EINVAL` on the other. Tessera passes it nowhere now: a directory is
  opened read-only and `getdents64` answers `ENOTDIR` on anything that is not
  one. Not buildable on a Mac; `ARCHES` is `aarch64` there.
- **Fedora/WSL.** The x86_64 and aarch64 boot tests pass here. What has not been run on Linux is
  `scripts/tessera build` as one command, and no kernel configuration decision
  has been exercised under a hardware accelerator on Linux at all — the GPU
  work was all done on Apple silicon.

aarch64 booting on an x86_64 host is a separate matter from either: QEMU's TCG
aarch64 emulation produces no console output at all from the kernel here, with
or without an initramfs. The kernel and the initramfs build, and `file` and
`readelf -d` on the built binary are quoted under **Build** above, but the boot
cannot be observed. That is an emulator and host limitation rather than a
Tessera one, and it predates the migration to the framework-hosted compiler. No
aarch64 boot has been observed on this host at any point, so aarch64 is verified
by build rather than by a boot log.

## Dependency pins, collected

Because they are currently spread across scripts and one of them is a URL that
could disappear:

| | |
|---|---|
| Linux | 7.2, patched from `patches/linux` |
| Kira | the driver built from `../klf-kira`'s definitions, installed under `~/.klf` |
| LLVM | the newest bundle under `~/.kira/toolchains/llvm`, 23.1.0 on this host |
| Mesa | 25.0.7, venus only |
| Vulkan-Loader / Headers | 1.4.321 |
| libdrm | 2.4.125 |
| virglrenderer | 1.1.1, patched four ways |
| QEMU | 10.0.2, patched |
| Debian sysroot | glibc 2.41, GLIBCXX 3.4.33 |

The sysroot row is what a Darwin host builds against, staged by
`scripts/setup-host.sh` out of Debian's `-arm64-cross` packages. A Fedora host
uses its own distribution's directory instead,
`/usr/aarch64-redhat-linux/sys-root/<release>`, plus the one file Fedora leaves
out — see **The host's part of the toolchain** above.

The Kira pin is not a version number this repository chooses: `knvm binstall`
installs whatever `../klf-kira` currently defines, and the launcher dispatches
to the channel `~/.klf/toolchains/kira/current.toml` names. Recording the channel
and version in a handoff is the only way to say afterwards which compiler a
result came from.

## Checkpoints

- [x] The system calls Kira has to know are the framework's, not a table this
      project has to extend.
- [x] No component compiles C; Vulkan is the one native library a Tessera
      program links. WebKit WPE is the exception since `AGENTS.md`'s WebKit
      exception.
- [x] x86_64 builds and its boot test passes on a Fedora host.
- [x] aarch64 cross-builds on that host from a sysroot, and no Zig is fetched.
- [ ] `scripts/tessera build` runs end to end from nothing.
- [ ] A fresh checkout of `../klf-kira` plus `knvm binstall` builds Tessera with
      no other host preparation.
- [x] Something asserts that the staged loader is the one the ELF header names.
      The x86_64 build says each of the 27 dynamically linked programs in the
      initramfs, 2 in the system volume and 5 in the installer finds its
      loader; with `/lib64/ld-linux-x86-64.so.2` removed it stops with
      `/init names the loader /lib64/ld-linux-x86-64.so.2, which the release
      does not carry`.
- [ ] CI builds and runs the x86_64 boot test on every commit, then aarch64 on an
      arm64 runner.
- [ ] The screendump check is a script and runs in CI.
- [x] A log survives a reboot and can be read. Two `-services` boots in a
      row: the second logs `read the last boot's log` with
      `/data/system/logs/system.log.0`, 814 lines, and the first's last line,
      `init: every service is stopped; powering off`.
- [ ] The Linux host boots aarch64, not only builds it.
- [x] aarch64 boots under an emulator on an x86_64 host, or the toolchain names
      one that does. The build names QEMU's TCG with `-machine virt -cpu
      max,pauth-impdef=on` for any host that cannot virtualise aarch64
      (`build/app/Machine.kira`). `kira build boot -arch aarch64 -emulate yes
      -test` forces that path on Apple silicon and prints "booted to the
      userland"; an x86_64 host has not run it.
