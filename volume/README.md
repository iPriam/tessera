# volume

The Tessera Volume service, which does what diskarbitrationd does on macOS:
it finds the disks and partitions DeviceKit reports, works out what each
holds, and mounts the file systems (roadmap/17-devicekit.md §25 in the
tessera repository).

At start it opens every DeviceKit BlockDevice and Partition entry, reads its
geometry and probes its first 68 KiB for a partition table or a file system
it knows: GPT, MBR, ext4, btrfs, EROFS, SquashFS or FAT. Every file system
it can mount is mounted at `/Volumes/<label>`, `noexec`, `nodev` and
`nosuid`, read-only when the device is. Two registry watches keep it
current: a disk attached later is probed and mounted, and one pulled out is
detached and forgotten.

It checks in with the broker as `tessera.volume` and answers `List`,
`Mount`, `Unmount` and `Eject` (roadmap/16-libtessera.md §25.4). `diskutil`
is its client.

It is built by `scripts/build-initramfs.sh` in the tessera repository and
started by init as `/bin/volume`.
