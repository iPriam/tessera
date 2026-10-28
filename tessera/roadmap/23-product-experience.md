# 23 — Product experience

The desktop slice proved that Tessera can run an application. The next steps
make it a device a person can organise, add software to and work on each day.
This is the product scope; the linked chapters hold more detailed checkpoints.
Every checkpoint here is planned work, not a claim that the feature exists.
The detailed chapters describe ownership, data and permissions, failure handling,
implementation order and acceptance. Proposed services and contracts in those
chapters must be designed and implemented; their names do not imply an existing API.

## 1. Make Home yours

Home is a remembered arrangement of pages, folders, favourites and widgets.
Installing an application adds it to the available library; the person decides
where it belongs. The Dock and system search remain quick ways back to it.
Wallpaper and appearance give the device a consistent identity at Home, on the
lock screen and behind windows. [24](24-home-and-personalisation.md) defines
the touch, pointer and keyboard workflows.

- [ ] Arrange applications across two pages and a folder, edit the Dock,
      reboot, and find every item where it was left.
- [ ] Remove an application from Home and find it in App Library; uninstalling
      it removes its placements without disturbing the rest of the layout.

## 2. Find and manage apps

The App Store is an application with browse, search, product pages, downloads
and updates. An app page explains what it does, who publishes it, which
platforms it supports and what access it requests. Installed apps can also be
managed in Settings. [25](25-app-store.md) defines this lifecycle.

- [ ] Find an app, inspect its page, install it and open it from Home without
      using a host command or plugging in a disk image.
- [ ] Update it without losing its data, then remove it through the ordinary
      app-management interface with an explicit choice about keeping its data.

## 3. Move between tasks

The existing switcher, windows and Stage Manager become a complete task flow:
search for an app or document, choose the right window, arrange work side by
side, and return to that arrangement later. Adaptive Desktop adds workspaces;
Adaptive Mobile adds a deliberate side-by-side workflow where the screen has
room. Console and Media need a usable home and controller navigation back to it.
Each platform keeps its own arrangement while sharing application identity.
System search is specified in [26](26-system-search.md), desktop/workspace/output
policy in [28](28-desktop-and-displays.md), and the complete controller/remote
experience in [38](38-console-and-media.md).

- [ ] System search opens a named app, a permitted document or a Settings page,
      and handles an empty result without stealing subsequent input.
- [ ] Choose a particular window of an app from the switcher, snap two windows
      side by side, and return to both with keyboard or pointer navigation.
- [ ] Move a window between desktop workspaces and switch workspaces without
      losing that window's state or receiving input in the hidden workspace.
- [ ] On a supported tablet layout, put two applications side by side, adjust
      their divider and return one to fullscreen with its content intact.
- [ ] Restore supported apps, window placement and the selected workspace after
      a normal restart. A missing app or display leaves the session usable.
- [ ] On Console and Media, launch from Home and return to it with a controller;
      neither flow needs a pointer or an already open Settings window.

## 4. Catch up and act

Notification banners already exist. Add a notification centre with history,
grouping, clear actions and links back to the relevant application context.
Settings controls each app's banners, sounds and badges. Focus gains schedules
and selected exceptions, applied consistently across platforms.

- [ ] Miss a banner, open notification history and activate it to reach the
      relevant app context. Clear one item or a group and see matching badges.
- [ ] Disable an app's banners or sounds in Settings; subsequent notifications
      follow the choice after reboot, with history handled as configured.
- [ ] A Focus schedule starts and ends at the chosen local times, permits its
      configured exceptions and can be overridden from Control Center.

Status (2026-10-26): Focus is kept in Settings (`focus.schedule`,
`focus.manual`, `focus.allowed`); PlatformKit's `FocusChoice` decides it from
the schedule, Control Center's choice, which holds until the schedule next
starts or ends, and the allowed applications, whose banners still show
(`tests/focus_kik`, 6 passed). Settings has a Focus pane, and Adaptive Mobile
follows it; Adaptive Desktop's banner call is the one change left to land.
Every notification goes into a history (`NotificationHistory`, kept in
`/data/system/Notifications.txt`), which Adaptive Mobile's lock screen lists
by application with Clear for one or a group; choosing one opens its
application, and Home's icons carry the count as a badge. Settings'
Notifications pane turns an application's banners off (`notifications.quiet`)
while its notifications still reach the history.

## 5. Move work between apps

Copy and paste, drag and drop and the Share sheet are system interactions with
consistent destinations and cancellation. Text, images and documents carry
their type; an application receives only the item the person transferred.
Screenshots and recordings can be previewed, saved, copied or shared through
the same flow. File destinations use [19](19-files-and-documents.md).

- [ ] Copy text in one app and paste it in another. An unsupported format has
      a clear unavailable state; background apps cannot silently monitor the clipboard.
- [ ] Drag a document between Inlay and an accepting app using a pointer,
      and send the same document through Share using touch or keyboard navigation.
- [ ] Capture a display, window or region, then preview, copy, save or discard
      it. Cancelling the flow leaves no unintended saved capture.
- [ ] Start and stop screen recording with a visible recording indicator;
      preview the playable result and save or discard it. An application's
      capture request requires an explicit selection and can be revoked.

### 5.1 Pasteboard decision

The pasteboard is a LibTessera service, `tessera.pasteboard`, run as
`/bin/pasteboard`, that holds one text item for the system. The client calls
are `pasteboardWrite` and `pasteboardRead` (LibTessera
`app/Pasteboard/Pasteboard.kira`), and the wire is a header and a payload, as
the Power and Devices services use.

- A write replaces the item. Any program may write, because copying is the
  person's action. A write reveals nothing about the item it replaces.
- A read returns the item and needs an interaction token: proof that a real
  input event reached a front-facing surface ([16](16-libtessera.md) §14.1).
  A read without a token is refused `NoInteraction`; a token is checked by
  the Presence service, which asks MosaicDisplay for its verdict.
- Nothing reads the item silently. A background program cannot watch it,
  because the service only answers reads, and each read needs a token.
- The item is UTF-8 text of at most 65536 bytes. Other formats are later work.
  A program asking for a format the item does not hold gets an unavailable state.
- Calculator copies its answer and pastes with the token of the key press.
  Kira UI's text fields answer Command-A, -C, -X and -V themselves; Tessera
  UI gives them the service (`usePasteboardService`), so text copied in one
  application pastes in another.

Status: the service, Presence and the clients are written. Its boot check is
in `tessera/build/app/BootServices.kira`. Copying between applications is not
yet shown on a boot.

## 6. Set up and personalise the device

First-run setup chooses language, keyboard, time zone, appearance and basic
connectivity, with accessibility controls available from the start. It can
finish offline without an online account. Settings remains the place to change
these choices, manage installed apps, inspect storage and configure updates.

The current lock screen opens without authentication. Authenticated unlock is
a new feature: setup offers a device credential, Settings can change it, and
lock behaviour follows [13](13-security-and-updates.md)'s trusted input boundary.
Recovery and forgotten-credential behaviour must be specified with it.
The detailed lock-screen plan is [27](27-lock-screen.md). Language and input
are in [33](33-language-and-input.md), and the cross-system accessibility
contract and assistive workflows are in [34](34-accessibility.md).

- [ ] Complete first-run setup offline with a chosen language, keyboard and
      time zone; the desktop and Settings agree after reboot.
- [ ] Complete setup and the Home → Store → app flow using only a keyboard.
      Text sizing, reduced motion and contrast preferences remain usable throughout.
- [ ] Configure a device credential, lock and unlock successfully, reject a
      wrong credential, and exercise the documented recovery flow without
      exposing the credential in logs or application input.
- [ ] Inspect app and document storage usage, remove an app, and see the freed
      space and any deliberately retained data reflected in Settings.

Status (2026-10-26): Welcome is the first-run setup: language, keyboard
(with a field to try it), time zone and appearance, each written to Settings
as chosen, then `setup.completed`. PID 1 opens it on a desktop boot until that
is set. MosaicDisplay types through the layout Settings names (`us`, `gb`,
`de`, `fr`) and follows a change from the next key; Settings has Language &
Keyboard. Settings' Storage and Apps panes show space and delete an app with
or without its documents. A device credential is not started. Not yet shown
on a boot.

## 7. Useful applications

[29 Everyday applications](29-system-apps.md) describes Photos, Preview,
Calculator, Calendar, Reminders, Weather, Contacts and a recorder. Each is an
ordinary confined app completing a real task, with shared format, scheduling,
search, media and document behaviour in the appropriate libraries or services.
The chapter specifies their initial scope, data handling and delivery order.

## 8. Space, backups and recovery

[30 Storage management](30-storage-management.md) covers trustworthy usage
breakdowns, large-file and cache cleanup, external-drive formatting and safe
removal. [31 Backup and recovery](31-backup-and-recovery.md) covers independently
stored automatic backups, per-document restore, migration to another machine
and a usable Recovery environment. Their acceptance requires recovering real
data; a successful backup write or a displayed erase button is insufficient.

## 9. Privacy and application behaviour

[32 Privacy and permissions](32-privacy-dashboard.md) covers app decisions,
active capture indicators, recent microphone/camera/location access and revocation.
[36 App lifecycle](36-app-lifecycle.md) gives one identity and registry model
to defaults, document/link opening, background tasks, startup items, Force Quit,
crash recovery and updates. These features must agree across the Store, Settings,
Home and resource-owning services.

## 10. Devices and continuity

[35 Device connections](35-device-connections.md) covers Bluetooth pairing,
headphones, keyboards, mice, controllers, printers, cameras and nearby transfer.
[37 Continuity](37-continuity.md) adds authenticated peer pairing, opt-in
clipboard transfer, document handoff, notification forwarding and selected
settings sync. Local use remains independent of an online account or peer.
Console and Media have their own complete distance/input flows in
[38](38-console-and-media.md), including recovery from a disconnected controller.

## Delivery order and dependencies

Start with Home's pages, folders and library. Add the App Store's complete
find → install → open → update flow once wired networking, stable app identity
and app-update support are available. Then expand task switching and notification
history, followed by sharing and capture. Console and Media reuse these services
with their own complete navigation flows rather than forking the OS below them.

Build the supporting app registry and activation flow early ([36](36-app-lifecycle.md)).
Design keyboard/composition and accessibility contracts while adding controls
([33](33-language-and-input.md), [34](34-accessibility.md)); retrofitting them
after every app has a different editing or focus model multiplies the work.
Provide independent backup and tested restore before treating the machine as
the only home for important work ([31](31-backup-and-recovery.md)).

System search can begin with app/Settings providers before content indexing.
Calculator and recording can exercise the utility-app model before Photos and
calendar integrations. Privacy decisions precede the corresponding microphone,
camera, location and capture features. Continuity follows reliable local
document, notification and preference models; cloud transport is a separate
decision. No ordering here marks unimplemented checkpoints as complete.

The existing [Files](19-files-and-documents.md), [Networking](20-networking.md),
[Audio](21-audio.md) and [Power](22-power.md) chapters remain part of this plan.
Their milestones supply document handling, downloads, sound and portable-device
behaviour. Foundation gaps stay in their own chapters and are prerequisites
only for the user flow that needs them.
