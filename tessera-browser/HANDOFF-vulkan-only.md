# Handoff: Vulkan-only WebKit conversion

For the worker that takes the Vulkan-only conversion of the no-GL WebKit WPE
build. Read `README.md` in this package first; it has the design.

## Rules that bind the work

- Rendering is Vulkan only, through Skia's Ganesh backend. No CPU raster path,
  no software fallback, not even temporary. If Vulkan is not ready somewhere,
  report it as a blocker. Do not fall back.
- No OpenGL, EGL, GLES, libepoxy, GBM or DRM anywhere in the build or its
  dependency closure. WebGL is off.
- WebKit is built on `ed` (aarch64 cross-build over NetBird), never on macOS.
- Do not delete or overwrite existing builds on `ed`. Use new build directories.
- Do not revert, clean, stash or commit in `/root/Dev/webkit` or
  `/root/Dev/webkit-nogl`. They hold uncommitted work. Report changes instead.
- Do not touch `~/.config/opencode`.
- No time estimates in reports. Report what was run and what it printed.

## Shared copy and the Sol worker

- The Sol worker is editing `/root/Dev/webkit-nogl` now, and it has its own
  build directory, `/root/Dev/webkit-build/wpe-aarch64-nogl-sol-vulkan-only`,
  configured by `/root/Dev/webkit-logs/nogl-sol-vulkan-only-configure.sh`.
  Its log is `/root/Dev/webkit-logs/configure-nogl-sol-vulkan-only-stage4.log`,
  and its build log is `/root/Dev/webkit-logs/build-sol-wpe-webkit-*.log`.
- Do not run ninja in `wpe-aarch64-nogl` while Sol is building the copy.
  `wpe-aarch64-nogl` was configured before the gates existed and includes
  Sol's in-progress files. Coordinate with the Sol worker before any reconfigure
  or build there.
- Sol's distributed-compile launcher is `/root/Dev/webkit-logs/sol-distcc-vulkan.sh`.
  It is separate from `distcc-tessera.sh`.

## Source copy on ed

- Path: `/root/Dev/webkit-nogl`. A copy of the `/root/Dev/webkit` working tree
  (git repo, uncommitted changes, not committed by anyone for this work).
- Original `/root/Dev/webkit` is not to be modified.
- Uncommitted changes in the copy, including the Vulkan work:
  `Source/WebCore/platform/graphics/skia/SkiaVulkanDevice.{h,cpp}` (new),
  vendored VMA under `Source/ThirdParty/skia/third_party/vulkanmemoryallocator/`
  and `Source/ThirdParty/skia/src/gpu/vk/vulkanmemoryallocator/` (new), and
  edits to `PlatformDisplaySkia.cpp`, `SkiaBackingStore.cpp`,
  `SkiaUtilities.cpp`, `AcceleratedSurface.*`, `ThreadedCompositor.cpp` and others.
  `git status --short` in the copy lists all of them.

## TESSERA_NO_GL state

The CMake switch `TESSERA_NO_GL` exists in the copy. What it gates today:

- `Source/cmake/OptionsWPE.cmake`: no `find_package(Epoxy)`, `USE_LIBEPOXY` off,
  `USE_GBM` and `USE_LIBDRM` not required when GL is off.
- `Source/WebCore/CMakeLists.txt`: the EGL/GLES link block is skipped; a
  `list(FILTER WebCore_SOURCES EXCLUDE ...)` runs before `WEBKIT_COMPUTE_SOURCES(WebCore)`
  and removes `platform/graphics/(android|gbm|egl)/`, `GraphicsContextGLCoordinatedEpoxy.cpp`
  and `CoordinatedPlatformLayerBufferVideo.cpp`.
- `Source/WebKit/CMakeLists.txt`: EGL link block skipped; a filter before
  `WEBKIT_COMPUTE_SOURCES(WebKit)` removes `UIProcess/glib/DRMMainDevice.cpp` and
  `WebProcess/GPU/graphics/gbm/`.
- `Source/WebKit/WPEPlatform/CMakeLists.txt`: no `Epoxy::Epoxy`; a filter before
  `add_library(WPEPlatform ...)` removes `wpe/WPEBufferDMABuf.cpp` and `wpe/WPEBufferAndroid.cpp`.
- `Source/ThirdParty/skia/CMakeLists.txt`: `SK_GL` and the EGL fallback are gated.
- `Source/WebCore/PlatformGTK.cmake`, `PlatformWPE.cmake`, `platform/SourcesSkia.txt`
  carry further edits from the earlier session.

The three `list(FILTER ...)` blocks were applied by
`/root/Dev/webkit-logs/gate-nogl-sources.py`. They have not been compiled yet.
The configured build directory predates them, so it must be reconfigured.

## Build directories and logs on ed

- Configure script: `/root/Dev/webkit-logs/nogl-configure.sh`. It configures
  `/root/Dev/webkit-build/wpe-aarch64-nogl` from `/root/Dev/webkit-nogl` with
  `ENABLE_WEBGL=OFF`, `TESSERA_NO_GL=ON`, `USE_VULKAN=ON`, `USE_GBM=OFF`,
  `USE_LIBDRM=OFF`, `ENABLE_GPU_PROCESS=OFF`, `ENABLE_WPE_PLATFORM_DRM=OFF`,
  `ENABLE_WPE_PLATFORM_WAYLAND=OFF`. Log: `/root/Dev/webkit-logs/configure-nogl.log`.
- Toolchain file: `/root/Dev/webkit-logs/aarch64-tessera.cmake`. The sysroot is
  `/root/Dev/webkit-sysroot/aarch64`. The compiler is LLVM 23.1.0 under
  `/root/.kira/toolchains/llvm/23.1.0/x86_64-linux-gnu/bin`.
- Build directory with objects already compiled: `wpe-aarch64-nogl`.
- GL-unit build: `/root/Dev/webkit-logs/nogl-gl-units.sh`, log
  `/root/Dev/webkit-logs/build-nogl-gl-units.log`, target list
  `/root/Dev/webkit-logs/nogl-gl-units.txt` (28 objects). The log ends with
  `EXIT=` when the run finishes.
- Latest result of that run: `EXIT=1`, 4 FAILED objects. The log holds this
  second run, not the first. The first run's log was overwritten when the script
  was run again.
  - `CoordinatedPlatformLayerBufferVideo.cpp.o`: `-Werror` on unused variables.
    The file is removed by the gate, so no fix is needed.
  - `egl/GLFenceEGL.cpp.o`: EGL sync and fence calls. The `egl/` directory is
    removed by the gate, so no fix is needed.
  - `skia/SkiaBackingStore.cpp.o`: `GL_RGBA8` and `GL_BGRA8_EXT`. A real
    Vulkan-only conversion item. Listed in the scope file.
  - `UnifiedSource-platform-74.cpp.o`: `UnixFileDescriptor` unknown in
    `platform/graphics/vulkan/VulkanTypes.cpp`. That file is being edited by the
    Sol worker right now (modified 22:33 on `ed`), so this is in progress, not a
    settled error.
- Existing builds, do not delete: `/root/Dev/webkit-build/wpe-aarch64` and
  `/root/Dev/webkit-build/wpe-x86_64`.
- Do not run ninja in the foreground over SSH. Use a background shell and wait
  for its completion.

## Distributed compile

- Windows host `100.85.181.198` runs `distccd` (distcc 3.4, LLVM 23.1.0, 8 jobs),
  allowing only `100.85.162.6` (ed), in `--enable-tcp-insecure` mode, which the
  user approved. Log: `C:\wk\distccd.log` on that host.
- Launcher on ed: `/root/Dev/webkit-logs/distcc-tessera.sh`. It runs
  `distcc <bare compiler name> args...`. The check that it works is
  `/root/Dev/webkit-logs/distcc-one-test.py`, run with `DISTCC_HOSTS=100.85.181.198/8 DISTCC_FALLBACK=0`.
  It compiled `Source/WTF/wtf/ASCIICType.cpp` remotely to an aarch64 ELF (`e_machine` 0xb7).
- Two rules for the compile lines. Pass the target as `-target aarch64-linux-gnu`,
  not `--target=`: distcc adds its own `-target <host>` when it sees `--target=`,
  and the object comes back x86-64. The compiler must be invoked by its bare name.
- Changing the compiler launcher or the target flags changes every compile
  command, so ninja rebuilds everything in that build directory. Do that in a new
  build directory only.

## The file and call-site list

Full list: `/root/Dev/webkit-logs/vulkan-only-scope.txt` on ed (145 lines).
It has the 40 files that include `GLContext.h`, the files with call sites of
`skiaGLContext()`, `makeSkiaContextCurrent()` and `skiaGrContext()`, and every
call-site line. Regenerate it with the grep in that file's header if the copy
changes.

Files with call sites (28 in WebCore and WebKit):

- WebCore page: `page/scrolling/coordinated/ScrollerCoordinated.cpp`
- WebCore platform/graphics: `DMABufBuffer.cpp`, `PlatformDisplay.h`
- WebCore coordinated: `CoordinatedPlatformLayerBufferSkiaDeferredImage.cpp`,
  `CoordinatedPlatformLayerBufferSkiaImage.cpp`, `CoordinatedPlatformLayerBufferVideo.cpp`,
  `CoordinatedTileBuffer.cpp`, `CoordinatedBackingStoreProxy.cpp`,
  `texmap/coordinated/CoordinatedPlatformLayerBufferNativeImage.cpp`
- WebCore gstreamer: `gstreamer/VideoFrameGStreamer.cpp` (GStreamer GL is off; check if it is compiled)
- WebCore skia: `SkiaCompositingLayer.cpp`, `GraphicsContextGLSkia.cpp`,
  `GraphicsContextSkia.cpp`, `ImageBufferSkiaAcceleratedBackend.cpp`,
  `ImageUtilitiesSkia.cpp`, `NativeImageSkia.cpp`, `PlatformDisplaySkia.cpp`,
  `SkiaBackingStore.cpp`, `SkiaGPUAtlas.cpp`, `SkiaPaintingEngine.cpp`,
  `SkiaReplayCanvas.cpp`, `SkiaSerializedImageBuffer.cpp`, `SkiaUtilities.cpp`
- WebKit: `WebProcess/WebPage/CoordinatedGraphics/LayerTreeHost.cpp`,
  `NonCompositedFrameRenderer.cpp`, `AcceleratedSurface.cpp`, `ThreadedCompositor.cpp`

Where the GL display path lives today:

- `PlatformDisplaySkia.cpp`. `skiaGLContext()` creates `SkiaGLContext` and
  `GrGLInterface`. The Vulkan context `SkiaVulkanContext` (lines ~375-444) is
  used only when `SkiaVulkanDevice::singleton()` returns a device. Otherwise the
  GL path is taken. The GL fallback must go under the no-GL build.
- `SkiaVulkanDevice::singleton()` returns null when Vulkan is off
  (`WEBKIT_SKIA_VULKAN=0`) or no device exists. Its callers assume GL.

## Next steps

1. Reconfigure `wpe-aarch64-nogl` from the current copy, so the three
   `list(FILTER)` gates take effect. Use the configure script, which needs no
   changes. Check the configure log ends with `EXIT=0`.
2. Make the Vulkan-only path the only path under `TESSERA_NO_GL`. Expose a
   build flag from `OptionsWPE.cmake` (for example `USE_TESSERA_VULKAN_ONLY`) and
   use `USE(...)` in the code. Route `skiaGrContext()`, `makeSkiaContextCurrent()`
   and the callers listed above through `SkiaVulkanContext`. Remove the GL
   context objects from that build.
3. Compile the 28 GL units and the files in the list above. Each compile
   error is a call site not yet converted.
4. Link the WPE libraries. Then run the closure check: `readelf -d` on
   `libWPEWebKit-2.0.so.1` and every library it loads must show no `libEGL`,
   `libGLES*`, `libGL`, `libepoxy`, `libgbm` or `libdrm`.
5. The Kira helper and the C++ backend are not started. They are separate work.
   Helper location is decided: a new package in this repository, depending on
   `tessera-projects/display-client` and `tessera-projects/devicekit`.
   The backend talks to MosaicDisplay through that helper over FFI. The helper
   exports Vulkan surfaces through DeviceKit `Surface.export`.

## Verification limits

- Compile and link can be checked on `ed`.
- A runtime check on aarch64 is not possible on the Fedora host:
  `HANDOFF.md` §3 says QEMU's TCG aarch64 emulation prints nothing from the kernel
  there. Say so in any report about rendering.
