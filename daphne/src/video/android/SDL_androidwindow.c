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
#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_ANDROID

#include "SDL_syswm.h"
#include "SDL_androidwindow.h"

// 2017.09.20 - RJS - these kept for legacy
int Android_CreateWindow(_THIS, SDL_Window * window) { return 0; }
void Android_SetWindowTitle(_THIS, SDL_Window * window)	{ return; }
void Android_DestroyWindow(_THIS, SDL_Window * window) { return; }
SDL_bool Android_GetWindowWMInfo(_THIS, SDL_Window * window, SDL_SysWMinfo * info) { return SDL_TRUE; }

#endif /* SDL_VIDEO_DRIVER_ANDROID */
