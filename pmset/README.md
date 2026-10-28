# pmset

The command-line tool for power, after macOS's `pmset`. It reads the page
the Power service keeps through LibTessera's `Power.read` and prints it in
pmset's words:

    pmset -g batt     where the power comes from, and the battery
    pmset -g therm    the thermal level and the processor's speed limit
    pmset -g          both

It only reads. The settings macOS's pmset writes, such as sleep timers,
have no counterpart in the Power service yet.

The page's layout is in `roadmap/16-libtessera.md` §18 in the tessera
repository. Build it with that repository's scripts.
