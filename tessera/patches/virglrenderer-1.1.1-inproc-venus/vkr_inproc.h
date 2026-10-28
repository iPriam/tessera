/* In-process venus context for Tessera.
 *
 * virglrenderer 1.1.1 routes venus through a forked render server, whose
 * VkDeviceMemory is exported as an OPAQUE_FD that is only valid in the
 * exporting process. On a host whose Vulkan driver cannot export dma_buf
 * (dzn / Vulkan-on-D3D12 under WSL), QEMU cannot re-import that fd, so
 * host-visible memory never maps and no command runs. Running venus in the
 * same process as the importer (QEMU) makes the opaque-fd round trip valid.
 *
 * This wraps the flat vkr_renderer_* API into a virgl_context, the role the
 * render server proxy plays when venus is out of process.
 */
#ifndef VKR_INPROC_H
#define VKR_INPROC_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "venus/vkr_renderer.h"
#include "virgl_context.h"
#include "virgl_resource.h"

struct vkr_inproc_res {
   struct vkr_inproc_res *next;
   uint32_t res_id;
};

struct vkr_inproc_context {
   struct virgl_context base;
   struct vkr_inproc_res *resources;
};

static bool
vkr_inproc_res_find(struct vkr_inproc_context *c, uint32_t id)
{
   for (struct vkr_inproc_res *r = c->resources; r; r = r->next)
      if (r->res_id == id)
         return true;
   return false;
}

static void
vkr_inproc_res_add(struct vkr_inproc_context *c, uint32_t id)
{
   struct vkr_inproc_res *r = calloc(1, sizeof(*r));
   if (!r)
      return;
   r->res_id = id;
   r->next = c->resources;
   c->resources = r;
}

static void
vkr_inproc_res_remove(struct vkr_inproc_context *c, uint32_t id)
{
   struct vkr_inproc_res **pp = &c->resources;
   while (*pp) {
      if ((*pp)->res_id == id) {
         struct vkr_inproc_res *d = *pp;
         *pp = d->next;
         free(d);
         return;
      }
      pp = &(*pp)->next;
   }
}

static void
vkr_inproc_destroy(struct virgl_context *base)
{
   struct vkr_inproc_context *c = (struct vkr_inproc_context *)base;
   vkr_renderer_destroy_context(base->ctx_id);
   struct vkr_inproc_res *r = c->resources;
   while (r) {
      struct vkr_inproc_res *n = r->next;
      free(r);
      r = n;
   }
   free(c);
}

static void
vkr_inproc_attach_resource(struct virgl_context *base, struct virgl_resource *res)
{
   struct vkr_inproc_context *c = (struct vkr_inproc_context *)base;
   if (vkr_inproc_res_find(c, res->res_id))
      return;

   enum virgl_resource_fd_type fd_type = res->fd_type;
   int fd = res->fd;
   uint64_t size = res->map_size;
   bool close_fd = false;

   if (fd_type == VIRGL_RESOURCE_FD_INVALID) {
      fd_type = virgl_resource_export_fd(res, &fd);
      if (fd_type != VIRGL_RESOURCE_FD_DMABUF) {
         if (fd_type != VIRGL_RESOURCE_FD_INVALID)
            close(fd);
         return;
      }
      size = lseek(fd, 0, SEEK_END);
      close_fd = true;
   } else if (fd_type != VIRGL_RESOURCE_FD_DMABUF &&
              fd_type != VIRGL_RESOURCE_FD_SHM) {
      return;
   }

   vkr_renderer_import_resource(base->ctx_id, res->res_id, fd_type, fd, size);
   if (close_fd && fd >= 0)
      close(fd);
   vkr_inproc_res_add(c, res->res_id);
}

static void
vkr_inproc_detach_resource(struct virgl_context *base, struct virgl_resource *res)
{
   struct vkr_inproc_context *c = (struct vkr_inproc_context *)base;
   if (!vkr_inproc_res_find(c, res->res_id))
      return;
   vkr_renderer_destroy_resource(base->ctx_id, res->res_id);
   vkr_inproc_res_remove(c, res->res_id);
}

static int
vkr_inproc_transfer_3d(struct virgl_context *base, struct virgl_resource *res,
                       const struct vrend_transfer_info *info, int transfer_mode)
{
   (void)base;
   (void)res;
   (void)info;
   (void)transfer_mode;
   return -1;
}

static int
vkr_inproc_get_blob(struct virgl_context *base, uint32_t res_id, uint64_t blob_id,
                    uint64_t blob_size, uint32_t blob_flags,
                    struct virgl_context_blob *blob)
{
   struct vkr_inproc_context *c = (struct vkr_inproc_context *)base;
   enum virgl_resource_fd_type fd_type = VIRGL_RESOURCE_FD_INVALID;
   int fd = -1;
   uint32_t map_info = 0;
   struct virgl_resource_vulkan_info vulkan_info = { 0 };

   if (!vkr_renderer_create_resource(base->ctx_id, res_id, blob_id, blob_size,
                                     blob_flags, &fd_type, &fd, &map_info,
                                     &vulkan_info))
      return -EINVAL;

   blob->type = fd_type;
   blob->u.fd = fd;
   blob->map_info = map_info;
   if (fd_type == VIRGL_RESOURCE_FD_OPAQUE)
      blob->vulkan_info = vulkan_info;

   vkr_inproc_res_add(c, res_id);
   return 0;
}

static int
vkr_inproc_submit_cmd(struct virgl_context *base, const void *buffer, size_t size)
{
   return vkr_renderer_submit_cmd(base->ctx_id, (void *)buffer, (uint32_t)size)
             ? 0
             : -EINVAL;
}

static int
vkr_inproc_get_fencing_fd(struct virgl_context *base)
{
   (void)base;
   return -1;
}

static void
vkr_inproc_retire_fences(struct virgl_context *base)
{
   (void)base;
}

static int
vkr_inproc_submit_fence(struct virgl_context *base, uint32_t flags,
                        uint32_t ring_idx, uint64_t fence_id)
{
   return vkr_renderer_submit_fence(base->ctx_id, flags, ring_idx, fence_id)
             ? 0
             : -EINVAL;
}

static struct virgl_context *
vkr_inproc_context_create(uint32_t ctx_id, uint32_t ctx_flags, uint32_t nlen,
                          const char *name)
{
   if (!vkr_renderer_create_context(ctx_id, ctx_flags, nlen, name))
      return NULL;

   struct vkr_inproc_context *c = calloc(1, sizeof(*c));
   if (!c) {
      vkr_renderer_destroy_context(ctx_id);
      return NULL;
   }

   c->base.destroy = vkr_inproc_destroy;
   c->base.attach_resource = vkr_inproc_attach_resource;
   c->base.detach_resource = vkr_inproc_detach_resource;
   c->base.transfer_3d = vkr_inproc_transfer_3d;
   c->base.get_blob = vkr_inproc_get_blob;
   c->base.submit_cmd = vkr_inproc_submit_cmd;
   c->base.get_fencing_fd = vkr_inproc_get_fencing_fd;
   c->base.retire_fences = vkr_inproc_retire_fences;
   c->base.submit_fence = vkr_inproc_submit_fence;
   return &c->base;
}

static void
vkr_inproc_log(enum virgl_log_level_flags level, const char *msg, void *user_data)
{
   (void)level;
   (void)user_data;
   fprintf(stderr, "vkr(inproc): %s\n", msg);
}

static void
vkr_inproc_retire_fence_cb(uint32_t ctx_id, uint32_t ring_idx, uint64_t fence_id)
{
   struct virgl_context *ctx = virgl_context_lookup(ctx_id);
   if (ctx && ctx->fence_retire)
      ctx->fence_retire(ctx, ring_idx, fence_id);
}

static const struct vkr_renderer_callbacks vkr_inproc_cbs = {
   .debug_logger = vkr_inproc_log,
   .retire_fence = vkr_inproc_retire_fence_cb,
};

#endif /* VKR_INPROC_H */
