# 34 — Accessibility

A person must be able to set up and use Tessera without depending on one way
of seeing, hearing, pointing or typing. Accessibility is part of the standard
toolkit and platform contracts. Adding labels to the first app does not make
system prompts, the store, recovery or another platform accessible.

This chapter covers a semantic accessibility tree, screen reading, full keyboard
navigation, magnification, text/contrast/motion preferences, captions and
alternative input. Features need acceptance through real complete tasks and
the relevant assistive output, in addition to tree inspection.

## Semantics and actions

KiraUI produces an accessibility tree from the widget tree: role, name, value,
state, help, relationships, bounds and supported actions. Decorative elements
are excluded, and composite controls expose a useful interaction model rather
than every internal drawing primitive. Custom controls must declare equivalent
semantics and actions to the standard controls they replace.

Stable element identity survives ordinary reconciliation. Dynamic updates send
events for changed values, selection, announcements and focus without rebuilding
the perceived page on every frame. Virtualised lists expose total extent,
position and a way to reach items outside the drawn region. Semantic focus and
keyboard focus have explicit relationships; reading an item does not activate
it or steal typing from an editor.

A privileged accessibility service brokers application trees over RIDL.
Applications publish only their own semantics. The service authenticates
providers and clients, checks action scope and filters locked or protected
surfaces. Inspecting another app's accessibility tree is powerful access, not
a free side channel around document and credential restrictions.

## Screen reading and keyboard navigation

The screen reader navigates controls, headings, text and lists; announces focus,
state and important asynchronous changes; and invokes exposed actions. Speech
output needs an implemented synthesis engine and supported voices. Until that
exists, a semantic inspector can verify structure but cannot close audible
screen-reader acceptance. Speech queues can be interrupted, with rate, voice
and verbosity controlled in Settings.

Tab and directional navigation have predictable order and visible focus.
Menus, dialogs, popovers, Home editing, notification history and app-store flows
are operable without a pointer. Modal scopes trap interaction only while active
and return focus to their origin on dismissal. Shortcuts have a discoverable
listing and conflicts with assistive commands are resolved explicitly.

Setup and Recovery expose accessibility entry points before ordinary setup has
finished or an authenticated session is open. The lock screen offers permitted
assistive navigation and reads neither hidden notification contents nor secret
credential characters. First-run accessibility choices persist into the session.

## Seeing and hearing

Text sizing scales semantic typography and layout rather than applying a final
bitmap stretch. Controls reflow, retain their meaning and remain reachable at
the supported maximum size. Contrast, reduced transparency and reduced motion
are system preferences delivered through the environment; animations and glass
effects have defined alternatives. Information conveyed by colour or animation
also has a textual, shape or state representation.

Magnification is a compositor transform with a defined relationship between
logical desktop coordinates, presented pixels and input hit testing. It tracks
focus or pointer according to the selected policy and works across displays
without moving content into an unreachable region. Cursor size and visibility
preferences remain consistent with real hit testing.

Captions use timed text tracks where media supplies them, with shared styling
preferences and correct seek/pause behaviour. Live caption generation is a later
recognition capability with an explicit engine, language support and privacy
policy; a captions button without a source must not promise live transcription.
Important sounds also have visible notification alternatives where appropriate.

## Alternative input

Sticky modifiers, slow/bounce keys, repeat settings and pointer speed belong
in the shared input path. An on-screen keyboard inserts through the text-input
contract ([33](33-language-and-input.md)). Switch scanning traverses semantic
groups and activates their actions; it does not rely on fixed screen coordinates.
Provide a clear escape/pause action so an assistive mode cannot strand its user.

Controller remapping and directional focus reuse the same actions and semantics
as Console and Media ([38](38-console-and-media.md)). Third-party assistive
devices require an explicit trusted integration, device discovery and a scoped
input source. Their input provenance must not accidentally count as an unrelated
application's synthetic input at a secure prompt.

## Implementation order

1. Define semantics, stable identity, actions and events in KiraUI; add an
   inspector and test standard controls, virtualised content and custom widgets.
2. Implement keyboard focus across all system flows, propagate display
   preferences and expose setup/lock/recovery accessibility entry points.
3. Add the brokered screen reader and actual speech output, then magnification
   with transformed input tests and captions with controlled timed tracks.
4. Add alternative-input policies, on-screen keyboard and switch scanning;
   test device-specific paths on the supported hardware.

Ends when a person can complete setup, find and install an app, edit and save
a document, and recover from an error using their supported assistive mode.

## Checkpoints

- [ ] Inspect and navigate standard and custom controls in Home, Settings and
      an app; their labels, roles, values and actions match what the controls do.
- [ ] Complete setup → Home → Store → install → open → save using keyboard only,
      with no focus traps and a visible indicator throughout.
- [ ] Complete the same supported flow with the screen reader and real speech,
      including a validation error, modal dialog and asynchronous progress.
- [ ] Increase text size and enable contrast, reduced transparency and reduced
      motion; content reflows and no essential command or state disappears.
- [ ] Magnify across two outputs, move focus and activate a transformed control;
      the input reaches the item actually presented under the pointer.
- [ ] Select a timed caption track, pause and seek playback, and retain caption
      styling without exposing private recording to an unauthorized caption service.
- [ ] Operate Home and a document editor with a supported switch device or
      on-screen keyboard and leave that mode without requiring another input device.
- [ ] Use accessibility at lock and Recovery while hidden private content and
      credential text remain protected.
