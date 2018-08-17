/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#if defined(__clang_analyzer__) && !defined(SDL_DISABLE_ANALYZE_MACROS)
#define SDL_DISABLE_ANALYZE_MACROS 1
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdlib.h>
#include "../SDL_internal.h"

/* This file contains portable string manipulation functions for SDL */

#include "SDL_stdinc.h"

size_t
SDL_strlcpy(SDL_OUT_Z_CAP(maxlen) char *dst, const char *src, size_t maxlen)
{
#if defined(HAVE_STRLCPY)
    return strlcpy(dst, src, maxlen);
#else
    size_t srclen = strlen(src);
    if (maxlen > 0) {
        size_t len = SDL_min(srclen, maxlen - 1);
        memcpy(dst, src, len);
        dst[len] = '\0';
    }
    return srclen;
#endif /* HAVE_STRLCPY */
}

char *
SDL_strchr(const char *string, int c)
{
#ifdef HAVE_STRCHR
    return SDL_const_cast(char*,strchr(string, c));
#elif defined(HAVE_INDEX)
    return SDL_const_cast(char*,index(string, c));
#else
    while (*string) {
        if (*string == c) {
            return (char *) string;
        }
        ++string;
    }
    return NULL;
#endif /* HAVE_STRCHR */
}

int
SDL_strcasecmp(const char *str1, const char *str2)
{
#ifdef HAVE_STRCASECMP
    return strcasecmp(str1, str2);
#elif defined(HAVE__STRICMP)
    return _stricmp(str1, str2);
#else
    char a = 0;
    char b = 0;
    while (*str1 && *str2) {
        a = toupper((unsigned char) *str1);
        b = toupper((unsigned char) *str2);
        if (a != b)
            break;
        ++str1;
        ++str2;
    }
    a = toupper(*str1);
    b = toupper(*str2);
    return (int) ((unsigned char) a - (unsigned char) b);
#endif /* HAVE_STRCASECMP */
}

int
SDL_strncasecmp(const char *str1, const char *str2, size_t maxlen)
{
#ifdef HAVE_STRNCASECMP
    return strncasecmp(str1, str2, maxlen);
#elif defined(HAVE__STRNICMP)
    return _strnicmp(str1, str2, maxlen);
#else
    char a = 0;
    char b = 0;
    while (*str1 && *str2 && maxlen) {
        a = tolower((unsigned char) *str1);
        b = tolower((unsigned char) *str2);
        if (a != b)
            break;
        ++str1;
        ++str2;
        --maxlen;
    }
    if (maxlen == 0) {
        return 0;
    } else {
        a = tolower((unsigned char) *str1);
        b = tolower((unsigned char) *str2);
        return (int) ((unsigned char) a - (unsigned char) b);
    }
#endif /* HAVE_STRNCASECMP */
}

int
SDL_snprintf(SDL_OUT_Z_CAP(maxlen) char *text, size_t maxlen, SDL_PRINTF_FORMAT_STRING const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);
    retval = vsnprintf(text, maxlen, fmt, ap);
    va_end(ap);

    return retval;
}

/* vi: set ts=4 sw=4 expandtab: */
