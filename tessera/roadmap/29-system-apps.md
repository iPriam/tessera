# 29 — Everyday applications

An OS needs useful things to do after setup. Photos, Preview, Calculator,
Calendar, Reminders, Weather, Contacts and a recorder form the next application
set, alongside the existing Notes, Clock, Inlay and Settings. Each should
complete a modest real task before accumulating extra panels or placeholder
controls.

These applications exercise shared OS features: document opening, selection,
search, notifications, scheduling, media, permissions and sharing. The common
behaviour belongs in the toolkit or system service that owns it. Applications
remain separate components using public Kira and RIDL interfaces and the same
confinement as other applications.

## Photos and Preview

Photos imports selected images, shows a navigable library, provides albums and
favourites, and supports basic rotation and cropping without destroying the
original. An import is a managed copy or a referenced original, explicitly
chosen and recorded; removing an album membership is different from deleting
the photo. Duplicate imports are detected by content identity without assuming
two identical files have the same ownership or desired album membership.

Preview opens supported images and PDFs from Files, search or an association.
It provides zoom, fit, page navigation and basic annotations for the formats
whose writer exists. Unsupported editing is unavailable rather than pretending
to save. Export or Save As preserves the source unless replacement was chosen.
Printing uses the common print flow ([35](35-device-connections.md)).

Use shared image decoding, document rendering and thumbnail generation, with
bounded decoding sizes and isolated failure handling. These are Kira libraries
or services, not a second rendering stack or a native C dependency. Format
coverage is published and added incrementally: common raster images first,
then PDF reading, annotations and format-specific export. A damaged file must
not take down the library, Inlay previews or another open document.

## Calculator

Calculator provides ordinary arithmetic, clear, undo of entry, a readable
expression and copyable results. Keyboard input and locale-sensitive decimal
entry work with the same evaluator used by system search. A visible history
can be reused or cleared. Scientific functions and unit conversion extend the
same typed evaluator; no expression is evaluated as source code.

Define precision, rounding, overflow, division by zero and unit compatibility
before expanding the button set. Tests cover those rules and the displayed
round trip. Financial or currency conversion is later work requiring an
explicit rate source and timestamp; an offline answer cannot masquerade as current.

## Calendar, Reminders and Contacts

Calendar starts with local calendars, day/week/month views, events, all-day
events, recurrence, time zones and notifications. Reminders has lists, due
dates, completion and recurrence. Contacts has editable people and organisations,
search, duplicate handling and import/export. They work without an online
account and use versioned data formats that can be backed up and migrated.

An event stores the information needed to preserve its meaning: all-day dates
are not arbitrary UTC instants, and a recurring local appointment records its
time zone and recurrence policy. A reminder's completion changes its schedule
atomically. Clock, Calendar and Reminders register durable schedules with a
system scheduling service so closing an app does not cancel an alarm.
Missed events, time changes and resume reconcile once without duplicate alerts.

App-owned records remain private. A contact picker or an explicitly granted
calendar integration exposes selected data through a typed interface. An
application never gains access to every contact by receiving one shared card.
Import previews additions and duplicates; export makes a portable user-owned
file. Network account integrations are subsequent features, with conflict and
credential handling specified before connection to a real provider.

## Weather and recording

Weather shows a chosen location, current conditions, forecast and the freshness
of its data. Manual location works without location permission. Automatic
location is optional and goes through the location authorization flow in
[32](32-privacy-dashboard.md). Offline mode shows the last successful result
with its timestamp and an unavailable state for information never fetched.
Choose a data provider, licensing, attribution and refresh budget before
shipping live weather. Test the UI against controlled responses, including
missing fields and expired results; a mock forecast is not live-service proof.

The recorder selects an authorized microphone, shows real input levels, records,
pauses where the chosen format permits it, stops and plays back. Recordings
can be named, trimmed non-destructively and exported through Files or Share.
Capture is owned by the audio service, with the same active recording indicator
and revocation behaviour as any app ([21](21-audio.md)). Incremental durable
writing bounds how much is lost on a crash; disk-full and input removal end in
a recoverable recording, with a clear account of any missing segment.

## Shared implementation and delivery

Use KiraUI's document, focus, selection and navigation conventions. Put reusable
format readers, arithmetic and date/recurrence logic in shared Kira libraries;
put durable scheduling in a supervised service. AppKit remains the application
bridge for menus and notifications; privileged work is requested through
LibTessera rather than performed by an exempt system app.

Ship complete narrow flows in this order: Calculator and recorder; Preview
and photo import; Contacts; Calendar and Reminders; Weather once networking
and a provider are ready. This order is about usable slices, not a requirement
to block independent work. Every app participates in search, standard document
associations, accessibility, backup and lifecycle where those features apply.

## Checkpoints

- [ ] Import images, organise an album, crop one and recover its original.
      Deleting an album does not delete its images; damaged images fail individually.
- [ ] Open an image and a multi-page PDF in Preview through Inlay, navigate,
      export a supported edit and reopen the result without corrupting the source.
- [ ] Calculate and reuse an expression through both Calculator and system
      search, with identical results and defined precision and error handling.
- [ ] Create a recurring event across a time-zone or daylight-saving change;
      closing Calendar and rebooting do not lose it or duplicate its notification.
- [ ] Complete a recurring reminder and receive its next occurrence at the
      documented time, including after sleep and clock adjustment.
- [ ] Import, edit, find and export a contact; sharing one contact exposes only
      that contact and duplicate resolution preserves the chosen records.
- [ ] View live weather from the chosen provider, then go offline and see the
      cached result labelled with its age. Manual location needs no device-location grant.
- [ ] Record, stop, replay and export a real clip. Revoke access, remove the
      microphone and fill the destination during separate runs; completed audio remains recoverable.
