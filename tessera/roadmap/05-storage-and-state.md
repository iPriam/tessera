# 05 — Storage and state

Where anything is remembered. A machine that cannot remember is a demonstration.

## Where it stands

- The volume is the btrfs below. The boot step (build/app/Install.kira) boots the
  installer against it with the system image the build packed (squashfs) as a
  second disk; the installer puts the btrfs on a blank disk with btrfs-progs's
  `mkfs.btrfs`, run without `-f` so it refuses a disk that carries a file
  system, makes `boot/`, `system/` and the `data` subvolume with its skeleton,
  copies the image's tree into a subvolume, snapshots it read-only as
  `system/<version>` (the version is the image's digest), keeps the system that
  was current before it, drops `system/dev`, and writes `boot/current` last.
- PID 1 in the initramfs mounts the first disk that mounts as btrfs, reads
  `boot/current`, mounts `system/<version>` read-only (or `system/dev`,
  writable, taken from it when missing, on `tessera.dev`, saying "unsealed") at
  `/sysroot`, moves it onto `/`, chroots into it and replaces itself with the
  system's init, which is the same program, passing its boot flags on. That
  init mounts the `data` subvolume at `/data` with
  `MS_NOEXEC|MS_NODEV|MS_NOSUID` and binds `/Applications`, `/Cache` and
  `/BuildCache` from it. A volume with no `boot/current` is refused.
- The initramfs holds PID 1, LibTessera and the C runtime they load; the boot
  splash is still MosaicDisplay's, in the system.
- The system is sealed. The build writes the composefs image of the system,
  with every file's fs-verity digest, into the system at `composefs.img`. The
  installer turns fs-verity on for every file it writes into the system
  subvolume. PID 1 mounts the read-only snapshot, then the image as EROFS,
  then overlayfs over both with `verity=require`, and logs the image's
  digest.
- Not yet: verifying the image's signature, the pending system and its boot
  counter, and per-agent subvolumes.
- `/tmp` as tmpfs, which dies with the machine.
- Files can be read.
- `getdents64` is in the syscall table with no caller, so nothing can list a
  directory.

`CONFIG_BTRFS_FS`, `CONFIG_EXT4_FS`, `CONFIG_TMPFS` and `CONFIG_VIRTIO_BLK` are
on.

## The volume layout

One btrfs volume, laid out as an APFS container is: the system and the data
are subvolumes of one filesystem, and each system is a read-only snapshot.

    /dev/vda    the volume     btrfs
      boot/                    which system is current, which is pending, and
                               the pending one's boot counter
      system/<version>         a system, a read-only snapshot, sealed
      system/dev               on a dev machine only: a writable snapshot of
                               the current system, unsealed
      data                     everything a person did; mounted noexec

**A system is immutable and interchangeable.** It holds the userland: the
display server, the platforms, the applications, the frameworks, the fonts.
Nothing in it is written after the installer or an update wrote it, which is
what makes a signature over the whole of it meaningful
([13](13-security-and-updates.md)). Every file in it has fs-verity on, and the
system carries a composefs image: an EROFS image of its tree, every name, mode
and directory, and for each file the fs-verity digest of its contents. PID 1
mounts the system through that image with overlayfs's `verity=require`, so a
file whose contents do not match its digest cannot be read, and the image's own
fs-verity digest is the one value a signature covers.

Snapshots replace the A/B pair: an update is a new `system/<version>`, the
previous one stays until the next update is known good, and two systems share
every block they have in common. The initramfs carries PID 1 and the boot splash
and nothing else ([02](02-kernel-and-syscalls.md), [07](07-graphics.md));
everything that links the C library is in the system.

**The data subvolume holds everything a person did.** Application state,
preferences, files. It is never executed from and is mounted `noexec`, so that
"data" and "code" are a property of the mount rather than of a path.

It is btrfs so that an agent's state is a copy-on-write subvolume. A snapshot
shares every unchanged block with its parent and diverges only where it is
written, so forking an agent's world and rolling it back are metadata operations
rather than copies of everything the agent ever wrote. ext4 stays in the kernel
for a volume that wants a plain filesystem; this one is not.

PID 1 finds the volume as the first `/dev/vd?` that mounts as btrfs, rather than
by a fixed device letter, and mounts the subvolumes by name.

`CONFIG_EROFS_FS`, `CONFIG_OVERLAY_FS`, `CONFIG_FS_VERITY` and
`CONFIG_BLK_DEV_LOOP` are on for this.

**A dev machine** (`tessera.dev`, [18](18-components.md)) boots `system/dev`
instead: a writable snapshot of the current system, mounted directly without the
composefs image, so a component's root can be installed into it while the
machine runs. It is unsealed, and PID 1 says "unsealed" at every boot. Resetting
it is deleting the snapshot and taking it again from the sealed system it came
from, which is instant.

Updates are in [13](13-security-and-updates.md); the layout is here because it
constrains the installer, and the installer is the only component allowed to
create state.

## The filesystem interface Kira needs

What exists: `openat`, `close`, `read`, `write`, `lseek`, `pread64`, `pwrite64`,
`fstat`, `statx`, `mkdirat`, `unlinkat`, `renameat2`, `ftruncate`, `fdatasync`,
`getdents64`, `fchmodat`, `statfs`, `syncfs`.

That is a complete set and almost none of it is wrapped. What is missing is not
syscalls but a **shape** — right now a caller opens a path, reads bytes into an
mmap'd block, and pulls fields out with load-at-offset. That is correct and it is
not something a hundred call sites should each reimplement.

The pieces worth building once:

- **Read a whole file into an array.** The single most common thing anything
  will do — a font, a manifest, a configuration. Bounded by `fstat` first so a
  file that grew between the two calls does not surprise anyone.
- **Write a file atomically.** Write to a temporary in the same directory,
  `fdatasync` it, `renameat2` over the target, `fdatasync` the directory. Four
  calls in that order, every time. This is the one piece of filesystem code that
  is genuinely subtle and it should exist exactly once — the failure it prevents
  is a file that is present, zero-length, and was fine before the power went
  out.
- **List a directory.** `getdents64` answers a packed buffer of variable-length
  records: `d_ino`, `d_off`, `d_reclen`, `d_type`, then a NUL-terminated name.
  Walk by `d_reclen`. Needed by the platform to find applications and by the
  installer to check what it wrote.
- **A path type.** Not string concatenation. Joining, the parent, the last
  component, and a rule that a path handed to an application is confined to its
  own directory ([12](12-apps.md)).

## Where things live

    /system              the current system, once it is the root
      /bin               PID 1, the display server, the platforms
      /apps/<id>/        the applications Tessera itself ships
      /share/fonts/      the fonts Tessera ships
    /Applications        the writable volume's Applications, bound here at boot
      /<id>.app          an installed application, as an image file
      /<id>.sig          its signature, when it has one
    /Cache               the writable volume's Cache, bound here at boot
      /<owner>/          one directory per application or service
    /BuildCache          the writable volume's BuildCache, bound here at boot
      /<owner>/          one directory per application or tool
    /data                the data subvolume, mounted noexec
      /Applications/     what /Applications shows
      /Cache/            what /Cache shows
      /BuildCache/       what /BuildCache shows
      /appdata/<id>/     that application's data, its own and nothing else's
      /system/           preferences, the display's chosen mode, whatever the
                         platform remembers
    /run                 tmpfs — sockets, pids, nothing that outlives a boot
    /tmp                 tmpfs

Applications a person installed are images on the writable volume and are
*mounted* read-only to be run ([12](12-apps.md)). That is what keeps `noexec`
honest: the volume holds application images as data, and the only thing the
kernel ever executes is a read-only mount whose contents were checked.

## Caches

Every cache on the machine lives in `/Cache`, and every expensive build product
lives in `/BuildCache`. Nothing else may keep one: not an application's data
directory, not `/data/system`, not a hidden directory next to the thing it
caches. One place means one answer to "what is using my disk", and one place to
clear.

- **`/Cache`** holds anything that can be made again from something else:
  decoded images, glyph atlases written to disk, downloaded files that can be
  downloaded again, thumbnails, shader pipelines. Tessera may delete any of it
  at any time, whole directories or single files, without asking — when the
  volume runs low, after an update, or because it wants to. A program that
  finds its cache gone rebuilds it and is not allowed to fail because of it.
- **`/BuildCache`** holds what a build made that is slow to make again:
  compilation caches, object files, fetched and built dependencies, a
  toolchain's intermediate output. Tessera never deletes it on its own; that
  is the person's decision.
- **Both belong to the person.** Both are writable and both can be emptied, a
  directory at a time or entirely, from the platform or a file window. Deleting
  either never loses anything that cannot be remade, and no program may store
  something there that it cannot remake.

Both sit at the root without a prefix, like `/Applications`, and are bound from
the writable volume at boot the same way. An application gets its own
`/Cache/<id>/` and, when it builds things, its own `/BuildCache/<id>/`, and sees
no other application's.

An application never sees another application's image or data. That is enforced
by what the launcher hands it, and then by a mount namespace — which stops being
a refinement and becomes a precondition once the code is code Tessera did not
write.

## Configuration

There is none, and the temptation is to invent a format. Resist it for as long
as possible: every setting that exists is a thing that can be wrong, and a sealed
appliance's answer to most settings is to not have them.

When it is needed, the shape that fits: **a file per setting**, in
`/data/system/`, holding a value and nothing else. No parser, no schema, no
merge order, and `getdents64` lists them. It is unfashionable and it is
debuggable at three in the morning.

The one thing that genuinely needs to be remembered early is the display mode,
because a machine that picks a different resolution every boot is a machine
something is wrong with.

## The installer

The only component allowed to create state, and therefore the only one that can
be wrong in a way nothing later corrects.

What it does, once:

1. Create the btrfs volume, with `boot/`, `system/` and the `data`
   subvolume and its directory skeleton above.
2. Write `system/<version>` from the image it was given: every file, fs-verity
   turned on for each, the composefs image beside them, and the subvolume made
   a read-only snapshot. Verify what it wrote against the image's signature.
3. Record that system as current in `boot/`.
4. Sync everything and say so.

It runs from its own image — an initramfs with the installer as PID 1 — and
never from a running system. That keeps it out of the sealed volume and means
the running system has no code path that creates a volume.

The guest formats its own disk rather than the build host doing it, with
btrfs-progs's `mkfs.btrfs` rather than filesystem code written here.

**It must be idempotent in the only way that matters:** an installer interrupted
halfway must leave a machine that refuses to boot rather than one that boots
into something half-written. Given refuse-to-boot, that is nearly free: write
the "this volume is current" record last, and a machine with no such record
refuses.

`renameat2`, `fdatasync` and the read direction earn their place here.

## Checkpoints

- [x] `getdents64` is wrapped and something lists a directory.
- [x] A file is read whole into a Kira array by one call.
- [x] The atomic-write sequence exists once and is used everywhere.
- [x] A data volume is mounted read-write at `/data`.
- [x] The display's chosen mode is written there and read back on the next boot.
- [x] The installer creates a machine from nothing, and an interrupted run
      produces a machine that refuses to boot. `kira build boot -interrupted`
      makes a blank volume, kills the installer's QEMU once it logs "made
      the data subvolume", and boots the volume: PID 1 refuses, "the system
      could not be mounted: the volume records no current system".
- [x] The system is a read-only btrfs snapshot with fs-verity on every file,
      and PID 1 mounts it through its composefs image with `verity=require`.
- [x] The initramfs carries PID 1 and the boot splash only. It carries PID 1
      and what PID 1 needs, and no splash: the release step's
      `initramfs-root` is 11 files, `init`, `LibTessera.framework`, the C
      library and its loader, `seal.pub` and `etc/tessera-release`, beside
      the mount points.
- [x] A dev machine boots a writable snapshot of the current system and says
      "unsealed" at every boot. PID 1 takes `system/dev` from the current
      system when there is none, mounts it unsealed and prints it at every
      dev boot (`init/app/Volume/System.kira`); `kira build boot -dev`'s
      serial log: "tessera: unsealed: booting system/dev, a writable
      snapshot".
- [x] An application image on the data volume is loop-mounted read-only and run
      from, while the volume itself stays `noexec`. The system's init mounts
      `data` with `noexec`, `nodev` and `nosuid` (`init/app/Volume/Mount.kira`),
      and opening `/Applications/<Name>.app` mounts the image read-only at
      `/run/Applications/<Name>` and starts the bundle there
      (`init/app/Supervise/Install.kira`), as [12](12-apps.md) records.
