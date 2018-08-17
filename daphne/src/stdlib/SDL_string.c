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


#define SDL_isupperhex(X)   (((X) >= 'A') && ((X) <= 'F'))
#define SDL_islowerhex(X)   (((X) >= 'a') && ((X) <= 'f'))

#define UTF8_IsLeadByte(c) ((c) >= 0xC0 && (c) <= 0xF4)
#define UTF8_IsTrailingByte(c) ((c) >= 0x80 && (c) <= 0xBF)

static int UTF8_TrailingBytes(unsigned char c)
{
    if (c >= 0xC0 && c <= 0xDF)
        return 1;
    else if (c >= 0xE0 && c <= 0xEF)
        return 2;
    else if (c >= 0xF0 && c <= 0xF4)
        return 3;
    else
        return 0;
}

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
SDL_strrev(char *string)
{
#if defined(HAVE__STRREV)
    return _strrev(string);
#else
    size_t len = strlen(string);
    char *a = &string[0];
    char *b = &string[len - 1];
    len /= 2;
    while (len--) {
        char c = *a;
        *a++ = *b;
        *b-- = c;
    }
    return string;
#endif /* HAVE__STRREV */
}

char *
SDL_strlwr(char *string)
{
#if defined(HAVE__STRLWR)
    return _strlwr(string);
#else
    char *bufp = string;
    while (*bufp) {
        *bufp = tolower((unsigned char) *bufp);
        ++bufp;
    }
    return string;
#endif /* HAVE__STRLWR */
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

char *
SDL_strrchr(const char *string, int c)
{
#ifdef HAVE_STRRCHR
    return SDL_const_cast(char*,strrchr(string, c));
#elif defined(HAVE_RINDEX)
    return SDL_const_cast(char*,rindex(string, c));
#else
    const char *bufp = string + strlen(string) - 1;
    while (bufp >= string) {
        if (*bufp == c) {
            return (char *) bufp;
        }
        --bufp;
    }
    return NULL;
#endif /* HAVE_STRRCHR */
}

char *
SDL_strstr(const char *haystack, const char *needle)
{
#if defined(HAVE_STRSTR)
    return SDL_const_cast(char*,strstr(haystack, needle));
#else
    size_t length = strlen(needle);
    while (*haystack) {
        if (SDL_strncmp(haystack, needle, length) == 0) {
            return (char *) haystack;
        }
        ++haystack;
    }
    return NULL;
#endif /* HAVE_STRSTR */
}

#if !defined(HAVE__LTOA) || !defined(HAVE__I64TOA) || \
    !defined(HAVE__ULTOA) || !defined(HAVE__UI64TOA)
static const char ntoa_table[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};
#endif /* ntoa() conversion table */

char *
SDL_itoa(int value, char *string, int radix)
{
#ifdef HAVE_ITOA
    return itoa(value, string, radix);
#else
    return SDL_ltoa((long)value, string, radix);
#endif /* HAVE_ITOA */
}

char *
SDL_uitoa(unsigned int value, char *string, int radix)
{
#ifdef HAVE__UITOA
    return _uitoa(value, string, radix);
#else
    return SDL_ultoa((unsigned long)value, string, radix);
#endif /* HAVE__UITOA */
}

char *
SDL_ltoa(long value, char *string, int radix)
{
#if defined(HAVE__LTOA)
    return _ltoa(value, string, radix);
#else
    char *bufp = string;

    if (value < 0) {
        *bufp++ = '-';
        SDL_ultoa(-value, bufp, radix);
    } else {
        SDL_ultoa(value, bufp, radix);
    }

    return string;
#endif /* HAVE__LTOA */
}

char *
SDL_ultoa(unsigned long value, char *string, int radix)
{
#if defined(HAVE__ULTOA)
    return _ultoa(value, string, radix);
#else
    char *bufp = string;

    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = '\0';

    /* The numbers went into the string backwards. :) */
    SDL_strrev(string);

    return string;
#endif /* HAVE__ULTOA */
}

char *
SDL_lltoa(Sint64 value, char *string, int radix)
{
#if defined(HAVE__I64TOA)
    return _i64toa(value, string, radix);
#else
    char *bufp = string;

    if (value < 0) {
        *bufp++ = '-';
        SDL_ulltoa(-value, bufp, radix);
    } else {
        SDL_ulltoa(value, bufp, radix);
    }

    return string;
#endif /* HAVE__I64TOA */
}

char *
SDL_ulltoa(Uint64 value, char *string, int radix)
{
#if defined(HAVE__UI64TOA)
    return _ui64toa(value, string, radix);
#else
    char *bufp = string;

    if (value) {
        while (value > 0) {
            *bufp++ = ntoa_table[value % radix];
            value /= radix;
        }
    } else {
        *bufp++ = '0';
    }
    *bufp = '\0';

    /* The numbers went into the string backwards. :) */
    SDL_strrev(string);

    return string;
#endif /* HAVE__UI64TOA */
}

int SDL_atoi(const char *string)
{
#ifdef HAVE_ATOI
    return atoi(string);
#else
    return strtol(string, NULL, 0);
#endif /* HAVE_ATOI */
}

double SDL_atof(const char *string)
{
#ifdef HAVE_ATOF
    return (double) atof(string);
#else
    return strtod(string, NULL);
#endif /* HAVE_ATOF */
}

int
SDL_strncmp(const char *str1, const char *str2, size_t maxlen)
{
#if defined(HAVE_STRNCMP)
    return strncmp(str1, str2, maxlen);
#else
    while (*str1 && *str2 && maxlen) {
        if (*str1 != *str2)
            break;
        ++str1;
        ++str2;
        --maxlen;
    }
    if (!maxlen) {
        return 0;
    }
    return (int) ((unsigned char) *str1 - (unsigned char) *str2);
#endif /* HAVE_STRNCMP */
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
