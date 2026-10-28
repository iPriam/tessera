# AGENTS.md

NEVER try to guess the intent of the user, always do EXPLICITS steps and or instructions given, inferring might means wrong thing and end up in useless work.

Read `HANDOFF.md` and `ROADMAP.md` before working.

## Siblings

This repository assembles Tessera. Every component and Kira dependency is its own sibling repository in
`../`, the flat release train workspace.
All of them are ours: commit a change there, never patch it from here.

Components talk to each other over RIDL only.

`patches/` is for upstreams only.

## Rules

Tessera builds in release by default: every component compiles with KIR's O2
optimizer (`-profile release`, the Build graph's fallback). Pass `-profile
debug` only to chase a fault the release build hides.

Pure Kira only, with one exception: the WebKit exception below.

No C anywhere except the Linux kernel and the WebKit exception. clang is used
for linking, and for building the WebKit exception on ed.

700 lines is a hard ceiling for every `.kira` file. Look for the split at 600,
into cohesive 300 to 500-line modules. `app/bindings/` is generated and exempt.

Use the autobind-generated structs. A field read by byte offset is a field
nothing checks.

Vulkan, never Dawn, GL or X11. The SDK is Kira and RIDL; Vulkan is the one
native library a Tessera program links, besides WebKit under the exception
below.

Paths live in `build/app/Host.kira`.

## The WebKit exception

WebKit built for the WPE port, its direct dependencies, and the Tessera-owned
WPE backend that connects WebKit to MosaicDisplay are the one exception to
"no C anywhere" and to "Vulkan is the one native library". Nothing else gains
it.

- WebKit is built without Wayland, X11, GBM, DRM, and no OpenGL, EGL, GLES or
  libepoxy anywhere in its dependency closure. WebGL is off. Tessera's graphics
  stack has no OpenGL, so the rendering path is Vulkan only (Skia's Ganesh
  backend on Vulkan). There is no CPU raster path or other software fallback.
  The backend is the only WPE platform Tessera ships.
- The backend is C++ and is the only C or C++ source Tessera carries.
- WebKit is built on ed, an aarch64 cross-build over NetBird. It is never built
  on macOS.
- Kira reaches WebKit through FFI declarations in `tessera-projects/tessera-browser`
  only.

## Commits

Commits explain why, are signed and verified, and carry no AI trailer.

Leave nothing uncommitted.

No branches. Work on the default branch and commit there. Never park work on a
side branch.

## Verification

Prove a claim: run `kira build boot -arch <arch> -test` and quote what it printed.

## Prose

When answering the user, always respond in a manner that is easy to read for the user, do not use a theatrical voice, long messages for what is 1 sentence and prefer shorter responses that actually mean things.

Write documentation and commit messages plainly. State the fact and stop.

No aphorisms, no closing flourishes, no sentence whose job is to sound wise.

Say "ends when X", not "**Ends when** X". Bold and em dashes are not emphasis by default.

Cut any sentence that would not change a reader's next move.
