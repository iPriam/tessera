# 31 — Backup, migration and recovery

A person must be able to recover a deleted document, move to another device
and get back to work when the installed system no longer boots. These are
different recovery tasks with different sources of truth. System-update
rollback preserves a known system version; a backup preserves the person's
data independently of that system and its disk.

This chapter builds on the existing snapshot/update model in
[05](05-storage-and-state.md) and [13](13-security-and-updates.md). A snapshot on
the same failing drive is not an independent backup, and a successful copy
without a tested restore is not completion of this milestone.

## Automatic backups

Settings lets the person select a supported destination, name the backup,
choose protected data, set a schedule and inspect the last successful backup.
The initial target is a locally attached writable volume. Missing destinations
defer work and produce a useful status; reconnecting resumes pending backup
without starting an unbounded pile of duplicate jobs.

Back up documents, application data, settings and the identity/configuration
needed to restore their meaning. Installed apps can be represented by accepted
images or trusted acquisition records, with an offline-restoration policy
defined explicitly. Exclude disposable caches and temporary state. Encryption
keys and grants require their own recovery treatment; copying their file bytes
is not assumed to make them valid on another machine.

The backup service produces versioned snapshots of a consistent source view.
Use filesystem snapshots where supported; otherwise coordinate application
checkpoints and record any file that could not be captured consistently. Copy
into an incomplete generation, verify stored objects and commit a manifest
last. An interrupted generation cannot replace the previous successful backup.
Retention removes only committed generations according to the selected policy
and never removes the last usable backup to make room for one not yet complete.

Backup metadata records version, time, source identities, included and excluded
scopes, content digests and errors. Incremental transfer compares stable records
and content rather than modification time alone. Storage sharing is an
implementation choice; every retained generation must remain independently
restorable when an older generation is removed.

## Encryption and verification

Offer encrypted backup with a credential or recovery key that is separate from
the device being backed up. Its cryptographic format, key derivation, integrity
protection and recovery flow need a reviewed design before being enabled.
Never invent an ad hoc cipher or claim encryption merely because filenames are
hidden. Losing all recovery material must have an honest, documented outcome.

Read-back verification detects missing and damaged backup objects. The UI
distinguishes last attempt, last successful backup and last verification.
Partial success lists omitted files; it must not display a wholly successful
backup. Verification errors preserve the remaining generations and let the
person choose a new destination before any repair that discards data.

## Restore individual items

Inlay or a backup browser opens the history of a selected document or folder.
Preview a version and restore it to its original location or another selected
destination. An existing item offers Replace or Keep Both; restore is staged
and committed so an interrupted operation does not destroy the newer copy.
Metadata and associated resources needed by the document are restored together.

Restoring an application's data is separate from copying arbitrary files into
its live container. Stop or coordinate with that app, restore a consistent
generation and check its data-format compatibility. A data migration cannot
be reversed just by installing an older executable. Report unsupported
combinations and permit export of recoverable documents where possible.

## Move to another device

Setup and Recovery offer migration from a supported backup or an explicitly
paired source device. Preview selected apps, documents and preferences, the
required space and anything that cannot move. Preserve stable app identities
and Home arrangements, then adapt display and hardware preferences to the new
machine. Do not copy stale process identifiers or assume identical peripherals.

Device-bound credentials and authorization decisions are re-established under
the documented policy. Pairing keys for other devices are not silently cloned.
An interrupted migration is resumable by operation identity, and the source
stays intact. A migration report identifies skipped data and unsupported apps
instead of presenting a partly migrated device as identical to its source.

## Recover an unbootable system

Recovery provides real Startup Disk, reinstall, backup restore and disk actions.
It can select a valid installed system, recover from a failed trial update or
install a verified system while preserving the data volume where the chosen
operation permits it. The person sees whether an action preserves, replaces
or erases data before it begins. A Terminal placeholder is not a recovery tool.

Recovery uses the same installer, Volume and backup operations as the running
system over typed interfaces. It does not maintain a second implementation of
image verification or filesystem layout. Recovery media must carry the tools
and contracts needed for supported offline operations, including access to an
encrypted backup through the recovery credential.

## Implementation order

1. Define the backup manifest, source consistency and commit protocol; back up
   a controlled document tree to a separate test volume and restore it.
2. Add scheduled incremental generations, retention, interruption recovery,
   read-back verification and Settings status.
3. Add per-document history and coordinated app-data restoration, then reviewed
   encryption and portable recovery material.
4. Integrate offline Recovery restore and reinstall, followed by selective setup
   migration. Pairing-based live migration follows [37](37-continuity.md).

Ends when a deleted document can be restored and a replacement machine can
recover the person's selected work from an independently stored backup.

## Status

Settings' Backup pane backs Documents up onto a chosen writable volume, as
dated snapshots under `Tessera Backups`: each holds the whole tree, files
unchanged since the last snapshot linked to it, and counts only once its
marker is written last, so an interrupted backup is never restored from and
is removed by the next. The newest snapshot's items restore into Documents
beside what is there, and the ten newest are kept. The rules are Inlay's
`FileOperations` (`Backups.kira`), tested on the host. Automatic backups,
per-document history and migration are not started.

## Checkpoints

- [ ] Schedule backups to a separate volume, disconnect and reconnect it, and
      obtain one complete new generation with accurate last-success status.
- [ ] Delete a document and restore an older version with Keep Both. Compare
      its bytes and metadata with the saved generation after reboot.
- [ ] Interrupt backup and restore writes. The previous backup and existing
      destination remain usable; incomplete generations are identified and reconciled.
- [ ] Corrupt or remove a stored object. Verification names the affected
      generations and does not claim they are fully restorable.
- [ ] Rotate retention and restore every remaining generation, including one
      sharing data with a removed generation.
- [ ] Unlock an encrypted backup on a replacement machine using only its
      documented recovery material; an incorrect key cannot produce trusted output.
- [ ] Restore through Recovery after a failed system update, preserving selected
      documents and verifying that the recovered system and app data are compatible.
- [ ] Migrate selected work to a different device and compare records, Home
      placement and skipped-item report. The source remains usable and unchanged.
