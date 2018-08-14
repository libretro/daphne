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
SDL_wcslen(const wchar_t * string)
{
#if defined(HAVE_WCSLEN)
    return wcslen(string);
#else
    size_t len = 0;
    while (*string++) {
        ++len;
    }
    return len;
#endif /* HAVE_WCSLEN */
}

size_t
SDL_wcslcpy(SDL_OUT_Z_CAP(maxlen) wchar_t *dst, const wchar_t *src, size_t maxlen)
{
#if defined(HAVE_WCSLCPY)
    return wcslcpy(dst, src, maxlen);
#else
    size_t srclen = SDL_wcslen(src);
    if (maxlen > 0) {
        size_t len = SDL_min(srclen, maxlen - 1);
        memcpy(dst, src, len * sizeof(wchar_t));
        dst[len] = '\0';
    }
    return srclen;
#endif /* HAVE_WCSLCPY */
}

size_t
SDL_wcslcat(SDL_INOUT_Z_CAP(maxlen) wchar_t *dst, const wchar_t *src, size_t maxlen)
{
#if defined(HAVE_WCSLCAT)
    return wcslcat(dst, src, maxlen);
#else
    size_t dstlen = SDL_wcslen(dst);
    size_t srclen = SDL_wcslen(src);
    if (dstlen < maxlen) {
        SDL_wcslcpy(dst + dstlen, src, maxlen - dstlen);
    }
    return dstlen + srclen;
#endif /* HAVE_WCSLCAT */
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

size_t SDL_utf8strlcpy(SDL_OUT_Z_CAP(dst_bytes) char *dst, const char *src, size_t dst_bytes)
{
    size_t src_bytes = strlen(src);
    size_t bytes = SDL_min(src_bytes, dst_bytes - 1);
    size_t i = 0;
    char trailing_bytes = 0;
    if (bytes)
    {
        unsigned char c = (unsigned char)src[bytes - 1];
        if (UTF8_IsLeadByte(c))
            --bytes;
        else if (UTF8_IsTrailingByte(c))
        {
            for (i = bytes - 1; i != 0; --i)
            {
                c = (unsigned char)src[i];
                trailing_bytes = UTF8_TrailingBytes(c);
                if (trailing_bytes)
                {
                    if (bytes - i != trailing_bytes + 1)
                        bytes = i;

                    break;
                }
            }
        }
        memcpy(dst, src, bytes);
    }
    dst[bytes] = '\0';
    return bytes;
}

size_t
SDL_strlcat(SDL_INOUT_Z_CAP(maxlen) char *dst, const char *src, size_t maxlen)
{
#if defined(HAVE_STRLCAT)
    return strlcat(dst, src, maxlen);
#else
    size_t dstlen = strlen(dst);
    size_t srclen = strlen(src);
    if (dstlen < maxlen) {
        SDL_strlcpy(dst + dstlen, src, maxlen - dstlen);
    }
    return dstlen + srclen;
#endif /* HAVE_STRLCAT */
}

char *
SDL_strdup(const char *string)
{
#if defined(HAVE_STRDUP)
    return strdup(string);
#else
    size_t len = strlen(string) + 1;
    char *newstr = malloc(len);
    if (newstr) {
        SDL_strlcpy(newstr, string, len);
    }
    return newstr;
#endif /* HAVE_STRDUP */
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
SDL_sscanf(const char *text, SDL_SCANF_FORMAT_STRING const char *fmt, ...)
{
    int rc;
    va_list ap;
    va_start(ap, fmt);
    rc = vsscanf(text, fmt, ap);
    va_end(ap);
    return rc;
}

int
SDL_snprintf(SDL_OUT_Z_CAP(maxlen) char *text, size_t maxlen, SDL_PRINTF_FORMAT_STRING const char *fmt, ...)
{
    va_list ap;
    int retval;

    va_start(ap, fmt);
    retval = SDL_vsnprintf(text, maxlen, fmt, ap);
    va_end(ap);

    return retval;
}

#ifdef HAVE_VSNPRINTF
int SDL_vsnprintf(SDL_OUT_Z_CAP(maxlen) char *text, size_t maxlen, const char *fmt, va_list ap)
{
    if (!fmt) {
        fmt = "";
    }
    return vsnprintf(text, maxlen, fmt, ap);
}
#else
 /* FIXME: implement more of the format specifiers */
typedef enum
{
    SDL_CASE_NOCHANGE,
    SDL_CASE_LOWER,
    SDL_CASE_UPPER
} SDL_letter_case;

typedef struct
{
    SDL_bool left_justify;
    SDL_bool force_sign;
    SDL_bool force_type;
    SDL_bool pad_zeroes;
    SDL_letter_case force_case;
    int width;
    int radix;
    int precision;
} SDL_FormatInfo;

static size_t
SDL_PrintString(char *text, size_t maxlen, SDL_FormatInfo *info, const char *string)
{
    size_t length = 0;
    size_t slen;

    if (info && info->width && (size_t)info->width > strlen(string)) {
        char fill = info->pad_zeroes ? '0' : ' ';
        size_t width = info->width - strlen(string);
        while (width-- > 0 && maxlen > 0) {
            *text++ = fill;
            ++length;
            --maxlen;
        }
    }

    slen = SDL_strlcpy(text, string, maxlen);
    length += SDL_min(slen, maxlen);

    if (info) {
        if (info->force_case == SDL_CASE_LOWER) {
            SDL_strlwr(text);
        } else if (info->force_case == SDL_CASE_UPPER) {
            while (*text != '\0')
            {
                *text = toupper(*text);
                text++;
            }
        }
    }
    return length;
}

static size_t
SDL_PrintLong(char *text, size_t maxlen, SDL_FormatInfo *info, long value)
{
    char num[130];

    SDL_ltoa(value, num, info ? info->radix : 10);
    return SDL_PrintString(text, maxlen, info, num);
}

static size_t
SDL_PrintUnsignedLong(char *text, size_t maxlen, SDL_FormatInfo *info, unsigned long value)
{
    char num[130];

    SDL_ultoa(value, num, info ? info->radix : 10);
    return SDL_PrintString(text, maxlen, info, num);
}

static size_t
SDL_PrintLongLong(char *text, size_t maxlen, SDL_FormatInfo *info, Sint64 value)
{
    char num[130];

    SDL_lltoa(value, num, info ? info->radix : 10);
    return SDL_PrintString(text, maxlen, info, num);
}

static size_t
SDL_PrintUnsignedLongLong(char *text, size_t maxlen, SDL_FormatInfo *info, Uint64 value)
{
    char num[130];

    SDL_ulltoa(value, num, info ? info->radix : 10);
    return SDL_PrintString(text, maxlen, info, num);
}

static size_t
SDL_PrintFloat(char *text, size_t maxlen, SDL_FormatInfo *info, double arg)
{
    int width;
    size_t len;
    size_t left = maxlen;
    char *textstart = text;

    if (arg) {
        /* This isn't especially accurate, but hey, it's easy. :) */
        unsigned long value;

        if (arg < 0) {
            if (left > 1) {
                *text = '-';
                --left;
            }
            ++text;
            arg = -arg;
        } else if (info->force_sign) {
            if (left > 1) {
                *text = '+';
                --left;
            }
            ++text;
        }
        value = (unsigned long) arg;
        len = SDL_PrintUnsignedLong(text, left, NULL, value);
        if (len >= left) {
            text += (left > 1) ? left - 1 : 0;
            left = SDL_min(left, 1);
        } else {
            text += len;
            left -= len;
        }
        arg -= value;
        if (info->precision < 0) {
            info->precision = 6;
        }
        if (info->force_type || info->precision > 0) {
            int mult = 10;
            if (left > 1) {
                *text = '.';
                --left;
            }
            ++text;
            while (info->precision-- > 0) {
                value = (unsigned long) (arg * mult);
                len = SDL_PrintUnsignedLong(text, left, NULL, value);
                if (len >= left) {
                    text += (left > 1) ? left - 1 : 0;
                    left = SDL_min(left, 1);
                } else {
                    text += len;
                    left -= len;
                }
                arg -= (double) value / mult;
                mult *= 10;
            }
        }
    } else {
        if (left > 1) {
            *text = '0';
            --left;
        }
        ++text;
        if (info->force_type) {
            if (left > 1) {
                *text = '.';
                --left;
            }
            ++text;
        }
    }

    width = info->width - (int)(text - textstart);
    if (width > 0) {
        char fill = info->pad_zeroes ? '0' : ' ';
        char *end = text+left-1;
        len = (text - textstart);
        for (len = (text - textstart); len--; ) {
            if ((textstart+len+width) < end) {
                *(textstart+len+width) = *(textstart+len);
            }
        }
        len = (size_t)width;
        if (len >= left) {
            text += (left > 1) ? left - 1 : 0;
            left = SDL_min(left, 1);
        } else {
            text += len;
            left -= len;
        }
        while (len--) {
            if (textstart+len < end) {
                textstart[len] = fill;
            }
        }
    }

    return (text - textstart);
}

int
SDL_vsnprintf(SDL_OUT_Z_CAP(maxlen) char *text, size_t maxlen, const char *fmt, va_list ap)
{
    size_t left = maxlen;
    char *textstart = text;

    if (!fmt) {
        fmt = "";
    }
    while (*fmt) {
        if (*fmt == '%') {
            SDL_bool done = SDL_FALSE;
            size_t len = 0;
            SDL_bool check_flag;
            SDL_FormatInfo info;
            enum
            {
                DO_INT,
                DO_LONG,
                DO_LONGLONG
            } inttype = DO_INT;

            SDL_zero(info);
            info.radix = 10;
            info.precision = -1;

            check_flag = SDL_TRUE;
            while (check_flag) {
                ++fmt;
                switch (*fmt) {
                case '-':
                    info.left_justify = SDL_TRUE;
                    break;
                case '+':
                    info.force_sign = SDL_TRUE;
                    break;
                case '#':
                    info.force_type = SDL_TRUE;
                    break;
                case '0':
                    info.pad_zeroes = SDL_TRUE;
                    break;
                default:
                    check_flag = SDL_FALSE;
                    break;
                }
            }

            if (*fmt >= '0' && *fmt <= '9') {
                info.width = strtol(fmt, (char **)&fmt, 0);
            }

            if (*fmt == '.') {
                ++fmt;
                if (*fmt >= '0' && *fmt <= '9') {
                    info.precision = strtol(fmt, (char **)&fmt, 0);
                } else {
                    info.precision = 0;
                }
            }

            while (!done) {
                switch (*fmt) {
                case '%':
                    if (left > 1) {
                        *text = '%';
                    }
                    len = 1;
                    done = SDL_TRUE;
                    break;
                case 'c':
                    /* char is promoted to int when passed through (...) */
                    if (left > 1) {
                        *text = (char) va_arg(ap, int);
                    }
                    len = 1;
                    done = SDL_TRUE;
                    break;
                case 'h':
                    /* short is promoted to int when passed through (...) */
                    break;
                case 'l':
                    if (inttype < DO_LONGLONG) {
                        ++inttype;
                    }
                    break;
                case 'I':
                    if (SDL_strncmp(fmt, "I64", 3) == 0) {
                        fmt += 2;
                        inttype = DO_LONGLONG;
                    }
                    break;
                case 'i':
                case 'd':
                    switch (inttype) {
                    case DO_INT:
                        len = SDL_PrintLong(text, left, &info,
                                            (long) va_arg(ap, int));
                        break;
                    case DO_LONG:
                        len = SDL_PrintLong(text, left, &info,
                                            va_arg(ap, long));
                        break;
                    case DO_LONGLONG:
                        len = SDL_PrintLongLong(text, left, &info,
                                                va_arg(ap, Sint64));
                        break;
                    }
                    done = SDL_TRUE;
                    break;
                case 'p':
                case 'x':
                    info.force_case = SDL_CASE_LOWER;
                    /* Fall through to 'X' handling */
                case 'X':
                    if (info.force_case == SDL_CASE_NOCHANGE) {
                        info.force_case = SDL_CASE_UPPER;
                    }
                    if (info.radix == 10) {
                        info.radix = 16;
                    }
                    if (*fmt == 'p') {
                        inttype = DO_LONG;
                    }
                    /* Fall through to unsigned handling */
                case 'o':
                    if (info.radix == 10) {
                        info.radix = 8;
                    }
                    /* Fall through to unsigned handling */
                case 'u':
                    info.pad_zeroes = SDL_TRUE;
                    switch (inttype) {
                    case DO_INT:
                        len = SDL_PrintUnsignedLong(text, left, &info,
                                                    (unsigned long)
                                                    va_arg(ap, unsigned int));
                        break;
                    case DO_LONG:
                        len = SDL_PrintUnsignedLong(text, left, &info,
                                                    va_arg(ap, unsigned long));
                        break;
                    case DO_LONGLONG:
                        len = SDL_PrintUnsignedLongLong(text, left, &info,
                                                        va_arg(ap, Uint64));
                        break;
                    }
                    done = SDL_TRUE;
                    break;
                case 'f':
                    len = SDL_PrintFloat(text, left, &info, va_arg(ap, double));
                    done = SDL_TRUE;
                    break;
                case 's':
                    len = SDL_PrintString(text, left, &info, va_arg(ap, char *));
                    done = SDL_TRUE;
                    break;
                default:
                    done = SDL_TRUE;
                    break;
                }
                ++fmt;
            }
            if (len >= left) {
                text += (left > 1) ? left - 1 : 0;
                left = SDL_min(left, 1);
            } else {
                text += len;
                left -= len;
            }
        } else {
            if (left > 1) {
                *text = *fmt;
                --left;
            }
            ++fmt;
            ++text;
        }
    }
    if (left > 0) {
        *text = '\0';
    }
    return (int)(text - textstart);
}
#endif /* HAVE_VSNPRINTF */

/* vi: set ts=4 sw=4 expandtab: */
