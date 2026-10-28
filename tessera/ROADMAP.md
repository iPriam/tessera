# Roadmap

Where Tessera is going. `HANDOFF.md` covers where it is and how it got there.

Tessera boots to a desktop, runs applications and remembers their data. The
next feature milestones cover Home, an App Store, everyday applications and
interactions, device connections, accessibility, and protecting work across
restarts and devices.
The original desktop slice is recorded below; detailed chapters carry the
acceptance checkpoints and the remaining gaps.

Every item below names what it unblocks. Cut anything nothing depends on.

## The documents

| | |
|---|---|
| [01 Principles](roadmap/01-principles.md) | What Tessera is, the decisions already made, and what it will never have |
| [02 Kernel and syscalls](roadmap/02-kernel-and-syscalls.md) | What the kernel provides and what Kira can reach of it |
| [03 Processes](roadmap/03-processes.md) | Spawning, supervision, signals, shutdown |
| [04 IPC](roadmap/04-ipc.md) | Sockets, descriptor passing, and the protocol above them |
| [05 Storage and state](roadmap/05-storage-and-state.md) | Volumes, files, the data partition |
| [06 Devices and input](roadmap/06-devices-and-input.md) | Enumeration, hotplug, keyboard, pointer, event pipeline |
| [07 Graphics](roadmap/07-graphics.md) | Buffers, scanout, Ember on Vulkan, composition |
| [08 Display server](roadmap/08-display-server.md) | Surfaces, wire protocol, composition, frame pacing |
| [09 Text](roadmap/09-text.md) | Fonts, shaping, rasterisation, glyph cache |
| [10 Toolkit](roadmap/10-toolkit.md) | Kira UI on Tessera: widgets, layout, input, theming |
| [11 Adaptive Desktop](roadmap/11-adaptive-desktop.md) | What a person sees and touches |
| [12 Applications](roadmap/12-apps.md) | What an app is, how it ships, how it is launched and confined |
| [13 Security and updates](roadmap/13-security-and-updates.md) | The seal, verity, A/B volumes, signing |
| [14 Tooling and ports](roadmap/14-tooling-and-ports.md) | Build, CI, debugging, the other two hosts |
| [15 Dynamic Platforms and input](roadmap/15-form-factors.md) | The four Dynamic Platforms Tessera runs on and the modes one compositor switches between |
| [16 LibTessera](roadmap/16-libtessera.md) | The system interface: every call, exported over RIDL, and what this tree implements |
| [17 DeviceKit](roadmap/17-devicekit.md) | The driver interface below LibTessera's services and Ember: registry, surfaces, fences, display, input, media, power, block, Hid, Usb and user-space drivers. All five build steps of §25 are done |
| [18 Components and dev roots](roadmap/18-components.md) | Components, SDKs, incremental builds and changing a running development machine |
| [19 Files and documents](roadmap/19-files-and-documents.md) | Inlay, Trash, Open and Save, document types, previews and search |
| [20 Networking](roadmap/20-networking.md) | Wired connections, Wi-Fi, saved networks and downloads |
| [21 Audio](roadmap/21-audio.md) | Volume, device selection, mixing, microphones and media controls |
| [22 Power](roadmap/22-power.md) | Brightness, battery behaviour, sleep, wake and power-saving modes |
| [23 Product experience](roadmap/23-product-experience.md) | The next product milestones: Home, apps, multitasking, notifications, sharing and setup |
| [24 Home and personalisation](roadmap/24-home-and-personalisation.md) | Home screen pages, folders, App Library, Dock, wallpaper and widgets |
| [25 App Store](roadmap/25-app-store.md) | Discovering apps, installing, managing updates and removing apps |
| [26 System search](roadmap/26-system-search.md) | Apps, documents, Settings, calculations and quick actions in one local search |
| [27 Lock screen](roadmap/27-lock-screen.md) | Customisation, private notifications, media, timers, alarms and authenticated unlock |
| [28 Desktop and displays](roadmap/28-desktop-and-displays.md) | Desktop files, snapping, workspaces, multiple displays and remembered arrangements |
| [29 Everyday applications](roadmap/29-system-apps.md) | Photos, Preview, Calculator, Calendar, Reminders, Weather, Contacts and recording |
| [30 Storage management](roadmap/30-storage-management.md) | Space accounting, cleanup, external-drive formatting and safe removal |
| [31 Backup and recovery](roadmap/31-backup-and-recovery.md) | Automatic backups, document history, migration and recovering an unbootable system |
| [32 Privacy and permissions](roadmap/32-privacy-dashboard.md) | Per-app decisions, active-use indicators, access history, location and revocation |
| [33 Language and input](roadmap/33-language-and-input.md) | Layouts, input methods, emoji, dictation, spellcheck, replacement and multilingual text |
| [34 Accessibility](roadmap/34-accessibility.md) | Semantic UI, screen reading, keyboard navigation, magnification, captions and alternative input |
| [35 Device connections](roadmap/35-device-connections.md) | Bluetooth, printers, cameras, controllers and nearby file transfer |
| [36 App lifecycle](roadmap/36-app-lifecycle.md) | Identity, defaults, links, background work, startup, Force Quit and crash recovery |
| [37 Continuity](roadmap/37-continuity.md) | Trusted pairing, shared clipboard, handoff, forwarded notifications and optional sync |
| [38 Console and Media](roadmap/38-console-and-media.md) | Complete Home, Store, downloads, settings and recovery flows for controllers and remotes |

## Next: the product experience

The next roadmap is organised around what a person does with the machine.
[23 Product experience](roadmap/23-product-experience.md) sets the sequence and
the acceptance conditions across the system:

1. **Make Home yours:** persistent pages, icon placement, folders, App Library
   and Dock editing; then wallpaper and widgets.
2. **Find and manage apps:** an App Store with search, app pages, installation
   progress, updates and removal. Direct installation remains available.
3. **Move between tasks:** system search, app switching, window arrangements,
   workspaces and session restoration.
4. **Catch up and act:** notification history, actions, per-app controls and
   Focus schedules.
5. **Move work between apps:** clipboard, drag and drop, sharing, screenshots
   and screen recording.
6. **Set up the device:** first-run setup, personalisation, accessibility and
   authenticated unlock.
7. **Use everyday apps:** photos, document previews, calculations, calendar,
   reminders, contacts, weather and recording.
8. **Protect and move work:** backups, document recovery, migration and optional
   continuity between paired devices.
9. **Connect and control devices:** Bluetooth, printing, cameras, controllers,
   multi-display layouts and usable Console and Media platforms.

Files ([19](roadmap/19-files-and-documents.md)), networking
([20](roadmap/20-networking.md)), audio ([21](roadmap/21-audio.md)) and power
([22](roadmap/22-power.md)) supply the corresponding everyday capabilities.
Home comes first. The App Store follows the minimum networking and app identity/
update support it needs; it does not wait for every Wi-Fi or power milestone.
Backup, language/input, accessibility and app lifecycle are requirements of a
complete daily-use release and are designed alongside each flow that needs them.
Existing isolation and delivery gaps remain in [12](roadmap/12-apps.md),
[13](roadmap/13-security-and-updates.md) and [14](roadmap/14-tooling-and-ports.md).
New feature checkpoints remain unchecked until their user-visible behaviour is
verified. The chapters identify hardware and authority boundaries where needed.

## The vertical slice

The shortest path to a desktop with one working application. Each step ends in
something that can be seen or asserted.

| # | Step | Ends when | Doc |
|---|---|---|---|
| 1 | ~~Two programs in one image~~ | **done** — `/bin/mosaicdisplay` is staged beside `/init` | [03](roadmap/03-processes.md) |
| 2 | ~~Spawn from Kira~~ | **done** — `clone3` returns twice and the runtime survives it | [03](roadmap/03-processes.md) |
| 3 | ~~Display out of PID 1~~ | **done** — the splash is drawn by pid 81, not by init | [03](roadmap/03-processes.md) |
| 4 | ~~Service supervision~~ | **done** — a killed display comes back as pid 82 | [03](roadmap/03-processes.md) |
| 5 | ~~Sockets in Kira~~ | **done** — the display reads `/etc/tessera-release` through a descriptor init opened | [04](roadmap/04-ipc.md) |
| 6 | ~~Blob framebuffers~~ | **done** — a GPU-rendered image is scanned out with no copy (07's checkpoints) | [07](roadmap/07-graphics.md) |
| 7 | ~~Flip events~~ | **done** — `UI frame 1` then `UI frame 2 reached the display`, each from its flip event | [07](roadmap/07-graphics.md) |
| 8 | ~~The protocol~~ | **done** — a client's exported Vulkan frame is shown, and two are composed on the GPU | [08](roadmap/08-display-server.md) |
| 9 | ~~Input routing~~ | **done** — the multi-client boot asserts a keypress reaches the focused client and no other | [08](roadmap/08-display-server.md) |
| 10 | ~~Ember inside Tessera~~ | **done** — a Kira program renders a widget tree through Ember, in the guest | [07](roadmap/07-graphics.md) |
| 11 | Text | A glyph, shaped and rasterised, is on the display | [09](roadmap/09-text.md) |
| 12 | Kira UI renders | A widget tree is on the display and responds to a key | [10](roadmap/10-toolkit.md) |
| 13 | The platform | Something launches an app and shows which one is focused | [11](roadmap/11-adaptive-desktop.md) |
| 14 | An application | A program nobody built into the image runs and draws | [12](roadmap/12-apps.md) |
| 15 | State | It remembers something across a reboot | [05](roadmap/05-storage-and-state.md) |
| 16 | Installing | Someone installs software Tessera did not build, and it is confined | [12](roadmap/12-apps.md) |

Steps 1–5 and 10 are done: there is a process system, the display is a
supervised service rather than something PID 1 does, and a socket between them
has carried a file descriptor.

Steps 6–9 are the rest of the display server and the largest piece of work here.
Steps 11–12 are integration; the libraries exist and have not been pointed at
this machine. Steps 13–16 cover apps, state, and installing software the project
never saw.

## Phases

Grouped by what unblocks what.

### Phase A — A process system — done

Steps 1–4. `clone` from Kira, more than one program per image, the display and
the keyboard out of PID 1, a service table, signals, and a shutdown that asks
before it kills.

Ended when PID 1 mounted, started the display, restarted it after a `SIGKILL`,
and stopped the machine by asking, with the splash still on screen. The shutdown
log shows signal 15 rather than 9: the service was asked and went, rather than
being killed on the deadline.

### Phase B — Processes that can talk — done

Step 5, plus generating struct offsets from the kernel's headers.

Ended when init made a `socketpair` before starting the display, handed the child
one end at descriptor 3, and sent it a message carrying an open descriptor to
`/etc/tessera-release`, which the display read out without knowing a path to it.

The offsets came first. Every DRM structure, `msghdr`, `cmsghdr` and the
`CMSG_SPACE`/`CMSG_LEN` arithmetic was a hand-written line checked against the
headers by a C file; that check is gone with the rest of the C. The
framework-hosted compiler makes a library's structs, enums and functions from
its headers through a manifest's `Autobind` record, but it does not follow
`#include` or evaluate `#if`, so the kernel's uapi headers are not yet among
them. See [02](roadmap/02-kernel-and-syscalls.md).

### Phase C — A display server

Steps 6–9, on the channel Phase B built. Blob framebuffers, flip events,
surfaces, the wire protocol, composition, input routing.

Ends when a program that is not the display server puts a pixel on the screen and
receives the keypress that follows.

### Phase D — A toolkit — started

Steps 10–12. Ember headless inside Tessera, text in Kira, a font that ships,
KCoreAnimation, Kira UI.

Ends when a Kira UI widget tree is on the display, drawn by the GPU, through the
display server.

Where it stands: `tessera-ui` is a supervised service that lowers a widget tree,
lays it out, renders it, and submits the frame, through Kira UI, Kira UI
Foundation, Ember, Dawn, WebGPU, Vulkan, venus, and the host's Metal. That
was the first boot; Tessera has since dropped Dawn, and applications render
through Ember's Vulkan backend into textures MosaicDisplay scans out. Text
waits on the Kira text engine ([09](roadmap/09-text.md)), because the SDK is
Kira and RIDL with Vulkan the one native library, and WebKit WPE the exception
([14](roadmap/14-tooling-and-ports.md)).
`scripts/build-ui-stack.sh` builds the whole thing. The pixels land in a GPU
texture and stop there. Getting them onto the screen is steps 6 and 8: a client
hands the compositor a descriptor, and Tessera has no compositor yet.
`mosaicdisplay` owns the card and knows nothing about surfaces.

`ui` checks clean again and lowers to KIR for aarch64:

```console
$ kira check ../ui
$ kira build ../ui --target aarch64-unknown-linux-gnu --emit kir
```

It does not, at that point, run. `tessera-ui` reaches a display through the guest
GPU stack, which this Fedora host has not built, so the initramfs here is built
without it and says so. The chain it now uses is `caLayoutTree` →
`caReconcileImmediate` into a `Compositor` → `caCompositorEncode`, and
`caCompositorEncode` builds the backend and owns the frame begin, so Tessera
constructs no backend of its own.

Two things had to be true first, and both are now, in the sibling repositories
rather than in a patch here:

- **Kira UI compiles, for the host and for aarch64 Linux.** It did not before,
  on any machine: Ember bound `objc_msgSend` under one name at many
  shapes, which the FFI rule refused. Kira now lets one FFI name carry several
  shapes of one symbol (a call picks one by arity, argument types, then expected
  result), and autobind takes the sysroot so a cross-target header reaching
  `<math.h>` resolves. Both landed in the old Kira implementation, since
  archived. The sysroot now arrives
  through the manifest's `Headers` block rather than a `kira build` flag,
  since `--sysroot` is no longer a flag the framework-hosted compiler takes.
- **sokol is gone from Ember.** It was GL, which pulled X11 and OpenGL
  onto the link line. `patches/ember-tessera` is deleted.
  `scripts/build-ui-stack.sh` builds the sibling checkouts directly and
  compiles no C: their Tessera rows link the Vulkan loader and nothing else.

### Phase E — A desktop

Steps 13–14. The platform, the app model, packaging, launching, confinement.

Ends when an application that was not built into the image is running.

### Phase E′ — Somebody else's software

Step 16. Image signing and verification, a mount namespace per application,
seccomp, a uid, and an install path that tells the owner what it is doing.

Ends when a person installs an application the project never saw, it runs, and it
cannot reach anything it was not given.

### Phase F — A machine someone could use

Step 15 and everything in [05](roadmap/05-storage-and-state.md),
[13](roadmap/13-security-and-updates.md) and
[14](roadmap/14-tooling-and-ports.md): a writable volume, an installer, A/B
updates, the seal, CI, and the other two hosts.

Ends when it survives a reboot with something a person put there.

## Sibling checkouts

Tessera builds against a compiler the Kira Language Framework installs, the
Kira libraries in `../../kira-projects`, and the framework's LLVM bundle.

The compiler is a driver built from the `.klfd` definitions in `../klf-kira` and
installed as `kira` under `~/.klf`. `klf` is the framework binary,
`/root/.klf/bin/kira` is a launcher that reads
`~/.klf/toolchains/kira/current.toml` and dispatches to the channel named there,
and `TESSERA_KIRA` names a compiler outright when the question matters. A cross
target also needs a sysroot holding that target's C library, because the LLVM
bundle the framework builds with carries a compiler and none. The framework
reads `KLF_SYSROOT_<TRIPLE>` first, with the triple's dashes as underscores, then
plain `KLF_SYSROOT`; the build package passes the staged sysroot as the second.
See [14](roadmap/14-tooling-and-ports.md).

Nothing comes from the old Kira implementation (`kira-projects/kira`), which is
archived: system calls are LibTessera's, Foundation ships with the toolchain,
and the LLVM bundle is the newest one under `~/.kira/toolchains/llvm`.

Anything Tessera needs that Kira has not committed is one `git checkout` from
being lost. This happened once: the syscall table existed only in the old Kira
checkout's working tree and the repository did not compile without it. The
framework carries every Linux call by name now. See
[14](roadmap/14-tooling-and-ports.md).
