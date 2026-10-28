# diskutil

The command-line tool for volumes, after macOS's `diskutil`. It asks the
Volume service, which owns every mount under `/Volumes`, and never mounts
anything itself:

    diskutil list
    diskutil info <volume>
    diskutil activity
    diskutil mount <volume>
    diskutil unmount <volume>
    diskutil eject <volume>

A volume is named by its node (`vda`, `vdb1`), its mount point, or the last
part of its mount point (`scratch` for `/Volumes/scratch`). Ejecting a
volume unmounts every volume on its disk, and the Volume service leaves the
disk alone until it starts again.

`activity` keeps a channel to the service open and prints every volume
that appears, is mounted or unmounted, or disappears, in the words of
macOS's `diskutil activity`, until the service goes.

The Volume service's wire is in `roadmap/16-libtessera.md` §25.4 in the
tessera repository. Build it with that repository's scripts.
