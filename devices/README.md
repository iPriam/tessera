# devices

The Tessera Devices service: device lists and streams for audio, cameras,
sensors and haptics (roadmap/17-devicekit.md §25 and roadmap/16 §16 in the
tessera repository).

It listens as the named service `devices` and speaks the wire LibTessera's
`Device.list` and `Stream` calls use (LibTessera `app/Device/Wire.kira`).
Every device it drives is opened through devicekitd, and every slot of a
stream is a Memory object it makes and shares with the client.

Registry watches on AudioEndpoint, Camera and HapticDevice entries tell
it when a device comes or goes, and it posts each one as
`System/deviceChanged` through the broker, which sends it on the System
channel of every process entitled to the device's class.

It is built by `scripts/build-initramfs.sh` in the tessera repository and
started by init as `/bin/devices`.
