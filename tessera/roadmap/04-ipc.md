# 04 — IPC

How two Tessera processes talk. Everything in [08](08-display-server.md) is
built on this, and the channel underneath it now exists.

## Where it stands

**A channel, and one message on it.** Kira's syscall table carries the socket
family; init makes a `socketpair` before it starts a service and hands the child
one end at descriptor 3; and the first message across it carries an open file
descriptor that the receiver reads through without knowing a path.

What is not here yet is the protocol — one opcode is not a protocol, there are
no objects to number, and nothing has ever sent a malformed message to find out
what happens. The rest of this document is still ahead.

## Why a unix socket and not something else

The candidates, and why the answer is not close:

- **Pipes.** Already have the syscalls. Carry bytes, and only bytes. Cannot
  carry a file descriptor, so a client could never hand the compositor a buffer.
  Useful for a service's log output ([03](03-processes.md)) and nothing more.
- **Shared memory with a file.** Works, and turns every message into a
  synchronisation problem the participants have to solve themselves. The kernel
  already solved it.
- **A unix socket.** Ordered, reliable, bidirectional, and — the point —
  `SCM_RIGHTS` lets a message carry file descriptors. A dma-buf is a descriptor.
  A GPU buffer is a dma-buf. There is no other way to do this on Linux.

**Unix sockets, `SOCK_SEQPACKET`.** Datagram semantics with reliability and
ordering, so a message is a message rather than a length prefix somebody has to
parse out of a stream. Wayland uses `SOCK_STREAM` and pays for it with framing
code in every implementation; there is no reason to inherit that.

## The syscalls

    socket  bind  listen  accept4  connect  socketpair
    sendmsg  recvmsg  shutdown  getsockname  setsockopt  getsockopt

All twelve are in Kira's table, as tags 51 through 62. Eleven of them an
interpreter may serve under `--backend vm`, on the same test as everything else
there: they are bounded by a descriptor the program already holds, or they mint
one and hand the number back the way `pipe2` does. `bind` is the exception —
it writes a name into the developer's filesystem, which is `mkdirat`'s refusal
wearing another hat.

`sendmsg`/`recvmsg` rather than `send`/`recv`, because the ancillary data that
carries descriptors only exists in the message forms. That makes `struct msghdr`
and `struct cmsghdr` two more structures to describe by offset — see
[02](02-kernel-and-syscalls.md) on generating those rather than typing them.
`cmsghdr` alignment is the classic place to get this wrong and the classic
symptom is a descriptor that arrives as `-1` with no error anywhere.

`socketpair` is worth calling out: it makes a connected pair with no filesystem
path at all, which is exactly what a parent hands a child it just spawned. The
display server does not need a socket in `/run` for a client it launched itself
— it needs a descriptor the child already has.

**Two connection models, and Tessera should use both.** A socket path in `/run`
for anything that connects on its own; an inherited `socketpair` for anything
the server or the platform started. The second needs no filesystem, no permissions
question and no race at startup.

## What a message looks like

Fixed-size header, variable body, descriptors in the ancillary data.

    u32  opcode
    u32  length        — of the whole message, header included
    u32  serial        — the sender's, echoed in a reply
    u32  target        — which object this is about, 0 for the connection

Everything little-endian, because both ends are the same machine and always will
be. Everything 8-byte aligned so a body can be read with the same load-at-offset
seam as a kernel structure.

No version negotiation beyond a single number in the first message. A sealed
system ships the server and its clients together; a protocol that negotiates is
solving a problem Tessera arranged not to have.

## Object lifetime

Objects — surfaces, buffers, callbacks — are numbered by the client, from a
range the server hands out at connection. Client-allocated identifiers mean a
client can create an object and use it in the same message without waiting for a
round trip, which is the difference between a protocol that feels immediate and
one that does not.

The server owns destruction: a client asks, the server acknowledges, and the
identifier is not reused until it has. Reusing an identifier too early is the
other classic bug in this shape of protocol, and it presents as a client
mysteriously operating on somebody else's surface.

## Errors

A protocol error is fatal to the connection and nothing else. The server sends
an error message naming the object and the reason, then closes. It does not
attempt recovery, and it certainly does not stop: a client that sends nonsense
is a bug in that client, and the machine carries on without it.

Every error must be traceable to a line of source. "Protocol error" with no
object and no reason is how a day gets spent.

## What this does not do

- **No network transport.** Tessera has no reason to show a window on another
  machine and every reason not to.
- **No serialisation library, no IDL, no code generation** until there is enough
  protocol to be worth generating. The message set in
  [08](08-display-server.md) is about twenty opcodes.
- **No general-purpose bus.** There is no D-Bus here. Services that need to talk
  get a socket between them, named in the service table.

## What exists today

`openChannel` in `init/app/Channel.kira` makes the pair; `start` in
`Services.kira` moves the child's end onto descriptor 3 with `dup3` and closes
the parent's copy the moment the child exists; `greet` sends opcode 1 with a
descriptor to `/etc/tessera-release` in its ancillary data. `mdTakeGreeting` in
`mosaicdisplay/app/Channel.kira` is the other half.

Three things in that are worth keeping when the protocol replaces the message:

- **Descriptor 3 is a convention between init and the programs it starts**, and
  is checked by nothing because there is nothing to check it against. Everything
  else in the seam — every offset, `SCM_RIGHTS`, and the `CMSG_SPACE`/`CMSG_LEN`
  arithmetic — is written by hand in Kira.
- **`dup3`, not `dup`.** The service needs a fixed number rather than whichever
  one was free, and `dup3`'s flags argument is the only way to be sure the copy
  is not close-on-exec — which the `execve` immediately after would undo.
- **A restart is a new connection.** The old socket went with the process that
  held the other end of it, so the supervisor closes its own end and makes a
  fresh pair before starting the replacement.

A service whose channel could not be made is still started. Nothing the display
does today depends on talking to init, and refusing to start it over a socket
would trade a working screen for a strictly worse machine.

## Checkpoints

- [x] `socketpair`, `sendmsg` and `recvmsg` are in Kira's table and wrapped.
- [x] Two Tessera processes exchange a message.
- [x] One of them passes a file descriptor to the other and the receiver reads
      through it.
- [x] A client that sends a malformed message is disconnected, and the machine
      carries on. The services boot deliberately sends a short fixed-width display
      packet after a successful frame and key event; MosaicDisplay closes that
      client session while QEMU and the current scanout remain alive.
