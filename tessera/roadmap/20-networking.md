# 20 — Networking

Connect the machine, see whether the connection works, and bring files onto it.
This extends [06](06-devices-and-input.md)'s network requirement.

## Starting point

The kernel configuration includes networking and virtio-net. LibTessera has
socket operations. Those are building blocks; this chapter plans connection
management and the interface a person uses to reach the network.

## 1. Wired connections and downloads

An Ethernet connection configures itself automatically, with an address, route
and DNS servers. Settings shows the interface, address and connection state,
and offers automatic or manual configuration. Unplugging and reconnecting
updates that state without restarting the desktop.

The status area and Control Center use the same state. Link present, obtaining
an address, connected and connection failed are distinct. A working link alone
does not establish Internet access; a local network remains usable when the
Internet is unavailable.

A download action accepts an HTTPS URL, asks for a destination through the
document workflow ([19](19-files-and-documents.md)), and shows progress,
cancellation and a useful failure. A partial download is visibly incomplete
and cannot replace the destination until it succeeds. Certificate failures
are reported; they never silently fall back to an unverified connection.

Ends when a person connects Ethernet and downloads a file without host-side
network configuration inside the guest.

- [ ] A fresh machine obtains its wired configuration automatically, resolves
      a name and downloads a known HTTPS file whose bytes match the expected file.
- [ ] Settings, the status area and Control Center agree when the link goes
      away and returns. An existing local connection works without Internet access.
- [ ] A manual address, route and DNS configuration survives reboot and can be
      returned to automatic configuration from Settings.
- [ ] Cancel a download and interrupt a connection midway. Neither produces a
      completed file or overwrites an existing destination; retry can succeed.
- [ ] An invalid server certificate fails visibly. Installing a downloaded
      application still follows [12](12-apps.md)'s image verification and owner choice.

## 2. Remembered Wi-Fi

Settings and Control Center list nearby networks, show the connected one and
let the person join, disconnect or forget it. Joining a protected network asks
for its credentials and distinguishes an incorrect password from an unavailable
network. Secrets are kept outside ordinary application access and diagnostics.

The machine remembers accepted networks and reconnects when they return.
Automatic joining can be disabled per saved network. Turning Wi-Fi off stays
off after reboot; reconnecting never requires re-entering a remembered secret.
If a network requires a web sign-in, Tessera identifies that condition and
reports the unsupported sign-in flow until one exists.

Ends when a person can join a supported personal Wi-Fi network once and use it
again after sleep and reboot.

- [ ] On supported Wi-Fi hardware, discover a protected network, join it and
      download a file. An incorrect password produces a recoverable failure.
- [ ] Reboot and wake from sleep on that network; both reconnect automatically
      when automatic joining is enabled.
- [ ] Forget the network and confirm it is not joined again without a new
      request. Wi-Fi off remains off across reboot.
- [ ] A disappearing access point or captive network is reflected in the UI
      without claiming working Internet access.

## 3. Connection preferences

Ethernet and Wi-Fi can both be connected. Settings explains which connection
carries ordinary traffic, allows its preference to change, and offers proxy
configuration. Switching connections updates new requests promptly; interrupted
transfers explain their state and can be retried.

- [ ] With both interfaces connected, changing the preferred connection changes
      the route used by a new download and survives reboot.
- [ ] A configured proxy is used by system downloads; its failure is named and
      its credentials do not appear in logs.

## Evidence

QEMU can prove wired configuration, DNS and downloads. Wi-Fi discovery,
credentials and reconnection require a supported adapter and access point.
An emulated Ethernet connection cannot close a Wi-Fi checkpoint. Applications
reach network policy through LibTessera's entitlement boundary ([16](16-libtessera.md));
the manifest and syscall enforcement gaps remain tracked in [12](12-apps.md).
