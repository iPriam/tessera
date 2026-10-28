# 07 — Graphics

From "a GPU exists" to "the pixels an application rendered are the pixels on the
glass".

## Where it stands

Proven, with the lines to show for it:

- **Vulkan reaches the host's GPU.** `vk_smoke` clears an image and reads back
  the colour, through Mesa's venus driver, across virtio-gpu, to MoltenVK and
  Metal.
- **Ember reaches it.** Every application renders through Ember's Vulkan
  backend into a texture it exports as a dma-buf, and MosaicDisplay scans that
  memory out. Ember is the layer Kira UI draws through, so it is the one that
  matters.
- **A mode is set and a frame is flipped.** PID 1 creates two dumb buffers on
  the card, draws into them, sets a mode with `SETCRTC` and page-flips between
  them.

Vulkan is the one native library a Tessera program links
([14](14-tooling-and-ports.md)), besides WebKit under the exception in `AGENTS.md`. The loader and Mesa's venus driver are built
into the guest's sysroot by `kira build sysroot` (build/app/GuestGraphics.kira); `vulkan` is the only
library Ember's Tessera rows link. Dawn is not part of Tessera.

## The one thing that does not join up

**What the GPU renders and what the crtc scans out are different memory.**

The splash path uses *dumb buffers*: memory with a width, a height and a pitch,
drawn into by the CPU. The GPU path renders into Vulkan images inside venus. To
show a rendered frame today, something would have to copy it — which
`HANDOFF.md` §12 rejected, correctly, because a copy per frame defeats the point
of rendering on the GPU at all.

The host used to answer `SET_SCANOUT` for a dumb buffer with
`ERR_INVALID_RESOURCE_ID`, because virglrenderer does not know what a dumb
buffer is. The darwin-venus patch now has QEMU keep a venus device's 2D
resources itself, so the splash is shown; GPU frames still need blobs.

### Blob resources

The fix is virtio-gpu's *blob* resources: memory allocated so that it is both a
GPU resource the host understands and something the guest can name as a
framebuffer.

The host side is already patched — QEMU no longer demands a dmabuf for a blob
scanout, and no longer claims a venus device needs a GL console. A guest that
renders into a blob and scans it out now works end to end, proven by
`venus_scanout_smoke.c`.

The shape that worked, which is simpler than the modifier dance first sketched
here:

1. Allocate a **host-visible** Vulkan image with a dma-buf export
   (`VkExternalMemoryImageCreateInfo` + `VkExportMemoryAllocateInfo`, handle
   type `DMA_BUF`). venus backs it with the host-pointer shared memory the
   host-pointer patch makes, which is what virglrenderer can map for scanout.
2. Render into it. `vkGetMemoryFdKHR` for a dma-buf, which on this stack is a
   virtio-gpu prime of the guest bo — no host dma-buf needed.
3. `PRIME_FD_TO_HANDLE` to a GEM handle, then plain `AddFB2`. The image is
   linear (the KosmicKrisp fix forces it), so the framebuffer is
   `DRM_FORMAT_MOD_LINEAR` and needs no modifier negotiation.
4. `SETCRTC` to it. The host maps the blob and shows it through a 2D surface.

Two patches made it possible. Guest venus exposes
`VK_EXT_external_memory_dma_buf` even though the Metal host exports none, and
does not forward the export request to the host device that cannot honour it
(`patches/mesa-25.0.7-venus-scanout`). QEMU flushes the scanned-out blob through
a 2D surface rather than a GL texture on a host without OpenGL
(`patches/qemu-10.0.2-darwin-venus`).

Ember's Vulkan backend does the same through `createExportedTexture`: the
image is allocated with a dma-buf export on the device that renders into it,
because venus cannot share memory across devices, and the descriptor, stride and
offset go to the display server with the frame. Every application, through
AppKit, renders this way.

Synchronisation is the part the emulator does not give. venus emulates the
sync-fd external semaphore rather than exporting a real one, so a fence cannot
cross to the display server as a sync-fd there; Ember waits for the GPU on the
CPU instead, and a submit returns once the frame is finished
([17](17-devicekit.md) §21.1). On real hardware (radv, anv) the fence crosses.

## Frame pacing

The current flip is fire-and-forget: no `DRM_MODE_PAGE_FLIP_EVENT`, so nothing
knows when it landed, and a second flip issued before the first completes is
refused with `EBUSY`. That is fine for one splash and useless for anything that
moves.

What is needed:

- Ask for the event, and read the card's descriptor for it. A completed flip
  arrives as a `drm_event_vblank` on the DRM fd — read it like any other file.
- **One flip in flight.** Draw into the buffer that is not being shown, flip,
  wait for the event, swap. Never issue a second flip before the first
  completes.
- The event is also the frame clock: a client's "you may draw now"
  ([08](08-display-server.md)) is sent when the flip that showed its last frame
  completes. That is what makes a client draw at the display's pace without
  polling and without a timer.
- Redrawing a buffer already on screen needs `DIRTYFB` (nr `0xb1`), because
  virtio-gpu only transfers what it is told changed. Relevant to the CPU-drawn
  splash and not to the GPU path, where a flip carries the new content.

## Ember inside Tessera

`openHeadlessGraphicsWithBackend(.Vulkan)` in
`ember/app/Public/Headless.kira` is the way in: a Vulkan device with no window,
rendering into textures. That is exactly Tessera's shape: a compositor is
precisely the program that should not have a window, and neither is an
application whose pixels the compositor shows.

    var device = try openHeadlessGraphicsWithBackend(.Vulkan) otherwise return
    let exported = try device.graphics.createExportedTexture(
        ExportedTextureDescriptor { label: "frame", width: 1280, height: 800 }
    ) otherwise return
    var frame = device.frame()
    // record passes into exported.texture
    device.submit()
    // hand exported.memory, its stride and offset to the display server

A program that does this links the Vulkan loader, which is a glibc-carrying
library, so it is a glibc-carrying program ([02](02-kernel-and-syscalls.md))
and lives on the system volume, not in the initramfs.

## Composition

The display server draws client surfaces into the scanout buffer. Two ways, and
Tessera should have both:

- **GPU composition.** One render pass per frame: each client's buffer as a
  texture, drawn as a quad, in stacking order, with whatever transform and
  opacity the platform asked for. This is the general case and it is what makes
  effects possible at all.
- **Direct scanout.** When one surface covers the display and needs no
  transform, hand *its* buffer to the crtc instead of compositing. Saves a whole
  pass and a whole buffer. With the system bar on screen
  ([11](11-adaptive-desktop.md)) two surfaces are always showing, so composition
  is the normal case and direct scanout is for a surface shown with no bar.

The rule for choosing between them wants to be one function with an obvious
answer, because a compositor that sometimes takes the fast path for reasons
nobody can state is a compositor nobody can debug. MosaicDisplay's is `routeFor`
(`mosaicdisplay/app/Display/Compose.kira`): the topmost surface is scanned out
directly when it covers the output exactly, and otherwise every surface is
drawn as a quad, bottom first, through Ember into one of two exported images
that alternate as framebuffers. It is written; it has not yet been booted.

## Damage

A client says which part of its surface changed. The server can then redraw only
what is affected, and on a system where composition is a GPU pass the saving is
real but not dramatic; on one where a buffer must be transferred to a host
(virtio-gpu, exactly this machine) it is large.

Not needed for correctness. Worth designing into the protocol from the start
because retrofitting damage into a protocol that never had it means every client
changes.

## Multiple displays

One connector, one crtc, one mode today. The code already enumerates properly —
it takes the first connected connector rather than assuming one exists — so the
step to several is a loop and a decision about what "the desktop" means across
them. Not on the critical path, and cheap to keep possible by never writing
"the display" as a global.

## Checkpoints

- [x] A blob resource is created, exported as a dma-buf and named as a DRM
      framebuffer. `venus_scanout_smoke.c` does it: a
      host-visible Vulkan image, `vkGetMemoryFdKHR` to a dma-buf, `PRIME` to a
      GEM handle, `AddFB2`.
- [x] A GPU-rendered image is scanned out with no copy and no
      `ERR_INVALID_RESOURCE_ID` — and no `ERR_UNSPEC` — on the host. The same
      program clears the image on the GPU, sets the mode with it, and the host
      accepts the `SET_SCANOUT_BLOB` with no error. What made it possible:
      guest venus now exposes `VK_EXT_external_memory_dma_buf` even though the
      Metal host exports none (`patches/mesa-25.0.7-venus-scanout`), and the
      venus device presents the mappable blob through a 2D surface rather than a
      GL texture (`patches/qemu-10.0.2-darwin-venus`, the
      `virtio_gpu_rect_update` hunk). "Shows the result" was verified as
      host-accepted headlessly by the graphics test image, since removed;
      applications now do the same through Ember's Vulkan backend.
- [x] Page-flip events are read, and two frames land in a row with one flip in
      flight. The services boot prints `UI frame 1 reached the display`, then
      `UI frame 2 reached the display`; each acknowledgement comes from the DRM
      flip-complete event carrying that frame's serial.
- [x] A Kira program using Ember runs inside Tessera and clears the
      screen to a colour. Each application renders through AppKit and headless
      Vulkan into its
      exported shared texture and MosaicDisplay scans that framebuffer out.
- [x] The same program draws a triangle. The services boot saves what the
      host shows of the display with QEMU's `screendump` and requires a
      1280x800 picture in at least 64 colours: the platform's lock screen,
      its gradient and its glyphs, drawn as triangles through Ember's Vulkan
      backend and read by the host through the scanout's mapping.
- [x] Several surfaces are composed into one picture in a single GPU pass,
      bottom first, and scanned out. The services boot asserts MosaicDisplay's
      `compositing on the GPU` and `composed a picture` with `surfaces: 2` and
      `stack: "2 1"`, the platform under the application, logged after the
      flip of the composed picture was accepted.
- [x] A fullscreen surface is scanned out directly, and the compositor says so.
      MosaicDisplay validates that the attached surface matches the output before
      page-flipping its imported framebuffer and logs `direct scanout` on the
      corresponding DRM flip-complete event.
