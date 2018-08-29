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
#include "./SDL_internal.h"

#if defined(__WIN32__)
#include "core/windows/SDL_windows.h"
#endif

/* Initialization code for SDL */

#include <assert.h>

#include "SDL.h"
#include "SDL_bits.h"
#include "SDL_revision.h"
#include "events/SDL_events_c.h"
#include "joystick/SDL_joystick_c.h"

int
SDL_InitSubSystem(uint32_t flags)
{
    /* Clear the error message */
    SDL_ClearError();

    if ((flags & SDL_INIT_GAMECONTROLLER)) {
        /* game controller implies joystick */
        flags |= SDL_INIT_JOYSTICK;
    }

    if ((flags & (SDL_INIT_VIDEO|SDL_INIT_JOYSTICK))) {
        /* video or joystick implies events */
        flags |= SDL_INIT_EVENTS;
    }

    return (0);
}

int
SDL_Init(uint32_t flags)
{
    return SDL_InitSubSystem(flags);
}

void
SDL_QuitSubSystem(uint32_t flags)
{
}

void
SDL_Quit(void)
{
    SDL_ClearHints();
    SDL_AssertionsQuit();
}

/* vi: set sts=4 ts=4 sw=4 expandtab: */
