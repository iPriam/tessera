# 32 — Privacy and permissions

A person should know which applications can use a microphone, camera, location
or document, which are using them now, and how to stop that access. Permission
controls belong to the operating system and must reflect real service state.
An application cannot make its own trustworthy recording indicator.

The dashboard joins app identity, grants and authorization from
[16](16-libtessera.md) with capture and device services. The Presence and
authorization implementation gaps remain prerequisites for their corresponding
flows; describing a prompt does not establish enforcement.

## Make a decision in context

An app requests a capability when its feature needs it, explaining the purpose
with the system presenting the authenticated app identity. The request names
the actual scope: one selected document, microphone capture, camera capture,
or a location accuracy and lifetime. Broad and narrow requests must not look
identical. Denial leaves the app able to explain and recover without repeatedly
opening the same prompt.

Settings shows each app's current decisions and each capability's authorized
apps. The person can change or revoke them. A stored decision is bound to
stable app/publisher identity, not its display name or install path. An update
preserves the decisions it is entitled to retain and requests any new access
under [25](25-app-store.md)'s update policy. Reinstall and publisher-key changes
have explicit retention rules rather than inheriting unrelated consent.

## Active use and history

The platform shows active microphone, camera and screen-capture indicators.
Opening an indicator identifies every using app and offers the appropriate
stop or Settings action. Simultaneous use is represented accurately; one app
stopping does not clear another's active state. Lock-screen details follow the
person's notification and privacy preferences ([27](27-lock-screen.md)).

The dashboard records recent capability access: app, capability, start/end time,
result and scope description where useful. It records that a microphone was
used, not what was recorded; that a document grant was used, not its contents.
The history has a stated retention period, can be cleared and is itself private
system data. No telemetry upload follows merely from enabling local history.

History distinguishes a permission check, an actual opened stream and a denied
attempt. The UI cannot infer capture from a requested entitlement or an app's
own log line. A crashing app leaves a closed session with its real end state.

## Location

Location is a new service with explicit source, accuracy, freshness and error
semantics. A manual location in Weather or another app requires no device
location permission. Device-derived location requires authorization, and an
approximate grant must actually reduce the returned information rather than
just change the prompt's wording.

Foreground and background requests have separate policy. The service applies
that policy when delivering updates and when the requesting app loses its
eligible state. An unavailable sensor, expired result or denied request is
reported as such. Choose and document supported sources before promising
physical location; a guessed network location is not a satellite fix.

## Enforcement and ownership

A supervised authorization/Presence service owns decisions and trusted prompts.
Services enforce authorization at the operation boundary and again when a
revocable resource is used. LibTessera carries typed requests and revocable
handles; the library check alone cannot be the only protection against a
program making its own system calls. The namespace, launch confinement and
device boundaries in [12](12-apps.md) and [16](16-libtessera.md) must enforce
the corresponding restrictions before untrusted use is accepted.

Resource-owning services publish active-use records and bounded audit events
to the privacy service. PlatformKit combines the current active set; PlatformUI
draws indicators and trusted permission surfaces. Settings presents stored
policy and history. Revocation closes or disables active handles and streams,
releases device resources and tells the app why it stopped. It cannot retract
data an app already received, and the dashboard must not imply otherwise.

Documents retain per-item grants. Microphone and camera streams retain their
session identities. Screen capture retains the selected output/window scope.
These types can share decision and history machinery without collapsing into
one unlimited permission. Administrative tools and system components have
named, auditable exceptions, not a general exemption for any app shipped in
the system image.

## Implementation order

1. Define decision identity, scope, persistence and revocation, then implement
   the trusted request path and Settings' per-app view for microphone capture.
2. Connect actual stream start/stop events and platform indicators; add camera
   and screen capture through the same lifecycle without broadening their scopes.
3. Add the per-capability dashboard and local access history, including clear
   and retention behaviour.
4. Add device location once a supported source and approximate/background
   delivery policies have been implemented and tested.

Ends when a person can identify an app recording now, revoke its access and
verify that recording stops while another permitted app continues.

## Checkpoints

- [ ] Allow and deny microphone access for two apps. Settings shows the right
      decisions after reboot, and denied capture supplies no samples.
- [ ] Record with two apps, stop one and revoke the other. Indicators track the
      actual active set and revocation ends the remaining stream immediately.
- [ ] Crash or uninstall a recording app. No stale active indicator or usable
      authorization remains under an unrelated replacement app's identity.
- [ ] Capture a selected window and revoke it; another window and its private
      content are never added to that capture by a resize or workspace change.
- [ ] Inspect allowed, denied and completed accesses in history, clear it and
      exercise retention. No captured media or document contents are stored there.
- [ ] Give an approximate foreground location grant and verify returned accuracy
      and delivery after backgrounding, denial, expiry and source loss.
- [ ] Invoke a protected operation outside its ordinary app UI and confirm the
      service/kernel boundary still enforces the decision.
