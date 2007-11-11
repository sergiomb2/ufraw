/*
 * UFRaw - Unidentified Flying Raw converter for digital camera images
 *
 * uf_glib.h - glib compatibility header
 * Copyright 2004-2007 by Udi Fuchs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation. You should have received
 * a copy of the license along with this program.
 */

#ifndef _UF_GLIB_H
#define _UF_GLIB_H

#ifdef  __cplusplus
extern "C" {
#endif

// glib is required for the GLIB_CHECK_VERSION macro
#include <glib.h>

// gstdio exists only since glib-2.6
#if GLIB_CHECK_VERSION(2,6,0)
#include <glib/gstdio.h>
#else
// Otherwise we define the gstdio functions as macros
#include <stdio.h>
#define g_fopen fopen
#define g_unlink unlink
#define g_rename rename
#endif

// Before glib-2.10 g_unlink was a macro, requiring some more headers
#if !GLIB_CHECK_VERSION(2,10,0)
#include <unistd.h> // for unlink
#endif

// g_win32_locale_filename_from_utf8 is needed only on win32
#if GLIB_CHECK_VERSION(2,8,0) && defined(WIN32)
#define uf_win32_locale_filename_from_utf8(__some_string__) \
    g_win32_locale_filename_from_utf8(__some_string__)
#define uf_win32_locale_filename_free(__some_string__) g_free(__some_string__)

// g_win32_locale_filename_from_utf8 exists since glib-2.8
// with glib-2.6, g_locale_from_utf8 is the cheap replacement
#elif GLIB_CHECK_VERSION(2,6,0) && defined(WIN32)
#define uf_win32_locale_filename_from_utf8(__some_string__) \
    g_locale_from_utf8(__some_string__, -1, NULL, NULL, NULL)
#define uf_win32_locale_filename_free(__some_string__) g_free(__some_string__)

// On other platforms nothing is needed
#else
#define uf_win32_locale_filename_from_utf8(__some_string__) (__some_string__)
#define uf_win32_locale_filename_free(__some_string__) (void)(__some_string__)

#endif

// One win32 command-line arguments need to be translated to UTF-8
#ifdef WIN32
#define uf_win32_locale_to_utf8(__some_string__) \
    g_locale_to_utf8(__some_string__, -1, NULL, NULL, NULL)
#define uf_win32_locale_free(__some_string__) g_free(__some_string__)
#else
#define uf_win32_locale_to_utf8(__some_string__) (__some_string__)
#define uf_win32_locale_free(__some_string__) (void)(__some_string__)
#endif

#ifdef  __cplusplus
}
#endif

#endif /*_UF_GLIB_H*/
