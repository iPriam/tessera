# 30 — Storage management

The person should know what uses the device's space, have a safe way to reclaim
it, and be able to prepare and remove an external drive. Storage management
joins Settings' usage view, Inlay's files and Trash, and Disk Utility's devices
and volumes. They must describe the same storage rather than maintain competing
estimates and independent erase paths.

The existing Volume service and disk utilities provide discovery, mount and
eject operations. This chapter plans trustworthy accounting, cleanup and real
format/partition workflows. A drawn recovery or Disk Utility control does not
establish that its operation is implemented.

## Explain space

Settings shows total capacity, used and available space, plus categories for
system versions, applications, app data, documents, media, caches, Trash and
backups where present. Categories identify what can be inspected or removed.
An unknown or unclassified amount stays visible, so incomplete accounting
cannot look like unexplained free space.

Applications are listed with executable size and retained data separately.
Large-file views can be scoped and sorted without exposing unrelated private
app data. Cache cleanup explains which caches are safely disposable and which
items require a rebuild or new download. BuildCache follows its existing
explicit-cleanup rule ([05](05-storage-and-state.md)); it is not silently treated
as an ordinary evictable cache.

The filesystem may share blocks across snapshots, reflinks and installed
versions. Logical file size and physically reclaimable space are different
measurements. The UI labels estimates, avoids counting shared extents as wholly
owned by every category, and does not promise that deleting a large logical
file frees that exact number of bytes. Filesystem free space is authoritative
for whether a new operation can proceed.

## Cleanup

Recommendations lead to an inspectable list: large documents, old downloads,
Trash, unused apps or removable caches. The person chooses items before a
delete operation. App removal uses [36](36-app-lifecycle.md)'s uninstall flow,
including retained-data choices; document cleanup uses Inlay's Trash rules.
Private data is not removed just because an app has not been opened recently.

Space-pressure handling may evict disposable caches, with per-owner accounting
and a recorded reason. It must preserve user documents and the ability to
recover or finish an in-progress write. Install, recording and download flows
reserve or check the space they need and report exhaustion without leaving a
completed-looking partial result. Successful cleanup refreshes the affected
categories and reports what was actually reclaimed.

## Drives, partitions and safe removal

Disk Utility presents a hierarchy of physical device, partition and mounted
volume, with capacity, format, label, mount state and read-only state. It
identifies the target consistently with Inlay and physical attachment events.
The person can mount, unmount, rename where supported, erase a supported volume
or initialise a selected device with a supported partition scheme and filesystem.

Before destructive work, show the exact device or volume, the scope of data
loss and any other partitions affected. A whole-device erase and a volume-only
erase are different operations. Re-identify the target immediately before
execution; a removed drive's reused device number cannot redirect an operation
onto another drive. The running system, mounted app images and active backup
targets have explicit busy/protection rules.

Eject first stops new work, identifies open uses and asks services to release
their resources. It flushes completed writes, unmounts affected volumes and
requests hardware eject when supported. A busy or failed flush is reported
before claiming the device can be removed. A successful eject disappears from
the mounted list and is reflected everywhere, including search and document
dialogs. Surprise removal reports interrupted work without hanging the desktop.

## Ownership and implementation

The Volume service owns authoritative device/partition/volume state and mount
operations. Add typed privileged formatting and partition requests through
RIDL; applications do not spawn a shell command or construct an ioctl payload.
DeviceKit remains the device boundary. Filesystem and partition writers are
implemented in Kira or through explicitly owned system components consistent
with the no-C userland rule; the list of supported operations must match what
can actually be executed and verified.

A storage accounting service performs bounded scans, combines filesystem
statistics with app ownership and snapshot metadata, and exposes progress and
generation-tagged results. It uses change notifications for updates and rescans
on overflow. Settings renders that state; Inlay and Disk Utility receive
the same operation progress and completion records. Long work is cancellable
only at safe boundaries; an operation that cannot safely be cancelled says so.

Build usage and cache accounting first, then inspectable cleanup, then volume
erase on disposable test media. Add whole-device partitioning and recovery
integration after target identity and failure behaviour are proven. Storage
repair and data recovery are distinct features and must not be implied by a
generic First Aid button without an implemented checker.

## Status

Settings' Storage pane shows each volume's free and total space
(`volumeSpaceAt`, statfs). Its Apps pane lists every application with the
space its bundle or image and its documents take, and deletes an application
installed from a disk image either keeping its documents or with them. Inlay
moves documents to the Trash and empties it. Category breakdowns, caches and
drive operations are not started; nothing here is shown on a boot yet.

## Checkpoints

- [ ] Fill a controlled volume with apps, documents, shared extents and caches.
      Settings accounts for it without double-counting claimed reclaimable space.
- [ ] Remove selected caches and Trash, retain app data during uninstall and
      see the actual resulting usage after refresh and reboot.
- [ ] Exhaust free space during a download or save; the existing destination
      survives and cleanup can make enough space to retry successfully.
- [ ] Erase a selected test volume and initialise a separate blank test device.
      Verify partition boundaries, mount the result and persist a file across reboot.
- [ ] Swap or remove a drive between confirmation and execution. The operation
      refuses a changed identity rather than touching a replacement device.
- [ ] Eject a drive with an open document and an in-progress write. Busy work
      is identified, completed writes are flushed, and successful eject is reflected system-wide.
- [ ] Interrupt a formatting operation on disposable media. Recovery reports
      the actual incomplete layout and never presents it as a healthy completed volume.
