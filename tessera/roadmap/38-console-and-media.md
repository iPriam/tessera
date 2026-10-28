# 38 — Console and Media

Console and Media are complete Dynamic Platforms, not fullscreen desktop views
waiting for a mouse. A person sitting away from the screen must be able to
start the device, discover and install software, launch an app, return Home,
change settings and recover from a disconnected controller or failed app using
only the supported controller or remote.

They share the OS, application identities, store, notifications and device
services with Adaptive Desktop and Adaptive Mobile. Their presentation and
navigation policies are their own, as established in
[15](15-form-factors.md). Do not create a separate installation or permission
system to make a television layout easier to draw.

## Two homes with a shared library

Console Home prioritises the person's installed apps/games, recently used
sessions, pinned favourites and current downloads. Media Home prioritises
playback applications, available local media and explicitly supplied recent
items or continuations. An app supplies a continuation through a defined
contract; the platform does not inspect its private library or invent viewing
history from its window title.

Both have an all-apps library, search, Store, downloads and Settings. Their
navigation, focus and Back hierarchy are consistent. Home always has a reserved
system action that returns from an application, and a visible way to go back
to the previous eligible task. A platform with no installed apps still offers
setup, Store and Settings instead of an empty screen.

Home content is laid out for viewing distance, safe display bounds, readable
type and a small set of clear actions. A focused item has a durable visible
indicator; motion and colour are supplementary signals. No necessary control
appears only on hover. Additional content loads without moving the current
selection to an unrelated item.

## Directional navigation and text

PlatformUI supplies directional focus groups, carousels, grids, lists, dialogs
and a distance-appropriate on-screen keyboard. PlatformKit tracks semantic
selection and Back/Home destinations. Focus restoration uses stable item
identity after a modal, a refresh or a visit to another surface.

Define controller/remote actions for confirm, back, home, menu, play/pause and
text entry, including remapping where supported. The display server routes
input by focus and trusted system reservations. Applications cannot intercept
the reserved Home action, and held keys/buttons are cleared on disconnection.
Text entry uses [33](33-language-and-input.md)'s composition protocol, with
password privacy, candidate selection and an optional paired hardware keyboard.

A disconnected primary controller or remote opens a system recovery flow
reachable from a reconnecting input device. Pairing during that flow cannot
require a pointer to an inaccessible desktop pane. A second controller can
navigate when policy permits it; player assignment and app input streams remain
explicit so background apps do not receive gameplay input.

## Store, downloads and app lifecycle

The App Store uses the same catalogue and install/update services as elsewhere,
with navigation and product pages adapted to distance and directional input.
Platform compatibility is visible before installation. Unsupported apps explain
which platform is needed; the system does not launch an unusable pointer-only
app into a dead end without a supported switch/return path.

A downloads view shows queued, active, paused, failed and ready items. It
supports prioritisation, cancellation and retry, and gives enough storage
information to recover from a full disk. Leaving Store or Home does not end
its service-owned transfers. Ready items become launchable through the ordinary
app registry, with no duplicate console-specific app database.

Switching apps, normal quit, Force Quit and crash reopening use
[36](36-app-lifecycle.md). A crashed fullscreen app reveals a usable system
surface and a controller-operable Reopen/Close choice. Suspending a game or
media app is promised only where the app and lifecycle contract actually
support it; retaining a screenshot is not a resumable session.

## Playback and system controls

Media transport keys and the platform overlay target the active media session
from [21](21-audio.md). They show real title, playback position and supported
actions. Captions, audio tracks, output selection and volume are accessible
without leaving playback when the app supplies their session contracts.
Unsupported controls are absent or disabled with an explanation.

Settings covers network selection, controller/remote pairing, audio routing,
display mode/scale, safe-area calibration, accessibility, storage and updates.
The available choices come from the same underlying services. Changes with a
risk of losing the picture, such as a display mode, have a timed confirmation
and return to the last known usable mode if it cannot be confirmed.

Lock, wake and notification policies use shared state with their own
distance-appropriate presentation. Private previews remain filtered. Focus
or a fullscreen playback preference determines interruption behaviour; urgent
system conditions have a defined visible path. Any shutdown or restart request
accounts for downloads and unsaved app work under the normal lifecycle policy.

## Implementation order

1. Build directional focus, Back/Home navigation and a real all-apps Home
   against the existing platform/app registry. Complete Settings return paths.
2. Add on-screen text entry, input-disconnection recovery and controller pairing,
   then the complete Store/install/open flow and a shared downloads view.
3. Integrate app switching, crash recovery and supported continuations, followed
   by media-session overlays, caption/audio controls and display calibration.
4. Complete cold boot, lock/wake, offline, full-disk and accessibility flows
   entirely with the supported remote/controller on the intended display.

Ends when a new device can be configured and used from the couch, including
installing and recovering an app, with no pointer or host intervention.

## Checkpoints

- [ ] Cold boot Console and Media independently, navigate Home and Settings,
      search the app library and return from an application using only directional input.
- [ ] Enter a network credential and a Store search through the on-screen
      keyboard, including composition and cancellation, without hidden focus.
- [ ] Find, install, launch and update a compatible app; leave the Store during
      download and manage its progress from Home with state matching Settings.
- [ ] Crash a fullscreen app and Force Quit a hung one. The system remains
      navigable and a chosen reopen follows the documented restoration policy.
- [ ] Disconnect the primary controller/remote and reconnect or pair a supported
      replacement, clearing held inputs and restoring a usable focus position.
- [ ] Control a real media session, select captions/audio where supported and
      change output volume. The overlay follows playback state rather than local mock state.
- [ ] Change display mode, decline or miss confirmation, and return to the last
      usable picture. Safe-area and readability preferences persist after reboot.
- [ ] Complete setup, lock/wake, offline retry and low-storage recovery on a
      television at its intended viewing distance with the supported input device.
