# 02 — Kernel and syscalls

The floor. Nothing here is interesting on its own and everything above it stops
if a piece is missing.

## The kernel is not the constraint

Linux 7.2, patched in place, configured in `kernel/config/`. It already has on:

    CONFIG_NET  CONFIG_INET  CONFIG_UNIX  CONFIG_VIRTIO_NET  CONFIG_VIRTIO_BLK
    CONFIG_EXT4_FS  CONFIG_TMPFS  CONFIG_DRM_VIRTIO_GPU  CONFIG_VIRTIO_INPUT
    CONFIG_INPUT_EVDEV  CONFIG_FRAMEBUFFER_CONSOLE  CONFIG_ARM64_16K_PAGES

Squashfs comes from defconfig. So unix sockets, a writable filesystem and a
network device are all *already there* — the graphics image's C init forks and
execs today. Everything missing is missing from Kira's reach, not from the
machine.

Deliberately off, each preventing a specific failure — see `HANDOFF.md` §8:
`DEBUG_INFO` (a 3 GB link), `LOGO` (Linux paints its mascot on the framebuffer),
`COMPAT` (no ARM backend in the pinned LLVM), `MTD` (the pflash probe kills the
emulator under hvf).

Not configured and each a decision rather than an oversight: `DM_VERITY`
(see [13](13-security-and-updates.md)), `OVERLAY_FS`, `UDMABUF`, sound, USB,
RTC, power management.

## What Kira can reach

48 calls. Present and used: `read write mount umount2 reboot execve wait4
exit_group sync ppoll openat close ioctl mmap munmap chdir chroot`.

**Present with zero callers anywhere** — not in Tessera, not in
`packages/linux`:

    clone  kill  setsid  pipe2  dup3  fcntl  rt_sigaction  rt_sigprocmask
    rt_sigreturn  signalfd4  epoll_create1  epoll_ctl  epoll_pwait  getdents64
    mprotect  lseek  pread64  pwrite64  fstat  statx  clock_gettime  nanosleep
    getpid  mkdirat  unlinkat  renameat2  ftruncate  fdatasync  fchmodat
    syncfs  statfs

The ability to start a second process has been sitting there unused since the
table was written.

## What is missing, in priority order

### The socket family — ~15 calls

`socket bind listen accept4 connect socketpair sendmsg recvmsg sendto recvfrom
shutdown getsockname getsockopt setsockopt`

`CONFIG_UNIX` is already on, so this is purely a Kira-side gap. It is first
because **`SCM_RIGHTS` is the only way to pass a buffer between processes**, and
a display server that cannot be handed a client's buffer is not a display
server. See [04](04-ipc.md).

### futex and the thread group

`futex set_robust_list clone3` and the `CLONE_THREAD` flags. Kira programs are
single-threaded and cannot be otherwise. Nothing needs this yet. Everything that
does real work eventually will, and Mesa already runs threads inside the
glibc-carrying processes where the kernel provides all of this natively.

### timerfd

`timerfd_create timerfd_settime`. There is `nanosleep` and `clock_gettime` and
no way to be woken later *while also* waiting on something else. Needed the
moment anything has a frame clock.

### The small ones

`getrandom` (identifiers, and anything cryptographic), `uname` (a machine that
cannot say what it is), `prctl` (process naming, `PR_SET_PDEATHSIG` so a child
dies with its parent), `memfd_create` (shared memory that is not a file),
`eventfd2` (waking one process from another cheaply).

### Netlink

Device hotplug arrives on a netlink socket. Needs the socket family first. See
[06](06-devices-and-input.md).

### The setuid family

Only when [13](13-security-and-updates.md) revisits privilege separation.

A general-purpose system lands somewhere around 90 to 110 calls. Tessera is at
48 and the next 15 are all sockets.

## The seam, and the thing under it

**Settled:** raw memory is Kira's own. `heapLoad<T>` and `heapStore<T>` are
compiler builtins, and blocks are `ByteBlock`s from the Heap library that free
themselves. Tessera once wrote nine functions of C for this and has deleted
them.

**Not settled, and the real question:** every one of those calls takes a byte
offset, and every offset in this userland was hand-transcribed from the kernel's
UAPI headers into a Kira constant that nothing checks. `HANDOFF.md` §6 is a
table of them. Fine at one modesetting program; not obviously fine at a display
server, a filesystem and a network stack.

- **Keep hand-writing them.** Every offset is a place to be silently wrong, and
  an ioctl answers `EFAULT` either way.
- **Generate them.** A build step that reads the kernel's own headers and emits
  Kira constants. The numbers stop being remembered.
- **Get the language feature.** A struct-overlay type that says "this memory is
  this shape", so an offset is derived rather than declared. A change in
  `../kira`, and the only answer that makes this pleasant rather than merely
  possible.

**Recommendation: generate them, argue for the overlay type in parallel.** The
generator stops being needed the day the feature lands, and until then it is the
cheapest large win available.

## Two classes of program

This was never decided; it accreted, and it should be said out loud because it
decides what the display server can be.

- **Freestanding.** Static, no libc, no dynamic loader, talks to the kernel
  through the syscall table. PID 1 is this. The initramfs carries no
  interpreter, so a program that names one is refused by the kernel before it
  runs, with nothing said about why.
- **Glibc-carrying.** Dynamically linked against the C library, with a loader,
  living on the system volume. The graphics image is this, because the Vulkan
  loader and Mesa's driver find each other by name at runtime and there is no
  version of that which is one static binary.

**Anything that uses Vulkan must be the second kind.** The display server
composes through Ember's Vulkan backend. So the display server is a glibc-carrying program on the system volume,
and PID 1 stays freestanding in the initramfs, and the boundary between them is
a process boundary.

That is not a compromise, it is the correct shape: the part that must never fail
carries nothing, and the part that carries everything can be restarted.

## Checkpoints

- [ ] Struct offsets are generated from `linux-7.2`'s own headers rather than
      typed, and the generated file is in the build rather than in the repo.
- [x] The socket family is in Kira's table and `packages/linux` wraps it.
      `packages/linux` is LibTessera's Linux backend now: `Syscalls.kira`
      names `socket`, `socketpair`, `bind`, `listen`, `accept4`, `connect`,
      `sendmsg` and `recvmsg` from the table, and `Channel.kira`,
      `Network.kira` and `Seam.kira` wrap them.
- [x] A Kira program calls `clone` and both sides come back. LibTessera's
      `kernelLaunch` calls `clone3`; the parent gets the child's pidfd, and
      the child of a confined launch runs `enterView` in Kira, in its new
      mount namespace, before it execs. The boot test requires Display
      Check's "checked the application's view", which only that child's
      view gives.
- [x] `timerfd`, `getrandom`, `uname`, `prctl`, `memfd_create` land with a named
      caller each: Power's wake alarm, `randomFill`, `System.info`, the
      confined launch's no-new-privileges, and every Memory object.
