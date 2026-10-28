# 15 — Dynamic Platforms and input

Tessera runs on four Dynamic Platforms and adapts to what is plugged into them.
This says what that means for the display server, the platforms, and the input
pipeline, because each of the three answers it differently and only one of them
is Phase C.

## The four Dynamic Platforms

Each is a platform of its own, on the one shared operating system. Tessera has
no shells: what a person sees and touches is the platform, a privileged program
of its own. Two are called *adaptive*
because each spans many form factors, not because they are one platform with
two faces.

- **Adaptive Mobile** — phones, folds, tablets. Fullscreen on a phone, split on
  an opened fold, and on a tablet the full range: fullscreen, split, freeform
  windows, and a stage-manager arrangement.
- **Console** — a machine driven from a couch, usually one application at a time
  and usually the reason the machine exists.
- **Adaptive Desktop** — a display, a pointer, and the expectation of windows,
  from a laptop to a desk with several displays.
- **Media** — a television. The furthest from a pointer, and the one where
  distance decides the design rather than screen size.

Each platform has a default window model and a default input model. The two
adaptive platforms are not held to theirs, and that is what makes them
adaptive. Adaptive Mobile starts in a stack driven by touch, and it also offers
freeform windows and a stage-manager arrangement, with a pointer, a keyboard or
a controller, when the device can use them. Adaptive Desktop starts freeform with
a pointer, and it also offers a stack and touch on a convertible. Console and
Media are limited: fullscreen, driven by a directional controller, and nothing
else. PlatformKit says this once, in `DynamicPlatform`'s `adapts`, `allowedWindowModels`
and `allowedNavigationModels`, so no platform decides it on its own.

## What is one thing and what is four

**One compositor. One PlatformKit. One PlatformUI. Four Dynamic Platforms, each
its own program.**

The compositor owns *mode*: how many surfaces are visible at once and what
geometry each is given. Fullscreen, split, freeform and stage manager are its
states, and it changes between them while running — a fold opens, a tablet
docks, a controller wakes.

In Stage Manager the sidebar contains the existing application surfaces. A
window keeps its size and has a separate remembered position on the stage;
leaving Stage Manager restores its freeform frame. The compositor animates
the same surface, including its glass and shadow, between the stage and its
preview. Up to six waiting windows fit down the leading edge, centered as a
group. The platform supplies badge artwork in an offscreen atlas in its
committed frame. The compositor attaches each badge to its window using the
same presentation geometry and transition progress, including reversals.
Each application's badge overlaps its own preview, and clicking
either activates that surface without delivering the click to its contents.
Focusing the desktop moves every application into the strip. Rendering and
hit testing use the same presentation geometry during a transition.

PlatformKit (`platform-kit`) owns the machinery every platform needs:
application and surface state, launch and switcher state, focus policy,
notifications, system status, restoration, accessibility hooks, and the
privileged display-server protocol. It is a library, not a platform, and it
draws nothing.

PlatformUI (`platform-ui`) owns the components every platform draws with, built
on KiraUI and its system bar APIs: the system bar's items, the stage strip,
the split divider, the switcher, notification banners, status items, alerts
and the focus ring. It is a private framework on KiraUI and PlatformKit, so
applications never see it. A window's chrome is drawn by its application
([08](08-display-server.md)), through KiraUI, which draws it while the
display's mode is a windowed one.
A component is chosen by window model, navigation model and viewing distance,
never by platform: a tablet in a stage and a desk in a stage draw the same
stage strip, and Console and Media get the couch-distance form of the same
switcher. What a platform shows, where, and when stays the platform's own
presentation policy; how a component looks for a window model, a navigation
model and a distance is PlatformUI's.

Each Dynamic Platform is its own program on top of both: `adaptive-desktop`,
`adaptive-mobile`, `media`, and `console`, each with that platform's
presentation and interaction policy. Adaptive Desktop and Adaptive Mobile use
more of PlatformKit and PlatformUI than Console and Media, whose navigation and
presentation are genuinely couch-distance experiences.

This split keeps [08](08-display-server.md) from having to know what a television
is without duplicating platform correctness four times. A platform is still a
client, with the privileged platform role, speaking the same protocol as
everything else. Compile any platform into the display server and Tessera has
committed to display policy inside the compositor; give every platform its own
machinery and its own components and launching, restoration, notifications,
focus semantics and the look of a window immediately fork four ways.

## Switching platforms while running

The Dynamic Platform a machine runs is switched while it runs, with no reboot.
That is the premise: one operating system under four platforms, so a tablet
docked to a display can become a desktop and a desktop can become the tablet
it is folded into, with every application still open.

The platform is the setting `platform.dynamic` ([16](16-libtessera.md) §14.4):
`adaptive-desktop`, `adaptive-mobile` or `media`, the platform programs the
system carries. Settings is where a person changes it. The running platform
hears the change and asks PID 1 to switch, a platform-only request like a mode
change. PID 1 ends the running platform and starts the one named in its place:
the same display client, so MosaicDisplay keeps its band and the same surface,
and a new platform link to every application, as a platform that is started
again gets. The applications keep running, and their windows stay where the
compositor has them until the new platform asks for its own mode.

At boot PID 1 starts the platform the setting names. A machine with no value
starts the one its boot words name (`tessera.adaptive-mobile`,
`tessera.media`), and without those the one its chassis is. The Dynamic
Platform that `DeviceInfo` reports is the one running, read the same way, so
everything on the machine draws for the platform it is on.

## What this changes in decisions already made

[01](01-principles.md) records *"One application at a time, full screen, with a
switcher. Windows later, if ever."* That was the right call for one machine with
one display. It does not survive four Dynamic Platforms: freeform and stage manager
*are* windows, and they are a shipping mode rather than a someday.

Most of the groundwork survives, which is the good news and worth saying
plainly. [11](11-adaptive-desktop.md) already says the protocol carries a size per
surface and a stacking order, and already sketches split view as "two
applications, each given half the output through `configure`". [01](01-principles.md)
already calls direct scanout an optimisation for the fullscreen case rather than
the path. None of that has to move.

**One thing does contradict.** [11](11-adaptive-desktop.md) said a floating model
was "a shell change rather than a protocol change", and that is no longer true.
If the compositor owns mode, then freeform is compositor state and a platform
asks for it. The line is replaced rather than softened: the *appearance* of a
floating window is the platform's, drawn with PlatformUI, and the decision that
windows float is the compositor's.

## Adaptive Input

**Input capability is a runtime set, not a property of the form factor.** This is
the part most likely to be got wrong by assuming, so it is stated first: a
handheld is a console with a touchscreen, a tablet is a gaming machine when a
controller is paired to it, and a television may be driven by a phone. Nothing
about the machine's shape says what is attached to it.

Three consequences.

**Hotplug is load-bearing.** [06](06-devices-and-input.md) already lists
enumeration and hotplug; what changes is that they stop being a convenience. A
controller pairing mid-session and a remote going to sleep are ordinary events,
not edge cases.

**Capability belongs in the protocol.** Not only "a key was pressed" but "a
pointer exists now" and "the gamepad went away". A platform choosing a navigation
model and an application choosing between button prompts and touch controls are
making the same decision from the same information, so capability is broadcast
to every client rather than reserved to the platform — unlike a mode request,
which is the platform's alone.

**Navigation follows the last-used input.** A tablet with a controller attached
wants a focus ring; the moment a finger touches the screen it wants direct
manipulation, and when the controller moves again it wants the ring back. This is
the mechanism that makes adaptive input more than a list of what is plugged in.

## The open question

**Who owns the last-used-input state.**

In the compositor, every platform inherits one behaviour and they cannot
disagree — which is right if this is a property of the machine. In the platform,
a television
could keep its focus ring through an incidental touch, and a desktop could
ignore a controller that is only there for a game — which is right if this is a
property of what the person is doing.

It is one line either way and an annoying one to move afterwards, because it
decides whether the protocol carries a mode or a fact. Decide it before
[08](08-display-server.md) has an input path.

## Where this lands in the plan

Not Phase E, despite sounding like platform work. The compositor is where a form
factor stops being a rendering choice and becomes an architecture, so the parts
that are Phase C are:

- a mode concept in the display server, with live transitions between modes;
- a privileged platform role, and platform-only requests to change mode;
- capability events alongside input events.

Everything else — four platforms, what each looks like, which of them ships
first — is Phase E and additive. **The vertical slice does not lengthen.** Step
13 is still one platform launching one application, and the other three come
afterwards.

## Checkpoints

- [x] The display server holds a mode, and a platform can ask it to change.
      MosaicDisplay holds an `exclusive`/`stack`/`freeform`/`stage` mode and
      derives each surface's geometry through it; adaptive-desktop sends
      `SetMode` from its window policy on the platform session, which
      MosaicDisplay honours only from that session. `-services` asserts the
      mode changes.
- [x] A mode change re-lays-out live, and a client survives being resized by it.
      A stable surface is re-Configured with the new mode's geometry and answers
      the fresh Configure from its session loop; `-services` asserts surface 2
      re-lays-out to 853x533 on the platform's freeform request.
- [x] A client is told what input exists, and is told again when that changes.
      MosaicDisplay broadcasts an input-capability message (kind bitmask +
      present flag) to every client when it opens the keyboard; `-services`
      asserts the application is told it has one. Hotplug re-broadcast waits on
      device enumeration ([06](06-devices-and-input.md)).
- [x] Two applications are on screen at once, each given part of the output.
      Adaptive Desktop's freeform windows overlap in a cascade
      (`-services`: `2 at 143,114 853x446, 3 at 171,142 853x446`), as
      Windowed Apps' do on Adaptive Mobile. Adaptive Mobile's stack is not
      split: the application in front fills the screen.
- [x] A second Dynamic Platform runs against the same display server with no
      change to it. media (Media, full-screen) boots under `tessera.media`
      in place of adaptive-desktop; MosaicDisplay serves it unchanged.
      `-services-media` asserts it.
- [x] Adaptive Desktop and Adaptive Mobile draw the same stage strip from
      PlatformUI and the same window chrome from KiraUI, chosen by window
      model. Both platforms supply PlatformUI's badge artwork through TesseraUI;
      MosaicDisplay attaches it to each animated window. TesseraUI's host has
      KiraUI draw the traffic lights on an
      application's window while the mode is `freeform` or `stage`, and on
      neither platform in `exclusive` or `stack`. A Stage Manager window on
      Adaptive Mobile shows them, and its minimize puts the window in the
      strip.
- [ ] The platform is switched while the machine runs. Writing
      `platform.dynamic` has the running platform ask PID 1 to switch; PID 1
      ends it and starts the one named against the same display server, with
      the applications still running and linked to it, and starts that one at
      the next boot.
