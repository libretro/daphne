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

#include "../SDL_internal.h"

#if defined(__WIN32__)
#include "../core/windows/SDL_windows.h"
#endif

#include "SDL_stdinc.h"

#if defined(__WIN32__) && (!defined(HAVE_SETENV) || !defined(HAVE_GETENV))
/* Note this isn't thread-safe! */
static char *SDL_envmem = NULL; /* Ugh, memory leak */
static size_t SDL_envmemlen = 0;
#endif

/* Retrieve a variable named "name" from the environment */
#if defined(HAVE_GETENV)
char *
SDL_getenv(const char *name)
{
    /* Input validation */
    if (!name || strlen(name)==0) {
        return NULL;
    }

    return getenv(name);
}
#elif defined(__WIN32__)
char *
SDL_getenv(const char *name)
{
    size_t bufferlen;

    /* Input validation */
    if (!name || strlen(name)==0) {
        return NULL;
    }
    
    bufferlen =
        GetEnvironmentVariableA(name, SDL_envmem, (DWORD) SDL_envmemlen);
    if (bufferlen == 0) {
        return NULL;
    }
    if (bufferlen > SDL_envmemlen) {
        char *newmem = (char *) realloc(SDL_envmem, bufferlen);
        if (newmem == NULL) {
            return NULL;
        }
        SDL_envmem = newmem;
        SDL_envmemlen = bufferlen;
        GetEnvironmentVariableA(name, SDL_envmem, (DWORD) SDL_envmemlen);
    }
    return SDL_envmem;
}
#else
char *
SDL_getenv(const char *name)
{
    int len, i;
    char *value;

    /* Input validation */
    if (!name || strlen(name)==0) {
        return NULL;
    }
    
    value = (char *) 0;
    if (SDL_env) {
        len = strlen(name);
        for (i = 0; SDL_env[i] && !value; ++i) {
            if ((strncmp(SDL_env[i], name, len) == 0) &&
                (SDL_env[i][len] == '=')) {
                value = &SDL_env[i][len + 1];
            }
        }
    }
    return value;
}
#endif

/* vi: set ts=4 sw=4 expandtab: */
