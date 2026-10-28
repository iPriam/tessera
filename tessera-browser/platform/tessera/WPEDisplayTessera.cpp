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
#include "WPEDisplayTessera.h"

#include "WPEExtensions.h"
#include "WPEToplevelTessera.h"
#include "WPEViewTessera.h"
#include <gio/gio.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/WTFGType.h>

/**
 * WPEDisplayTessera:
 *
 * The [class@WPEPlatform.Display] of the Tessera platform.
 *
 * There is no native display to connect to: the embedding application owns the
 * window and the GPU, so this display hands it buffers and takes input from it.
 * Frames are DMA-BUF buffers of linear Vulkan images; nothing here touches
 * EGL, GBM or DRM.
 */
struct _WPEDisplayTesseraPrivate {
};
WEBKIT_DEFINE_FINAL_TYPE_WITH_CODE(WPEDisplayTessera, wpe_display_tessera, WPE_TYPE_DISPLAY, WPEDisplay,
    wpeEnsureExtensionPointsRegistered();
    g_io_extension_point_implement(WPE_DISPLAY_EXTENSION_POINT_NAME, g_define_type_id, "tessera", 100))

static gboolean wpeDisplayTesseraConnect(WPEDisplay*, GError**)
{
    return TRUE;
}

static WPEView* wpeDisplayTesseraCreateView(WPEDisplay* display)
{
    return WPE_VIEW(g_object_new(WPE_TYPE_VIEW_TESSERA, "display", display, nullptr));
}

static WPEToplevel* wpeDisplayTesseraCreateToplevel(WPEDisplay* display, guint)
{
    return WPE_TOPLEVEL(g_object_new(WPE_TYPE_TOPLEVEL_TESSERA, "display", display, nullptr));
}

static WPEBufferFormats* wpeDisplayTesseraGetPreferredBufferFormats(WPEDisplay*)
{
    // The producer allocates linear, dedicated, exportable Vulkan images, and
    // Tessera's importer reads exactly that layout.
    constexpr guint32 fourccARGB8888 = 0x34325241; // 'AR24'
    constexpr guint32 fourccXRGB8888 = 0x34325258; // 'XR24'
    auto* builder = wpe_buffer_formats_builder_new(nullptr);
    wpe_buffer_formats_builder_append_group(builder, nullptr, WPE_BUFFER_FORMAT_USAGE_RENDERING);
    wpe_buffer_formats_builder_append_format(builder, fourccARGB8888, 0);
    wpe_buffer_formats_builder_append_format(builder, fourccXRGB8888, 0);
    return wpe_buffer_formats_builder_end(builder);
}

static gboolean wpeDisplayTesseraUseExplicitSync(WPEDisplay*)
{
    return TRUE;
}

static void wpe_display_tessera_class_init(WPEDisplayTesseraClass* displayTesseraClass)
{
    WPEDisplayClass* displayClass = WPE_DISPLAY_CLASS(displayTesseraClass);
    displayClass->connect = wpeDisplayTesseraConnect;
    displayClass->create_view = wpeDisplayTesseraCreateView;
    displayClass->create_toplevel = wpeDisplayTesseraCreateToplevel;
    displayClass->get_preferred_buffer_formats = wpeDisplayTesseraGetPreferredBufferFormats;
    displayClass->use_explicit_sync = wpeDisplayTesseraUseExplicitSync;
}

/**
 * wpe_display_tessera_new:
 *
 * Create a new #WPEDisplayTessera
 *
 * Returns: (transfer full): a #WPEDisplay
 */
WPEDisplay* wpe_display_tessera_new(void)
{
    return WPE_DISPLAY(g_object_new(WPE_TYPE_DISPLAY_TESSERA, nullptr));
}
