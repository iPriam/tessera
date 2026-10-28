# reportcrash

The crash reporter, after macOS's ReportCrash. PID 1 names it in the kernel's
`core_pattern`, so the kernel starts it as a process dies of a signal, with
the process's core on standard input, and keeps the process until it is done:

    |/bin/reportcrash %P

It reads the dying process's threads, registers and stacks from the core, and
its program, arguments, mappings and status from /proc, and walks each
thread's frame pointers. It writes what it found as a report in
`/data/system/DiagnosticReports`, `<process>-<date>.crash`: the exception,
the lines the process last wrote to the system log, every thread's frames as
an image and an offset in it, the crashed thread's registers, and every
binary image with its build ID. The newest reports are kept.

The frames name images and offsets, as an unsymbolicated report on a Mac
does; `kira build devroot crashes` in the tessera repository fetches the
reports and names them from the build's symbols.

Build it with the tessera repository's scripts.
