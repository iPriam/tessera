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

#include "config.h"
#include <fcntl.h>
#include "WPEViewTessera.h"

#include "WPEToplevelTessera.h"
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/WTFGType.h>

/**
 * WPEViewTessera:
 *
 * The [class@WPEPlatform.View] of the Tessera platform.
 *
 * It has no native surface. The newest frame the web process drew waits in the
 * view until the embedding application takes it, imports its memory and draws
 * it, then says when the frame is on the glass and when the memory may be
 * drawn into again. Input comes the other way, from the application.
 */
struct _WPEViewTesseraPrivate {
    GRefPtr<WPEBuffer> pendingFrame;
};
WEBKIT_DEFINE_FINAL_TYPE(WPEViewTessera, wpe_view_tessera, WPE_TYPE_VIEW, WPEView)

static void wpeViewTesseraConstructed(GObject* object)
{
    G_OBJECT_CLASS(wpe_view_tessera_parent_class)->constructed(object);

    auto* view = WPE_VIEW(object);
    g_signal_connect(view, "notify::toplevel", G_CALLBACK(+[](WPEView* view, GParamSpec*, gpointer) {
        auto* toplevel = wpe_view_get_toplevel(view);
        if (!toplevel) {
            wpe_view_unmap(view);
            return;
        }

        int width;
        int height;
        wpe_toplevel_get_size(toplevel, &width, &height);
        if (width && height)
            wpe_view_resized(view, width, height);

        wpe_view_map(view);
    }), nullptr);
}

static void wpeViewTesseraDispose(GObject* object)
{
    WPE_VIEW_TESSERA(object)->priv->pendingFrame = nullptr;

    G_OBJECT_CLASS(wpe_view_tessera_parent_class)->dispose(object);
}

static gboolean wpeViewTesseraRenderBuffer(WPEView* view, WPEBuffer* buffer, const WPERectangle*, guint, GError**)
{
    auto* priv = WPE_VIEW_TESSERA(view)->priv;
    // A frame the application never took is stale, and the web process is
    // waiting for its memory back.
    if (priv->pendingFrame)
        wpe_view_buffer_released(view, priv->pendingFrame.get());
    priv->pendingFrame = buffer;
    return TRUE;
}

static void wpe_view_tessera_class_init(WPEViewTesseraClass* viewTesseraClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(viewTesseraClass);
    objectClass->constructed = wpeViewTesseraConstructed;
    objectClass->dispose = wpeViewTesseraDispose;

    WPEViewClass* viewClass = WPE_VIEW_CLASS(viewTesseraClass);
    viewClass->render_buffer = wpeViewTesseraRenderBuffer;
}

/**
 * wpe_view_tessera_take_frame:
 * @view: a #WPEViewTessera
 *
 * Take the newest frame the web process drew, if there is one. The frame
 * stays the view's until wpe_view_tessera_frame_released() is called for it.
 *
 * Returns: (transfer full) (nullable): a #WPEBuffer
 */
WPEBuffer* wpe_view_tessera_take_frame(WPEView* view)
{
    g_return_val_if_fail(WPE_IS_VIEW_TESSERA(view), nullptr);

    return WPE_VIEW_TESSERA(view)->priv->pendingFrame.leakRef();
}

static WPEBufferDMABuf* importableBuffer(WPEBuffer* buffer)
{
    if (!buffer || !WPE_IS_BUFFER_DMA_BUF(buffer))
        return nullptr;
    auto* dmaBuffer = WPE_BUFFER_DMA_BUF(buffer);
    if (wpe_buffer_dma_buf_get_n_planes(dmaBuffer) != 1)
        return nullptr;
    return dmaBuffer;
}

/**
 * wpe_view_tessera_frame_importable:
 * @buffer: a #WPEBuffer taken from a view
 *
 * Returns: %TRUE when @buffer is one plane of a DMA-BUF the application can import
 */
gboolean wpe_view_tessera_frame_importable(WPEBuffer* buffer)
{
    return importableBuffer(buffer) ? TRUE : FALSE;
}

/**
 * wpe_view_tessera_frame_dup_fd:
 * @buffer: an importable #WPEBuffer
 *
 * Returns: a duplicate of the DMA-BUF descriptor, the caller's to give away
 * or close, or -1
 */
int wpe_view_tessera_frame_dup_fd(WPEBuffer* buffer)
{
    auto* dmaBuffer = importableBuffer(buffer);
    if (!dmaBuffer)
        return -1;
    return fcntl(wpe_buffer_dma_buf_get_fd(dmaBuffer, 0), F_DUPFD_CLOEXEC, 3);
}

/**
 * wpe_view_tessera_frame_dup_fence:
 * @buffer: an importable #WPEBuffer
 *
 * Returns: a duplicate of the sync file the web process signals when the frame
 * is drawn, the caller's to wait on and close, or -1 when there is none
 */
int wpe_view_tessera_frame_dup_fence(WPEBuffer* buffer)
{
    if (!importableBuffer(buffer))
        return -1;
    int fence = wpe_buffer_get_rendering_fence(buffer);
    if (fence < 0)
        return -1;
    return fcntl(fence, F_DUPFD_CLOEXEC, 3);
}

guint32 wpe_view_tessera_frame_width(WPEBuffer* buffer)
{
    return importableBuffer(buffer) ? wpe_buffer_get_width(buffer) : 0;
}

guint32 wpe_view_tessera_frame_height(WPEBuffer* buffer)
{
    return importableBuffer(buffer) ? wpe_buffer_get_height(buffer) : 0;
}

guint32 wpe_view_tessera_frame_format(WPEBuffer* buffer)
{
    auto* dmaBuffer = importableBuffer(buffer);
    return dmaBuffer ? wpe_buffer_dma_buf_get_format(dmaBuffer) : 0;
}

guint32 wpe_view_tessera_frame_offset(WPEBuffer* buffer)
{
    auto* dmaBuffer = importableBuffer(buffer);
    return dmaBuffer ? wpe_buffer_dma_buf_get_offset(dmaBuffer, 0) : 0;
}

guint32 wpe_view_tessera_frame_stride(WPEBuffer* buffer)
{
    auto* dmaBuffer = importableBuffer(buffer);
    return dmaBuffer ? wpe_buffer_dma_buf_get_stride(dmaBuffer, 0) : 0;
}

guint64 wpe_view_tessera_frame_modifier(WPEBuffer* buffer)
{
    auto* dmaBuffer = importableBuffer(buffer);
    return dmaBuffer ? wpe_buffer_dma_buf_get_modifier(dmaBuffer) : 0;
}

/**
 * wpe_view_tessera_frame_presented:
 * @view: a #WPEViewTessera
 * @buffer: the frame now on the glass
 */
void wpe_view_tessera_frame_presented(WPEView* view, WPEBuffer* buffer)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));
    wpe_view_buffer_rendered(view, buffer);
}

/**
 * wpe_view_tessera_frame_released:
 * @view: a #WPEViewTessera
 * @buffer: a frame the GPU has finished reading
 *
 * Call it once the GPU is done with @buffer, not when a frame is merely
 * displayed: the web process draws into the memory again after it.
 */
void wpe_view_tessera_frame_released(WPEView* view, WPEBuffer* buffer)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));
    wpe_view_buffer_released(view, buffer);
}

void wpe_view_tessera_resize(WPEView* view, int width, int height)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));

    auto* toplevel = wpe_view_get_toplevel(view);
    if (toplevel)
        wpe_toplevel_resize(toplevel, width, height);
    else
        wpe_view_resized(view, width, height);
}

void wpe_view_tessera_pointer_move(WPEView* view, guint32 time, guint modifiers, double x, double y)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));

    GRefPtr<WPEEvent> event = adoptGRef(wpe_event_pointer_move_new(WPE_EVENT_POINTER_MOVE, view, WPE_INPUT_SOURCE_MOUSE, time, static_cast<WPEModifiers>(modifiers), x, y, 0, 0));
    wpe_view_event(view, event.get());
}

void wpe_view_tessera_pointer_button(WPEView* view, guint32 time, guint modifiers, gboolean pressed, guint button, double x, double y)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));

    auto type = pressed ? WPE_EVENT_POINTER_DOWN : WPE_EVENT_POINTER_UP;
    guint pressCount = pressed ? wpe_view_compute_press_count(view, x, y, button, time) : 0;
    GRefPtr<WPEEvent> event = adoptGRef(wpe_event_pointer_button_new(type, view, WPE_INPUT_SOURCE_MOUSE, time, static_cast<WPEModifiers>(modifiers), button, x, y, pressCount));
    wpe_view_event(view, event.get());
}

void wpe_view_tessera_scroll(WPEView* view, guint32 time, guint modifiers, double dx, double dy, double x, double y)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));

    GRefPtr<WPEEvent> event = adoptGRef(wpe_event_scroll_new(view, WPE_INPUT_SOURCE_MOUSE, time, static_cast<WPEModifiers>(modifiers), dx, dy, TRUE, FALSE, x, y));
    wpe_view_event(view, event.get());
}

void wpe_view_tessera_key(WPEView* view, guint32 time, guint modifiers, gboolean pressed, guint keycode, guint keyval)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));

    auto type = pressed ? WPE_EVENT_KEYBOARD_KEY_DOWN : WPE_EVENT_KEYBOARD_KEY_UP;
    GRefPtr<WPEEvent> event = adoptGRef(wpe_event_keyboard_new(type, view, WPE_INPUT_SOURCE_KEYBOARD, time, static_cast<WPEModifiers>(modifiers), keycode, keyval));
    wpe_view_event(view, event.get());
}

void wpe_view_tessera_focus(WPEView* view, gboolean focused)
{
    g_return_if_fail(WPE_IS_VIEW_TESSERA(view));

    if (focused)
        wpe_view_focus_in(view);
    else
        wpe_view_focus_out(view);
}
