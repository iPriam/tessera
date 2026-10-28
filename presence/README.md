# presence

The Tessera Presence service (roadmap/16 §14.1): it verifies an interaction
token before anything acts on the person's behalf.

An interaction token is proof that a real hardware key or pointer button reached
a front-facing surface. MosaicDisplay mints one for each hardware press and
keeps it for 30 seconds. Presence asks MosaicDisplay whether a token is good:
minted for the surface named, unspent, unexpired, and on the surface that is
front-facing now. A valid token is spent by the check, so it is good once.

It listens as `tessera.presence` and speaks the device protocol LibTessera's
`presenceVerify` uses (LibTessera `app/Presence/Presence.kira`). A caller sends
the token's three words, and the answer's code is the verdict's error: `Ok`
for a valid token, `NotFrontmost`, or `NoInteraction` for anything else.

It has a display link from PID 1 and no surface. The link is the channel to
MosaicDisplay, on which the checks go (MosaicDisplay's `Input/Presence.kira`).
Without the link every verification is refused.

It is built with the other programs by `tessera/build` and started by init as
`/bin/presence`.
