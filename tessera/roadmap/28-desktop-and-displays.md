# 28 — Desktop, workspaces and displays

A desktop should remember where work belongs. Documents can be kept on it,
windows can be arranged into workspaces, and attaching another display should
give more room without making windows or input disappear. Removing that display
must leave all work reachable on what remains.

This extends the existing freeform windows and Stage Manager. The single-output
compositor path is a starting point, not proof of multi-display support.

## Desktop items

Desktop files are ordinary documents in a designated writable location, shown
by the desktop surface and managed through the same operations as Inlay
([19](19-files-and-documents.md)). Renaming, moving to Trash, opening, copying
and drag and drop use the same file identities and collision rules. The desktop
does not keep a second independent copy of a document's name or contents.

Icon positions and sorting are preferences over that location. A missing
thumbnail has a usable type icon; a failed preview cannot stop selection or
opening. Stage Manager may hide the desktop items while working with a stage,
but bringing the desktop forward reveals the same items and selection model.

## Workspaces and window arrangements

A workspace is a named set of window placements and a wallpaper choice. The
person creates, reorders, switches and removes workspaces, moves windows
between them and chooses whether an app appears on one or all. Removing a
workspace relocates its windows to a surviving one; it does not close them.

Snapping offers a small set of understandable arrangements, including halves
and quarters where there is room. A preview precedes committing a drag, and a
keyboard command offers the same action. Unsnap restores the previous freeform
frame. Minimum sizes are respected, and a layout that cannot fit is unavailable
or offers an explicit alternative rather than placing controls offscreen.

The switcher and system search reveal the workspace of a window and can bring
it forward. Only the visible focused window receives input. Switching workspaces
does not launch duplicate application processes or lose unsaved state.

## Multiple displays

Settings lists connected displays, identifies them on screen and lets the
person arrange their relative positions, choose supported modes and scale,
and select the primary display. Extended desktop comes first. Mirroring is a
separate supported mode with a shared logical output and an explicit treatment
of different aspect ratios and scales.

Pointer movement crosses adjacent display edges in the configured arrangement.
Windows move across them without changing document identity or losing their
content. A window's rendering scale follows its selected output with a fresh
configure and correctly sized buffers; moving between scales cannot reuse
incompatible buffers or change the logical size unexpectedly.

Display removal moves stranded windows to a remaining output and keeps dialogs
and menus with their owner. Reattaching a known display restores its layout
where possible. Preferences use a persistent hardware identity with a fallback
for indistinguishable displays; connector enumeration order alone is insufficient.
Unknown devices get a safe layout with all controls reachable.

## Ownership and rendering

MosaicDisplay owns output discovery, scanout, per-output frame scheduling and
input coordinates. Its protocol must carry stable session output identifiers,
geometry, scale and output membership. Each output tracks its own in-flight
presentation; a slow or removed output cannot indefinitely block another one.
Window geometry uses one defined logical coordinate system and explicit
transforms into output pixels. Shared edges and negative desktop coordinates
must be covered by the same transforms used for hit testing.

PlatformKit owns workspace membership, placement policy and restoration.
Adaptive Desktop owns its workspace and display-arrangement presentation.
PlatformUI supplies overview and snapping controls. Desktop.app draws the
per-workspace wallpaper and desktop items. Settings persists display choices,
and the compositor validates that requested modes are actually supported.
All new output and workspace contracts cross component boundaries through RIDL.

Saved state separates stable display identity, workspace identity, app/window
restoration identifiers and last useful frames. App relaunch uses restoration
tokens from [36](36-app-lifecycle.md), not stale process or surface numbers.
Switching to another Dynamic Platform preserves desktop preferences without
forcing that platform to use desktop workspaces.

## Implementation order

1. Add desktop-file presentation using Inlay's operations and persistent icon
   arrangement; implement snapping against the existing single-output path.
2. Introduce workspace identity, membership, overview and per-workspace wallpaper,
   with focus and restoration tests before adding another output.
3. Extend display discovery and the surface protocol for several outputs;
   build extended desktop, scale transitions and safe hot-unplug handling.
4. Add saved monitor arrangements, mirroring and full session restoration,
   including combinations with Stage Manager and platform switching.

Ends when a person arranges work across two displays and several workspaces,
disconnects a display, and can continue every document without relaunching it.

## Status

Adaptive Desktop's desktop shows the entries of `/Documents/Desktop` down the
trailing edge, as a Mac's: a click chooses one, a second opens a folder in
Inlay (through `openApplicationAt`), Return renames, Command-Delete moves to
the Trash and Command-Shift-N makes a folder. Inlay lists the same folder as
Desktop. Workspaces, snapping and displays are not started here. Not yet
shown on a boot.

## Checkpoints

- [ ] Create, rename and Trash a desktop document; Inlay immediately shows
      the same state and reboot preserves the document and icon placement.
- [ ] Create and reorder workspaces, give each a wallpaper, move a window
      between them and remove one workspace without closing its windows.
- [ ] Snap and unsnap through pointer and keyboard. Restore the original frame
      and reject layouts smaller than the app's declared minimum size.
- [ ] Arrange two outputs at different scales. Pointer hit testing, window
      dimensions, menus and capture regions agree on their positions.
- [ ] Remove the output containing the focused app and a modal dialog. Both
      remain reachable and input goes to the intended window on the surviving output.
- [ ] Reconnect displays in a different enumeration order and reboot; saved
      arrangements and workspace wallpapers return to the intended devices.
- [ ] Mirror outputs of different sizes and return to extended mode without
      losing window state. Verify real simultaneous output on supported hardware.
