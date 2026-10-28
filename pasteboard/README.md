# pasteboard

The Tessera pasteboard service: the one text item the system's copy and
paste share (roadmap/23 §5, decision in §5.1 of that chapter).

It listens as `tessera.pasteboard` and speaks the wire LibTessera's
`pasteboardWrite` and `pasteboardRead` calls use (LibTessera
`app/Pasteboard/Pasteboard.kira`). A write replaces the item, and any program
may make one. A read needs an interaction token: without one it is refused
`NoInteraction`; with one, the service asks the Presence service to verify it
(roadmap/16 §14, `presence`), and sends the item only when the token is valid.
Presence's refusal is passed on. The item is never sent to a program without a
verified token, and nothing watches the item.

It is built with the other programs by `tessera/build` and started by init
as `/bin/pasteboard`.
