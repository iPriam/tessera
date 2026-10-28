# 27 — Lock screen

The lock screen is the device's resting face: a readable clock, the person's
wallpaper, useful ongoing information and a clear way back into the session.
It also defines which information and actions remain available while the
session is locked. Those choices must be deliberate rather than whatever an
application happened to put in a banner.

The current platform screen opens on input without authentication. Adding a
credential is a separate security milestone, coordinated with
[13](13-security-and-updates.md). A dimmed display, a drawn padlock and a secure
locked session are distinct states and must not be confused in the UI or tests.

## Appearance and customisation

The person chooses a wallpaper, crop, clock treatment and a limited set of
information slots. Editing previews the result before applying it, with Home
and lock wallpapers independently selectable as described in
[24](24-home-and-personalisation.md). Clock, date and chosen information remain
legible across wallpapers, display sizes and larger text settings.

Customisation belongs in Settings and a deliberate lock-screen editing flow
entered after authentication when credentials are enabled. Cancel preserves
the previous configuration. Platform-specific layouts use the same saved
semantic choices while adapting placement for a phone, desktop or television;
coordinates from one display are not treated as meaningful on another.

## Information and actions while locked

Notifications have a configurable preview policy: show contents, show only the
app and notification count, or hide them while locked. Revealing a sensitive
notification or opening its app follows authentication. Locking an already
open session immediately replaces existing private previews with their locked
form, including in animations and captured thumbnails.

Media controls expose the active session's allowed play, pause, seek and volume
actions. Artwork and titles follow the person's privacy choice. A timer shows
its remaining duration and permits the selected stop or snooze action; an
alarm shows its ringing state. These are narrow actions on existing sessions,
not a general way to invoke app commands while locked.

Clock's alarms and timers must be maintained by system scheduling rather than
depending on its window being open. Distinguish elapsed timers from wall-clock
alarms. Time-zone and clock changes have defined effects: a ten-minute timer
still measures ten elapsed minutes, while a repeating morning alarm follows
its configured local-time rule. [29](29-system-apps.md) covers the app itself.

## Ownership and trusted input

PlatformKit owns the lock state machine; each Dynamic Platform presents it
through PlatformUI. Wallpaper configuration comes from Settings. Notification,
media and alarm services expose restricted views and actions for the locked
state. Applications never provide arbitrary widgets capable of receiving raw
lock-screen input.

Authenticated unlock requires a credential service and the trusted prompt/input
boundary described in [13](13-security-and-updates.md). That service owns
verification, attempt throttling and credential changes. Platforms request a
result and never persist plaintext credentials. Credential storage, recovery
material and forgotten-credential behaviour are designed together before the
feature is called secure. This adds a device credential to a single-user OS;
online accounts and multiple local users are separate decisions.

Locking revokes normal app focus and prevents clipboard, search and private
preview surfaces from remaining visible above the lock. Display wake input
is consumed by the system. Suspend, platform replacement, display removal and
service failure preserve the locked state. A failed credential service leaves
a recoverable locked screen rather than returning to the desktop.

The trusted input boundary and absence of a hardware root of trust remain
documented limits. A device credential does not by itself promise encryption
of stored documents or protection against a replaced kernel.

## Implementation order

1. Separate unlocked, locked, display-asleep and authenticating states, and
   make their transitions explicit across platforms and wake paths.
2. Add persistent wallpaper/clock configuration and a privacy-filtered
   notification view; verify locking never leaves private content visible.
3. Integrate restricted media, timer and alarm actions with their real service
   state, including events arriving while the display is asleep.
4. Add credentials, attempt policy, trusted input and a documented recovery
   flow. Test interruption and service failure before enabling secure locking.

Ends when a person can customise the resting screen, use allowed media and
alarm controls, and unlock an authenticated session without exposing its apps.

## Status

A device passcode can be set, changed and turned off in Settings' Passcode
pane, each change after the current one is given. Only a salted
PBKDF2-HMAC-SHA256 of it is kept (`/data/system/Passcode`, PlatformKit's
`PasscodeRecord`, checked against the published vectors). PlatformUI's lock
screen asks for it, with a keypad on touch, and after five wrong passcodes
waits a minute, doubling to an hour. Forgetting it means erasing the data
volume from Recovery. The platforms do not yet route their unlock through it;
that change is next. Nothing here is shown on a boot.

## Checkpoints

- [ ] Change clock appearance and lock wallpaper, cancel a second edit and
      reboot. The applied configuration and privacy preferences return.
- [ ] Lock with a private notification already visible. Its content disappears
      immediately; activating it requires unlock before revealing app content.
- [ ] Pause and resume a real player and stop a ringing timer from the locked
      screen without opening the underlying app or exposing unrelated commands.
- [ ] Close Clock, suspend and wake through an alarm-capable supported source.
      The scheduled alarm fires according to its documented time-zone rule.
- [ ] Reject a wrong credential with throttling, accept the right one, change
      it and exercise the documented forgotten-credential recovery flow.
- [ ] Restart a platform and interrupt credential verification while locked.
      Application input and private previews remain inaccessible.
- [ ] Wake input does not type into an app. Keyboard-only, controller and
      assistive navigation can reach every permitted action and unlock control.
