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
#ifndef WPEDisplayTessera_h
#define WPEDisplayTessera_h

#if !defined(__WPE_TESSERA_H_INSIDE__) && !defined(BUILDING_WEBKIT)
#error "Only <wpe/tessera/wpe-tessera.h> can be included directly."
#endif

#include <glib-object.h>
#include <wpe/wpe-platform.h>

G_BEGIN_DECLS

#define WPE_TYPE_DISPLAY_TESSERA (wpe_display_tessera_get_type())
WPE_API G_DECLARE_FINAL_TYPE (WPEDisplayTessera, wpe_display_tessera, WPE, DISPLAY_TESSERA, WPEDisplay)

WPE_API WPEDisplay *wpe_display_tessera_new (void);

G_END_DECLS

#endif /* WPEDisplayTessera_h */
