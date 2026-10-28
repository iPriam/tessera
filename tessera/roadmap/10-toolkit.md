# 10 — Toolkit

Kira UI, running on Tessera. This is mostly integration: the libraries exist,
they are somebody's real work, and they have never been pointed at this machine.

## What exists, elsewhere

    KiraUI                 widgets, modifiers, a declarative tree
      └ KiraUIFoundation   retained tree, reconciliation, dirty flags, materials
          ├ KiraLayout     layout
          ├ KiraUIMotion   animation
          ├ Ember          Vulkan / Metal / Dawn backends; Vulkan on Tessera
          └ KCoreAnimation layers, symbols, the glyph atlas, text
      └ KCoreUI

The API is SwiftUI-shaped:

    RunWidgetApp(
        VStack(spacing = Spacing.Medium) {
            Text("Hello, Kira UI!").typography(.title)
            Card(material = Material.Frosted) { Text("Frosted glass card") }
        }
        .inset(horizontal = Spacing.Large, vertical = Spacing.Large)
    )

None of it has run on Tessera, and every layer between it and the hardware is a
different answer here than on a desktop.

## What has to change for Tessera

### There is no window

`RunWidgetApp` opens one. Every runner in Ember opens one. Tessera has
no window system — a client of the display server has a *surface*, whose size the
server tells it ([08](08-display-server.md)).

`openHeadlessGraphicsWithBackend(.Vulkan)` already exists for exactly this
reason, described in the source as being for "a COMPONENT of a compositor"
whose "pixels belong to another process". That is precisely a Tessera
application. So the path is:

    openHeadlessGraphicsWithBackend(.Vulkan)
      → a Vulkan device with no swapchain
    createExportedTexture
      → render into a texture whose memory is a dma-buf
    attach that descriptor to a surface, commit
      → the display server shows it

What is missing is a **runner** — the piece that is `RunWidgetApp` on a desktop
and on Tessera is: connect, receive `configure`, build the tree, render on
`frame_done`, feed input events in, reconnect if the server restarted.

The runner is TesseraUI's (`tessera-ui`), a public framework every
application links. KiraUI stays general: it names no platform it is hosted on,
and gives a host what it needs in general terms, such as the environment's
`isStageManagerActive` and `hostUsesCommandKeySet`. An application starts with
`RunTesseraApplication`, which runs the runner in place of the path that opens
a window, so every application and every platform draws through KiraUI and
nothing else; there is no second way onto the display.

### AppKit is the bridge to the platform

Some of what an application does is the platform's to show: its menus go in the
system bar ([11](11-adaptive-desktop.md)). There are four Dynamic Platforms
([15](15-form-factors.md)), so an application never talks to one itself. AppKit
is the bridge between KiraUI and whichever platform is running: it holds the
application's end of the platform link PID 1 mints, sends the menus
KiraUI collected from `.commands {}`, and runs in KiraUI the command a person
chose in the bar. AppKit draws nothing and does not run the display session;
that is TesseraUI's runner.

### PlatformUI is what the platforms draw with

Every Dynamic Platform draws through KiraUI like an application, and the
components they share are PlatformUI's ([15](15-form-factors.md)): the system
bar's items, the stage strip, the split divider, the switcher,
notification banners, status items, alerts and the focus ring, each chosen by
window model, navigation model and viewing distance rather than by platform.
PlatformUI is built on KiraUI and PlatformKit and is a private framework, as
AppKit is: an application never draws a platform's components.

### The frame loop is inverted

A desktop toolkit draws when it wants and asks the window system to present. A
Tessera client draws when it is told ([08](08-display-server.md)): it asks for a
frame, receives `frame_done`, and has until the next one.

That inversion is good — it is what makes an idle application cost nothing — and
it means the runner drives the toolkit rather than the other way round. Kira UI
Foundation's dirty-flag model fits it exactly: a frame that finds nothing dirty
should not ask for another.

### Input arrives as protocol messages

Not from a device, not from a window system callback. `key`, `pointer_motion`,
`pointer_button`, with modifiers and a resolved character already attached
([06](06-devices-and-input.md)). The runner turns those into whatever the
toolkit's event type is.

Focus is the server's; a client that loses focus stops receiving input and
should visibly say so.

### Scale

The display server announces a scale factor with the output. Everything the
toolkit lays out is in logical units multiplied by it. Getting this wrong is
invisible on a 1× display and unmissable on a 2× one, so it should be wired
before there is a second display to test on.

It is wired: MosaicDisplay's `output` message carries the scale
([08](08-display-server.md), "Connection"), the session keeps it, and the
runner makes its textures the configured size times the scale and has
KCoreAnimation lay the tree out in points and draw it at the scale, which is
what the windowed runner does with a Mac's backing scale factor. MosaicDisplay
composes in points at the same scale, so a window, the cursor and the picture
under them agree.

## The unknowns, honestly

- **Text.** Kira UI draws words through KCoreAnimation, whose text engine on
  other platforms is C (`kiratext`). Tessera links no C, so text waits on the
  Kira engine ([09](09-text.md)). Symbols are already Kira.
- **The Vulkan backend on venus, in anger.** Applications already render into
  exported textures that MosaicDisplay scans out. Composing several of them in
  one pass, every frame, is a heavier use of the same extensions and the place
  where "it works" and "it works on venus" diverge.
- **Kira UI's material system** — frosted glass, blur-behind, glow — assumes it
  can sample what is behind a surface. In a compositor that is the *server's*
  content, not the client's. Either those effects are within-application only,
  or the protocol grows a way to ask for a blurred backdrop, which is a real
  feature with a real cost. Glass within an application is the application's
  own and refracts its own content. Glass over other clients is the
  server's: a client asks for it with `set_glass`
  ([08](08-display-server.md), "Surfaces"), and MosaicDisplay draws the same
  Regular glass with the same KCoreAnimation batch, between the surfaces it
  composes. The platforms' Dock, notification banners, a desktop's
  Control Center panel and its switcher, and each Control Center module on
  a touch screen stand on it.

## What Tessera should not do

- **Not fork the toolkit.** Every change wanted for Tessera should be a change
  in `kira-ui` or `ui-foundation`, made in general terms, or in TesseraUI,
  which hosts KiraUI on the display. A fork is how two codebases and one maintainer become a
  dead project.
- **Not build a second toolkit.** There is a real one; the work is making it run
  here.
- **Not put policy in the toolkit.** What is focused and what launches is the
  platform's ([11](11-adaptive-desktop.md)).

## Checkpoints

- [x] A Kira package with dynamic native libraries cross-builds for
      `aarch64-linux-gnu` and runs inside Tessera. Each application is such a
      package.
- [x] Ember renders into an exported Vulkan shared texture and the display
      server scans it out directly.
- [x] The runner exists: connect, configure, render, present, reconnect. The
      display-restart boot kills MosaicDisplay and proves the surviving UI renders
      again through the replacement link.
- [x] The runner is TesseraUI's, and AppKit holds only the bridge to the
      platform. TesseraUI runs applications (`RunTesseraApplication`),
      the platforms (`RunDynamicPlatform`) and the display check; AppKit is
      three files of `Bridge/`: menus, notifications and the links.
- [x] A Kira UI `Text` widget is on the display, in a real font. The desktop's
      bar, its Dock and Notes set their text in Inter, which KCoreAnimation
      ships in its framework's `Resources/Fonts`, reads and rasterises in Kira.
- [x] A `VStack` of widgets lays out at the output's size and scale: on a
      2880x1800 output with no EDID (`OUTPUT_SIZE=2880x1800`), the platform and
      the desktop lay out at 1440x900 points and draw at scale 2.
- [x] A keypress changes what a widget shows: typing into the launcher's
      search shows the letters in the field and filters the list.
- [x] An idle application asks for no frames and the machine issues no flips. The
      services test asserts exactly two requested flips, then leaves the client idle.
