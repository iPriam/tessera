# 35 — Device connections

Connecting headphones, a keyboard, a controller, a camera or a printer should
have a visible result and a clear way to recover when it fails. Settings owns
remembered connections and detailed configuration. Control Center provides
short actions. Applications use the resulting system capabilities rather than
each managing their own pairing and device permissions.

DeviceKit and the Devices service already provide parts of discovery and
streaming. Bluetooth, printing, camera import and nearby transfer need their
own complete workflows and supported-device statements. A discovered USB entry
alone does not prove the device's useful function.

## Bluetooth

Settings shows available devices, paired devices and connection state. Pairing
identifies the device and applies the appropriate confirmation or code-entry
flow. The person can connect, disconnect, forget and reconnect. Remembered
devices survive reboot; turning Bluetooth off has a persistent, visible effect.
Pairing mode has a bounded lifetime and does not make the device discoverable
indefinitely without the person's choice.

A Bluetooth service owns adapter state, discovery, pairing material and profile
sessions. It reaches kernel/device operations through the system's backend
boundary and exports typed RIDL operations. Implement the required protocol
and profile support in owned Kira components; integrating a Linux daemon or
native C library is not the assumed shortcut under Tessera's SDK rules.
Choose a first adapter and a specific set of profiles before claiming broad
compatibility.

Input profiles publish keyboards, mice and controllers through DeviceKit's
normal registry and input pipeline. Audio profiles publish endpoints to the
same Audio system ([21](21-audio.md)), so output selection, mute and active
capture policy remain shared. Connection loss removes the relevant capability
and clears stuck keys/buttons. Reconnection cannot create duplicate devices.
Pairing keys are protected system data and are not ordinary app-readable settings.

## Printing

A shared print dialog chooses a discovered or manually added printer, paper,
orientation, page range, copies and supported finishing choices, with a preview
of the content to be sent. Unsupported options are unavailable. The printer's
reported capabilities and the document's pagination determine the preview,
including scaling and margins; printing a screenshot of the current window
is not a document-print implementation.

A supervised print service owns printer discovery, capabilities, spool data,
queue and retry state. Begin with a documented network print protocol and a
controlled printer target, then add supported USB or vendor-specific paths.
Applications submit a bounded document representation through a shared Kira
print API; the service negotiates a format the target actually accepts.
The target protocol, raster/vector formats and parser limits must be specified
before enabling arbitrary printers.

Queue UI distinguishes queued, sending, held, completed, cancelled and failed.
Disconnecting a printer retains a recoverable job. A retry after uncertain
delivery explains possible duplicate pages rather than pretending completion
can always be inferred. Cancelling stops future transfer and requests remote
cancellation when supported; it cannot retract already printed sheets.
Spool files are private and removed under a defined completion/retention policy.

## Cameras and controllers

Camera capture uses the Devices stream and privacy authorization paths. A
device exposes its supported modes and controls through typed capabilities;
the requesting app can select a valid mode and recover from removal. Photo
import from a camera is a separate data-transfer workflow: preview selected
items, copy them durably and offer source deletion only after successful import.
Photos integrates it through [29](29-system-apps.md), preserving originals and
reporting unsupported formats individually.

Controllers expose buttons, axes, identity, battery where available and player
assignment. Calibration, dead zones and remapping are system preferences over
the device, with a visible test surface. The platform receives navigation actions
while apps receive the eligible gameplay stream; focus and a reserved Home
action decide which is which. Unplugging releases held inputs and opens a
recoverable disconnected state on Console, without needing a pointer.

## Nearby file transfer

Nearby sharing discovers eligible devices only under the selected visibility
policy and offers a named recipient. Each transfer shows sender, filenames,
types, sizes and progress; receiving requires acceptance under a defined trusted
device policy. Data goes to a selected destination and is opened only by a
separate user action. Discovery proximity is not proof of sender identity.

Use the pairing and authenticated transport model in [37](37-continuity.md).
Transfer manifests bind file identity, size and digest; partial data is staged
until verified. Cancellation and disconnection leave no completed-looking file,
and a reused name follows Inlay's collision choices. Directory transfers have
explicit path validation and may not escape the chosen destination.

## Implementation order

Build Bluetooth adapter/pairing state and one input profile first, then audio
profiles using the existing audio route controls. Build printer submission and
the queue against a controlled endpoint before discovery convenience. Add camera
import and controller configuration over their ordinary DeviceKit paths. Nearby
transfer follows authenticated device pairing and the Files destination flow.
Each class can progress independently once its system contract is defined.

## Checkpoints

- [ ] Pair and remember a supported keyboard and mouse, reboot, reconnect and
      forget them. Key/button state clears on disconnection with no duplicate devices.
- [ ] Pair supported headphones, select them for playback and microphone use,
      disconnect and recover through Audio's route and authorization rules.
- [ ] Print a multi-page document with selected pages and copies, inspect its
      queue state and verify the physical output matches the preview.
- [ ] Disconnect the printer midway, cancel and retry according to the reported
      delivery state. Private spool data follows the documented retention policy.
- [ ] Authorize camera capture and import selected photos; removing the camera
      preserves completed copies and never deletes unverified source items.
- [ ] Calibrate and remap a controller, use it in an app and return Home, then
      unplug and reconnect without stuck inputs or an inaccessible system prompt.
- [ ] Accept a nearby transfer from an authenticated peer, verify its bytes,
      reject another request and interrupt a third. No rejected or partial transfer is opened.
- [ ] Record the real adapters, profiles, printers and peripherals used for
      acceptance. Enumeration-only and emulated checks remain separately labelled.
