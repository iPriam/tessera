# 08 — The display server

The largest single piece of work in this roadmap, and the thing every visible
feature is downstream of.

## What it is

The one process that opens `/dev/dri/card0` and keeps it open. It sets modes,
owns buffers, composites what clients give it, reads every input device, decides
who has focus, and paces itself off the display's own flip events.

Everything else that draws is a client of it, including the platform.

## Why it is a process

**DRM master is exclusive.** One open descriptor owns the display. Today that is
PID 1, which is why nothing else can ever have it ([03](03-processes.md)).

And it must be a *glibc-carrying* process, because it uses Vulkan
([02](02-kernel-and-syscalls.md)). PID 1 is freestanding. They cannot be the
same program even in principle, which settles the question that would otherwise
be a matter of taste.

## Why not Wayland

Wayland is the obvious answer and it is the wrong one here.

- **What it buys** is clients Tessera does not want: GTK, Qt, Firefox, Electron.
  A sealed system ships what was built ([01](01-principles.md)), so
  compatibility with a software ecosystem is compatibility with things that will
  never run.
- **What it costs** is real: `wl_shm` and `linux-dmabuf` and `xdg-shell` and
  subsurfaces and viewporters, a C library or a hand-written wire protocol, and
  a set of semantics — the commit model, the frame callback contract, the
  configure/ack dance — that exist to serve twenty years of toolkits.
- **What Tessera needs** is about twenty messages between programs it built
  itself, all on one machine, all speaking the same version.

So: **an own protocol, small, and designed so a Wayland shim could be written
later** if a reason ever appears. That last clause costs nothing today — it
means using dma-buf and not inventing a buffer-sharing mechanism, and keeping
the commit model recognisable.

## The protocol

Over `SOCK_SEQPACKET` unix sockets ([04](04-ipc.md)). A message is a
fixed header, a body, and descriptors in the ancillary data.

### Connection

    client → server   hello(version)
    server → client   welcome(id_range_base, id_range_count)
    server → client   output(id, width, height, refresh, scale)

An output is announced rather than asked for. There may be more than one later;
a client that only ever handles the first is correct today and obviously wrong
later, which is the right way round.

The welcome hands a client the surface numbers it may name: MosaicDisplay gives
each client one, its own surface, so the range is that number and a count of
one. The output follows it, before the first `configure`: its mode in pixels,
its refresh in millihertz and its scale, how many pixels a point is along each
side. Everything else the protocol carries is in points: a `configure`'s size,
a placement, the bands the platform keeps, a pointer position and a window's
margin. A client draws its buffers at its configured size times the scale, as
an application on a Mac draws at the backing scale factor, and MosaicDisplay
lays the picture out in points and composes it in pixels. The scale is two once
the panel holds 192 pixels an inch or more, twice the 96 a point stands for,
and one below; a display that does not say its size is taken as two once it is
taller than 1600 pixels.

### Surfaces

    client → server   create_surface(id)
    client → server   attach(id, token)
    client → server   damage(id, x, y, width, height)
    client → server   commit(id)
    client → server   destroy_surface(id)

    server → client   configure(id, width, height, serial)
    client → server   ack_configure(id, serial)

`attach` carries the token DeviceKit exported the surface under
([17](17-devicekit.md) §9), in four words, and no descriptor: size, format,
stride and modifier travel with the surface. The server imports it as a texture and does not
copy it. **`commit` is what makes anything take effect** — an attach and a damage
before it are staged, and the surface changes all at once or not at all. That is
the one piece of Wayland's model worth keeping verbatim, because a compositor
that can show half an update is a compositor that flickers.

`configure` is the server telling a client how big it should be, which for a
full-screen-one-app platform ([11](11-adaptive-desktop.md)) is the output's size and
happens once. The `ack` exists so the server knows which size a given buffer was
drawn for.

    server → client   insets(id, top, leading, bottom, trailing)

Where the platform draws over a surface that fills the screen under it: the
status bar along its top and the home indicator along its bottom, as on an
iPad. The client draws its background into those bands and keeps what is read
or touched out of them, as an application keeps to its safe area. The server
sends them when they change, and a surface told none has none. A surface placed
clear of the platform's bands, a window in `Freeform`, is told none.

    server → platform   bar_backdrop(luminance)

How bright the composed picture is under the platform's bar, in thousandths,
read along two rows of its band that the bar's own text does not reach. The
server measures it at most four times a second and sends it when it moved by
2% or more, and again after the platform sets its bar. The platform picks its
bar's ink from it, independently of the system's appearance (PlatformUI's
`StatusBarTone`).

    client → server   set_margin(id, margin)

A window draws its own shadow, since effects are drawn by whoever owns the
pixels. `set_margin` says how far round the window its buffer reaches to hold
one, the same on every side, as Wayland's window geometry does. `configure`
still gives the window's own size; the buffers that follow are `margin` larger
on each side, with the window at (`margin`, `margin`) in them. The server
places, stacks, hit-tests and keeps on the output the window alone, and draws
the buffer round it, so the shadow takes no pointer and a click on it reaches
what is under it. Pointer positions are relative to the buffer, so the
window's own top left is at (`margin`, `margin`); both are in points, and the
buffer is the scale times larger again. Only an application's surface
has a margin, of at most 128.

    client → server   set_glass(id, piece, x, y, width, height, corner, shape, interface, recipe)

The one effect a client cannot draw itself. Liquid Glass refracts and blurs
what is behind it, and what is behind a surface is other clients' pixels,
which only the server has. So a client that wants a piece of glass over what
is under its surface, as the platform does for its Dock and banners, asks the
server to draw it. The server draws the toolkit's glass of `recipe`
([10](10-toolkit.md)), at the size the toolkit resolves that recipe at and in
the appearance the system is in, at that place under the surface's own
pixels, refracting everything composed below. The client draws only what sits
on the glass and leaves the glass itself out.

The glass is in the surface's own coordinates, from its buffer's top left.
`x`, `y`, `width`, `height` and `corner` are in sixty-fourths of a point,
because a layout puts a glass's edge between points. `shape` is the corner:
0 for a circular arc of `corner`, 1 for a continuous corner of `corner`, and
2 for a capsule, whose corner is half its shorter side. `interface` is the
interface the glass belongs to, as the toolkit draws it there: 0 a touch
screen's, the iOS kit's glass, and 1 a desktop's, a Mac's glass, edged only
by the light inside its rim. `recipe` is the glass the piece is made of:
0 Regular, 1 Clear, 2 Dock, 3 Control Center, 4 Notification Center, and
the toolkit's other recipes from 5 in the order `DisplayGlassRecipe` lists.
On the wire `shape` is the wide word's low byte, `interface` the byte above
it and `recipe` the byte above that. A client keeps up to eight pieces,
numbered `piece` 0 to 7. A piece set again replaces the old
one, and a piece with no width or height is taken away. A piece takes effect
with the surface's next commit, as `set_margin` does, so glass moved under a
moving picture moves with it, and goes with the surface.


    platform → server set_stage_badge(window, slot)

The platform's committed buffer has a 64-point column after its visible
width, with sixteen 64-point badge tiles. Its buffer height is at least
1024 points. Each tile contains a 32-point icon inset 16 points for its
shadow. `set_stage_badge` (opcode 68) associates a tile with a window;
window zero clears the pending table. The table takes effect with the
platform's next commit. The compositor crops the visible platform surface
and samples each badge tile beside its window using the same presentation
geometry and animation progress. Entering and leaving the strip fades the
badge with that transition, including when it reverses midway. Platform
overlays draw above the windows and their badges.

### Frames

    client → server   frame(id, serial)
    server → client   frame_done(id, serial, time)

A client asks to be told when to draw and is told once, after the flip that
showed its last frame completed ([07](07-graphics.md)). No timers, no polling,
no client running faster than the display. A client that never asks never draws
again, which is exactly right for something idle.

### Input

    server → client   focus_in(surface_id)
    server → client   focus_out(surface_id)
    server → client   key(surface_id, time, keycode, state, modifiers, utf8)
    server → client   pointer_motion(surface_id, time, x, y)
    server → client   pointer_button(surface_id, time, button, state)
    server → client   pointer_axis(surface_id, time, axis, value)
    server → client   pointer_enter/leave(surface_id)

Events go **only** to the focused surface. A client cannot ask for input it is
not being given, cannot see another client's input, and cannot open an input
device — it has no path to one ([12](12-apps.md)).

    platform → server hold_input(held)

While the platform shows something over the applications, a menu dropped from
its bar or the switcher, it holds the input: keys and the pointer go to the
platform, and focus stays where it was, so the window in front stays active
and keeps its colour, as a Mac's does while a menu is out. Taking focus
instead would grey the window for as long as the menu is out.

    platform → server set_overlay(x, y, width, height)

What the platform shows over the applications without holding the input,
its notification banners, takes the pointer through `set_overlay`: the
pointer over that rectangle goes to the platform, as over its bar and Dock
bands, while focus and the keys stay with the window in front. A click on a
banner reaches the banner, not the window under it, and typing goes on where
it was. The rectangle is in points; an empty one gives the part back. The
platform claims it when a banner comes and gives it back when the last goes.

    platform → server set_idle(dim_after, blank_after)
    server → platform idle_changed(state)

The output dims and then blanks when no input has come for a while, as a
Mac's display does before it sleeps. The platform sets the two times, in
seconds without input, through `set_idle`; zero is never, and until the
platform sets them the output stays awake. Dimmed, the picture is composed at
half its brightness, darkened under a translucent black because a virtual
machine's output has no backlight to lower. Blanked, the output shows black
and nothing is composed, so an idle machine uses no GPU. Any key, button or
pointer motion wakes the output. While the output is blanked, the key or
button press that wakes it reaches no client, as the first touch on a
sleeping iPhone's screen only wakes it; while it is dimmed, input goes where
it would have gone. The server sends `idle_changed` to the platform each time
the state changes, `awake`, `dimmed` or `blanked`, and the platform closes its
lock screen when the output blanks, so the lock screen is what the output
wakes to ([11](11-adaptive-desktop.md)).

Focus, the mode, the bar and Dock bands, the overlay, grabs, placement,
the idle times, holding the input and the trusted path are the platform's to set
([11](11-adaptive-desktop.md)). An application that sends one of those messages, a focus grant for itself above all, is refused:
the server logs it, changes nothing, and the application's session goes on.

`key` carries both the keycode and the resolved character, because every client
would otherwise carry a keymap ([06](06-devices-and-input.md)) and they would
disagree. The keycode is the key's HID usage as DeviceKit reports it
([17](17-devicekit.md) §13): the id alone on the keyboard page (7), so `a` is 4
and `g` is 10, and the whole `(page << 16) | id` for a key on any other page.
No evdev number reaches a client.

### Windows

    client → server   request_window(id, action, edges)
    server → platform window_requested(id, action, edges)
    platform → server grant_move(id)
    platform → server grant_resize(id)
    platform → server place_surface(id, x, y, width, height)
    server → platform surface_placed(id, x, y, width, height)
    platform → server set_hidden(id, hidden)
    server → platform surface_pressed(id)

A window's chrome is drawn by its application, so a person acts on a window
through the application: it drags the title bar, or clicks one of the traffic
lights. The application asks with `request_window`, and the server forwards
the request to the platform, which decides ([11](11-adaptive-desktop.md)). The
actions are move (1), zoom (2), minimize (3) and resize (4). `edges` is one
word naming the edges a resize moves: top 1, leading 2, bottom 4, trailing 8,
two of them for a corner. Every other action names none.

A request is honoured only from the surface's own session and only while it has
focus; a move or a resize only while a button is down. The server holds the
pointer for it from then, and the press goes to no client.

A move or a resize starts once the platform grants it. From then until the
last button goes up the server follows the pointer itself, with no round trip
per motion. A moved window keeps at least 64 pixels on the output and stays
below the bar and above the Dock. A resized window is no smaller than 280 by
180. The application is configured at each size it passes through, one
`configure` at a time: the next is sent once the last was acknowledged, and
until then its buffers are drawn at their own size. When the button goes up the
platform is told where the window ended with `surface_placed`.

The edges of a freeform window are outside what its application draws, so a
press in the band along them, 6 pixels outside to 2 inside and 14 along from a
corner, is the server's: it asks the platform for the resize on the
application's behalf. Only windows in `Freeform` have edges. Over the band, and
while a resize lasts, the cursor is a double-headed arrow along the way the edge
moves, diagonal at a corner, as on a Mac.

`place_surface` is the platform placing a window, as a zoom does, and the
window is sent the frame as a `configure`. `set_hidden` takes a window off the
screen, as a minimize does: a hidden surface is not drawn, takes no pointer and
cannot have focus. `surface_pressed` tells the platform a button went down on a
window without focus, and the press goes nowhere, as a click on a Mac's window
behind brings it forward without acting on it.

### Errors

    server → client   error(object_id, code, message)

Then the connection closes. Fatal to that client, invisible to the machine.

## The frame loop

The whole server, in the order it happens:

    wait on: the socket(s), the input devices, the DRM fd, the timers
      a client message      → stage or commit
      an input event        → batch until EV_SYN, route to focus
      a flip completed      → send frame_done to whoever was shown
      a timer               → key repeat, cursor blink, animation
    if anything changed since the last frame
      compose or pick a direct scanout
      flip, asking for the event
    repeat

One `ppoll` over everything, which is why `signalfd4` and `timerfd` are on the
syscall list ([02](02-kernel-and-syscalls.md)). One flip in flight. **No frame
drawn when nothing changed** — an idle machine should use no GPU at all, and
that property is much easier to keep than to add.

## Composition

Covered in [07](07-graphics.md): a GPU pass drawing each client's buffer as a
quad, with direct scanout when one surface covers the output untransformed.

The server composites what it is given and nothing more. The system bar
([11](11-adaptive-desktop.md)) is an ordinary surface of the platform's, stacked
above the application it is drawn over and placed where the platform configured
it; the menus in it travel on the platform link, never through this server. With
a bar on screen no surface covers the output alone, so composition is the
common case and direct scanout the one for platforms without a bar.

The cursor is the server's, drawn on a DRM cursor plane if virtio-gpu offers one
and as a quad otherwise ([06](06-devices-and-input.md)).

## Buffer lifetime

The hard part of any compositor, and the source of every mysterious flicker.

- A client attaches a buffer and must not draw into it again until told it is
  free. The server releases it — implicitly, by `frame_done` for the frame after
  the one it was last used in.
- **Two buffers per client minimum**, three if it wants to draw ahead. The
  client owns that decision; the server only says when one is released.
- The server holds a reference for as long as it might scan out, which with
  direct scanout is until the *next* flip completes, not the current one. Getting
  that off by one is how a frame tears.
- A client that dies leaves buffers the server is still using. They are
  descriptors, so closing them is enough — but not before the flip that uses
  them retires.

## Restart

A display server that crashes should come back ([03](03-processes.md)). When it
does:

- Its DRM master dropped, the console took the mode, and the replacement takes
  it again. A visible flicker, and better than a black machine.
- Every client's connection died. **Clients must be able to reconnect**, or a
  server restart takes every application with it. That means a client's socket
  is not a one-shot: it retries, and re-creates its surfaces.
- Nothing about a client's *state* lives in the server. Surfaces and buffers are
  recreated from what the client already has.

Designing for this from the start costs one retry loop in the client library.
Retrofitting it means every application changes.

## What it is not

- Not a window manager. Policy — what is focused, what is where, what launches —
  is the platform's ([11](11-adaptive-desktop.md)), and the platform is a client with
  privileges the protocol grants it explicitly.
- Not a scene graph. It composites surfaces. Anything richer is the toolkit's
  job ([10](10-toolkit.md)).
- Not a place for effects. A blur or a shadow is drawn by whoever owns the
  pixels.

## Checkpoints

- [x] A client process connects over a socket and is told the output's size.
      The services boot logs `was configured` from
      Display Check, with `width: 1280` and `height: 800`.
- [x] A surface is configured before it can receive focus. MosaicDisplay sends
      x/y/width/height with configure serial 1, the client acknowledges that
      serial, and the multi-client boot requires both acknowledgements before
      its focus assertions can pass.
- [x] A client renders with Ember into an exported Vulkan dma-buf,
      attaches it, commits, and it is on the screen. `attach` carries the
      DeviceKit token the surface was exported under, never a descriptor, and
      MosaicDisplay imports it and commits it to the display through DeviceKit. Two independent shared
      textures now complete as frame serials 1 and 2 in one services boot.
- [x] Two clients are composited in a stacking order. MosaicDisplay draws the
      desktop, then every application by its z-order, then the platform's
      surface over them, and logs the order when it changes; the multi boot
      asserts `the stacking order changed` with `stack: "2 3 1"`, back to
      front: client 2, client 3 over it, and the platform (surface 1) on top.
- [x] A keypress reaches the focused client and no other. The multi-client boot
      puts focus on surface 2, injects key 4, observes it at client 2, and asserts
      that client 1 never receives that key.
- [x] A client draws only when it asks for a frame and is told, and an idle
      machine issues no flips. MosaicDisplay sleeps on one LibTessera port over
      the UI sockets and the keyboard; the services test asserts exactly two requested flips
      and then routes input without granting or flipping a third frame.
- [x] Killing the display server brings it back with clients reconnecting. The
      `-services-restart` boot asks PID 1 to SIGKILL MosaicDisplay, asserts a
      second server process starts, then proves the surviving UI accepts its
      replacement link and reaches direct scanout again with frame 1.
- [x] A malformed client is disconnected and the machine carries on. The services
      test sends a deliberately short fixed-protocol packet after two successful
      frames and a real key event; MosaicDisplay drops only that client session,
      retains the current scanout, and QEMU remains alive.
