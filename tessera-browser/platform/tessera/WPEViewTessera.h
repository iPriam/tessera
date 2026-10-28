/*
 * Tessera backend for the WPE platform API.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.
 */
#ifndef WPEViewTessera_h
#define WPEViewTessera_h

#if !defined(__WPE_TESSERA_H_INSIDE__) && !defined(BUILDING_WEBKIT)
#error "Only <wpe/tessera/wpe-tessera.h> can be included directly."
#endif

#include <glib-object.h>
#include <wpe/wpe-platform.h>
#include <wpe/tessera/WPEDisplayTessera.h>

G_BEGIN_DECLS

#define WPE_TYPE_VIEW_TESSERA (wpe_view_tessera_get_type())
WPE_API G_DECLARE_FINAL_TYPE (WPEViewTessera, wpe_view_tessera, WPE, VIEW_TESSERA, WPEView)

WPE_API WPEBuffer *wpe_view_tessera_take_frame    (WPEView *view);
/* The frame's layout, one scalar per call, for a caller that binds this API
 * through a foreign-function interface. Each answers 0 (or -1 for a
 * descriptor) for a buffer the application cannot import: not a DMA-BUF, or
 * not one plane. */
WPE_API gboolean wpe_view_tessera_frame_importable (WPEBuffer *buffer);
WPE_API int      wpe_view_tessera_frame_dup_fd     (WPEBuffer *buffer);
WPE_API int      wpe_view_tessera_frame_dup_fence  (WPEBuffer *buffer);
WPE_API guint32  wpe_view_tessera_frame_width      (WPEBuffer *buffer);
WPE_API guint32  wpe_view_tessera_frame_height     (WPEBuffer *buffer);
WPE_API guint32  wpe_view_tessera_frame_format     (WPEBuffer *buffer);
WPE_API guint32  wpe_view_tessera_frame_offset     (WPEBuffer *buffer);
WPE_API guint32  wpe_view_tessera_frame_stride     (WPEBuffer *buffer);
WPE_API guint64  wpe_view_tessera_frame_modifier   (WPEBuffer *buffer);
WPE_API void     wpe_view_tessera_frame_presented (WPEView *view, WPEBuffer *buffer);
WPE_API void     wpe_view_tessera_frame_released  (WPEView *view, WPEBuffer *buffer);

WPE_API void     wpe_view_tessera_resize          (WPEView *view, int width, int height);
WPE_API void     wpe_view_tessera_pointer_move    (WPEView *view, guint32 time, guint modifiers, double x, double y);
WPE_API void     wpe_view_tessera_pointer_button  (WPEView *view, guint32 time, guint modifiers, gboolean pressed, guint button, double x, double y);
WPE_API void     wpe_view_tessera_scroll          (WPEView *view, guint32 time, guint modifiers, double dx, double dy, double x, double y);
WPE_API void     wpe_view_tessera_key             (WPEView *view, guint32 time, guint modifiers, gboolean pressed, guint keycode, guint keyval);
WPE_API void     wpe_view_tessera_focus           (WPEView *view, gboolean focused);

G_END_DECLS

#endif /* WPEViewTessera_h */
