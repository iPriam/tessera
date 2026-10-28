# 13 — Security and updates

Tessera describes itself as sealed. It is not, and the gap has a name.

## Where it stands

- The system volume is read-only squashfs. **Read-only is true.** It gives way
  to a read-only btrfs snapshot sealed through composefs
  ([05](05-storage-and-state.md)), which is how the seal is made below.
- **Trusted is not true**, and `HANDOFF.md` is right to say the seal is absent
  rather than stubbed. A stub would be worse: it would look like a seal.
- Everything runs as root, because the `setuid` family is not in the syscall
  table.

## The threat model, stated

Without one, "security" is a word that justifies any amount of work.

The shape of Tessera's is unusual and worth stating precisely, because the system
and the application layer have opposite postures ([01](01-principles.md)): the
system is closed and entirely Tessera's own software; the application layer runs
code a person chose and Tessera did not write.

So there are two boundaries, not one, and they need different defences.

**Boundary one — is this the system it claims to be?**

Defended against:

- A system volume modified by a failed write, a bad disk, or someone with the
  image, booting as though it had not.
- An update that installs a volume that is not the one it was meant to be.

Not defended against:

- Someone with physical access who can replace the bootloader or the kernel.
  There is no hardware root of trust in a VM on a Mac, and pretending otherwise
  is the failure mode this section exists to avoid.
- A compromised build machine. The system image is trusted because of where it
  came from and nothing further.

**Boundary two — what can an installed application do?**

This one exists because applications are the exception to the closed system: a
person installs software Tessera did not write ([12](12-apps.md)).

Defended against:

- An application whose bytes changed after the owner accepted it being launched
  as though nothing had happened.
- An application reading or writing another application's data.
- An application reading the system volume, or naming any path outside its own
  image and data directory.
- An application reaching a device, a socket or a capability its manifest did
  not declare.
- An application making a syscall it has no business making.

Not defended against:

- An application doing something unwelcome with exactly the capabilities it
  declared and was granted. A notes editor that was given a data directory can
  fill it with rubbish. Capability *granting* is a decision a person makes; the
  system's job is to make sure nothing exceeds what was granted.
- An owner who accepts a key they should not have. Tessera can tell them who
  signed something; it cannot tell them whether to trust that person, and a
  system that claimed to would be lying.
- The owner themselves. A person with physical possession of their own computer
  is not an adversary, and designing as though they were is how an operating
  system becomes an appliance.

**An earlier draft of this roadmap said the opposite** — that a malicious
application was out of scope because applications ship with the volume, so if the
volume is trusted, so are they. That reasoning was sound for a system where no
foreign code ever runs and is void now. It is recorded here because it is the
kind of assumption that survives in code long after the sentence that justified
it has been deleted.

## Cryptography, which Tessera does not have

Signature verification is now on the critical path — an application cannot be
installed or launched without it ([12](12-apps.md)) — and there is currently no
cryptography of any kind in this system. Not a hash, not a signature, not a
source of randomness (`getrandom` is not in the syscall table).

What is actually needed, and it is a short list:

- **A hash.** SHA-256, for fs-verity digests and for image digests.
- **A signature verification.** Ed25519 is the right choice: small keys, small
  signatures, no parameter choices to get wrong, and verification is the only
  operation Tessera needs — **it never signs anything.** Signing happens on the
  build machine, with a key that never reaches the device.
- **`getrandom`**, for anything that needs a nonce later. Not needed for
  verification.

Verification-only is a large simplification and should be defended: it means no
key generation, no key storage, no side-channel concerns worth the name, and a
public key that can be compiled into the system volume where it is covered by
the volume's own seal.

**Where the code comes from** is the one place [01](01-principles.md)'s "Tessera's
own software" rule needs a considered answer. Writing Ed25519 from scratch is
exactly the kind of thing that looks done and is subtly wrong.
The SDK takes no C ([14](14-tooling-and-ports.md)), so it is not vendored as C
either: it is a Kira port of a small, audited reference implementation, line for
line, and it must pass that implementation's test vectors and RFC 8032's before
anything trusts it. A port checked against vectors is not a design of Tessera's
own, which is the part that goes subtly wrong. Saying so rather than quietly
writing one is the honest move.

## Trust in application signatures

**The machine belongs to the person using it.** A signature tells them what they
are running. It does not decide whether they are allowed to run it.

That distinction is the whole of this section, and an earlier draft got it
wrong — it proposed a single Tessera key and a curated set, which is a gate the
owner cannot open. `HANDOFF.md` §7 already decided Tessera is macOS-shaped rather
than ChromeOS-shaped, and a locked application layer is neither; it is
iOS-shaped, and nobody asked for that.

### What verification is for

- **Identity.** This image was signed by a key, and here is which one. A second
  version from the same developer is recognisably from the same developer.
- **Integrity.** This image is byte-for-byte what was signed. An image that
  changed after the owner accepted it is refused — that is tamper detection, and
  it serves the person whose machine it is.

### What it is not for

- **Permission.** There is no key Tessera requires and no mandatory central
  signer or notarisation service. The [App Store](25-app-store.md) may curate
  its catalogue; applications obtained elsewhere follow the same signing and
  installation rules. Store inclusion is not permission to run.
- **A developer mode.** Running code you wrote on a machine you own is not a
  special state that needs a flag on the console. It is Tuesday.

### The model

1. Any image can be installed. If it carries a signature, it is verified and the
   key recorded.
2. A key the owner has accepted before installs without ceremony.
3. A new key, or no signature at all, is shown to the owner once — who signed
   it, or that nobody did — and they decide. The decision is remembered, pinned
   to that key.
4. An image whose bytes stop matching what was accepted is refused, loudly. This
   is the only refusal the owner cannot wave through, because it means something
   changed behind their back and that is exactly the case they need told.
5. **The sandbox applies either way** ([12](12-apps.md)). A signature is not a
   permission slip and never buys an application more access.

Self-signed is normal, not a warning state. A developer generates a key, signs
their images with it, and their machine — and anybody else's who accepts that
key — installs them like anything else.

### How the system is sealed

As APFS seals a Mac's system volume: one signed value covers every file, name,
mode and directory of the system, and the kernel checks each file as it is read.

- Every file of the system has fs-verity on. Its fs-verity digest, SHA-256 over
  a Merkle tree of its contents, is fixed when it is written; the kernel refuses
  a read of a block that does not match.
- The system's composefs image is an EROFS image of the whole tree, which
  records for each file its fs-verity digest. Its own fs-verity digest is the
  system's digest.
- The build signs the system's digest with Ed25519. The public key is in the
  initramfs, beside PID 1, whose image the boot loader loaded.
- PID 1 reads the digest of the composefs image from the kernel, checks the
  signature over it, and mounts the system with overlayfs, the image as its
  metadata layer and the snapshot as its data layer, with `verity=require`. A
  system whose signature does not check, or whose image is not the one signed,
  refuses to boot and says which.

The build computes fs-verity digests and writes the EROFS image itself, in
Kira; nothing on the build host has to be Linux.

A dev machine ([18](18-components.md)) boots a writable snapshot without this,
and says "unsealed" at every boot.

### Where the seal actually stops

The system volume is sealed because a person wants to know their operating system
was not tampered with. Their applications are their business. The seal covers
Tessera's own software and stops at the boundary in [01](01-principles.md) — that
is not a limitation of the mechanism, it is the correct extent of it.

## Cryptography, which Tessera does not have

Signature verification is on the path — an application's integrity cannot be
checked without it — and there is currently no cryptography of any kind in this
system. Not a hash, not a signature, not a source of randomness (`getrandom` is
not in the syscall table).

The list is short:

- **A hash.** SHA-256, for fs-verity digests and image digests.
- **Signature verification.** Ed25519: small keys, small signatures, no parameter
  choices to get wrong. **Tessera verifies and never signs** — signing happens
  wherever the developer is, with a key that never reaches a device.
- **`getrandom`**, for nonces later. Not needed to verify.

Verification-only is a large simplification: no key generation, no key storage,
no side channels worth the name.

**Where the code comes from** is the one place [01](01-principles.md)'s
"Tessera's own software" rule needs a considered answer. Writing Ed25519 from
scratch is exactly the kind of thing that looks done and is subtly wrong.
The SDK takes no C ([14](14-tooling-and-ports.md)), so it is not vendored as C
either: it is a Kira port of a small, audited reference implementation, line for
line, and it must pass that implementation's test vectors and RFC 8032's before
anything trusts it. A port checked against vectors is not a design of Tessera's
own, which is the part that goes subtly wrong. Saying so rather than quietly
writing one is the honest move.

## Updates

System snapshots, the A/B decided in [01](01-principles.md) generalised, constrained by the layout
in [05](05-storage-and-state.md).

### The mechanism

Systems are read-only btrfs snapshots ([05](05-storage-and-state.md)). One is
current; an update adds another.

1. Write the new system into `system/<version>`: its files, fs-verity on for
   each, and its composefs image, then make it a read-only snapshot. Files the
   current system already has unchanged are reflinked from it, so the update
   writes only what changed.
2. Verify it: its composefs image against the signature, and the files through
   `verity=require` as they are read.
3. Record the new system as *pending* in `boot/`, with a boot counter.
4. Reboot into it.
5. If it reaches a known-good point, mark it current and delete the system
   before the old current one. If it does not, the counter runs out and the
   machine boots the current one again.

Step 5 is what makes this a rollback rather than a hope. "A known-good point" has
to be defined and it should be late — the display server running and the platform
drawing, not merely PID 1 alive, because the failures worth rolling back from are
exactly the ones that leave init working and nothing else.

### What "current" is recorded in

Not in a system, because systems are read-only and interchangeable: in `boot/`,
at the top of the volume, beside them. It is the one
mutable thing the boot path reads, and it is therefore the one thing an attacker
would change — which is another reason the chain above matters.

### Interaction with refuse-to-boot

They compose well, and it is worth being explicit. Refuse-to-boot is what happens
when the *current* system will not mount or verify. Rollback is what happens when
a *pending* system boots and does not get far enough. A machine that has
exhausted both — no good system at all — refuses, says why, and stops. There is
no recovery mode by design ([01](01-principles.md)).

### The data across updates

Never reformatted. An update changes the system and leaves the data, which means
the data format is a compatibility surface and the first schema change is where
that becomes real. `HANDOFF.md`'s note about `renameat2` and `fdatasync` earning
their place is about exactly this.

## Privilege separation

Everything runs as root, and with installed applications that stops being
acceptable rather than merely untidy.

The order that follows from [12](12-apps.md):

1. **Descriptor passing** — available now, no kernel machinery, prevents every
   accident and no attack.
2. **A mount namespace per application** — required before any downloaded code is
   run for real. This is the one that matters, and it is `CLONE_NEWNS` plus a
   `pivot_root` that PID 1 already knows how to do.
3. **A uid per application, and seccomp** — so the kernel's own checks agree with
   the model rather than merely permitting it.
4. **The display server as a non-root process**, last. Separating the one
   privileged service from a set of *confined* applications is worth doing;
   separating it from a set of equally privileged ones would have been theatre,
   which is why it was not first.

Step 2 is the honest gate on shipping the ability to install anything.

## Lock

A machine that can be walked away from. With one user and no password, "locked"
means "will not accept input until something", and that something has to be
decided:

- A passphrase, which means storing a verifier and having a keyboard-driven
  entry UI before there is a toolkit to build one with.
- Nothing — no lock at all, which is defensible on a VM and indefensible on a
  laptop.

**A lock screen, and no lock, until there is a passphrase.** A half-implemented
lock is worse than none because it is trusted, so what Tessera has is the
cover a phone without a passcode has: the platform draws a lock screen over
everything at start and on wake, with the time and the date, and any key, a
click or a swipe up opens it. Nothing is verified and nothing is refused, and
the lock screen says so by what it lacks: no padlock, no field, no prompt that
reads as a boundary. It is presentation, owned by the platform
([11](11-adaptive-desktop.md)); it holds no secret and protects nothing.

A passphrase is what turns the cover into a lock. When there is one, it is
entered on this same screen, verified against a stored verifier, and typed
through the trusted path ([08](08-display-server.md)), so no application can
draw the screen or feed it keys. Until then the documentation says plainly
that the lock screen is not a lock.

## Checkpoints

- [x] The lock screen covers everything at start and on wake, opens on a key,
      a click or a swipe, and is documented as not being a lock. Both adaptive
      platforms start locked; the platform closes it when MosaicDisplay
      blanks the output for want of input, so it is what the output wakes to.
      `-services-mobile` opens it with a key, waits out twenty seconds of
      display sleep, asserts it closed as the output blanked, wakes the output
      with a key that reaches no client, and opens it again with a click. A
      swipe opens it the same way, from the touch that starts it. This
      section says it is not a lock.
- [ ] SHA-256 and Ed25519 verification exist, are vendored rather than written,
      and are named on the substrate list. They are not vendored: with no C
      in the SDK they are Kira ports, as "Where the code comes from" says,
      so they are Tessera's and not substrate. LibTessera's `Digest.kira`,
      `Sha512.kira` and `Ed25519.kira` (after TweetNaCl), and
      `KnownAnswers.kira`, which PID 1 runs before it checks the seal:
      FIPS 180-4's SHA-256 and SHA-512 vectors, and RFC 8032's tests 1 to 3,
      each refused once its signature changes.
- [x] A developer signs an image with their own key and it installs.
      `scripts/container.sh` signs Almanac's image with a key of the
      developer's own; Inlay lists it on the plugged volume, its Install
      asks PID 1, which checks the signature against the keys the owner
      accepted ("an image is signed by a key the owner accepted"), installs
      it, and Inlay moves it to Installed from Disk Images.
- [x] An unsigned image installs, after the owner is told it is unsigned, as
      [12](12-apps.md) records.
- [x] An image that changed after being accepted is refused, and says so, as
      [12](12-apps.md) records.
- [x] No key is required by the system, and there is no path by which the
      project could refuse someone else's software. No key is built in:
      `init/app/Supervise/Signature.kira` reads accepted keys only from
      `/data/system/keys`, which the owner fills by installing. A new key or
      no signature goes ahead on the owner's second request, and the only
      refusal is an image whose bytes do not match its own signature.
- [x] An application runs in a mount namespace that cannot name the system
      volume. It runs in one of its own that holds the runtime part of the
      system volume read-only, and none of the rest ([12](12-apps.md)).
      Nor the disk under it: its `/dev` has no block device, its `/proc`
      shows no process outside its PID namespace, and it holds no
      capability to mount one back.
- [x] The build computes every file's fs-verity digest and writes the
      composefs image, in Kira. Archive's `FsVerity.kira` and
      `Composefs.kira`, and the Build library's `composefs` action.
- [x] PID 1 checks the signature over the image's digest and mounts the
      system through it with `verity=require`. `init/app/Volume/Seal.kira`
      checks the Ed25519 signature over the digest the kernel answers for
      the composefs image, and `System.kira`'s `mountSealed` mounts the
      overlay with `verity=require`. The boot test requires "mounted the
      system sealed".
- [x] A modified system refuses to boot, or refuses the read of the file that
      changed, and says which. `kira build boot -tampered` installs the
      system with a file its seal does not cover: the signature is over the
      system's composefs image and the image beside it lists the extra file.
      PID 1 refuses to boot it, naming the system and the digest it read
      ("system/<version>'s image is not the one signed").
- [x] The documentation states plainly that the root of trust is absent.
      `README.md`, "What it trusts".
- [x] `tessera.shell` is gone, or the build says "unsealed" at every boot.
      It is gone: PID 1 reads only `tessera.autoexit`, `tessera.system` and
      `tessera.restart-test`, and the initramfs carries no shell program.
- [x] An update writes a new system snapshot, reboots into it, and marks it
      good. The installer, given `tessera.update`, writes the new
      `system/<version>` and records it in `boot/trial`, keeping the current
      one. PID 1 writes `boot/tried` and boots the trial; the system's init
      makes it `boot/current` once its services are up ("marked the system
      good"). `kira build boot -update` checks it.
- [x] An update that does not reach the platform rolls back on its own. A
      trial PID 1 finds already tried is dropped with its system, and the
      kept one boots ("rolled back an update that did not reach its
      services"). `boot -update` installs the tampered system as the update:
      its first boot is refused, and the next rolls it back.
- [x] The data volume survives both. Neither touches `data`; the boot after
      the rollback reads the log the update's boot wrote ("read the last
      boot's log"), and `boot -update` requires it.
