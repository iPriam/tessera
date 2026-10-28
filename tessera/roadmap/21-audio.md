# 21 — Audio

Hear applications together, choose where sound plays, and control what records
the microphone. This builds on the Devices service and DeviceKit in
[17](17-devicekit.md).

## Starting point

The Devices service opens audio streams, discovers endpoints and reports device
changes. The desktop plays a startup chime. One output stream currently owns
its PCM device, so a second stream on the same output fails busy. Control
Center has volume controls in its UI; this chapter requires them to control
the sound that reaches the output.

## 1. Playback and output controls

Applications can play concurrently. System volume and mute affect their combined
output. Keyboard volume and mute keys, Settings and Control Center agree on
the current value. A system sound cannot reset the chosen volume.

Settings lists outputs with meaningful names and lets the person select one.
Connecting an output adds it immediately. Removing the selected output chooses
an available fallback and updates the UI. The selected output and its volume
are remembered when the device returns; a missing device leaves usable controls
for the outputs that remain.

Ends when two applications play together and the person controls their real
output from Control Center.

- [ ] Play two distinguishable sources at once on one output. Both continue
      when a third system sound plays; ending one does not stop the other.
- [ ] Change volume and mute through Control Center and keyboard keys. The
      audible output and every volume control follow the change, including zero.
- [ ] Select another supported output while playing and hear the continuing
      stream there. Unplug it and hear the documented fallback without restarting apps.
- [ ] Reconnect an output and reboot. Its saved volume and output preference
      return, while a missing output does not prevent playback elsewhere.

## 2. Microphones and recording

Settings lists inputs, chooses the default, adjusts input level and shows a
live level meter. Muting input stops captured sound and clearly indicates the
muted state. An application identifies itself when requesting microphone
access; that decision is remembered and can be revoked in Settings.

Recording shows an active microphone indicator with the recording application's
name. Stopping capture, quitting the application or revoking its access removes
the indicator and ends its stream. Another application's capture is unaffected.
These actions require the authorization boundary in [16](16-libtessera.md) §14.

Ends when a person can make a recording and see and control who is recording.

- [ ] Allow a recording application, select an input and record a short clip
      that can be played back. Denying the request delivers no microphone samples.
- [ ] Change input gain and mute while recording; the recorded samples and live
      meter reflect both operations.
- [ ] The active indicator names every capturing application. Revoking one
      application's access stops its capture immediately; quitting leaves no stale indicator.
- [ ] Removing an input produces a visible interrupted state and a choice of
      remaining inputs, without silently recording from a different microphone.

## 3. Application and media controls

Sound settings lists applications currently producing audio, with independent
volume and mute. One application's setting does not alter another's. Notification
sounds can be adjusted separately from media playback.

Now Playing shows the active media application's title and playback state.
Play, pause and supported seek actions in Control Center and media keys reach
that application. Closing it clears its controls. Bluetooth audio is a later
transport integration; it uses the same device selection and playback controls.

- [ ] Mute one of two playing applications and continue hearing the other.
      Unmute restores that source at its chosen level.
- [ ] Disable notification sounds while music continues at its previous level.
- [ ] Control Center and media keys pause and resume the active player; its
      title and playback state update, and quitting removes the session.

## Evidence

A loopback device can prove mixing, gain and mute in the resulting samples.
Audible playback, device routing and microphone capture also need checks on the
supported audio hardware. A slider animation alone closes no audio checkpoint.
