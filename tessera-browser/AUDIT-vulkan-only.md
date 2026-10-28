# Vulkan-only build audit

Historical snapshot before the conversion. The buffer decision is now resolved;
see README.md for the design and subsequent status. Source references below
describe the earlier tree, not the converted implementation.

Checked on ed, 2026-10-22. Conversion is blocked; configure success is not
compile, link, dependency-closure or rendering verification.

## Configure

New directory: `/root/Dev/webkit-build/wpe-aarch64-nogl-sol-vulkan-only`.
Source: `/root/Dev/webkit-nogl`, base commit
`7b230e3bac531305c5a3fae01cf348360f3b33e5`, with the pre-existing uncommitted
changes. No WebKit source was changed during this audit.

The existing configure script was copied with new output and log paths:

- Script: `/root/Dev/webkit-logs/nogl-sol-vulkan-only-configure.sh`
- Log: `/root/Dev/webkit-logs/configure-nogl-sol-vulkan-only.log`
- Result: `Configuring done`, `Generating done`, `EXIT=0`.

It uses the existing aarch64 Tessera LLVM toolchain and Debian arm64 sysroot.
Generated `cmakeconfig.h` has `ENABLE_WEBGL=0`, `USE_GBM=0`, `USE_LIBDRM=0`,
`USE_LIBEPOXY=0`, `USE_VULKAN=1`. A search of `build.ninja` found no direct
shared-library paths matching EGL, GL, GLES, epoxy, GBM, DRM, Wayland or X11.
That search does not prove the transitive dependency closure.

## Source blockers verified on ed

Paths below are relative to `/root/Dev/webkit-nogl/Source`.

- `WebCore/platform/graphics/skia/SkiaVulkanDevice.cpp:129-130,231-233`
  accepts a CPU Vulkan device when no higher-ranked device is available.
  A Vulkan-only build must reject software devices, not merely prefer hardware.
- The same file at `136-148` permits Vulkan to be disabled or initialization
  to fail. `PlatformDisplaySkia.cpp:475-482` then attempts GL. Missing Vulkan
  must instead fail explicitly in the no-GL build.
- `WebKit/WebProcess/WebPage/CoordinatedGraphics/AcceleratedSurface.cpp:617-619`
  forces Vulkan frames to shared memory.
- The same file at `449-456` falls back from a failed GPU render target to
  a bitmap surface; `463-475` reads rendered pixels into shared memory.
  Neither is a permitted presentation path.
- `WebKit/WPEPlatform/wpe/WPEDisplay.cpp:34,78-82` still includes epoxy and
  carries EGL display state despite the no-GL CMake gates.
- `WebKit/WPEPlatform/wpe/WPEBufferDMABuf.cpp:30,61,71-86` embeds EGL image
  import. CMake currently excludes this file rather than replacing its EGL
  implementation. No Vulkan buffer replacement has been implemented.

## Buffer decision needed

The README leaves the Vulkan WPE buffer type undecided. Choose one:

1. Make the existing `WPEBufferDMABuf` descriptor carrier EGL-free under the
   no-GL build; keep its file descriptors, offsets, strides, format and modifier.
2. Add a Tessera-specific `WPEBuffer` subclass carrying the exported Vulkan
   allocation and layout, with matching WebProcess/UIProcess IPC.

Either choice requires an exportable Vulkan render target, GPU completion and
release/reuse handling, and the Tessera backend/helper. Neither may use SHM
presentation, CPU readback, GL, GBM or libdrm.

The Tessera-side transport already exists: DeviceKit
`app/Surface/Token.kira:63-80` exports surfaces as tokens, and display-client
`app/Channel/Protocol.kira:77-95` carries those tokens in `DisplayAttach`.
Ember's `app/Backend/Vulkan/VulkanExport.kira:4-18,56-85,114-132` provides a
precedent for dedicated, linear, DMA-BUF-exportable Vulkan allocations.
These were read, not modified or runtime-tested. DMA-BUF use does not itself
require linking GBM or libdrm.

## Verification limits and unchanged files

No compilation or link was attempted during this audit. No new WPE shared
libraries exist from this run, so `readelf -d` closure verification was not run.
Runtime rendering remains unverified; the handoff records that aarch64 runtime
rendering cannot be observed on ed.

No WebKit patch set was produced: this audit did not modify its source or
claim ownership of its existing uncommitted changes. Existing source trees,
build directories and logs were left intact. No packages were installed.

The requested local fallback-wording search found only prohibitions in
`tessera/AGENTS.md` and this package's README/handoff, not instructions to use
a software renderer. No Tessera file needed a wording fix.
