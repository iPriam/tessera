# 22 — Power

Control the display, understand the battery, and close and reopen a portable
machine without losing work. This extends [11](11-adaptive-desktop.md)'s idle
display behaviour and [17](17-devicekit.md)'s Power interfaces.

## Starting point

The Power service reports power sources and thermal state and manages wake
requests. Settings has a battery pane and a display-sleep timeout. MosaicDisplay
dims and blanks an idle display, then wakes it on input. Full system suspend,
real brightness control and the policies below have separate checkpoints.

## 1. Brightness and battery state

On displays with controllable backlights, keyboard brightness keys, Settings
and Control Center change the real brightness and show the same value. A
display without brightness control is identified as such. Brightness is
remembered per display and restored after wake.

Battery state is shared by Settings, Control Center and the status area:
charge, charging, full, discharging or no battery, with external power indicated.
Connecting a charger updates the interface without reopening its panes. An
unknown reading is shown as unknown rather than as a guessed percentage.

Ends when brightness controls change the panel and connecting a charger updates
the battery UI while the person uses it.

- [ ] On a supported panel, adjust brightness from each control and observe the
      backlight change. The controls agree and the chosen value returns after wake.
- [ ] Connect and disconnect external power while Settings and Control Center
      are open; they and the status area report the same charge and power state.
- [ ] A desktop without a battery and a display without brightness control show
      appropriate unavailable states. Reconnecting a display restores its preference.

## 2. System sleep and wake

Sleep from the system menu suspends the machine after applications and devices
have been notified. A portable's lid follows its configured sleep policy. Wake
returns to the session with its windows and unsaved edits intact, behind the
platform's existing lock screen. That screen's authentication policy remains
defined in [13](13-security-and-updates.md).

Display sleep and system sleep have separate settings for battery and external
power. An application playing media or doing work may request an appropriate
temporary wake lock. Settings shows what prevents sleep and for how long;
ending the application releases its lock. An automatic sleep may be deferred
by policy, while a person's explicit Sleep request is handled visibly.

Ends when a person can close a supported laptop, reopen it and continue an
unsaved document without a reboot.

- [ ] Sleep and wake through a supported wake source, then continue typing in
      the same unsaved Notes document with its windows and focus restored.
- [ ] Close and reopen a supported laptop's lid. The configured policy runs,
      the session returns, and the wake input does not type into the document.
- [ ] Repeated sleep/wake cycles restore display, keyboard, pointer, audio and
      network service. No application needs to be relaunched to recover them.
- [ ] A temporary wake lock appears in Settings, affects the documented idle
      policy and disappears on expiry or process exit. Explicit Sleep reports
      any failure instead of leaving an unexplained frozen interface.

## 3. Battery and power-saving policies

Low Power Mode is a persistent preference, selectable automatically on battery
or manually. Its description names the policies it changes, such as brightness,
idle timeouts and background activity. Returning to normal restores the person's
preferences, including changes made while Low Power Mode was active.

A low-battery notification appears at the configured threshold and clears when
the condition ends. At critical charge, Tessera warns, asks applications to save
and follows a defined sleep or shutdown policy supported by the hardware. A
misbehaving application cannot defer that action indefinitely.

- [ ] Enable Low Power Mode, switch between battery and external power and
      reboot. Its configured policy and all visible indicators agree.
- [ ] Change a preference while Low Power Mode is active, then turn it off;
      the person's new preference is retained.
- [ ] Drive a controlled battery source through low and critical states: one
      low-battery warning appears, critical action follows the documented
      deadline, and saved application data survives the subsequent boot or wake.

## Evidence

Virtual power sources can prove state changes and policy decisions. Panel
brightness, lid behaviour, battery readings and suspend/resume require the
supported hardware. Each checkpoint records which device and wake source were
used; a blank VM window alone proves neither suspend nor resume.
