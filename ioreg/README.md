# ioreg

Prints DeviceKit's registry, after macOS's `ioreg`:

    ioreg               every entry the caller may see, as a tree
    ioreg -c Camera     only the entries of one class
    ioreg -l            each entry's properties under it

An entry is printed under its parent in the Service plane, by its `name`
property, else its device node's name, else its class:

    +-o vda  <class BlockDevice, id 0x2000000000003>

A property that is a list, and any after it in the same entry, is left
out for now.

The registry is in `roadmap/17-devicekit.md` §7 in the tessera repository.
Build it with that repository's scripts.
