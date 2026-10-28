# devicekitd

The DeviceKit device manager. It is the one process that opens device nodes:
a DeviceKit client asks it over the `tessera.devicekit` socket, it checks the
asking program's entitlement for the device family, opens the node and passes
the descriptor back. It also keeps the table of transfer tokens a surface or
a fence crosses between processes with.

The protocol and the reasons for it are in `roadmap/17-devicekit.md` §20 in
the tessera repository. Build it with that repository's scripts; init starts
it before any other service.
