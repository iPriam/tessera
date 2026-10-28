# date

The time, and setting the clock, after macOS's `date`:

    date                          Tue Jan  1 00:00:00 UTC 2030
    date mmddHHMM[[cc]yy][.ss]    sets the clock, then prints it

Tessera keeps the clock in UTC and has no time zones yet, so `-u` changes
nothing. Setting the clock is a system tool's business: LibTessera lets no
application set a clock (`roadmap/16-libtessera.md` §8.1 in the tessera
repository), so date makes the kernel's call itself, as macOS's date calls
`settimeofday`. The Power service sees the clock jump and posts
`timeChanged` to every process.

Build it with the tessera repository's scripts.
