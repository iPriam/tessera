# bootstrap

The service broker. Every named service checks in with it over the
`tessera.system` socket, and every client reaches a service by asking it,
so no program reaches a service by a socket name of its own. The broker
knows which program may offer which name and which entitlement a client
needs to reach it, and it hands each side one end of a new channel.

A service checks in on the broker's own 256-byte wire. Every other process
holds a System channel to it, one connection kept while the process runs,
whose messages are Channel messages: `System/connectService` and
`System/listServices` are calls on it, answered under their transaction
ids. A service posts its events, such as `deviceChanged`, up its check-in
connection, and the broker sends each one on the System channel of every
process that holds the entitlement the service names. A service posts
only the events the broker's table names for it.

The protocol and the reasons for it are in `roadmap/16-libtessera.md` §12
and §25.4 in the tessera repository. Build it with that repository's
scripts; init starts it before any other service.
