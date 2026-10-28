# devroot

Installs a component's build over a running development machine, as a root
does on a Mac, and takes it off again. It runs only on a machine booted with
`tessera.dev`, as a service PID 1 starts.

A root is the install root of one component build (`scripts/build-component.sh`
in the tessera repository), packed by `scripts/devroot.sh`, which also carries
it into the machine on a volume labelled `devroot`. The volume holds a
`request` file, an `id` line and then `install <name> [force]` or
`uninstall <name> [force]` lines, and a `<name>.root` archive for each root it
installs.

Roots form an ordered stack, newest on top, kept in `/var/db/devroots`: each
root's archive and its manifest (path, kind, mode and SHA-256 of every entry),
and a base layer holding the files that no root owned before one replaced them.

- Installing backs up every file it replaces that no root owns into the base
  layer, then writes its own.
- Uninstalling, path by path: a path a higher root also has only loses this
  root's record. Otherwise the next lower layer's version is put back, from a
  lower root or the base layer, or the file is removed when no layer has it.
  The order roots are removed in does not change what is left.
- A file whose contents no longer match the manifest of the root that wrote it
  is not replaced or removed unless the request says `force`.

After each change the service sends PID 1 the paths it changed, and PID 1
restarts every process that has one of them mapped.

Build it with the tessera repository's scripts.
