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
#include "WPEToplevelTessera.h"

#include <wtf/glib/WTFGType.h>

/**
 * WPEToplevelTessera:
 *
 * The [class@WPEPlatform.Toplevel] of the Tessera platform. The window belongs
 * to the embedding application, which tells the toplevel its size.
 */
struct _WPEToplevelTesseraPrivate {
};
WEBKIT_DEFINE_FINAL_TYPE(WPEToplevelTessera, wpe_toplevel_tessera, WPE_TYPE_TOPLEVEL, WPEToplevel)

static void wpeToplevelTesseraConstructed(GObject* object)
{
    G_OBJECT_CLASS(wpe_toplevel_tessera_parent_class)->constructed(object);

    wpe_toplevel_state_changed(WPE_TOPLEVEL(object), WPE_TOPLEVEL_STATE_ACTIVE);
}

static gboolean wpeToplevelTesseraResize(WPEToplevel* toplevel, int width, int height)
{
    wpe_toplevel_resized(toplevel, width, height);
    wpe_toplevel_foreach_view(toplevel, [](WPEToplevel* toplevel, WPEView* view, gpointer) -> gboolean {
        int width, height;
        wpe_toplevel_get_size(toplevel, &width, &height);
        wpe_view_resized(view, width, height);
        return FALSE;
    }, nullptr);
    return TRUE;
}

static void wpe_toplevel_tessera_class_init(WPEToplevelTesseraClass* toplevelTesseraClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(toplevelTesseraClass);
    objectClass->constructed = wpeToplevelTesseraConstructed;

    WPEToplevelClass* toplevelClass = WPE_TOPLEVEL_CLASS(toplevelTesseraClass);
    toplevelClass->resize = wpeToplevelTesseraResize;
}
