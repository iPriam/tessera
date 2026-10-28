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
#ifndef WPEToplevelTessera_h
#define WPEToplevelTessera_h

#if !defined(__WPE_TESSERA_H_INSIDE__) && !defined(BUILDING_WEBKIT)
#error "Only <wpe/tessera/wpe-tessera.h> can be included directly."
#endif

#include <glib-object.h>
#include <wpe/wpe-platform.h>
#include <wpe/tessera/WPEDisplayTessera.h>

G_BEGIN_DECLS

#define WPE_TYPE_TOPLEVEL_TESSERA (wpe_toplevel_tessera_get_type())
WPE_API G_DECLARE_FINAL_TYPE (WPEToplevelTessera, wpe_toplevel_tessera, WPE, TOPLEVEL_TESSERA, WPEToplevel)

G_END_DECLS

#endif /* WPEToplevelTessera_h */
