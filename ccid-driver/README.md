# ccid-driver

A sample Tessera user-space driver (roadmap/17-devicekit.md §18 in the
tessera repository). devicekitd starts it for a CCID smart card reader that
no kernel driver takes, such as QEMU's `usb-ccid` (08e6:4433), with a claim
token in its arguments. The driver redeems the token, opens the reader for
DeviceKit's Usb family, claims the CCID interface, asks slot 0 for its state
with `PC_to_RDR_GetSlotStatus`, and publishes a Driver entry under the reader
with what it found. The entry lasts as long as the driver runs.
