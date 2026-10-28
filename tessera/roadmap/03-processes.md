# 03 — Processes

**Built.** What follows is the design and the reasoning; the checkpoints at the
end are all met. Two things that were open questions when this was written have
answers now, both recorded in the section on spawning.

The immediate gap. Nothing visual moves until this is done, because the display
has to leave PID 1 and there is nowhere for it to go.

## Where it stands

One process. PID 1 mounts four filesystems, prints a banner, sets a display
mode, draws a splash, reads the keyboard, hands over to a system volume or
refuses to boot, and otherwise reaps forever.

`execve`, `wait4` and `ppoll` are wrapped and used. `clone`, `kill`, `setsid`,
`pipe2`, `dup3`, `signalfd4` and the `epoll` family are in the syscall table
with no callers.

`/bin` exists in the initramfs and is empty. `build-initramfs.sh` builds one
package and stages one file, and its header comment says so.

## Why PID 1 must stop owning the display

**DRM master is exclusive.** Whoever holds `/dev/dri/card0` open owns the
display and nobody else can set a mode while they do. PID 1 holds it for the
life of the machine on purpose — closing it hands the console its mode back and
takes the splash with it.

So the thing that makes the splash work is the same thing that would stop Kira
UI from ever having the display. This is not tidiness. It is the architecture,
and it is why Phase A comes first.

It compounds: the display server must be a glibc-carrying program because it
uses Vulkan ([02](02-kernel-and-syscalls.md)), and PID 1 is freestanding. They
cannot be the same program even in principle.

## What an init should be

Small, and boring, and never the thing that crashes.

    mount what the userland expects
    say what this machine is
    start the services, in order
    watch them; restart what should be restarted
    wait to be told to stop
    stop them, in reverse, ask before killing
    sync, and power off

Everything else — the display, the keyboard, the network, the platform — is a
service. PID 1's own code should shrink as this roadmap progresses, not grow.

## The pieces

### More than one program per image

`build-initramfs.sh` builds `init` and copies one binary to `/init`.
It needs to build a list of packages and stage each one where the service table
expects it.

The same for the system volume, which currently carries a single `/init` and
will carry the display server, the Dynamic Platforms, and the applications.

### Spawn

**`clone3`, not `clone`.** Legacy `clone` takes its arguments positionally and
the last two are in a different order on aarch64 than on x86-64, so the same
source is a different call per architecture and the mistake is silent. `clone3`
takes a `struct clone_args` and a size, identically everywhere — and building a
structure at byte offsets is what this userland is good at. It also carries
`CLONE_PIDFD`.

**The runtime survives a syscall that returns twice.** That was the open
question and the answer is yes: the child is a copy-on-write image of a
single-threaded process continuing from the same instruction with a different
return value, and nothing in the Kira runtime objects.

**The signal mask survives both the fork and the `execve`**, which is the trap
that cost this session a rebuild. PID 1 blocks `SIGTERM` so it can read it from
a descriptor; a child that inherits that block can never be *asked* to stop,
only killed, and every service then takes the full shutdown deadline to die for
a reason nothing prints. The child resets its mask with `SIG_SETMASK` before
`execve`.

A spawn wants, in order: `clone`; in the child, `setsid` if it leads a session,
the descriptor plumbing below, then `execve`; in the parent, the child's pid
recorded against the service that owns it.

### A service table

What to start, where it lives, in what order, and what to do when it stops.

    service      display-server
      path       /bin/mosaicdisplay
      after      (nothing)
      restart    always
      fatal      yes      — the machine is useless without it

    service      adaptive-desktop
      path       /bin/adaptive-desktop
      after      display-server
      restart    always
      fatal      no

Deliberately not a general dependency graph. Tessera has a handful of services
and a list with an `after` is enough; a graph is what gets built when the list
was not enough and it never was.

**Restart policy matters more than ordering.** A display server that segfaults
should come back with the screen intact, which means the restart path has to
work for a program that holds a DRM master — the master is dropped when the
process dies, the console takes the mode back, and the replacement takes it
again. That flicker is the honest cost and it is better than a black machine.

Guard against a service that dies immediately and forever: a restart count in a
window, and after that a line on the console and a stop.

### Signals

`signalfd4` turns signal delivery into a readable descriptor, which is what lets
PID 1 wait on children *and* on being told something with one `ppoll`. The
alternative — a signal handler setting a flag — needs `rt_sigaction`,
`rt_sigreturn` and a correct understanding of what may be called from a handler,
and none of that is worth it when the descriptor exists.

`SIGCHLD` on the descriptor, then `wait4(-1, WNOHANG)` in a loop until it says
there is nothing left. `SIGTERM` and `SIGINT` mean shut down.

### Shutdown

A boot stops through `tessera.autoexit`, through `SIGTERM` to PID 1, or through
Ctrl-Alt-Del. PID 1 switches Ctrl-Alt-Del off in the kernel, so the key arrives
as `SIGINT` and takes the same path; the services boot tests end that way. The
path:

1. Stop accepting new work.
2. `SIGTERM` every service, in reverse start order.
3. Wait, with a deadline — a few seconds, not forever.
4. `SIGKILL` whatever is left.
5. `sync`, unmount what can be unmounted, `reboot`.

The deadline is the whole design. An init that waits forever for a service to
exit is an init that hangs the machine on shutdown, which is the single most
visible bug an operating system can have.

A service that waits on a LibTessera port hears the request with
`processWatchTermination` (16 §11.3) and returns from main: power, devices,
volume, bootstrap, devicekitd and MosaicDisplay. devicekitd stops its drivers
with `processAskToStop` before it leaves. PID 1 itself hears the stop,
Ctrl-Alt-Del and its children ending with `processWatchSystem` (16 §25.5),
and stops the machine with `machineStop`; the signal descriptor and `reboot`
above are how LibTessera's Linux backend does it, and no program names
either. The others (ui and the platforms) wait in `recvmsg` and are ended by
the signal itself. PID 1 then logs one entry, `stopped the services`, naming the
services that exited by themselves with status 0 and those that ended
otherwise, in start order. One entry rather than one per service because the
log framework prints only some repeats of the same entry.

### Descriptor plumbing

`pipe2` and `dup3`, so a child's output goes somewhere chosen. Today a child
would inherit whatever PID 1 has, which is the console. That is fine for one
service and wrong for five: their output interleaves and nothing says which line
came from where.

The smallest useful version is a pipe per service, read by PID 1, each line
prefixed with the service name and written to the console. That is also the
first half of a log ([14](14-tooling-and-ports.md)).

Built with a LibTessera channel in place of the pipe, since a standard stream
is "Channel or File" ([16](16-libtessera.md) §11.4). PID 1 makes one channel
per service and gives its child end as both StdOut and StdErr; `kernelLaunch`
puts it on descriptors 1 and 2 and makes them wait when full, as a pipe does.
A program writes plain bytes there, so a packet that brought no handles and
does not start with LibTessera's empty prefix is read as bare payload. PID 1
waits on the channel on its port beside the service's process, writes each
whole line as `name: line`, keeps an unfinished line until the rest comes or
the service closes the channel, and reads what is left before it reports a
service's end, at shutdown included.

Linux queues ten packets on a unix socket and caps a send buffer at 208 KiB.
PID 1 raises both to §10.1's channel limits, 1 024 messages and 4 MiB, before
it starts anything.

### Sessions and process groups

`setsid` so a service leads its own session and a signal to a group does not
reach the wrong things. Cheap, and much harder to retrofit than to do now.

### Process identity

`prctl(PR_SET_PDEATHSIG)` so a child dies if its parent does — which matters for
the display server's own children, if it ever has any, more than for PID 1's.

## What moves out of PID 1

All of it, in one cut:

- the DRM/KMS code and the framebuffer fallback
- the splash drawing
- the keyboard reading — **input is a display server's job**, not an init's, and
  the two were only ever together because there was one process

PID 1 keeps: mount, banner, boot flags, spawn, supervise, signals, shutdown,
handover to the system volume, refuse to boot.

The new program is `mosaicdisplay`, and it is the display server's first
form: at this stage it opens the card, sets a mode, draws the splash, reads the
keyboard and idles. Every later section of this roadmap grows that program
rather than replacing it.

Note the ordering problem this exposes: the display server wants to be a
glibc-carrying program on the system volume, but the splash should appear
*before* the system volume is mounted, otherwise the machine is black for the
whole of boot. Two answers — a small freestanding splash service in the
initramfs that hands off, or a display server that starts from the initramfs
with no GPU stack and gains it later. **The first is simpler and honest**: a boot
splash and a display server are different programs that happen to draw the same
mark.

## Checkpoints

- [x] The initramfs stages two programs and `/bin` is not empty.
- [x] `clone` returns twice in a Kira program and both sides carry on.
      init starts every service with `clone3`.
- [x] The splash is drawn by a process that is not PID 1: MosaicDisplay.
- [x] `kill -9` on that process, and the splash comes back.
      `tessera.restart-test`, checked by `-services-restart`.
- [x] `SIGTERM` to PID 1 stops the machine cleanly, with each service's exit
      named on the console. The boot tests send Ctrl-Alt-Del, which is the
      same path through `SIGINT`.
- [x] A service's output is prefixed with its name.
      Every boot log reads `bootstrap: info[TesseraBootstrap.main]: listening`,
      `devicekitd:     = bundle: "tablet-driver.dext"` and so on, and all six
      boot tests pass with it.
- [x] A service that dies ten times in ten seconds is given up on, and said so.
      `tessera.crash-loop-test` runs `date` as a restarted service, which ends
      as soon as it starts; `-services-restart` waits for PID 1's fault
      `a service died ten times in ten seconds and will not be started again`
      with `service: "crash-loop"` and `starts: 10`, and the boot goes on.
