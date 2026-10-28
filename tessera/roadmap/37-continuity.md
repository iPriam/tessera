# 37 — Cross-device continuity

Two devices owned by the same person should be able to pass a selected task
between them without turning every document and notification into public
network state. Continuity includes nearby clipboard transfer, document handoff,
notification forwarding and optional synchronisation of chosen preferences.
Each is independently enabled and can be stopped without making local features
unusable.

This is an optional layer over working local capabilities. It does not require
an online account to use Tessera, introduce remote login or expose a mounted
network filesystem. Account-backed relays or cloud storage require their own
provider and operating decisions before implementation.

## Pair trusted devices

Pairing starts with an explicit action and visible device identities on both
ends. A comparison/confirmation step binds the peer's cryptographic identity;
network address, device name and physical proximity alone are insufficient.
The person sees paired devices, last connection and enabled capabilities in
Settings, and can disconnect temporarily or remove trust entirely.

A continuity service owns peer identities, protected key material, discovery,
authenticated transport and feature sessions. Discovery reveals only what the
selected visibility policy permits and has a bounded pairing mode. Once paired,
capabilities are negotiated independently: clipboard sharing does not also
authorize notification forwarding, migration or document access.

Use a reviewed authenticated-encryption and pairing design implemented within
Tessera's Kira/RIDL constraints. Choose protocols, key rotation and recovery
before accepting real personal data. Detect replayed session messages and bind
transfers to the intended peer and feature. Revoking a peer closes active
sessions and prevents new requests with the old authorization.

## Clipboard and nearby transfer

Clipboard sharing extends the ordinary local clipboard. The person chooses
whether an eligible clipboard item may be offered to a paired device. Secure
fields, password-like data marked sensitive and excluded applications are not
automatically exported. A receiving paste is intentional and shows provenance
where appropriate; the transport does not give a peer a live feed of all editor
contents or permanent access to clipboard history.

Items carry format, size, origin and expiry. Large images/files use a transfer
with progress, cancellation and digest verification rather than an unbounded
message. A disconnected source produces a clear unavailable paste or a retained
local copy according to the feature policy. Nearby file reception uses
[35](35-device-connections.md)'s accept/destination flow and cannot write outside
the accepted scope.

## Hand off a document or task

An app offers a typed continuation: stable application identity, supported
activity kind, minimal versioned state and references to selected content.
The destination shows the source app/device and asks to continue. It launches
a compatible local app through [36](36-app-lifecycle.md), with newly established
local grants, not foreign process handles or copied capabilities.

A document may already exist through a separately configured sync provider,
or be transferred explicitly for this handoff. State must say which, and a
missing document cannot become a guessed path. Before editing a transferred
copy, the receiving app explains whether changes return to the original or
remain a separate copy. Concurrent edits and return transfer use conflict rules
that preserve both versions until resolved.

An unsupported app/version leaves the original task intact and offers a
supported fallback, such as receiving the document alone. Sending a task does
not close the source or discard its unsaved changes until the person chooses
that action. Expired continuation records are removed without affecting the
underlying documents.

## Notifications and preference sync

Notification forwarding is enabled per source and app, with locked-device
privacy enforced at the source and destination. Forwarded records include
origin, stable notification identity and allowed actions. Dismissal and action
acknowledgements reconcile with the source; an unavailable source is shown as
such rather than pretending its app was opened remotely. A notification already
forwarded is never forwarded back into a loop.

Preference sync is a separate opt-in list of domains, such as appearance or
Home organisation. Sync semantic choices with schema versions and change
identities. Hardware-specific geometry, local credentials, permission grants,
device pairing keys and private app data are excluded unless a domain explicitly
defines their portable representation and consent requirements.

Every synced domain specifies merge/conflict rules, deletion propagation,
offline edits and reset behaviour. A last-writer policy cannot silently erase
a whole Home layout because device clocks disagree. Use domain revisions and
conflict preservation; show unresolved choices in Settings. Turning sync off
keeps local work unless the person explicitly chooses to remove it.

## Transport and delivery

Start on a paired local network, with a controlled discovery scope and no
Internet relay. Features work across reconnection through operation/session
identities, bounded queues and expiry. Durable content transfers commit only
after integrity checks. Exactly-once UI effects are reconciled from stable
identities; transport retry does not imply every remote side effect is safe
to repeat.

First deliver pairing and one explicit file transfer, then opt-in clipboard
and a Notes continuation. Add notification forwarding after stable actions and
privacy filtering, then selected settings sync with explicit conflict tests.
Use the same authenticated pairing model for setup migration in
[31](31-backup-and-recovery.md), with a distinct migration grant.

Ends when two paired devices can intentionally continue a document, while an
unpaired or revoked device receives neither its data nor unrelated capabilities.

## Checkpoints

- [ ] Pair two real peers, compare their identities and enable only file
      transfer. An unpaired device and a peer with the wrong key cannot receive content.
- [ ] Copy eligible text on one peer and intentionally paste it on the other;
      expiry, secure-field exclusion and clipboard-sharing disablement are enforced.
- [ ] Handoff a Notes document to a compatible app, edit the received copy and
      exercise the documented return/conflict flow with simultaneous source edits.
- [ ] Disconnect midway through a content transfer, resume or cancel, and
      verify exactly one completed destination with matching bytes.
- [ ] Forward a permitted notification, dismiss it remotely and reconnect.
      Origin, privacy filtering and dismissal reconcile without forwarding loops.
- [ ] Edit a synced Home preference offline on both peers, including deletion;
      reconnect with skewed clocks and preserve conflicts under the declared merge policy.
- [ ] Revoke a paired device during active sharing. Sessions end and old
      credentials cannot restart them; disabling sync preserves chosen local state.
- [ ] Demonstrate each flow from separate devices. Requests sent to the same
      host's own address do not establish cross-device acceptance.
