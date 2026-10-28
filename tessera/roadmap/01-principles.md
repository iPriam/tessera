# 01 — Principles

What Tessera is, so that everything after this can be judged against something.

## The thing itself

A single-user operating system whose userland is written in Kira, on a Linux
kernel, with a GPU-first display path and a sealed system volume.

Three consequences, and most of the roadmap is downstream of them:

- **Not a distribution.** No package manager for system software, no `/usr/bin`
  full of other people's programs, no shell in a sealed build.
- **Not POSIX-complete, and not trying to be.** Kira reaches the kernel through
  a table it names call by call. The question is never "is this in POSIX" but
  "does Tessera need it, and for what".
- **The userland is the product.** The kernel is Linux and stays Linux. Every
  interesting decision is above the syscall boundary.

## The system is Tessera's own software

**Everything a person experiences as Tessera is written for Tessera.** Init, the
display server, the Dynamic Platforms, the toolkit, the protocol between them. Not adapted,
not shimmed, not a compatibility layer over somebody else's desktop.

This is a stronger rule than "no distribution" and it decides arguments before
they start:

- **No Wayland**, and not because it is bad. It is somebody else's protocol,
  designed to carry somebody else's toolkits. Tessera's display server talks to
  Tessera's clients. See [08](08-display-server.md).
- **No X11, no D-Bus, no systemd, no udev, no busybox, no GTK, no Qt.** Each is
  a good piece of software solving a problem Tessera either does not have or
  intends to solve itself.
- **No borrowed desktop.** A platform that is somebody else's desktop makes the
  operating system a theme.

### Where the line actually is

The rule has to be testable or it is a slogan, and Tessera plainly ships other
people's code: Linux, glibc, Mesa and the Vulkan loader. Millions of lines of
it, in the sealed volume.

**The line is user-facing behaviour.** The kernel, the C library and the GPU
stack are substrate: they have no surface a person touches, they are replaceable
without anything above noticing, and writing them again would be a decade spent
not building an operating system. Everything above the substrate — every process
whose behaviour a person could describe — is Tessera's.

Stated as a test: *if a user could notice it behaving a certain way, Tessera
wrote it.* Mesa fails that test and is fine. A window manager passes it and is
therefore not allowed to be someone else's.

The substrate list is closed and should stay short. Adding to it is a decision,
not a convenience. It is also below the SDK: a component is Kira and RIDL, and
Vulkan is the only substrate a component links directly
([14](14-tooling-and-ports.md)), and WebKit under the exception in `AGENTS.md`. Text, which once leaned on FreeType and
HarfBuzz, is Kira ([09](09-text.md)).

## Applications are the exception

**A person can install applications Tessera did not build.** That is the one
place foreign code runs, and it is deliberate — a machine that only runs what its
author shipped is an appliance, not a computer.

It is also the single largest consequence in this roadmap, because everything
about the sealed model assumed the opposite:

- Applications cannot live on the sealed system volume, because they arrive after
  it was written. See [05](05-storage-and-state.md).
- An application is no longer trusted by virtue of having shipped. The sandbox
  stops being a later refinement and becomes a precondition. See
  [12](12-apps.md).
- Something has to decide whether an application may be installed at all, which
  means signatures, which means Tessera needs cryptography it does not have. See
  [13](13-security-and-updates.md).

The rule of thumb: **the system is closed, the application layer is open, and the
boundary between them is enforced rather than assumed.**

## The machine belongs to the person using it

Stated separately because it is the thing a sealed system is most likely to
forget, and because an earlier draft of this roadmap forgot it.

- **A person may run what they choose.** There is no key Tessera requires and
  no mandatory store or review for installing software. The planned
  [App Store](25-app-store.md) is a way to discover and manage applications;
  direct installation remains available. Signatures say *who wrote this* and
  *has it changed*; they never say *you may not*.
- **The seal covers Tessera's own software and stops there.** It exists so a
  person can know their operating system was not tampered with. Their
  applications are their business.
- **Confinement is for the owner's benefit, not against them.** An application is
  sandboxed so that it cannot reach another application's data or the system —
  which protects the person, and is why it applies to trusted and untrusted
  software alike.
- **There is no developer mode.** Running code you wrote on a machine you own is
  not a special state.

`HANDOFF.md` §7 already said this: setup is macOS-shaped, not ChromeOS-shaped.
An application layer that requires the vendor's blessing is neither — it is
iOS-shaped, and it is not what Tessera is.

## Decisions already made

From `HANDOFF.md` §7, restated here because a roadmap that relitigates them is
not a roadmap.

- **Refuse to boot.** A system volume that will not mount stops the machine,
  with the reason on the console. No recovery mode — a boot that repairs is a
  boot that did not refuse.
- **Setup is macOS-shaped, not ChromeOS-shaped.** An installer creates state
  once; later boots verify rather than reconcile.
- **The seal is absent, not stubbed.** Read-only is true. Trusted is not, and
  saying so is the point.
- **The card is opened once and never closed.** A DRM master that closes hands
  the console its mode back.
- **GL is not a path.** Apple deprecated it, Sokol is leaving Ember, and
  zink and llvmpipe are both roads back to it.
- **16K pages on aarch64**, because the host maps guest memory at its own
  granularity and a 4K guest asking for a host-visible buffer kills the
  emulator.
- **No C in the userland.** The runtime carries the raw-memory seam; Tessera
  wrote nine functions it did not need and has deleted them. The graphics test
  image is the exception and is C on purpose, because the stack it exercises is.

## Decisions this roadmap makes

Each is argued where it belongs. Collected here so they can be disagreed with in
one place.

| Decision | Answer | Where |
|---|---|---|
| Where the raw-memory seam lives | Kira's runtime. Generate struct offsets from kernel headers next. | [02](02-kernel-and-syscalls.md) |
| One libc story or two | Two, named: freestanding programs and glibc-carrying programs, separated by a process boundary. | [02](02-kernel-and-syscalls.md) |
| Display server: process or library | Process, with a protocol, from the start. DRM master is exclusive. | [08](08-display-server.md) |
| Wayland or an own protocol | Own. Smaller, fits a sealed system, and Wayland buys compatibility Tessera does not want. | [08](08-display-server.md) |
| Composition: GPU or DRM planes | GPU through Ember's Vulkan backend, with direct scanout when one surface covers the display. | [08](08-display-server.md) |
| Window model | A mode the compositor holds and changes while running: fullscreen, split, freeform, stage manager. Which are offered is the form factor's. | [15](15-form-factors.md) |
| One platform or several | Several Dynamic Platforms on one operating system. One compositor owns mode; a platform owns presentation and is a privileged client of it. | [15](15-form-factors.md) |
| What decides the input model | What is attached, not what the machine is. Capability is a runtime set, broadcast to every client. | [15](15-form-factors.md) |
| What an application is | A read-only image on the data volume, mounted and integrity-checked at launch. | [12](12-apps.md) |
| Whether foreign code runs | Yes. Sandboxed always; permitted by the owner, never by the project. | [13](13-security-and-updates.md) |
| Update model | Read-only system snapshots with rollback, the A/B pair generalised. | [13](13-security-and-updates.md) |
| Multi-user | No. Single-user until there is a second customer for privilege separation. | [13](13-security-and-updates.md) |

## Non-goals

Named so nobody builds them by accident.

- **A shell.** A sealed system that hands out a shell is not sealed.
  The `tessera.shell` boot flag that once handed one out is gone, and the
  initramfs carries no shell program.
- **A package manager for system software.** The system updates as a whole
  volume ([13](13-security-and-updates.md)) or not at all. Applications install
  individually; nothing else does.
- **X11, or Wayland compatibility.** Neither is a client Tessera wants, and
  supporting either would mean the toolkits behind them.
- **A POSIX application environment.** An application is a Kira program against
  Tessera's own interfaces. Porting an existing Linux binary is not a goal and
  the absence of a shell, a package manager and most of libc makes it moot.
- **A browser, other than WebKit.** The largest program anyone could try to
  port, and the one that would drag in every dependency the sealed model exists
  to avoid. WebKit WPE is the one engine allowed, under the exception in
  `AGENTS.md`; no other engine is.
- **Multi-user, remote login, or a network filesystem.**
- **POSIX completeness.** Calls arrive when something needs them, with a reason.
- **Backwards compatibility with anything.** There is nothing to be compatible
  with yet, and the freedom is worth more than the promise.

## How to read a milestone

Every section ends in checkpoints rather than tasks. A checkpoint is something a
person can see happen or a script can grep for. If a step cannot be stated that
way it is not understood well enough to start.
