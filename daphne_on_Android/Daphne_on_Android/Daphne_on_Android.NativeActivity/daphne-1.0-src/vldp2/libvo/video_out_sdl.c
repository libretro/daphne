/*
 * video_out_sdl.c
 *
 * Copyright (C) 2000-2002 Ryan C. Gordon <icculus@lokigames.com> and
 *                         Dominik Schnitzer <aeneas@linuxvideo.org>
 *
 * SDL info, source, and binaries can be found at http://www.libsdl.org/
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef WIN32
#pragma warning (disable:4996)
#endif

// RJS CHANGE
// #include "config.h"
#include "../include/config.h"

#ifdef LIBVO_SDL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// RJS CHANGE
// #include <SDL/SDL.h>
#include <SDL.h>

#include <inttypes.h>

// RJS CHANGE
// #include "video_out.h"
#include "../include/video_out.h"

// RJS ADD
extern SDL_Renderer *g_renderer;

// RJS ADD
extern SDL_Window *g_screen;

// RJS START
// typedef struct {
//     vo_instance_t vo;
//     int width;
//     int height;
//     SDL_Surface * surface;
//     Uint32 sdlflags;
//     Uint8 bpp;
// } sdl_instance_t;

typedef struct
{
	vo_instance_t vo;
	int width;
	int height;
	SDL_Window * surface;
	Uint32 sdlflags;
	Uint8 bpp;
} sdl_instance_t;
// RJS END

static void sdl_setup_fbuf (vo_instance_t * _instance,
			    uint8_t ** buf, void ** id)
{
    sdl_instance_t * instance = (sdl_instance_t *) _instance;
	
	// RJS CHANGE
	// SDL_Overlay * overlay;
	SDL_Texture * overlay;

	// RJS START - Planes are not in textures, some of this needs to be thought out.
    // *id = overlay = SDL_CreateYUVOverlay (instance->width, instance->height,
	// 				  SDL_YV12_OVERLAY, instance->surface);
    // buf[0] = overlay->pixels[0];
    // buf[1] = overlay->pixels[2];
    // buf[2] = overlay->pixels[1];
    *id = overlay = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, instance->width, instance->height);
    void * overlay_pixels = NULL;
	buf[0] = NULL;
	buf[1] = NULL;
	buf[2] = NULL;
	if (SDL_LockTexture(overlay, NULL, &overlay_pixels, NULL) == 0)
	{
		buf[0] = (uint8_t *) overlay_pixels;
		SDL_UnlockTexture(overlay);
	}
	// RJS END
}

static void sdl_start_fbuf (vo_instance_t * instance,
			    uint8_t * const * buf, void * id)
{
	// RJS START
    // SDL_LockYUVOverlay ((SDL_Overlay *) id);
	void * overlay_pixels = NULL;
	SDL_LockTexture((SDL_Texture *) id, NULL, &overlay_pixels, NULL);
	// RJS NOTE !!! the overlay_pixels needs to be passed back or lock not done
	// done using this routine but with caller.
	// RJS END
}

static void sdl_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
	// RJS REMOVED - unsused
    // sdl_instance_t * instance = (sdl_instance_t *) _instance;

	// RJS CHANGE
    // SDL_Overlay * overlay = (SDL_Overlay *) id;
    SDL_Texture * overlay = (SDL_Texture *) id;
    SDL_Event event;
	
	// RJS START
    // while (SDL_PollEvent (&event))
	// if (event.type == SDL_VIDEORESIZE)
	//   instance->surface =
	// 	SDL_SetVideoMode (event.resize.w, event.resize.h,
	// 			  instance->bpp, instance->sdlflags);
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_WINDOWEVENT)
		{
			switch (event.window.event)
			{
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
				{
					SDL_SetWindowSize(g_screen, event.window.data1, event.window.data2);
				}
			}
		}
	}
	// RJS END

	// RJS START - Do we need to use a rect for this?  Routine is "draw frame".
    // SDL_DisplayYUVOverlay (overlay, &(instance->surface->clip_rect));
    SDL_RenderCopy(g_renderer, overlay, NULL, NULL);
	// RJS END
}

static void sdl_discard (vo_instance_t * _instance,
			 uint8_t * const * buf, void * id)
{
	// RJS CHANGE
    // SDL_UnlockYUVOverlay ((SDL_Overlay *) id);
	SDL_UnlockTexture((SDL_Texture *) id);
}

#if 0
static void sdl_close (vo_instance_t * _instance)
{
    sdl_instance_t * instance;
    int i;

    instance = (sdl_instance_t *) _instance;
    for (i = 0; i < 3; i++)
	SDL_FreeYUVOverlay (instance->frame[i].overlay);
    SDL_FreeSurface (instance->surface);
    SDL_QuitSubSystem (SDL_INIT_VIDEO);
}
#endif

static int sdl_setup (vo_instance_t * _instance, int width, int height,
		      vo_setup_result_t * result)
{
    sdl_instance_t * instance;

    instance = (sdl_instance_t *) _instance;

    instance->width = width;
    instance->height = height;
	
	// RJS START
    // instance->surface = SDL_SetVideoMode (width, height, instance->bpp,
	// 				  instance->sdlflags);
    instance->surface = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
										width, height, instance->sdlflags);
	// RJS END

    if (! (instance->surface)) {
	fprintf (stderr, "sdl could not set the desired video mode\n");
	return 1;
    }

    result->convert = NULL;
    return 0;
}

vo_instance_t * vo_sdl_open (void)
{
    sdl_instance_t * instance;
	// RJS REMOVE - Um, what?  Improper, or at least, undefined use of const.
    // const SDL_VideoInfo * vidInfo;

    instance = (sdl_instance_t *) malloc (sizeof (sdl_instance_t));
    if (instance == NULL)
	return NULL;

    instance->vo.setup = sdl_setup;
    instance->vo.setup_fbuf = sdl_setup_fbuf;
    instance->vo.set_fbuf = NULL;
    instance->vo.start_fbuf = sdl_start_fbuf;
    instance->vo.discard = sdl_discard;
    instance->vo.draw = sdl_draw_frame;
    instance->vo.close = NULL; /* sdl_close; */
	//RJS CHANGE - HW/SW is determined by SDL2 for you
    // instance->sdlflags = SDL_HWSURFACE | SDL_RESIZABLE;
	instance->sdlflags = SDL_WINDOW_FULLSCREEN;

	// RJS START
    // putenv("SDL_VIDEO_YUV_HWACCEL=1");
    // putenv("SDL_VIDEO_X11_NODIRECTCOLOR=1");
	static char strVideoHWAccel[]	= "SDL_VIDEO_YUV_HWACCEL=1";
	static char strVideoX11[]		= "SDL_VIDEO_X11_NODIRECTCOLOR=1";
    putenv(&strVideoHWAccel[0]);
    putenv(&strVideoX11[0]);
	// RJS END

    if (SDL_Init (SDL_INIT_VIDEO)) {
	fprintf (stderr, "sdl video initialization failed.\n");
	return NULL;
    }

	// RJS START - a window, which is an SDL2 concept, is resizable by its nature so
	// skipping that check.
    // vidInfo = SDL_GetVideoInfo ();
    // if (!SDL_ListModes (vidInfo->vfmt, SDL_HWSURFACE | SDL_RESIZABLE)) {
	// instance->sdlflags = SDL_RESIZABLE;
	// if (!SDL_ListModes (vidInfo->vfmt, SDL_RESIZABLE)) {
	//     fprintf (stderr, "sdl couldn't get any acceptable video mode\n");
	//     return NULL;
	// }
    // }
    // instance->bpp = vidInfo->vfmt->BitsPerPixel;
    // if (instance->bpp < 16) {
	// fprintf(stderr, "sdl has to emulate a 16 bit surfaces, "
	// 	"that will slow things down.\n");
	// instance->bpp = 16;
    // }
	int nCurr_Mode = 0;
	int nMax_Modes	= SDL_GetNumDisplayModes(0);
	SDL_DisplayMode tDisplayModeInfo;
	tDisplayModeInfo.format = SDL_PIXELFORMAT_UNKNOWN;
	for (; nCurr_Mode < nMax_Modes; nCurr_Mode++)
	{
		if (SDL_GetDisplayMode(0, nCurr_Mode, &tDisplayModeInfo) == 0)
		{
			if (SDL_BITSPERPIXEL(tDisplayModeInfo.format) >= 16) break;
		}
	}
	// RJS NOTE - not sure if 16 should be hardcoded
	instance->bpp = SDL_BITSPERPIXEL(tDisplayModeInfo.format);
	if (instance->bpp < 16) instance->bpp = 16;
	if (nCurr_Mode >= nMax_Modes)
	{
		fprintf(stderr, "SDL2 has to emulate a 16 bit surfaces, that will slow things down.\n");
	}
	// RJS END

    return (vo_instance_t *) instance;
}
#endif
