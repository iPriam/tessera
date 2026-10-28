# tablet-driver

A sample user-space driver for DeviceKit. devicekitd starts it for QEMU's
`usb-tablet`, a HID device the kernel's usbhid drives. Its manifest may
detach a kernel driver, so claiming the HID interface takes it from usbhid.
It reads the tablet's report descriptor with a control transfer and
publishes a Driver entry under the tablet that says what it found.

The model is `roadmap/17-devicekit.md` §17.2 and §18 in the tessera
repository. Build it with that repository's scripts.
