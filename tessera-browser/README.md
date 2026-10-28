# Tessera Browser

A minimal browser for Tessera: an address bar, back, forward and reload, and a
WebKit page that fills the rest of the window. The page is rendered by WebKit's
WPE port.

Status: the Vulkan-only WebKit WPE patch set compiles, links and passes the
closure check as an aarch64 cross-build. It has never been run. The Kira app is
not written. See "Status" at the end.

## Rules this depends on

The WebKit exception in `tessera/AGENTS.md` allows WebKit WPE, its direct
dependencies, and the Tessera-owned WPE backend, and nothing else. Two terms
from it matter here.

- No OpenGL, EGL, GLES, libepoxy, GBM, DRM, Wayland or X11 anywhere in the build
  or its dependency closure. WebGL is off.
- Rendering is Vulkan only, through Skia's Ganesh backend. There is no CPU raster
  path and no software fallback. If Vulkan is not ready on a target, that is a
  blocker to report, not a case to fall back from.

## What WebKit gives us

WPE selects its display platform with the `WPE_PLATFORM` environment variable
and loads extra platform modules from `WPE_PLATFORMS_PATH`
(`Source/WebKit/WPEPlatform/wpe/WPEExtensions.cpp`). A Tessera backend is a
platform module of that kind. It owns the window, the buffers and the input.

Frames reach the platform as `WPEBuffer`s. Vulkan frames use an EGL-free
`WPEBufferDMABuf` carrying descriptors, format, modifier, stride and offset.
`WPEBufferSHM.cpp` is a CPU buffer and is not used.
The UI process hands them to `wpe_view_render_buffer`
(`Source/WebKit/UIProcess/wpe/AcceleratedBackingStore.cpp:292`). Input comes
back through `wpe_view` events. The backend translates Tessera's key and pointer
messages into those events.

### Buffer decision

Keep the existing DMA-BUF descriptor carrier and separate its EGL import code
from the no-GL build. DMA-BUF is a kernel memory-sharing interface, not a
requirement to link libdrm or GBM. DeviceKit and MosaicDisplay already use it;
a Tessera-specific WPE buffer class would duplicate the layout, lifetime and
descriptor-transfer contract without adding a needed capability.

The WebProcess renders with Ganesh into dedicated, exportable Vulkan images.
The UI process receives the descriptor and layout through WebKit IPC and
passes the buffer to the Tessera platform module. That module adopts the
allocation through DeviceKit and presents its token through display-client.
A Vulkan `SYNC_FD` rendering fence orders completion before consumer access;
release fences and release acknowledgement control reuse.
No EGL import, mapped pixel presentation or SHM readback is involved. Vulkan,
export or allocation failure is an error, never a change of renderer.

Frame synchronization uses `VK_KHR_external_semaphore_fd`, not a blocking CPU
wait in the compositor. Ganesh signals an exportable binary semaphore; its
sync-file travels in WebKit's existing frame IPC. Release sync-files are imported
as Vulkan semaphores and queued as GPU waits before reacquiring the image.
Missing sync-file import/export support is a rendering blocker. WPE's existing
asynchronous fence monitor can delay delivery until the rendering fence signals
when a platform module does not consume explicit fences itself.

Use the existing linear DMA-BUF layout supported by Tessera's Vulkan importer
first. Other modifiers require importer support and capability negotiation;
they must not be advertised before that support exists. Keep the carrier
platform-neutral and Tessera's window, token and input policy in its module.

The Vulkan producer hands images out in `GENERAL` layout with ownership released
to `VK_QUEUE_FAMILY_EXTERNAL`. The consumer must pair that release with an
external-family acquire before sampling, then release back to external ownership
in `GENERAL` before signalling buffer release. A display `FrameDone` message is
frame pacing, not permission to reuse GPU memory. The backend must not translate
it directly into `wpe_view_buffer_released` without GPU-completion evidence.

Ember's importer (`kira-projects/ember`, `VulkanImport.kira`,
`VulkanLayout.kira`) now acquires from `VK_QUEUE_FAMILY_EXTERNAL` and takes its
memory type from `vkGetMemoryFdPropertiesKHR`. Still open: Ember has no call that
releases an imported texture back to external ownership in `GENERAL` once
sampling has finished, which the buffer-release step above needs. That call
belongs in the browser backend's frame lifecycle. Do not remove the producer's
ownership transfer or rely on implicit driver synchronization.

## Why the build needs changes

Upstream WPE 2.55 cannot be built without GL. Three things force it:

- `Source/cmake/OptionsWPE.cmake:417` sets `USE_LIBEPOXY TRUE` unconditionally.
- `ENABLE_WEBGL` drives `USE_ANGLE` (`OptionsWPE.cmake:422`).
- About 45 translation units in the aarch64 build include `epoxy/`, `EGL/`, GBM
  or DRM headers. Some are the ANGLE tree, which `ENABLE_WEBGL=OFF` removes. The
  rest are WebCore, WebKit and WPEPlatform code, which needs gating or a
  Tessera replacement.

The gating is in a copy of the source, `/root/Dev/webkit-nogl` on `ed`, made
from the working tree of `/root/Dev/webkit` with its uncommitted Vulkan work. The
original tree and every existing build directory are left alone. The change is
in `tessera/build` only as far as the configure flags go. See "Status".

## Build

Everything is built on `ed` (`root@100.85.162.6`, NetBird
`workstation-side-edmond-linux`), as an aarch64 cross-build with the Tessera LLVM
bundle and the Debian trixie arm64 sysroot (`/root/Dev/webkit-logs/aarch64-tessera.cmake`).
Nothing is built on the Mac.

The no-GL configure flags are in `/root/Dev/webkit-logs/nogl-configure.sh`:

    -DTESSERA_NO_GL=ON -DENABLE_WEBGL=OFF -DUSE_VULKAN=ON
    -DENABLE_GPU_PROCESS=OFF -DUSE_GBM=OFF -DUSE_LIBDRM=OFF
    -DENABLE_WPE_PLATFORM_DRM=OFF -DENABLE_WPE_PLATFORM_WAYLAND=OFF
    -DENABLE_WPE_PLATFORM_HEADLESS=OFF -DUSE_GSTREAMER_GL=OFF

The build directory is `/root/Dev/webkit-build/wpe-aarch64-nogl`.

## Closure check

The dependency closure must not contain `libepoxy`, `libgbm`, `libdrm`,
`libEGL`, `libGLES*`, `libGL`, Wayland or X11 libraries. The check is `readelf -d` on
`libWPEWebKit-2.0.so.1` and every library it loads, the same walk used to size the
existing aarch64 build (149 files, 315 MB, before any GL removal).

`tools/check-closure.py` walks `DT_NEEDED`, checks that each ELF is aarch64,
and fails for forbidden or unresolved libraries. Pass private rebuilt library
directories before the Debian sysroot. It inspects target files without running
them. This does not cover `dlopen` modules: the package must also check every
shipped WPE module, Vulkan driver and media plugin as a root and restrict plugin
search paths to the shipped allowlist.

### Dependency profile

Keep accessibility and media support. Do not remove those features merely to
make a closure check pass. Debian's AT-SPI 2.56.2 links X11, and GStreamer's
allocators 1.26.2 links libdrm. Rebuild AT-SPI with `x11=disabled` and the
allocators with `drm=disabled`; their DMA-BUF descriptor support remains usable
without DRM dumb-buffer allocation. The rebuilt ATK bridge, ATK and AT-SPI must
be packaged together rather than resolving AT-SPI from the original sysroot.

`tools/build-dependencies-on-ed.sh` records the source versions, archive hashes,
cross toolchain and options. It requires a new build directory on ed and stages
only into its private prefix. It does not install system packages or change the
existing sysroot. Both rebuilt dependency closures have passed the checker:
22 aarch64 libraries, no forbidden or missing libraries. That is a dependency
check, not yet the final WebKit artifact check.

## Status

Latest result (patch `patches/0001-vulkan-only-wpe.patch`, 58 files, applies on
WebKit `7b230e3`):

- Built on `ed` in `/root/Dev/webkit-build/wpe-aarch64-nogl-sol-vulkan-only`,
  with `tools/configure-on-ed.sh`. `libWPEWebKit-2.0.so.1.12.0`, `WPEWebProcess`
  and `WPENetworkProcess` compile and link, with no failed steps. WPEPlatform is
  linked into `libWPEWebKit`; there is no separate library.
- Closure check (`tools/check-closure.py`, which also checks 64 KiB-compatible
  segment alignment): 124 libraries for `libWPEWebKit`, 126 with the two
  processes, no forbidden, missing, non-aarch64 or misaligned libraries. AT-SPI
  (without X11) and GStreamer allocators (without DRM) come from a private
  prefix rebuilt for this. `libvulkan` is loaded at run time, so the static
  check does not list it.
- No GL, EGL, GBM or DRM translation unit is in the build graph. `about:gpu`
  reports Vulkan only.
- WebGL is off. Video and accessibility are kept.
- Frame hand-off: Skia on Vulkan renders into linear DMA-BUF images and exports
  them through `WPEBufferDMABuf`, with `SYNC_FD` semaphores.
- Not verified: anything at run time. This host has no GPU path to observe
  rendering, so Vulkan device selection, DMA-BUF export and the module loading
  are untested.
- Ember importer: fixed in `kira-projects/ember` (`VulkanImport.kira`,
  `VulkanLayout.kira`). It now acquires imported images from
  `VK_QUEUE_FAMILY_EXTERNAL`, pairing the producer's release, and moves them
  from `Preinitialized` to the sampling layout. `kira check` passes; the
  acquire has not run on a device.
- Not done: the Tessera WPE platform backend, the Kira app, and the WebKit FFI
  binding.
- Build notes: distcc between `ed` and the Windows machine
  (`tools/distcc-launcher-on-ed.sh`). distcc adds `-target <host>`, so the
  launcher passes `-target aarch64-linux-gnu`. The launcher also runs
  `tools/normalize-depfile.py`, because Clang's dependency files name sysroot
  headers through a symlink that Ninja collapses wrongly.

## The Browser application and its payload

`app/` is the Browser, a Kira UI application. It drives WebKit through the
`@FFI.Extern` declarations in `app/WebKit/Foreign.kira` (the one place C is
reached), runs GLib's main context from the session's own turn, imports each
frame the web process drew through Ember and shows it as a texture under an
address bar. `tessera-ui` gives an application a tick that runs each turn with
the session's graphics (`EmbeddedContent`).

WebKit is not built by the Tessera graph. `tools/stage-payload.py`, run on the
build host, lays the engine, the two processes, every library they load,
GStreamer plugins and GIO modules whose closure is clean, fonts and certificates
out as a release root; `tessera/build/app/WebKit.kira` unions it into the system
volume and puts the engine's library in the link sysroot. The payload goes under
`payload/<architecture>` (not committed) or where `TESSERA_WEBKIT_PAYLOAD` names.
`state/` holds the WebKit working tree's changes on top of `7b230e3` that the
patch set does not yet carry.

`kira build boot -arch aarch64 -test -services-browser` starts the Browser on a
data URL and waits for it to say it shows a page.

Not verified: that the pixels are right (the host cannot show them), the
sandbox's view of fonts and GIO modules, and any media decoder: the sysroot's
GStreamer carries only the core elements.
