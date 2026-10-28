# 17 DeviceKit

The driver interface specification, v0.1. DeviceKit is to Tessera what IOKit is
to macOS: the one library that reaches hardware. It is written in Kira and
exported over RIDL as the module `DeviceKit`. Nothing in this tree implements it
yet; §25 records the plan.

Version 0.1 · target CalVer 2026.10 · status: design draft for review

DeviceKit holds what LibTessera ([16](16-libtessera.md)) deliberately does not:
the device registry, driver connections, GPU adapters, shareable GPU buffers
(surfaces), GPU fences, display control, raw input, and the hardware back halves
of audio, camera, sensors, haptics, power and block storage. It sits below
LibTessera's services and below Ember. Applications never call it.

Reading guide: §1–6 are the ground rules: principles, layering, conventions,
references, errors and completions. §7–8 are the registry and how a process
opens a device. §9–11 are the GPU objects Ember uses. §12–13 are display and
input, used by MosaicDisplay only. §14–17 are the service-only families. §18 is
user-space drivers. §19 is entitlements. §20 is the kernel backends. §21 maps
consumers onto calls. §22 lists the changes LibTessera takes. §23 is the call
index. §24 lists the decisions still open. §25 is the plan for this tree.

## 1. Principles

1. Below, not beside. DeviceKit is the only code that touches a driver. LibTessera's Devices, Power and Volume services, Ember and MosaicDisplay are its clients. An application reaches hardware through one of those, never directly.
1. Per kernel, like IOKit. LibTessera's surface is kernel-neutral and stable for apps. DeviceKit's is versioned with the OS and may expose kernel objects where a client needs one (a dma-buf for Vulkan import). It is rewritten per kernel.
1. Authority lives in the manager. A device node is opened by the device manager service `tessera.devicekit` after an entitlement check, never by the client. Linking DeviceKit grants nothing.
1. One completion path. DeviceKit owns no event loop. Its asynchronous results arrive on a LibTessera `Port` (§6), so a client waits on device events, files, channels and timers in one `Port.wait`.
1. Typed families, no pass-through. Every device family has typed calls and typed records. There is no generic ioctl, no raw property write, and no path to a device node in any call.
1. IOKit's shape. A registry of entries with typed properties, arranged in planes, found by matching, watched for arrival and termination. A connection per opened device, the analogue of `IOUserClient`. Surfaces and shared fences cross processes as IOSurface and `MTLSharedEvent` do.
1. Numbers are Kira's `Number`, as in LibTessera: every count, size, time, offset and code crosses as `Number`, or `UNumber` when it is never negative. DeviceKit converts to the fixed widths hardware and wire formats need inside itself.
1. Additive evolution within an OS release line. Declarations carry CalVer availability. Because only system code links DeviceKit, a shape may change at an OS release when every client changes with it; within a release it does not.

## 2. Layering

### 2.1 Who uses what

| Client | Process | DeviceKit families |
|---|---|---|
| Ember | every GUI app | Gpu, Surface, Fence |
| MosaicDisplay | `tessera.display` | Display, Input, Surface, Fence, Gpu |
| Devices service | `tessera.devices` | Audio, Camera, Sensor, Haptic, Registry |
| Power service | `tessera.power` | PowerSource, Thermal, Backlight, Suspend, WakeSource, Rtc |
| Volume service | `tessera.namespace` back half | Block, Mapper |
| User-space drivers | per driver | Hid, Usb, Registry.publish |
| Device manager | `tessera.devicekit` | implements the registry, connections and tokens |

An application reaches audio, camera, sensors and haptics through LibTessera
Streams (16 §16), power through LibTessera Power (16 §18), files through
LibTessera Namespace (16 §13), and the GPU and display through Ember and the
`tessera.display` protocol ([08](08-display-server.md)).

### 2.2 Link graph

```
 App code ──► LibTessera
 App code ──► Ember ──► DeviceKit ──► LibTessera (Port, types, private seam)
 Services ──► LibTessera, DeviceKit
 LibTessera ──✗ DeviceKit       (never)
```

LibTessera, the library, never links DeviceKit. LibTessera's services are
separate processes that link both. DeviceKit imports from LibTessera's surface
only `Handle` (to name a Port), `Completion`, `TesseraError` and `Buffer`, plus
the private seam in §6.3.

### 2.3 Keeping apps out

Not linking DeviceKit is a build rule. Entitlements are the enforcement.

- DeviceKit is a private framework, `/System/Library/PrivateFrameworks/DeviceKit.framework`. Only `Tessera.Internal.sdk` carries it, and only packages on the system volume list (Ember, MosaicDisplay, the services, drivers) build against that SDK. `Tessera.sdk` has no DeviceKit ([18](18-components.md) §2).
- The Launcher refuses an image outside the system volume whose own RIDL import table names `DeviceKit`. Ember's statically linked import does not count; the check reads the app module's imports, not the linked archive's.
- Neither rule is the security boundary. App code that calls DeviceKit anyway holds only `tessera.gpu`, so the manager gives it exactly what Ember would get: GPU adapters, its own surfaces and fences, and tokens it was sent.

### 2.4 What crosses between LibTessera and DeviceKit

- A Port, named by its `Handle`, as the destination of every asynchronous call (§3.4).
- `TesseraError` and `Completion`, unchanged.
- Bytes. A surface or fence goes to another process as a 16-byte token written into a LibTessera `Channel` message (§9.6). DeviceKit objects never enter LibTessera's handle table and are never sent as handles.

## 3. Conventions

### 3.1 Types at the boundary

The boundary is the RIDL surface `kira build --emit ridl ../devicekit`
writes. LibTessera's table (16 §2.1) applies, with these additions.

| Type | RIDL | Width | Meaning |
|---|---|---|---|
| `Ref` | `Ref { raw: U64 }` | 8 | a DeviceKit object reference; 0 is invalid (§4) |
| `RefRights` | `RefRights { raw: U64 }` | 8 | §4.3 |
| `DeviceEvents` | `DeviceEvents { raw: U32 }` | 4 | §4.4 |
| `EntryId` | `U64` | 8 | a registry entry, stable for one boot; 0 is none |
| `Token` | `Token { high: U64, low: U64 }` | 16 | a single-use transfer token (§9.6) |
| `FourCc` | `U32` | 4 | a pixel format, the DRM fourcc values |
| `Modifier` | `U64` | 8 | a buffer layout, the DRM modifier values; 0 is linear |
| `NativeHandle` | `NativeHandle { kind: U32, value: U64 }` | 16 | a kernel object for a graphics API import (§9.7) |

Records DeviceKit writes into a caller's `Buffer` keep the byte layouts stated
here. A variable-length list in a `Buffer` is a `U32` count, a `U32` record
size, then the records; the record size lets a later version append fields.

### 3.2 Naming

- `Family.verb` is exported as `familyVerb` in module `DeviceKit`: `Surface.create` is `surfaceCreate`, `Display.commit` is `displayCommit`. This document keeps writing `Family.verb`.
- No export may share a name with a LibTessera export, because a service links both. That is why enumeration is `Registry.match` and not `Device.list`, which LibTessera exports as `deviceList`.
- Verbs follow LibTessera's vocabulary (16 §2.2).

### 3.3 Export shape

As LibTessera (16 §2.3): `Result<T, TesseraError>` for a call with a result,
`TesseraError` for a call without one, one struct for several results, no
out-pointers.

`scripts/build-component.sh devicekit <arch>` builds the shared library
`DeviceKit.framework/DeviceKit` and its surface `DeviceKit.ridl` into the
component's roots, and a consumer imports it from the internal SDK when it is
built ([18](18-components.md)).

### 3.4 Synchronous vs asynchronous calls

LibTessera's rule (16 §2.4), with one addition.

- Calls that act on an open connection and never wait are synchronous: mapping a surface, reading a fence value, a non-blocking read of input records, a test-only display commit.
- Calls that wait on hardware or on the manager are asynchronous. They take `port: Handle, key: U64` last, return `TesseraError` for the submission only, and deliver exactly one `Completion` of kind `Device` (§6).
- A data path that is a stream of records (input events, HID reports, audio frames, sensor samples, USB transfers) is non-blocking synchronous calls plus readiness. The call answers `ShouldWait` when there is nothing to do, and `Ref.waitAsync` with `READABLE` or `WRITABLE` says when to call again. This is LibTessera's Channel model (16 §10.2).

The in-flight rules of 16 §2.4 apply: buffers stay valid until the completion,
closing a `Ref` cancels its operations with `Cancelled`, and `Port.cancel`
reaches DeviceKit operations by key.

### 3.5 Versioning

- Availability is per declaration, `@introduced("2026.10")`, as LibTessera.
- Within one OS release line nothing is renamed, removed or reshaped.
- At an OS release a shape may change. The old symbol is kept for one release so a system component built against the previous release still links. Apps are unaffected because they never link DeviceKit.
- `Ref.info` topics and `Buffer` records are versioned by record size (§3.1).

### 3.6 Threading

LibTessera's rules (16 §2.6) apply unchanged: every call is thread-safe, no call
allocates on the caller's behalf except a returned `Ref`, and there are no
callbacks.

## 4. Objects and references

### 4.1 Ref value

A `Ref` is a `U64` naming a DeviceKit object in a per-process table, separate
from LibTessera's handle table. 0 is invalid. Non-zero values encode an index
and a generation, so a closed `Ref` is never confused with a later one. A `Ref`
is process-local and cannot be sent: a surface or fence crosses processes as a
token (§9.6).

### 4.2 Object types

```
 enum DeviceObject: UInt32
   None          = 0
   Connection    = 1     an opened device, one per family call set        (§8)
   Surface       = 2     a shareable image buffer                         (§9)
   Fence         = 3     a 64-bit timeline                                (§10)
   Framebuffer   = 4     a surface registered for scanout                 (§12)
   RegistryWatch = 5     a stream of registry changes                     (§7)
   WakeSource    = 6     a named reason to stay awake                     (§15)
   Mapper        = 7     a device-mapper target                           (§16)
   Driver        = 9     a user-space driver's claim on an entry          (§18)
   // 8 and 10–255 reserved
```

### 4.3 Rights

```
 Duplicate    = 1 << 0     may create another Ref to the object
 Export       = 1 << 1     may mint a transfer token (§9.6)
 Read         = 1 << 2     read data, records, state
 Write        = 1 << 3     write data, submit work, change state
 Map          = 1 << 4     may be mapped into the address space
 Wait         = 1 << 7     may wait on events
 Inspect      = 1 << 8     may read info
 Manage       = 1 << 9     reconfigure, reset, set power state
 SetProperty  = 1 << 10    may set the diagnostic name
 Native       = 1 << 16    may take a NativeHandle (§9.7)
 Master       = 1 << 17    Display: may commit and set modes
 Grab         = 1 << 18    Input: holds the device exclusively
```

Bits 0–15 mean what LibTessera's same bits mean (16 §3.3). A `Ref` created by
the calling process has every right its object type supports except `Master`
and `Grab`, which come only from `Device.open` options the entitlement allows.
An imported surface or fence has the rights the exporter named in the token.

### 4.4 Events

```
 READABLE      = 1 << 0    a record or report is waiting
 WRITABLE      = 1 << 1    a write would not answer ShouldWait
 LOST          = 1 << 2    the device was removed or its connection revoked
 SIGNALED      = 1 << 3    Fence: reached the awaited value (waitAsync only)
 IDLE          = 1 << 4    Surface: no device access is pending
 ERROR         = 1 << 5    the object is in an error state; Ref.info says why
 RESET         = 1 << 6    Gpu: the adapter was reset; contexts are lost
 CHANGED       = 1 << 7    registry, hotplug, mode list or property change
 SUSPENDING    = 1 << 8    the system is about to suspend
 RESUMED       = 1 << 9    the system resumed
```

Per type: Connection READABLE | WRITABLE | LOST | ERROR | CHANGED | SUSPENDING |
RESUMED, plus RESET on a Gpu connection; Surface IDLE | LOST; Fence SIGNALED;
RegistryWatch READABLE; WakeSource LOST.

### 4.5 Generic calls

All synchronous except `Ref.waitAsync`.

```
 Ref.close(ref: Ref) -> TesseraError
```

Invalidates the `Ref`. In-flight operations complete with `Cancelled`. The
object is destroyed when its last `Ref` in every process is closed and no device
still uses it. A surface on screen stays on screen until the next commit
replaces it. Errors: `InvalidHandle`.

```
 Ref.duplicate(ref: Ref, rights: RefRights) -> Result<Ref, TesseraError>
```

Requires `Duplicate`. `rights` is a subset of the source's, or `U64` max for the
same rights. Errors: `InvalidHandle`, `AccessDenied`.

```
 Ref.info(ref: Ref) -> Result<RefInfo, TesseraError>

 struct RefInfo (32 bytes)
   type:      DeviceObject (u32)  @0
   state:     u32                 @4    0 Ok, 1 Lost, 2 Error, 3 Suspended
   rights:    RefRights (u64)     @8
   entry:     EntryId (u64)       @16   the registry entry behind it, or 0
   objectId:  u64                 @24   equal for Refs to the same object
```

```
 Ref.setName(ref: Ref, name: String) -> TesseraError
```

Requires `SetProperty`. Diagnostic name, at most 64 bytes, shown in tooling and
in the manager's audit lines.

```
 Ref.waitAsync(ref: Ref, events: DeviceEvents, options: U32, port: Handle, key: U64) -> TesseraError
```

Requires `Wait`. Options as LibTessera's `WaitOptions`: `Once` 0, `Repeat` 1,
`Edge` 2. Delivers a `Device` completion with `result0` = the events asserted
and `aux` = `DeviceCompletion.Event` (§6.2). `Repeat` is multi-shot and sets
`MORE`. Errors: `InvalidHandle`, `AccessDenied`, `InvalidArgument` (an event
the type never asserts).

## 5. Errors

DeviceKit reports `TesseraError` (16 §4) with `subsystem = DeviceKit` (10). It
uses LibTessera's codes where they fit and its own range 100–139 where none do.

Codes it shares with LibTessera, with their DeviceKit meaning:

| Code | Meaning here |
|---|---|
| `InvalidArgument` 1 | a record or descriptor is malformed; `detail` = byte offset of the bad field |
| `InvalidHandle` 2 | the `Ref` is 0, closed, or from another process |
| `WrongType` 3 | the `Ref` is not the family the call needs |
| `AccessDenied` 4 | the `Ref` lacks the right |
| `NotEntitled` 5 | the process lacks the family's entitlement (§19); `detail` = audit reference |
| `Unsupported` 10 | the device cannot do this, or the backend does not implement it |
| `NotFound` 20 | no such entry, or the token was never minted |
| `BadState` 23 | wrong state for the call (a Mapper already active) |
| `ShouldWait` 24 | non-blocking call has nothing to do; wait for `READABLE` or `WRITABLE` |
| `Cancelled` 26 | the operation was cancelled or its `Ref` closed |
| `OutOfMemory` 30 | device or system memory exhausted; `flags.RETRYABLE` when transient |
| `DeviceLost` 70 | the device was removed or failed |
| `DeviceBusy` 71 | another client holds it exclusively |
| `FormatUnsupported` 72 | no format/modifier pair satisfies the request |
| `Overrun` 73, `Underrun` 74 | media data lost or starved |
| `Internal` 90 | backend failure; `detail` = backend code |

DeviceKit's own codes:

```
 enum ErrorCode (continued)
   // GPU
   GpuReset              = 100    the adapter reset; detail bit0 = this client was guilty
   ContextLost           = 101    the object's GPU state is gone; recreate it
   IncompatibleModifier  = 102    the modifier is not valid for this device and usage
   FenceRegressed        = 103    a signal value below the fence's current value
   // display
   NotMaster             = 110    another client holds display control
   ModeRejected          = 111    the commit is invalid; detail = index of the first bad record
   ScanoutRejected       = 112    the surface cannot be scanned out (stride, placement, format)
   FlipPending           = 113    a non-blocking commit for that CRTC is still in flight
   // transfer tokens
   TokenExpired          = 120    the token outlived its lifetime
   TokenSpent            = 121    the token was already imported
   TokenNotForYou        = 122    the token is bound to another process (§24.2)
   // power and state
   Suspended             = 130    the device or the system is suspended
   Purged                = 131    a purgeable surface's contents were discarded
   // block
   IntegrityFailure      = 135    a verity block did not match its hash; detail = block number
   KeyRejected           = 136    a crypt target refused the key
```

Every refusal by the manager on entitlement grounds carries an audit reference,
as LibTessera's policy failures do.

## 6. Completions

### 6.1 Kind

Every DeviceKit completion has `kind = Device` (11), a new `CompletionKind`
in LibTessera (§22). The layout is LibTessera's 64-byte `Completion`
(16 §5.2). `timestamp` is `Clock.Monotonic` when the port observed the result.

### 6.2 What a completion carries

```
 enum DeviceCompletion: UInt32      in aux
   Event      = 1      Ref.waitAsync          result0 = events, result1 = 0
   Open       = 2      Device.open            result0 = Ref, result1 = family
   Import     = 3      Surface.import,        result0 = Ref, result1 = object type
                       Fence.import
   Fence      = 4      Fence.waitAsync        result0 = the awaited value, result1 = value observed
   Flip       = 5      Display.commit         result0 = commit serial, result1 = CRTC mask
   Transfer   = 6      Block.read/write,      result0 = bytes, result1 = offset
                       Usb.submit
   Suspend    = 7      Suspend.enter          result0 = suspended duration in ns, result1 = wake reason
   Token      = 8      Surface.export,        result0 = token.high, result1 = token.low
                       Fence.export
```

`aux` low 32 bits hold the `DeviceCompletion` value; the high 32 bits are
call-specific and stated with the call. A returned `Ref` is in the caller's
table when the packet is observed; when the error is not `Ok`, `result0` is 0.

### 6.3 How DeviceKit reaches a Port

DeviceKit cannot run code inside `Port.wait`. Every DeviceKit completion is
therefore one of two things LibTessera's port engine can finish by itself:

- Readiness. A kernel object becomes ready (a fence's eventfd, an out-fence, a DRM node, the manager's socket). The completion's fields were fixed when it was submitted, except `result0` for `Ref.waitAsync`, which the port fills with the events seen.
- Transfer. A read or write at an offset into a caller `Buffer`, the operation LibTessera already performs for `File.read`. `result0` is filled with the byte count.

Both go through a private seam that LibTessera exports in a separate RIDL module,
`LibTesseraBackend`, which only DeviceKit imports:

```
 Port.attachSource(port: Handle, source: SourceSpec, completion: Completion, options: U32) -> TesseraError

 struct SourceSpec (40 bytes)
   kind:      u32  @0    1 Readiness, 2 ReadAt, 3 WriteAt
   events:    u32  @4    Readiness: the readiness bits (read, write, error, hang-up)
   source:    u64  @8    the backend's kernel object (on Linux, a descriptor)
   offset:    u64  @16   ReadAt / WriteAt
   buffer:    Buffer @24 ReadAt / WriteAt
```

`completion` is the packet to post with `kind` forced to `Device`. `options` is
`Once` or `Repeat`. `Port.cancel` reaches it by `completion.key`. The seam is
kernel-specific by design: it names a backend object and is not part of
LibTessera's public surface. §24.1 records the alternative.

Results that need decoding (a flip's vblank time, an input record, a registry
change) are not put in the completion. The completion says that something is
ready; a synchronous call reads it (`Display.lastFlip`, `Input.read`,
`RegistryWatch.read`).

## 7. Registry and matching

### 7.1 Model

The registry is every device the kernel knows, as a tree of entries. Each entry
has an id, a class, a family, a set of typed properties, and a place in three
planes:

- Service: the provider. A connector's provider is a display controller; a display controller's is a GPU; a GPU's is a PCI function.
- Bus: physical topology. PCI root, bridge, function; USB host, hub, port, device, interface.
- Power: what must be powered for this entry to work.

The device manager builds the registry at boot from a walk of the kernel's device
model and keeps it current from hotplug notifications. It publishes a read-only
snapshot in shared memory with a generation number, so registry reads are
synchronous and never leave the process. A process maps the snapshot through the
manager when DeviceKit initialises.

Visibility follows entitlement: with `tessera.devicekit.registry` a process sees
every entry; otherwise it sees only the entries of the families its
entitlements open (a `tessera.gpu` process sees `GpuAdapter` entries).

### 7.2 Types

```
 enum DeviceClass: UInt32
   Unknown = 0, Bus = 1, GpuAdapter = 2, DisplayController = 3, Connector = 4,
   InputDevice = 5, AudioCard = 6, AudioEndpoint = 7, Camera = 8, Sensor = 9,
   HapticDevice = 10, PowerSource = 11, ThermalZone = 12, Backlight = 13,
   BlockDevice = 14, Partition = 15, HidDevice = 16, UsbDevice = 17,
   UsbInterface = 18, Rtc = 19, Driver = 20   (an entry a user-space driver published)

 enum Family: UInt32
   None = 0, Gpu = 1, Display = 2, Input = 3, Audio = 4, Camera = 5, Sensor = 6,
   Haptic = 7, PowerSource = 8, Thermal = 9, Backlight = 10, Block = 11,
   Hid = 12, Usb = 13, Rtc = 14

 enum Plane: UInt32       Service = 0, Bus = 1, Power = 2

 struct EntryInfo (48 bytes)
   id:          EntryId    @0
   class:       u32        @8
   family:      u32        @12   the family Device.open takes for it, or None
   parent:      EntryId    @16   in the Service plane
   generation:  u64        @24   the registry generation that last changed it
   flags:       u32        @32   bit0 REMOVABLE, bit1 BUILT_IN, bit2 WAKEUP_CAPABLE,
                                 bit3 DEFAULT (of its class), bit4 VIRTUAL
   childCount:  u32        @36   in the Service plane
   name:        u64        @40   offset of the name in the property area, see 7.3
```

### 7.3 Properties

A property is a key and a typed value. Keys are ASCII, at most 64 bytes, and
from the well-known list below or prefixed `vendor.`.

```
 enum PropertyType: UInt32
   Bool = 1, I64 = 2, U64 = 3, String = 4, Bytes = 5, List = 6

 record Property (variable)
   type:    u32  @0
   keyLen:  u32  @4
   valueLen: u32 @8      bytes, or the item count for List
   reserved: u32 @12
   key:     [u8] @16
   value:   follows the key, 8-byte aligned; a List is Property records
```

Well-known keys: `name`, `vendorId`, `productId`, `revision`, `serial`,
`location` (a stable physical path such as `pci-0000:00:02.0` or
`usb-1-2.3:1.0`), `driver` (the kernel driver's name), `subsystem`,
`busType` (`pci`, `usb`, `platform`, `virtio`, `i2c`, `hid`), `removable`,
`edid` (Connector), `connectorType` (Connector), `seat` (always `seat0` until
multi-seat exists), and on a UsbInterface `interfaceClass`,
`interfaceSubClass` and `interfaceProtocol`, with its device's `vendorId` and
`productId`.

### 7.4 Matching

```
 record MatchSpec (variable)
   class:   u32   @0     0 = any
   family:  u32   @4     0 = any
   flags:   u32   @8     bit0 MATCH_DEFAULT_ONLY
   count:   u32   @12    property constraints that follow
   constraints: [Property]   each must be present and equal; a String value may end in *
```

A spec matches an entry when every field and every constraint holds. Matching is
how a client finds "the default audio output", "every keyboard", or "the USB
device 1209:0001".

### 7.5 Calls

```
 Registry.match(spec: Buffer, out: Buffer) -> Result<U32, TesseraError>
```

Synchronous. Writes the `EntryInfo` list of matching entries visible to the
caller, in tree order. Returns the count. `BufferTooSmall` with the size in
`detail`.

```
 Registry.lookup(entry: EntryId) -> Result<EntryInfo, TesseraError>
 Registry.children(entry: EntryId, plane: Plane, out: Buffer) -> Result<U32, TesseraError>
 Registry.parent(entry: EntryId, plane: Plane) -> Result<EntryId, TesseraError>
 Registry.properties(entry: EntryId, out: Buffer) -> Result<U64, TesseraError>
 Registry.property(entry: EntryId, key: String, out: Buffer) -> Result<U64, TesseraError>
```

Synchronous reads of the snapshot. `properties` writes every property; `property`
writes one `Property` record. Errors: `NotFound` (no entry, or not visible),
`BufferTooSmall`.

```
 Registry.watch(spec: Buffer) -> Result<Ref, TesseraError>
 RegistryWatch.read(watch: Ref, out: Buffer) -> Result<U32, TesseraError>

 record RegistryChange (24 bytes)
   kind:       u32  @0    1 Matched, 2 Terminated, 3 PropertiesChanged
   class:      u32  @4
   entry:      EntryId @8
   generation: u64  @16
```

`watch` returns a `RegistryWatch`. It is `READABLE` when changes matching the
spec are waiting; `read` drains them in order and answers `ShouldWait` when none
are left. The first `read` after `watch` reports every entry already matching as
`Matched`, so a client never races arrival against its first `match`. A watch
that falls 4096 changes behind reports one record of kind 4 `Resync` and the
client calls `match` again.

## 8. Connections

### 8.1 Model

A connection is an opened device: the `Ref` a family's calls take. It is the
analogue of an `IOUserClient`. `Device.open` asks the manager, which checks the
family's entitlement, opens the kernel device, and passes it to the caller's
DeviceKit, which wraps it in a `Ref`. The kernel object never appears in any
call.

### 8.2 Calls

```
 Device.open(entry: EntryId, family: Family, options: U32, port: Handle, key: U64) -> TesseraError

 enum OpenOptions: UInt32
   None = 0, Exclusive = 1, Master = 2 (Display), Grab = 4 (Input), ReadOnly = 8
```

Asynchronous, because the manager answers it. Completion `Open`: `result0` =
the connection `Ref`, `result1` = the family. `Exclusive` fails `DeviceBusy`
if any other connection exists and blocks new ones while held. Errors:
`NotEntitled`, `NotFound`, `DeviceBusy`, `NotMaster` (Master requested and held
elsewhere), `Unsupported` (the entry is not of that family).

```
 Device.setPowerState(connection: Ref, state: U32) -> TesseraError
```

Requires `Manage`. `state`: 0 Auto (runtime power management decides), 1 On,
2 Suspended. Only the Power service holds `Manage` on connections it did not
open itself.

```
 Device.entry(connection: Ref) -> Result<EntryId, TesseraError>
```

A connection asserts `LOST` when its device is removed, when the manager
restarts, or when an entitlement change revokes it. Every later call on it
fails `DeviceLost`.

## 9. Surfaces

### 9.1 Model

A surface is an image buffer that the CPU, GPU, display and video engines can all
reach without copying, and that can be shared between processes. It is the
analogue of IOSurface. A surface has one to four planes, a fourcc format, a
modifier, and a set of usages it was allocated for. Its memory belongs to
DeviceKit: it is not a LibTessera `Memory`, and the only way to share it is a
token.

A surface has an implicit access state. Device work that reads or writes it
keeps it busy; `IDLE` asserts when none is pending. CPU access goes through
`Surface.lock`, which waits for device writes and makes the caches coherent.

### 9.2 Types

```
 enum SurfaceUsage: UInt32 (bits)
   Render = 1, Sample = 2, Scanout = 4, CpuRead = 8, CpuWrite = 16,
   VideoDecode = 32, VideoEncode = 64, Camera = 128, Protected = 256

 struct SurfaceDescriptor (64 bytes)
   width:        u32  @0
   height:       u32  @4
   format:       FourCc @8
   usage:        u32  @12
   gpu:          EntryId @16   the adapter it will be rendered on, or 0
   modifierCount: u32 @24      0 = linear only
   flags:        u32  @28      bit0 PURGEABLE, bit1 CONTIGUOUS
   modifiers:    [Modifier; 4] @32   acceptable layouts in preference order

 struct SurfacePlane (24 bytes)
   offset:  u64  @0
   stride:  u32  @8
   reserved: u32 @12
   size:    u64  @16

 struct SurfaceInfo (128 bytes)
   width, height:  u32, u32     @0, @4
   format:         FourCc       @8
   usage:          u32          @12
   modifier:       Modifier     @16
   planeCount:     u32          @24
   flags:          u32          @28   bit0 PURGEABLE, bit1 PURGED, bit2 IMPORTED, bit3 ADOPTED
   planes:         [SurfacePlane; 4] @32
```

### 9.3 Allocation

```
 Surface.create(descriptor: SurfaceDescriptor) -> Result<Ref, TesseraError>
```

Allocates a surface that satisfies every usage, in the first listed modifier the
devices behind those usages all accept. Usages without `Render`, `Sample` or a
video usage are always allocatable. Usages that need a GPU layout are allocated
by the adapter in `gpu` when it reports `AllocatesSurfaces` (§11.2); otherwise
the call fails `Unsupported` and the client allocates through its graphics API
and adopts the result. Errors: `FormatUnsupported`, `IncompatibleModifier`,
`OutOfMemory`, `InvalidArgument`.

```
 Surface.adopt(native: NativeHandle, info: SurfaceInfo) -> Result<Ref, TesseraError>
```

Wraps memory allocated by a graphics API (a Vulkan image exported as a dma-buf)
into a surface, so it can be exported, scanned out and locked like any other.
`info` describes the layout the API reported; DeviceKit checks it against the
memory's size. The caller keeps its own `NativeHandle` and closes it when done.
This is the path Ember uses on every stack without a DeviceKit allocator,
including venus. Requires `tessera.gpu`. Errors: `InvalidArgument`,
`Unsupported` (a kind the backend cannot adopt).

### 9.4 Calls

```
 Surface.info(surface: Ref) -> Result<SurfaceInfo, TesseraError>
```

```
 Surface.lock(surface: Ref, access: U32, port: Handle, key: U64) -> TesseraError
 Surface.unlock(surface: Ref) -> TesseraError
```

`access`: 1 Read, 2 Write, 3 both. Requires `Map` and `CpuRead` or `CpuWrite`
usage. The completion (`Event`, `result0` = `IDLE`) arrives when device writes
(and, for Write, device reads) are done and caches are coherent. Between
completion and `unlock` the CPU may touch the mapping; `unlock` flushes. Locks
nest per process for Read; Write is exclusive.

```
 Surface.map(surface: Ref) -> Result<RawPtr, TesseraError>
 Surface.unmap(surface: Ref) -> TesseraError
```

Maps every plane, contiguous, at the plane offsets `Surface.info` reports.
Requires `Map`. A mapping is valid until `unmap` or `Ref.close`. Reading or
writing outside a lock is allowed and gives no coherence guarantee.

```
 Surface.setPurgeable(surface: Ref, purgeable: Bool) -> Result<Bool, TesseraError>
```

A purgeable surface's memory may be reclaimed under pressure. Setting it back
returns whether the contents survived; when they did not, later uses fail
`Purged` until the client redraws. Mirrors `IOSurfaceSetPurgeable`.

```
 Surface.inUse(surface: Ref) -> Result<U32, TesseraError>
```

How many processes hold the surface, and bit 31 set when a display is scanning
it out. A compositor uses this to reuse buffers safely.

### 9.5 Lifetime

A surface lives while any process holds a `Ref` to it or a display scans it
out. Closing the last `Ref` of a surface on screen keeps it alive until a commit
replaces it.

### 9.6 Transfer tokens

```
 Surface.export(surface: Ref, rights: RefRights, lifetime: U64, port: Handle, key: U64) -> TesseraError
 Surface.import(token: Token, port: Handle, key: U64) -> TesseraError
```

`export` requires `Export`. The manager records the surface and the rights, and
the completion (`Token`) carries a 128-bit random token. The exporter writes the
16 bytes into a LibTessera `Channel` message. The receiver passes them to
`import`; its completion (`Import`) carries the new `Ref`.

- A token is single use. A second import fails `TokenSpent`.
- A token expires after `lifetime` nanoseconds; 0 means 10 seconds; the maximum is 60 seconds. Expiry fails `TokenExpired`.
- `rights` caps what the importer gets. Without `Export` in `rights` the importer cannot pass the surface on.
- A token never names the exporter. The audit log records both ends.

`Fence.export` and `Fence.import` (§10.3) work the same way.

Tokens are the only way a DeviceKit object leaves a process. MosaicDisplay
receives each client buffer once, at surface attach ([08](08-display-server.md)),
so the manager round trip is paid once per buffer, not per frame.

### 9.7 Native handles

```
 enum NativeKind: UInt32
   None = 0, DmaBufFd = 1, SyncFileFd = 2, DrmSyncobjFd = 3, MachPort = 4

 Surface.nativeHandle(surface: Ref, plane: U32) -> Result<NativeHandle, TesseraError>
```

Requires `Native`, which only `tessera.gpu` and `tessera.devicekit.display`
processes receive. Returns a new kernel object the caller owns and must close,
for one purpose: importing the surface into a graphics API
(`VkImportMemoryFdInfoKHR` with `VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT`).
It is the one place DeviceKit admits a kernel exists, and the reason DeviceKit
is per kernel. On Linux `value` is a descriptor; on XNU a Mach port name.

## 10. Fences

### 10.1 Model

A fence is a 64-bit timeline that only moves forward. Work signals it to a value
when done; a waiter asks to be told when it reaches a value. It is the analogue
of `MTLSharedEvent`, and it is the only GPU synchronisation object Tessera has:
binary fences and semaphores are timeline points.

### 10.2 Calls

```
 Fence.create(initial: U64) -> Result<Ref, TesseraError>
 Fence.value(fence: Ref) -> Result<U64, TesseraError>
 Fence.signal(fence: Ref, value: U64) -> TesseraError
```

`signal` is a CPU signal. Requires `Write`. Errors: `FenceRegressed` when
`value` is below the current value.

```
 Fence.waitAsync(fence: Ref, value: U64, port: Handle, key: U64) -> TesseraError
```

Requires `Wait`. Completion `Fence` when the fence reaches `value`, at once if it
already has. `result1` is the value observed, which may be higher.

```
 Fence.attachNative(fence: Ref, value: U64, native: NativeHandle) -> TesseraError
 Fence.nativeAt(fence: Ref, value: U64, kind: NativeKind) -> Result<NativeHandle, TesseraError>
 Fence.nativeHandle(fence: Ref) -> Result<NativeHandle, TesseraError>
```

Require `Native`. `attachNative` makes the fence reach `value` when a binary
kernel fence (a `SyncFileFd`) signals: this is how a Vulkan queue submit's
exported semaphore becomes a timeline point. `nativeAt` returns a binary kernel
fence for one point, which a display commit or a graphics API waits on; it fails
`ShouldWait` if nothing has yet been submitted to reach that point.
`nativeHandle` returns the whole timeline (`DrmSyncobjFd`) for
`VK_KHR_external_semaphore_fd` with a timeline semaphore.

### 10.3 Transfer

```
 Fence.export(fence: Ref, rights: RefRights, lifetime: U64, port: Handle, key: U64) -> TesseraError
 Fence.import(token: Token, port: Handle, key: U64) -> TesseraError
```

As §9.6. An importer without `Write` can wait on the fence but not signal it.
A client gives MosaicDisplay a fence with `Wait` only; MosaicDisplay gives a
client its release fence the same way.

## 11. Gpu

### 11.1 Model

A GPU adapter is a registry entry of class `GpuAdapter`. DeviceKit does not
submit GPU work: the graphics API's driver (Mesa's Vulkan drivers on Linux)
does. DeviceKit gives Ember what the graphics API cannot: which adapter is which,
its formats for scanout and sharing, its memory budget, its priority, and resets.

The Vulkan driver opens the render node itself. A process with `tessera.gpu`
has the render nodes of the adapters visible to it in its mount namespace and
nothing else from the kernel's device directory (§24.3).

### 11.2 Types

```
 struct GpuInfo (96 bytes)
   entry:         EntryId  @0
   vendorId:      u32      @8
   deviceId:      u32      @12
   renderMajor:   u32      @16    matches VkPhysicalDeviceDrmPropertiesEXT.renderMajor
   renderMinor:   u32      @20    and .renderMinor
   primaryMajor:  u32      @24    the display node, 0 if none
   primaryMinor:  u32      @28
   capabilities:  u64      @32    bit0 AllocatesSurfaces, bit1 Scanout, bit2 TimelineFences,
                                  bit3 ExplicitSync, bit4 Integrated, bit5 Virtual,
                                  bit6 ResetNotification, bit7 PriorityHigh
   localMemory:   u64      @40    bytes of device-local memory, 0 when unified
   sharedMemory:  u64      @48    bytes of system memory the GPU may use
   driver:        [u8; 32] @56    the kernel driver's name, NUL-padded
   reserved:      u64      @88

 record FormatModifier (16 bytes)
   format:    FourCc  @0
   planes:    u32     @4
   modifier:  Modifier @8
```

### 11.3 Calls

```
 Gpu.list(out: Buffer) -> Result<U32, TesseraError>
```

Synchronous. Writes a `GpuInfo` list. Ember matches each against Vulkan
physical devices by render node numbers. Requires `tessera.gpu`.

```
 Gpu.formats(gpu: Ref, usage: U32, out: Buffer) -> Result<U32, TesseraError>
```

The format and modifier pairs the adapter supports for every usage bit in
`usage`. For `Scanout` the list comes from the display planes the adapter drives.
`gpu` is a `Gpu` connection from `Device.open`.

```
 Gpu.memoryUsage(gpu: Ref) -> Result<GpuMemory, TesseraError>

 struct GpuMemory (32 bytes)
   localUsed, localBudget:    u64, u64  @0, @8
   sharedUsed, sharedBudget:  u64, u64  @16, @24
```

The calling process's usage and the budget the system gives it. The budget
shrinks for background processes; Ember trims caches when usage passes it.

```
 Gpu.setPriority(gpu: Ref, priority: U32) -> TesseraError
 Gpu.priority(gpu: Ref) -> Result<U32, TesseraError>
```

`priority`: 0 Low, 1 Normal, 2 High, 3 Realtime. High and Realtime need
`tessera.devicekit.display`. The value is what Ember requests with
`VK_EXT_global_priority`; DeviceKit records and checks it, and the kernel driver
enforces it.

A `Gpu` connection asserts `RESET` when the adapter resets. Every surface
allocated by it and every fence it signals may then be lost, and later calls on
them fail `ContextLost`. The completion error is `GpuReset`, with `detail`
bit0 set when this process's work caused the reset.

## 12. Display

### 12.1 Model

Display control is an atomic model: one commit sets the state of every CRTC and
plane it names, all at once or not at all, at the next vertical blank. It is
held by one process at a time, the master, which on Tessera is MosaicDisplay.
Requires `tessera.devicekit.display` and a connection opened with `Master`.

Resources: a display controller has connectors (physical outputs), CRTCs
(scanout engines), and planes (layers a CRTC composes: one primary, a cursor,
and overlays when the hardware has them).

### 12.2 Types

```
 record ConnectorInfo (variable)
   id:          u32  @0
   type:        u32  @4    0 Unknown, 1 Internal (eDP, DSI), 2 HDMI, 3 DisplayPort, 4 Virtual, 5 USB-C
   state:       u32  @8    0 Disconnected, 1 Connected, 2 Unknown
   crtcMask:    u32  @12   CRTCs it can be driven by
   widthMm:     u32  @16
   heightMm:    u32  @20
   modeCount:   u32  @24
   edidLength:  u32  @28
   modes:       [Mode]     modeCount records
   edid:        [u8]       edidLength bytes, 8-byte aligned

 record Mode (48 bytes)
   width, height:     u32, u32   @0, @4
   refreshMilliHz:    u32        @8
   flags:             u32        @12   bit0 PREFERRED, bit1 INTERLACED, bit2 VRR_CAPABLE
   clockKHz:          u32        @16
   hTotal, vTotal:    u32, u32   @20, @24
   hSyncStart, hSyncEnd, vSyncStart, vSyncEnd: u16 x4  @28
   reserved:          u64 + u32  @36

 record CrtcInfo (16 bytes)
   id: u32 @0, index: u32 @4, active: u32 @8, gammaSize: u32 @12

 record PlaneInfo (variable)
   id:          u32  @0
   type:        u32  @4    0 Primary, 1 Cursor, 2 Overlay
   crtcMask:    u32  @8
   zposMin:     u16  @12
   zposMax:     u16  @14
   capabilities: u32 @16   bit0 Scaling, bit1 Rotation, bit2 Alpha, bit3 Damage, bit4 InFence
   formatCount: u32  @20
   formats:     [FormatModifier]

 record CrtcState (32 bytes)
   crtc:        u32  @0
   flags:       u32  @4    bit0 ACTIVE, bit1 VRR
   connector:   u32  @8    0 = leave
   mode:        u32  @12   index into the connector's modes, 0xFFFFFFFF = leave
   gammaLut:    u64  @16   offset of a Lut record in the extra buffer, 0 = leave
   ctm:         u64  @24   offset of a 3x3 S31.32 matrix in the extra buffer, 0 = leave

 record PlaneState (72 bytes)
   plane:       u32  @0
   crtc:        u32  @4    0 = disable the plane
   framebuffer: Ref  @8
   srcX, srcY, srcW, srcH: u32 x4  @16   16.16 fixed point, in surface pixels
   dstX, dstY:  i32, i32     @32
   dstW, dstH:  u32, u32     @40
   zpos:        u16          @48
   alpha:       u16          @50   0xFFFF opaque
   rotation:    u32          @52   bit0 0°, bit1 90°, bit2 180°, bit3 270°, bit4 flip X, bit5 flip Y
   inFence:     Ref          @56   a Fence, 0 = none
   inFenceValue: u64         @64
   (damage rectangles, when present, follow in the extra buffer and are referenced by
    the commit's damage list)
```

### 12.3 Calls

```
 Display.resources(display: Ref, out: Buffer) -> Result<U64, TesseraError>
```

Writes a header (connector, CRTC and plane counts) followed by every
`ConnectorInfo`, `CrtcInfo` and `PlaneInfo`. A Display connection asserts
`CHANGED` on hotplug or a mode list change; call again.

```
 Display.framebuffer(display: Ref, surface: Ref) -> Result<Ref, TesseraError>
```

Registers a surface with `Scanout` usage for scanout and returns a `Framebuffer`.
Errors: `ScanoutRejected`, `IncompatibleModifier`.

```
 Display.commit(display: Ref, crtcs: Buffer, planes: Buffer, extra: Buffer, options: U32, port: Handle, key: U64) -> Result<U64, TesseraError>

 enum CommitOptions: UInt32
   TestOnly = 1, NonBlock = 2, AllowModeset = 4, FlipEvent = 8
```

Requires `Master`. Validates and applies the CRTC and plane records together.
Returns the commit serial, which increases by one per accepted commit.

- `TestOnly` checks without applying and returns 0. It is synchronous; no completion.
- `NonBlock` returns at once; a second non-blocking commit touching a CRTC whose last commit has not flipped fails `FlipPending`.
- `AllowModeset` is needed when a record changes a mode, a connector or `ACTIVE`.
- `FlipEvent` delivers a `Flip` completion when the commit is on screen: `result0` = serial, `result1` = the CRTC mask.

In-fences in plane records are waited on by the display engine, not the CPU.
Errors: `NotMaster`, `ModeRejected` (`detail` = record index), `ScanoutRejected`,
`FlipPending`, `DeviceLost`.

```
 Display.timeline(display: Ref) -> Result<Ref, TesseraError>
```

A `Fence` whose value is the serial of the last commit on screen. MosaicDisplay
derives each client's release point from it: a buffer replaced by commit N is
free when the timeline reaches N.

```
 Display.lastFlip(display: Ref, crtc: U32) -> Result<Vblank, TesseraError>
 Display.vblank(display: Ref, crtc: U32) -> Result<Vblank, TesseraError>

 struct Vblank (24 bytes)
   sequence:  u64  @0     vertical blank count on that CRTC
   timestamp: u64  @8     Clock.Monotonic of the blank
   serial:    u64  @16    lastFlip: the commit that flipped then; vblank: 0
```

`lastFlip` is the exact time a commit reached the screen, for frame pacing.
`vblank` is the current blank count and time.

```
 Display.setBacklight(display: Ref, connector: U32, level: U32) -> TesseraError
 Display.backlight(display: Ref, connector: U32) -> Result<U32, TesseraError>
```

0 to 65535. Only for `Internal` connectors with a backlight entry; the Power
service may cap it.

```
 Display.release(display: Ref) -> TesseraError
 Display.acquire(display: Ref, port: Handle, key: U64) -> TesseraError
```

Gives up and retakes master without closing the connection, for a session
switch. While released every commit fails `NotMaster`.

## 13. Input

### 13.1 Model

An input device is a registry entry of class `InputDevice`. Its connection yields
fixed-size records. Codes are USB HID usages (page and id), whatever the kernel's
own codes are, so a keyboard means the same thing on every backend. Requires
`tessera.devicekit.input`; on Tessera only MosaicDisplay holds it, and it routes
input to apps over the display protocol.

### 13.2 Types

```
 enum InputKind: UInt32 (bits, in the entry's `inputKinds` property)
   Keyboard = 1, Pointer = 2, Touchscreen = 4, Touchpad = 8, Tablet = 16,
   Switch = 32, Gamepad = 64, Buttons = 128 (power, volume)

 struct InputEvent (32 bytes)
   timestamp: u64  @0     Clock.Monotonic
   type:      u16  @8     1 Key, 2 Relative, 3 Absolute, 4 Switch, 5 Frame, 6 Dropped
   flags:     u16  @10    Key: bit0 REPEAT
   usage:     u32  @12    HID page << 16 | usage id
   value:     i32  @16    Key: 0 up, 1 down; Relative: delta; Absolute: position
   slot:      i32  @20    touch contact, -1 when not a contact
   tracking:  i32  @24    contact tracking id, -1 on lift
   reserved:  u32  @28

 struct AxisInfo (24 bytes)
   usage: u32 @0, resolution: u32 @4 (units per mm), minimum: i32 @8,
   maximum: i32 @12, fuzz: i32 @16, flat: i32 @20
```

A `Frame` record ends a group of records that happened together. A `Dropped`
record means the kernel buffer overflowed: discard the group in progress and
query the device state with `Input.state`.

### 13.3 Calls

```
 Input.read(input: Ref, out: Buffer) -> Result<U32, TesseraError>
```

Non-blocking. Writes whole `InputEvent` records and returns their count;
`ShouldWait` when none are waiting. Wait for `READABLE`.

```
 Input.axes(input: Ref, out: Buffer) -> Result<U32, TesseraError>
 Input.state(input: Ref, out: Buffer) -> Result<U32, TesseraError>
 Input.setLeds(input: Ref, leds: U32) -> TesseraError
```

`axes` writes an `AxisInfo` per absolute axis. `state` writes the usages
currently held down as `U32`s. `setLeds` takes HID LED usages as bits (bit0 Num
Lock, bit1 Caps Lock, bit2 Scroll Lock, bit3 Compose, bit4 Kana).

A connection opened with `Grab` receives the device's records exclusively, so no
other reader (a console, another session) sees them.

## 14. Media

These families serve the Devices service, which turns them into LibTessera
Streams (16 §16). Each requires `tessera.devicekit.media`. All data calls are
non-blocking with readiness (§3.4).

### 14.1 Audio

```
 struct AudioConfig (32 bytes)
   sampleRate:     u32 @0
   channels:       u32 @4
   sampleFormat:   u32 @8      I16 = 0, I24 = 1, I32 = 2, F32 = 3
   periodFrames:   u32 @12
   periods:        u32 @16
   channelMap:     u64 @24     bit per channel position

 Audio.formats(audio: Ref, out: Buffer) -> Result<U32, TesseraError>
 Audio.configure(audio: Ref, config: AudioConfig) -> Result<AudioConfig, TesseraError>
 Audio.start(audio: Ref) -> TesseraError
 Audio.stop(audio: Ref, drain: Bool) -> TesseraError
 Audio.write(audio: Ref, frames: Buffer) -> Result<U32, TesseraError>
 Audio.read(audio: Ref, frames: Buffer) -> Result<U32, TesseraError>
 Audio.position(audio: Ref) -> Result<AudioPosition, TesseraError>

 struct AudioPosition (24 bytes)
   frames: u64 @0 (frames played or captured since start), delay: u32 @8 (frames
   between the call and the speaker or from the microphone), state: u32 @12
   (0 Stopped, 1 Running, 2 Xrun), timestamp: u64 @16
```

`configure` returns what the device accepted. `write` and `read` move whole
frames and return the count; `Underrun` and `Overrun` mean an xrun, after which
the stream restarts at the next `write` or `read`.

### 14.2 Camera

```
 struct CameraFormat (32 bytes)
   format: FourCc @0, width: u32 @4, height: u32 @8, framesPerSecondQ16: u32 @12,
   colorSpace: u32 @16, flags: u32 @20, reserved: u64 @24

 struct CameraFrame (40 bytes)
   slot: u32 @0, bytes: u32 @4, sequence: u64 @8, timestamp: u64 @16,
   flags: u32 @24 (bit0 KEYFRAME, bit1 ERROR), reserved: u32 @28, reserved: u64 @32

 Camera.formats(camera: Ref, out: Buffer) -> Result<U32, TesseraError>
 Camera.configure(camera: Ref, format: CameraFormat, slots: U32) -> Result<CameraFormat, TesseraError>
 Camera.attach(camera: Ref, slot: U32, memory: Buffer) -> TesseraError
 Camera.queue(camera: Ref, slot: U32) -> TesseraError
 Camera.dequeue(camera: Ref) -> Result<CameraFrame, TesseraError>
 Camera.start(camera: Ref) -> TesseraError
 Camera.stop(camera: Ref) -> TesseraError
 Camera.control(camera: Ref, control: U32, value: I64) -> Result<I64, TesseraError>
 Camera.controls(camera: Ref, out: Buffer) -> Result<U32, TesseraError>
```

`attach` gives the device caller memory for a slot. The Devices service passes
the mapping of the Stream's ring buffer, so frames land in memory the app
already maps, without a copy. `dequeue` answers `ShouldWait` until a frame is
ready. Controls have stable ids: 1 Exposure, 2 Gain, 3 Focus, 4 WhiteBalance,
5 Zoom, 6 Torch, 7 PowerLineFrequency.

### 14.3 Sensor

```
 struct SensorChannel (16 bytes)
   kind: u32 @0 (LibTessera SensorKind), index: u32 @4, scale: i32 @8
   (value = raw x 10^scale), format: u32 @12 (1 S16, 2 S32, 3 U64 count)

 Sensor.channels(sensor: Ref, out: Buffer) -> Result<U32, TesseraError>
 Sensor.configure(sensor: Ref, rateHz: U32, batchNs: U64) -> Result<U32, TesseraError>
 Sensor.start(sensor: Ref) -> TesseraError
 Sensor.stop(sensor: Ref) -> TesseraError
 Sensor.read(sensor: Ref, out: Buffer) -> Result<U32, TesseraError>
```

`read` writes samples, each a `u64` monotonic timestamp followed by one value
per enabled channel in channel order, and returns the bytes written.

### 14.4 Haptic

```
 Haptic.capabilities(haptic: Ref) -> Result<U32, TesseraError>
 Haptic.upload(haptic: Ref, pattern: Buffer) -> Result<U32, TesseraError>
 Haptic.play(haptic: Ref, effect: U32, repeat: U32) -> TesseraError
 Haptic.stop(haptic: Ref, effect: U32) -> TesseraError
 Haptic.erase(haptic: Ref, effect: U32) -> TesseraError
```

A pattern is LibTessera's `AmplitudeF32AtHz` format. `upload` returns an effect
id. Capabilities: bit0 Rumble, bit1 Periodic, bit2 Custom waveform, bit3 Gain.

## 15. Power

For the Power service only, which publishes LibTessera's `PowerInfo` page and
owns wake locks (16 §18). Requires `tessera.devicekit.power`.

```
 struct PowerSourceInfo (64 bytes)
   kind: u32 @0 (1 Battery, 2 Mains, 3 Usb, 4 Wireless), state: u32 @4 (LibTessera
   batteryState values), online: u32 @8, percent: u32 @12, energyNowUwh: u64 @16,
   energyFullUwh: u64 @24, powerNowUw: u64 @32, voltageUv: u64 @40,
   timeToEmpty: u64 @48, timeToFull: u64 @56

 struct ThermalInfo (32 bytes)
   kind: u32 @0 (1 Cpu, 2 Gpu, 3 Battery, 4 Skin, 5 Other), tripCount: u32 @4,
   milliCelsius: i32 @8, policyLevel: u32 @12 (0 Nominal to 3 Critical),
   reserved: [u64; 2] @16

 struct ThermalTrip (16 bytes)
   milliCelsius: i32 @0, hysteresis: i32 @4, type: u32 @8 (1 Passive, 2 Hot,
   3 Critical), reserved: u32 @12

 PowerSource.read(source: Ref) -> Result<PowerSourceInfo, TesseraError>
 Thermal.read(zone: Ref) -> Result<ThermalInfo, TesseraError>
 Thermal.trips(zone: Ref, out: Buffer) -> Result<U32, TesseraError>
 Thermal.setCoolingState(zone: Ref, device: U32, state: U32) -> TesseraError
 Backlight.read(backlight: Ref) -> Result<U32, TesseraError>
 Backlight.write(backlight: Ref, level: U32) -> TesseraError
```

Power source and thermal connections assert `CHANGED` when the kernel reports a
change. Backlight levels are 0 to 65535, scaled to the device's range.

```
 Suspend.enter(state: U32, port: Handle, key: U64) -> TesseraError
 Suspend.inhibitors(out: Buffer) -> Result<U32, TesseraError>
```

`state`: 1 Idle (suspend-to-idle), 2 Memory, 3 Disk. Every DeviceKit connection
asserts `SUSPENDING` first. The completion (`Suspend`) arrives after resume:
`result0` = time suspended, `result1` = the waking `EntryId` or 0. Errors:
`Busy` when a wake source is active (`inhibitors` lists them by name).

```
 WakeSource.create(name: String) -> Result<Ref, TesseraError>
 WakeSource.activate(source: Ref, timeout: U64) -> TesseraError
 WakeSource.deactivate(source: Ref) -> TesseraError
 Rtc.setWake(rtc: Ref, at: U64) -> TesseraError
 Rtc.read(rtc: Ref) -> Result<U64, TesseraError>
```

A `WakeSource` keeps the system from suspending while active; LibTessera's
`WakeLock` objects are backed by these in the Power service. `Rtc.setWake` arms
the hardware alarm for `Power.scheduleWake` deadlines that must survive
suspend, in `Clock.Boottime` nanoseconds.

## 16. Block

For the Volume service only, which serves LibTessera's Namespace. Requires
`tessera.devicekit.block`.

```
 struct BlockInfo (48 bytes)
   size: u64 @0, logicalBlock: u32 @8, physicalBlock: u32 @12, optimalIo: u32 @16,
   flags: u32 @20 (bit0 READ_ONLY, bit1 ROTATIONAL, bit2 REMOVABLE, bit3 DISCARD,
   bit4 WRITE_ZEROES, bit5 FUA), maxTransfer: u64 @24, discardGranularity: u64 @32,
   reserved: u64 @40

 Block.info(block: Ref) -> Result<BlockInfo, TesseraError>
 Block.read(block: Ref, offset: U64, buffer: Buffer, port: Handle, key: U64) -> TesseraError
 Block.write(block: Ref, offset: U64, buffer: Buffer, flags: U32, port: Handle, key: U64) -> TesseraError
 Block.discard(block: Ref, offset: U64, length: U64, port: Handle, key: U64) -> TesseraError
 Block.flush(block: Ref, port: Handle, key: U64) -> TesseraError
 Block.eject(block: Ref) -> TesseraError
```

`eject` has the drive let its medium go, as DADiskEject does once the disk's
volumes are unmounted: a CD or DVD drive opens its tray, and any other SCSI
disk, such as a USB stick, is powered off and leaves the registry until it is
plugged in again. A disk with no medium to let go of, such as a virtio disk or
a partition, answers `Unsupported`. A drive still mounted or held open by
another program answers `Busy`. It needs only the read right: ejecting writes
nothing, and a drive with a read-only medium opens only for reading.

Offsets and lengths are multiples of `logicalBlock`, and the buffer address is
aligned to it. `write` flags: bit0 FUA. Completions are `Transfer` with the byte
count. A connection opened `Exclusive` keeps every other opener out, including
the kernel's own mounts.

```
 struct MapperTarget (variable)
   kind: u32 @0 (1 Linear, 2 Verity, 3 Crypt), start: u64 @8, length: u64 @16,
   argumentLength: u32 @24, arguments: [u8] (kind-specific, below)

 Mapper.create(name: String, targets: Buffer, flags: U32) -> Result<Ref, TesseraError>
 Mapper.activate(mapper: Ref, port: Handle, key: U64) -> TesseraError
 Mapper.entry(mapper: Ref) -> Result<EntryId, TesseraError>
 Mapper.status(mapper: Ref) -> Result<U32, TesseraError>
```

Verity arguments name the data and hash entries, the block sizes, the salt and
the root hash; a read that fails verification completes `IntegrityFailure`.
Crypt arguments name the cipher and carry the key, which DeviceKit hands to the
kernel and wipes from its own memory before returning. A mapper's result is a
new `BlockDevice` entry, opened with `Device.open`. Closing a mapper `Ref`
removes the device once nothing has it open. These back the A/B
volumes and the seal in [13](13-security-and-updates.md).

## 17. Hid and Usb

For user-space drivers (§18) on the entries they matched, and for system
services that need raw access. Requires `tessera.driver` or
`tessera.devicekit.hid` / `tessera.devicekit.usb`.

### 17.1 Hid

```
 Hid.descriptor(hid: Ref, out: Buffer) -> Result<U32, TesseraError>
 Hid.readReport(hid: Ref, out: Buffer) -> Result<U32, TesseraError>
 Hid.writeReport(hid: Ref, report: Buffer) -> Result<U32, TesseraError>
 Hid.getFeature(hid: Ref, reportId: U32, out: Buffer) -> Result<U32, TesseraError>
 Hid.setFeature(hid: Ref, report: Buffer) -> TesseraError
```

`readReport` is non-blocking; wait for `READABLE`. A keyboard, mouse or touch
device the kernel already drives appears as an `InputDevice`; a `HidDevice` entry
is offered to drivers only when no kernel driver claims the device, or when the
device's vendor protocol has no kernel driver.

### 17.2 Usb

```
 struct UsbTransfer (48 bytes)
   endpoint: u32 @0 (bit7 IN), type: u32 @4 (1 Control, 2 Bulk, 3 Interrupt,
   4 Isochronous), buffer: Buffer @8, timeout: u64 @24, setup: u64 @32 (Control:
   the 8-byte setup packet), flags: u32 @40, reserved: u32 @44

 Usb.descriptors(usb: Ref, out: Buffer) -> Result<U32, TesseraError>
 Usb.claimInterface(usb: Ref, interface: U32) -> TesseraError
 Usb.releaseInterface(usb: Ref, interface: U32) -> TesseraError
 Usb.setAlternate(usb: Ref, interface: U32, alternate: U32) -> TesseraError
 Usb.submit(usb: Ref, transfer: UsbTransfer, port: Handle, key: U64) -> TesseraError
 Usb.clearHalt(usb: Ref, endpoint: U32) -> TesseraError
 Usb.reset(usb: Ref) -> TesseraError
```

`submit` completes `Transfer` with the byte count; many transfers may be in
flight. Claiming an interface the kernel drives fails `DeviceBusy` unless the
driver's manifest says to detach the kernel driver and its entitlement allows
it.

## 18. Drivers

### 18.1 Model

A user-space driver is a program on the system volume that serves a device the
kernel has no driver for, in the manner of DriverKit. It has a manifest of match
specs. When a matching entry appears, the manager asks the Launcher to start the
driver (or tells the running instance), with a token for the entry in its launch
arguments. The driver claims the entry, talks to it through Hid or Usb, and
publishes the result as a new registry entry. A crash costs one device, not the
system.

### 18.2 Manifest

```
 DriverManifest (in the bundle's Info.plist, one per personality)
   name:          String
   matches:       Vector<MatchSpec>
   score:         I32          higher wins when several drivers match
   publishes:     DeviceClass  the class of the entry it publishes
   detachKernel:  Bool         may claim an interface the kernel drives
   service:       String       the tessera service name it serves, if any
```

### 18.3 Calls

```
 Driver.claim(token: Token, port: Handle, key: U64) -> TesseraError
 Registry.publish(driver: Ref, class: U32, properties: Buffer) -> Result<EntryId, TesseraError>
 Registry.update(driver: Ref, entry: EntryId, properties: Buffer) -> TesseraError
 Registry.unpublish(driver: Ref, entry: EntryId) -> TesseraError
```

`claim` completes `Open` with a `Driver` `Ref`; `Device.open` on the claimed
entry then succeeds for the Hid or Usb family. Published entries are children of
the claimed entry in the Service plane and are terminated when the driver exits.
A driver that publishes an input or audio device feeds it to its consumer over
its own service protocol; DeviceKit does not carry the data.

## 19. Entitlements

| Entitlement | Grants | Held by |
|---|---|---|
| `tessera.gpu` | `Gpu.list`, Gpu connections, Surface and Fence calls, `Native` on its own surfaces and fences, render nodes in the mount namespace | every GUI app, by default; reached through Ember |
| `tessera.devicekit.display` | Display connections with `Master`, High and Realtime GPU priority | MosaicDisplay |
| `tessera.devicekit.input` | Input connections, `Grab` | MosaicDisplay |
| `tessera.devicekit.media` | Audio, Camera, Sensor, Haptic connections | the Devices service |
| `tessera.devicekit.power` | PowerSource, Thermal, Backlight, Suspend, WakeSource, Rtc, `Device.setPowerState` | the Power service |
| `tessera.devicekit.block` | Block, Mapper | the Volume service |
| `tessera.devicekit.registry` | the whole registry, every class | system services |
| `tessera.devicekit.hid`, `tessera.devicekit.usb` | Hid or Usb connections on any entry | system services that need raw access |
| `tessera.driver` | `Driver.claim`, Hid and Usb on claimed entries, `Registry.publish` | user-space drivers |

All but `tessera.gpu` are system-only: the Launcher grants them only to images
on the system volume, as `tessera.driver` already is (16 §11.6).

## 20. Kernel backends

### 20.1 The device manager

`tessera.devicekit` is a system service started by PID 1 before the services
that use it. It is the only process that opens device nodes.

- Registry: a walk of `/sys` at start, then `NETLINK_KOBJECT_UEVENT` for changes. The snapshot is a sealed memfd, replaced on change; an eventfd per watcher signals a new generation.
- Connections: on `Device.open` the manager checks the caller's entitlement set, opens the node, and sends the descriptor over its socket with `SCM_RIGHTS`. Its socket is a DeviceKit-owned `SOCK_SEQPACKET` connection made at DeviceKit initialisation, not a LibTessera Channel.
- Tokens: the manager keeps a table from token to descriptor, rights and expiry. `export` sends the descriptor in; `import` sends it out and deletes the entry.
- Audit: every open, export, import and refusal is a line to the Audit service.

### 20.2 Linux mapping

```
 Object / call         Linux mechanism
```

```
 Ref                   in-process table {descriptor or index, type, rights, generation};
                       descriptors are O_CLOEXEC and never exposed except by nativeHandle
```

```
 Port seam             Port.attachSource with the descriptor; LibTessera's engine polls
                       it (ppoll today, IORING_OP_POLL_ADD or READ later)
```

```
 Registry              sysfs walk; NETLINK_KOBJECT_UEVENT; planes from sysfs parent
                       links (Service), the devpath (Bus), and power/supplier links (Power)
```

```
 Surface               dma-buf. create: DRM_IOCTL_MODE_CREATE_DUMB + PRIME_HANDLE_TO_FD
                       for Scanout|Cpu*, /dev/udmabuf over a sealed memfd for Cpu*; adopt:
                       the exported dma-buf. map: mmap. lock: DMA_BUF_IOCTL_SYNC
                       (START/END), waiting for implicit fences via poll first. IDLE: poll
                       on the dma-buf. Native: dup of the dma-buf descriptor.
```

```
 Fence                 drm_syncobj timeline on a render node: SYNCOBJ_CREATE,
                       SYNCOBJ_TIMELINE_SIGNAL, SYNCOBJ_QUERY, SYNCOBJ_EVENTFD for
                       waitAsync, SYNCOBJ_FD_TO_HANDLE with IMPORT_SYNC_FILE + SYNCOBJ_TRANSFER
                       for attachNative, SYNCOBJ_TRANSFER + HANDLE_TO_FD with
                       EXPORT_SYNC_FILE for nativeAt, HANDLE_TO_FD for nativeHandle
```

```
 Gpu                   render node; DRM_IOCTL_VERSION; PCI ids and memory from sysfs;
                       fdinfo drm-memory-* for memoryUsage; reset from the WEDGED uevent
                       and devcoredump; formats from IN_FORMATS plane blobs
```

```
 Display               primary node as DRM master: SET_CLIENT_CAP (ATOMIC,
                       UNIVERSAL_PLANES), GETRESOURCES, GETCONNECTOR, GETPLANERESOURCES,
                       GETPLANE, OBJ_GETPROPERTIES, ADDFB2 with modifiers, ATOMIC with
                       TEST_ONLY / NONBLOCK / ALLOW_MODESET / PAGE_FLIP_EVENT, IN_FENCE_FD,
                       OUT_FENCE_PTR (the out-fence feeds Display.timeline), FB_DAMAGE_CLIPS,
                       GAMMA_LUT, CTM, VRR_ENABLED, CRTC_GET_SEQUENCE for vblank and
                       lastFlip, DROP_MASTER / SET_MASTER for release and acquire;
                       backlight through /sys/class/backlight
```

```
 Input                 evdev: EVIOCGBIT, EVIOCGABS, EVIOCGKEY, EVIOCGRAB, EV_LED writes;
                       evdev codes translated to HID usages by a generated table;
                       SYN_DROPPED becomes Dropped
```

```
 Audio                 ALSA PCM ioctls on /dev/snd/pcm*: HW_REFINE, HW_PARAMS,
                       SW_PARAMS, PREPARE, START, DROP, DRAIN, WRITEI_FRAMES,
                       READI_FRAMES, STATUS; no alsa-lib
```

```
 Camera                V4L2: VIDIOC_QUERYCAP, ENUM_FMT, S_FMT, REQBUFS with USERPTR,
                       QBUF, DQBUF, STREAMON, STREAMOFF, QUERYCTRL, S_CTRL
```

```
 Sensor                IIO: scan_elements, buffer/enable, the character device read
```

```
 Haptic                evdev force feedback: EVIOCSFF, EVIOCRMFF, EV_FF writes
```

```
 Power                 power_supply and thermal sysfs with uevents; /sys/power/state
                       and mem_sleep; /sys/power/wake_lock and wake_unlock;
                       RTC_WKALM_SET; per-device power/control
```

```
 Block                 block node with O_DIRECT; BLKGETSIZE64, BLKSSZGET, BLKPBSZGET,
                       BLKDISCARD, fdatasync; device-mapper DM_DEV_CREATE,
                       DM_TABLE_LOAD, DM_DEV_SUSPEND
```

```
 Hid / Usb             hidraw: HIDIOCGRDESC, HIDIOCGFEATURE, HIDIOCSFEATURE;
                       usbfs: USBDEVFS_CLAIMINTERFACE, SETINTERFACE, SUBMITURB,
                       REAPURBNDELAY, CLEAR_HALT, RESET, DISCONNECT_CLAIM
```

DeviceKit's Linux backend takes struct layouts from autobind and keeps the
ioctl request numbers, which are macros autobind cannot carry, in one module.

### 20.3 XNU mapping

On XNU DeviceKit is a thin layer over IOKit: the registry is the IORegistry,
matching is `IOServiceMatching` with the same specs, connections are
`IOServiceOpen`, surfaces are IOSurfaces, fences are `IOSurface`-backed shared
events, display is IOMobileFramebuffer or IOFramebuffer, input is IOHIDFamily,
audio is the HAL, block is IOBlockStorageDevice, power is IOPM, and
`NativeHandle` values are Mach port names.

### 20.4 Kernel-specific residue

All of DeviceKit's backend and the device manager. Its front, the records and
calls in §4–18, stays the same across kernels; its ABI may still change at an
OS release (§3.5).

## 21. Consumers

### 21.1 Ember

Ember on Tessera uses its Vulkan backend for rendering and DeviceKit for the
rest.

| Ember needs | DeviceKit |
|---|---|
| pick the adapter | `Gpu.list`, matched to `VkPhysicalDeviceDrmPropertiesEXT` |
| scanout-capable formats | `Gpu.formats(Scanout | Render)` |
| a shareable render target | Vulkan image exported as dma-buf, then `Surface.adopt`; or `Surface.create` where the adapter allocates |
| import a surface into Vulkan | `Surface.nativeHandle` into `VkImportMemoryFdInfoKHR` |
| a GPU-done signal | `Fence.attachNative` with the exported sync-fd of the submit |
| wait for the compositor's release | `Fence.nativeAt` into an imported semaphore, or `Fence.waitAsync` |
| give a buffer to MosaicDisplay | `Surface.export`, token over the `tessera.display` channel |
| memory budget and resets | `Gpu.memoryUsage`, `RESET` |

### 21.2 MosaicDisplay

`Device.open(Display, Master)`, `Display.resources`, `Display.framebuffer` on
imported client surfaces, `Display.commit` with in-fences and `FlipEvent`,
`Display.timeline` for release fences, `Display.lastFlip` for pacing,
`Registry.watch` for input hotplug, `Input.read`, `Input.setLeds`.

### 21.3 LibTessera's services

- Devices: `Registry.match` and `Registry.watch` for `Device.list` and `deviceChanged`; Audio, Camera, Sensor and Haptic behind `Stream.*`.
- Power: PowerSource, Thermal, Backlight behind the `PowerInfo` page; WakeSource behind `WakeLock`; Rtc and Suspend behind `Power.scheduleWake`.
- Volume: Block and Mapper behind the Namespace service and the sealed system volume.

## 22. Changes to LibTessera

These edits to [16](16-libtessera.md) go with this document.

1. §4: `Subsystem.DeviceKit = 10`, and the range `ErrorCode` 100–139 reserved for DeviceKit.
1. §5.2: `CompletionKind.Device = 11`.
1. §17: DeviceKit sits below LibTessera's services and Ember, and DeviceKit takes a LibTessera Port as the destination of its completions.
1. A private RIDL module `LibTesseraBackend` with `Port.attachSource` (§6.3), linked only by DeviceKit.
1. The service back halves in 16 §21.5 that touch hardware (Devices, Power, Namespace's volume half, Display) go through DeviceKit.

## 23. Call index

Sync unless marked A (asynchronous, (port, key) last). 120 calls.

Ref (5) Ref.close, Ref.duplicate, Ref.info, Ref.setName, Ref.waitAsync A
Registry (8) Registry.match, Registry.lookup, Registry.children,
Registry.parent, Registry.properties, Registry.property, Registry.watch,
RegistryWatch.read
Device (3) Device.open A, Device.setPowerState, Device.entry
Surface (12) Surface.create, Surface.adopt, Surface.info, Surface.lock A,
Surface.unlock, Surface.map, Surface.unmap, Surface.setPurgeable,
Surface.inUse, Surface.export A, Surface.import A, Surface.nativeHandle
Fence (9) Fence.create, Fence.value, Fence.signal, Fence.waitAsync A,
Fence.attachNative, Fence.nativeAt, Fence.nativeHandle, Fence.export A,
Fence.import A
Gpu (5) Gpu.list, Gpu.formats, Gpu.memoryUsage, Gpu.setPriority, Gpu.priority
Display (10) Display.resources, Display.framebuffer, Display.commit (A with
FlipEvent), Display.timeline, Display.lastFlip, Display.vblank,
Display.setBacklight, Display.backlight, Display.release, Display.acquire A
Input (4) Input.read, Input.axes, Input.state, Input.setLeds
Audio (7) Audio.formats, Audio.configure, Audio.start, Audio.stop, Audio.write,
Audio.read, Audio.position
Camera (9) Camera.formats, Camera.configure, Camera.attach, Camera.queue,
Camera.dequeue, Camera.start, Camera.stop, Camera.control, Camera.controls
Sensor (5) Sensor.channels, Sensor.configure, Sensor.start, Sensor.stop,
Sensor.read
Haptic (5) Haptic.capabilities, Haptic.upload, Haptic.play, Haptic.stop,
Haptic.erase
Power (13) PowerSource.read, Thermal.read, Thermal.trips,
Thermal.setCoolingState, Backlight.read, Backlight.write, Suspend.enter A,
Suspend.inhibitors, WakeSource.create, WakeSource.activate,
WakeSource.deactivate, Rtc.setWake, Rtc.read
Block (9) Block.info, Block.read A, Block.write A, Block.discard A,
Block.flush A, Mapper.create, Mapper.activate A, Mapper.entry, Mapper.status
Hid (5) Hid.descriptor, Hid.readReport, Hid.writeReport, Hid.getFeature,
Hid.setFeature
Usb (7) Usb.descriptors, Usb.claimInterface, Usb.releaseInterface,
Usb.setAlternate, Usb.submit A, Usb.clearHalt, Usb.reset
Drivers (4) Driver.claim A, Registry.publish, Registry.update,
Registry.unpublish

## 24. Decisions still open

1. The Port seam. §6.3 has LibTessera's engine finish DeviceKit completions from readiness and transfers, which keeps decoding in synchronous DeviceKit calls. The alternative is a DeviceKit event thread that blocks on its own sources and posts finished completions through a privileged `Port.postSystem`. It needs `Thread.spawn`, which this tree's LibTessera does not have, and costs a thread per process. I recommend the seam.
1. Token binding. A token is a bearer secret for up to 60 seconds. Binding it to a recipient process at export (`TokenNotForYou`) closes the window where a leaked token is usable by a third process, at the cost of the exporter knowing the recipient. MosaicDisplay is always the recipient of client buffers, so binding is cheap there.
1. Render nodes in the app namespace. The Vulkan driver opens `/dev/dri/renderD*` by path, so a `tessera.gpu` app needs the node in its mount namespace and its seccomp allowlist needs the DRM render ioctls. The alternative is a Vulkan loader layer that takes the node from DeviceKit; Mesa does not support that without a patch.
1. Display leasing. VR headsets and a second session want a lease of one connector and CRTC (`DRM_IOCTL_MODE_CREATE_LEASE`). Not specified until a client needs it.
1. Surface allocation without GBM. `Surface.create` with GPU usages needs a per-driver allocator, which Linux exposes only through GBM in Mesa. Tessera has no libgbm. Ember adopts Vulkan-allocated images instead. A DeviceKit allocator would mean porting the allocation half of GBM to Kira per driver.
1. Loop devices. Presenting a file as a block device needs the file's kernel object inside DeviceKit, so a LibTessera `File` would cross into DeviceKit. The layering allows only a Port to cross. Left out until a disk image needs mounting; the A/B volumes are partitions.
1. Multi-seat. Every entry is `seat0`. A seat model belongs here when Tessera supports more than one person at one machine.

## 25. This implementation

DeviceKit is built in five steps. All five are done.

1. **Display and Input.** The `devicekit` repository is a Kira library built as a component ([18](18-components.md)). It produces the shared library `DeviceKit.framework/DeviceKit` and its RIDL surface. It makes no system call of its own and reaches the kernel through LibTessera's backend seam (see below). MosaicDisplay drives the card with `Display.commit` and `FlipEvent`, and reads the keyboard with Input. It no longer opens a node itself.
2. **The manager, surfaces and fences.** `devicekitd` is its own repository, started by PID 1 before every other service. It listens on the abstract socket `tessera.devicekit`. It opens nodes for the program asking and checks that program against the family (§19, as program paths for now). It also keeps the transfer tokens surfaces and fences cross with. Each application, through AppKit, hands its frames to MosaicDisplay as surface tokens.
3. **Gpu.** `Gpu.list` and Gpu connections give the render node. Ember reaches it through `Surface.adopt`, because on the emulator (venus over virtio-gpu) no adapter reports `AllocatesSurfaces`. venus only emulates sync-fd ([07](07-graphics.md)), and `Fence.attachNative` hits that limit there, so Ember falls back to `Fence.waitAsync` on the CPU.
4. **Media, Power and Block, and the services above them.** LibTessera no longer drives devices. Device.list and Stream are answered by the `devices` service, and Power.read, WakeLock and scheduleWake by the `power` service. Both are DeviceKit clients (16 §25.4 gives their wire). The `volume` service opens every BlockDevice and Partition and probes it, and mounts each file system it finds at `/Volumes/<label>`, as diskarbitrationd does, with `noexec`, `nodev` and `nosuid`. The kernel mounts from the node's path, which is the one place a service names a node. init starts all three after devicekitd. The services boot asserts that each one listens. It also asserts that the Volume service finds the scratch disk as read-only ext4 and mounts it at `/Volumes/scratch`. It watches the registry for BlockDevice and Partition entries, so the media boot plugs a USB stick in through QEMU's monitor and asserts that the service mounts it at `/Volumes/stick`, then pulls it out and asserts that the service lets it go. The volume service unmounts, mounts again and ejects on request, and `diskutil`, after macOS's, asks it to. One services boot runs `diskutil unmount scratch` and another `diskutil eject scratch`, and each asserts the service let go. Eject has the drive let its medium go with `Block.eject`, so the restart boot puts an ext4 CD labelled `disc` in a virtio-scsi drive, runs `diskutil eject disc`, and asserts through QEMU's monitor that the tray opened. The service opens disks read-only, which is all it reads them for, so a CD opens like any other disk. The mobile boot runs `diskutil info scratch`, which describes the volume from the service's list in the shape of macOS's `diskutil info`. The plain services boot runs `diskutil list /dev/vda`, which prints the disk named, or every disk without a name, each followed by its partitions, in the columns of macOS's `diskutil list`, with sizes in powers of 1000 and an asterisk on each whole disk's. The media boot runs `pmset -g`, after macOS's, which prints the Power service's page through `Power.read`. The mobile boot runs `caffeinate -i -t 1`, which holds a WakeLock for a second, and asserts that the Power service logs the lock taken and let go under its reason, as `pmset -g assertionslog` does. The restart boot runs `ioreg -l -c BlockDevice`, after macOS's `ioreg`, which prints the registry's block devices from one `Registry.match` with their properties. `diskutil activity` sends the service a `Watch` request and keeps the channel. The service answers every volume it holds as appeared, then idle, and from then on each volume that appears, is mounted or unmounted, or disappears, as DiskArbitration's callbacks do. A volume that comes appears unmounted and then changes to its mount point, as DiskArbitration reports a disk and then its volume path. The media boot runs it and asserts that it prints the stick appearing, mounted at `/Volumes/stick`, and disappearing. The boot also asserts that the Adaptive Desktop platform reads the power page and plays its startup chime through the Devices service onto the ALSA loopback card. The Devices service watches the registry for AudioEndpoint, Camera and HapticDevice entries too, and posts every device that comes or goes as `deviceChanged` through the broker onto the System channels of the processes entitled to its class. The multi boot plugs QEMU's `usb-audio` speaker in and pulls it out, and asserts that the platform hears it added and removed, by the name `Device.list` gives it.
5. **Hid, Usb and Drivers.** Hid is hidraw: the report descriptor and feature reports are its ioctls, and a report is one `read` or `write`. Usb is usbfs: the descriptors are read from the node, and claiming, alternates, halts, resets and control, bulk and interrupt transfers are usbdevfs ioctls. devicekitd answers manager ops 4 to 7: Claim, Publish (and update), Unpublish and Published. Once a second it matches the registry against its driver manifests and starts a driver for each entry newly matched, with a claim token in its arguments. Redeeming the token lets that connection open the entry for Hid or Usb. The sample driver is the `ccid-driver` repository. The services boot attaches QEMU's `usb-ccid` reader to an xHCI controller, and asserts that devicekitd starts the driver, that the driver finds the reader's CCID interface and reads slot 0 as empty with `PC_to_RDR_GetSlotStatus`, and that it publishes a Driver entry under the reader. It also attaches QEMU's `usb-tablet`, which the kernel's usbhid drives. The `tablet-driver` manifest may detach a kernel driver, so the boot asserts that the driver claims the tablet's HID interface from usbhid, reads its report descriptor with a control transfer, and publishes.

Where this differs from the sections above:

- **The seam is wider than `Port.attachSource`, and it is in `LibTessera`.** A Kira package exports one RIDL module, so the seam is in LibTessera's own module, not in `LibTesseraBackend`, and no application binding calls it. Besides `portAttachSource` it has the object calls DeviceKit's backend and devicekitd make: `backendOpen`, `backendClose`, `backendRead` and `backendWrite`, at an offset or not, `backendControl` (an ioctl on Linux), `backendObjectControl`, `backendDirectoryRead`, `backendReadLink`, `backendMap` and `backendUnmap`, `backendSeek`, `backendSyncData`, `backendPoll`, the event, memory and timer objects, and the socket calls. Each takes and answers the backend's own numbers and layouts and carries the backend's code for a failure in `detail`. Neither DeviceKit's backend nor devicekitd imports a kernel package or declares a system call. They still know Linux, as §20.4 allows: errno values, ioctl numbers, sysfs and `/proc` paths and kernel struct layouts. The rest of each takes from LibTessera what LibTessera already offers: the clock from `Time.read`, randomness from `Random.fill`, the architecture from `System.info`, a driver's arguments from its `LaunchInfo`, and devicekitd starts drivers with `Process.launch` and hears them end on its port.
- **Entitlements are program paths.** devicekitd grants a family to a program by the path of its executable. `/bin/mosaicdisplay` gets Display, Input, Haptic and Backlight. `/bin/devices` gets Audio, Camera, Sensor and Haptic. `/bin/power` gets PowerSource, Thermal, Backlight and Rtc. `/bin/volume` gets Block. Render nodes are open to every program. A driver gets Hid or Usb only on the entry it claimed, which is what `tessera.driver` grants. This changes when the Launcher grants entitlements from signed images.
- **Services are reached through a broker.** The `bootstrap` program implements `System.connectService` and `listServices` (16 §25.4). The Devices, Power and Volume services check in with it as `tessera.devices`, `tessera.power` and `tessera.volume`, and only their own programs may. devicekitd is not one of them: DeviceKit sits below LibTessera, and its manager keeps its own socket, `tessera.devicekit`.
- **Eject forgets a disk; it does not move one.** DeviceKit's Block family has no eject request, so `Eject` unmounts every volume on the disk and leaves the disk out of the list and unmounted until the service starts again. A drive whose medium the kernel can eject, such as a CD tray or a USB card reader, is not told to.
- **init passes arguments.** A service in init's table carries its arguments, its own name first, as launchd's `ProgramArguments` does. This is how a boot runs `diskutil` with a verb.
- **The Devices service mixes nothing.** An audio output stream is one open PCM. A second stream on the same output fails `Busy`.
- **Mapper arguments are dm-table text.** A `MapperTarget` is `kind` u32 @0, `start` u64 @8 and `length` u64 @16 in bytes, and `argumentLength` u32 @24, followed by the kernel's own table parameters for the target as text at @32. The next target starts at the next 8-byte boundary.
- **The registry is not shared.** Each process walks sysfs itself, rather than reading a snapshot the manager publishes. It keeps the walk, and a uevent socket opened before it. The next call walks again only when a uevent has arrived since, or the socket overran. An entry's id is FNV-1a 64 over its sysfs path, so every process sees the same ids. `Registry.watch` is a `NETLINK_KOBJECT_UEVENT` socket of its own that walks again on every uevent.
- **Published entries are the one shared part.** They live in devicekitd. A process asks for the list (op 7) with the generation it holds and gets it, in a memfd, only when that is stale. It merges the list into its walk, each entry under its parent's path so tree order puts it there. devicekitd hands its own DeviceKit the list with `Registry.adoptPublished` rather than asking itself. A published entry's id is the parent's mixed with a serial, top bit set, so no walk makes it. A publish carries its properties as a match spec's property records (§7.4) after a `u32` count, in a memfd.
- **Driver manifests are plist personalities.** A driver ships as a DriverKit bundle, `/System/Library/DriverExtensions/<name>.dext`, holding its executable and an XML `Info.plist`. The plist names the executable under `CFBundleExecutable`, and each dictionary under `IOKitPersonalities` is one manifest (§18.2): `IOProviderClass` is the entry class by DeviceKit's name, such as `UsbDevice`; `idVendor` and `idProduct` are what it matches, and `bInterfaceClass`, `bInterfaceSubClass` and `bInterfaceProtocol`, each optional, the kind of interface a matched `UsbDevice` must have: such a manifest matches the UsbInterface entries with all of them and serves the device each belongs to, since a driver opens the device; `IOProbeScore` is the score; and `TesseraMayDetachKernelDriver` is `detachKernel`. A manifest matches by vendor, product and interface kind only, and does not name a `service`. devicekitd reads every bundle once when it starts, as kernelmanagerd reads the extensions at boot. It logs each bundle it loads and leaves out one it cannot read. The plist sits in the driver's repository, and `stage_driver` in build-initramfs.sh puts the bundle together. When several manifests match one entry, the highest score wins. The manager says in its answer to a driver's Usb open (flags @48, bit0) whether the driver's manifest may detach a kernel driver. Only then does `Usb.claimInterface` take an interface a kernel driver holds, with `USBDEVFS_DISCONNECT_CLAIM`. Another program's usbfs claim is never taken. Releasing the interface gives it back to the kernel. The token rides in the arguments as `--claim=` and 32 hex digits, which `Driver.launchToken` reads. A driver is started once per appearance of its entry. One that exits is started again only when its device comes back, so a crashing driver does not spin. Its published entries go when its connection to devicekitd closes.
- **Transfers finish during `submit`.** A Usb transfer runs to its end or its timeout in the call, and its `Transfer` completion is posted at once. Isochronous transfers answer `Unsupported`. Claiming an interface a kernel driver holds fails `DeviceBusy` unless the manifest has `detachKernel`.
