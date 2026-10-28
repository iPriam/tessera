# 25 — App Store

Discover software, understand what it does, install it and keep it current.
The App Store is a planned system application using the application image and
installation model in [12](12-apps.md).

## 1. Browse and choose

The store has browse collections, categories and search. A product page shows
the app's icon, name, description, screenshots, publisher, version, release
notes, download size, supported Dynamic Platforms and system compatibility.
Requested access is explained before installation. Installed apps show Open
or Update, and incompatible apps explain why they cannot be installed.

Search distinguishes no results from a connection failure. Cached pages can
still be read offline and say when their information was last refreshed.
Opening a product link reaches the same page inside the store.

- [ ] Browse a collection and a category, search for an app, inspect its page
      and navigate back with the previous search and scroll position retained.
- [ ] A product link opens the intended app page. An unknown or withdrawn
      listing produces a useful unavailable state.
- [ ] A listing incompatible with the system or current platform explains the
      constraint. An app supporting another installed platform says how it can run.
- [ ] Go offline: cached listings remain readable, new requests show an offline
      state, and no stale listing claims a completed download or available install.

## 2. Install and open

Get starts a download with progress and cancellation. The store reports the
stages that matter: downloading, verifying, installing, ready or failed. Leaving
the store does not cancel a download. The same app cannot be installed twice
by repeated clicks, and interrupted work is reconciled on the next launch.

Once installed, the app appears in App Library and in Home according to the
person's placement preference ([24](24-home-and-personalisation.md)). Open
launches that installed version through the ordinary platform path.

The catalogue identifies the expected publisher and artifact. Downloaded bytes
are checked before installation. Store metadata is not a substitute for image
signature verification, and an app is not ready until installation succeeds.
A failed install leaves a clear retry or cancel action and no broken app entry.

Ends when a person can find, install and open an app entirely on Tessera.

- [ ] Install an app from its product page, leave the store during the download
      and open the completed installation from Home and the store.
- [ ] Cancel, lose the network and run out of space during separate installs.
      Each ends in a recoverable state with accurate storage and app listings.
- [ ] A modified download or a publisher mismatch is refused before launch;
      the existing installed app, if any, remains usable.
- [ ] Repeated Get actions and restarting the store during installation result
      in at most one completed installation of the selected app.

## 3. Updates and installed apps

Updates lists available versions, release notes and their download sizes.
The person can update one app or all eligible apps, and choose an automatic
update policy. The store explains additional access a new version requests;
existing consent does not silently grant new access.

An update retains app identity, document associations, Home placement and app
data. A running app is not overwritten in place: the update waits for a safe
activation point and says when closing or reopening the app is needed.
An interrupted update keeps a launchable accepted version. App data migration
must be considered before promising a downgrade to an older version.

Settings' Apps page and the store agree on installed versions and pending
updates. Uninstall distinguishes removing the application from deleting its
saved data, then removes stale launch and Home entries. System components
continue to update as part of the OS rather than through app updates.

- [ ] Update an app and reopen its previous data with its Home placement and
      document associations intact; Settings and the store show the new version.
- [ ] Interrupt an update and restart the machine. One accepted version remains
      launchable, with the update status reconciled and no partial version offered.
- [ ] Update a running app without disrupting its unsaved work; the pending
      version activates at the documented close or relaunch point.
- [ ] Toggle automatic updates, reboot and observe the chosen policy. An update
      requesting additional access follows the documented review before that access is granted.
- [ ] Uninstall with data retained, reinstall and recover that data. A separate
      delete-data choice removes the named app data without affecting another app.

## Distribution and product decisions

The App Store adds a catalogue and an update channel. Direct downloads and
disk-image installation remain supported, with the same confinement and image
integrity rules. A catalogue can decide what it lists without becoming the
authority over everything the owner can run.

App identity must remain stable across versions and be bound to its publisher;
a matching name or icon is insufficient. The store must distinguish apps it
manages from directly installed apps, and cannot silently replace the latter
with a catalogue entry sharing their name. Taking over update management needs
an explicit matching identity and the owner's choice.

[36 App lifecycle](36-app-lifecycle.md) owns the shared identity, registry,
activation and update state. The store presents those operations and their
progress; it does not create separate installations invisible to Settings,
Home, document associations or background-task management.

Before a public store, decide catalogue ownership, publishing and review,
publisher-key changes, withdrawn listings and retention of installed versions.
Accounts, purchases, subscriptions, ratings and reviews need separate product
decisions; this chapter does not assume a billing service or an account requirement.

## Dependencies and evidence

The first complete flow needs network configuration, DNS and HTTPS from
[20](20-networking.md), plus app identity, update activation and installation
support. It can be proven against a controlled catalogue with two app versions;
a static storefront mockup does not close install or update checkpoints.
Publishing to a public catalogue and introducing payments are later work.
