# power

The Tessera Power service: the power page, wake locks and scheduled wakes
(roadmap/17-devicekit.md §25 and roadmap/16 §18 in the tessera repository).

It listens as the named service `power` and speaks the wire LibTessera's
`Power.read`, `WakeLock.acquire` and `Power.scheduleWake` calls use
(LibTessera `app/Power/Power.kira`). The page is one Memory object every
client maps read-only, refreshed every two seconds from the DeviceKit power
sources and thermal zones. A wake lock is a connection holding a DeviceKit
wake source, and the service logs each one taken and let go under the
reason its client gave; a scheduled wake arms the real-time clock's alarm.

It is built by `scripts/build-initramfs.sh` in the tessera repository and
started by init as `/bin/power`.
