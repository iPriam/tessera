# caffeinate

Keeps the machine from sleeping for a while, after macOS's `caffeinate`:

    caffeinate -i -t 30

It takes a LibTessera WakeLock named "caffeinate command-line tool", which
the Power service holds as a wake source until the time is up. `-i`, `-s`
and `-m` keep the processor running; `-d` asks to keep the display on,
which only the front-most application may. Without `-t` the lock lasts ten
minutes, the library's limit for a lock with no timeout. Running a utility
under the lock is not supported yet.

WakeLock is in `roadmap/16-libtessera.md` §18 in the tessera repository.
Build it with that repository's scripts.
