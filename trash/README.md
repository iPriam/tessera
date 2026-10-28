# trash

The Tessera Trash service (roadmap/19, decisions in its Decisions section).

It listens as `tessera.trash`, and serves one request per connection. A move
renames an item on the data volume into `/data/Trash/items` and writes its
record into `/data/Trash/records`, which names the item's original path. The
record goes first; if the rename fails, the record is taken back. Items on
another volume are refused `Unsupported`, and paths outside `/data`, under
`/data/system` or under the Trash are refused.

Put Back (`trashPutBack`) takes an id the move answered. It renames the item
back to the path its record names, and never replaces anything: when a file is
at that path, the answer is `AlreadyExists` and nothing changes. The record is
removed after the item is back. At start, a record whose item is gone is
removed.

LibTessera's `trashMove` and `trashPutBack` call it. Only programs the system
runs hold the `tessera.trash` entitlement. Emptying the Trash and listing it
are later slices.

It is built with the other programs by `tessera/build` and started by init as
`/bin/trash`.
