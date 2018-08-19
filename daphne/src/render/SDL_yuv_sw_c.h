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

#include <stdint.h>

#include "../SDL_internal.h"

#include "SDL_video.h"

/* This is the software implementation of the YUV texture support */

#ifndef _SDL_SW_YUVTexture_
#define _SDL_SW_YUVTexture_
struct SDL_SW_YUVTexture
{
    uint32_t format;
    uint32_t target_format;
    int w, h;
    uint8_t *pixels;
    int *colortab;
    uint32_t *rgb_2_pix;
    void (*Display1X) (int *colortab, uint32_t * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod);
    void (*Display2X) (int *colortab, uint32_t * rgb_2_pix,
                       unsigned char *lum, unsigned char *cr,
                       unsigned char *cb, unsigned char *out,
                       int rows, int cols, int mod);

    /* These are just so we don't have to allocate them separately */
    uint16_t pitches[3];
    uint8_t *planes[3];

    /* This is a temporary surface in case we have to stretch copy */
    SDL_Surface *stretch;
    SDL_Surface *display;
};
#endif
typedef struct SDL_SW_YUVTexture SDL_SW_YUVTexture;

SDL_SW_YUVTexture *SDL_SW_CreateYUVTexture(uint32_t format, int w, int h);

#ifdef __cplusplus
extern "C" {
#endif
extern DECLSPEC SDL_SW_YUVTexture * SDLCALL SDL_RJS_SW_CreateYUVBuffer(uint32_t format, uint32_t target_format, int w, int h);
extern DECLSPEC int SDLCALL SDL_RJS_SW_CopyYUVToRGB(SDL_SW_YUVTexture * swdata, const SDL_Rect * srcrect, uint32_t target_format, int w, int h, void *pixels, int pitch);
#ifdef __cplusplus
}
#endif

int SDL_SW_QueryYUVTexturePixels(SDL_SW_YUVTexture * swdata, void **pixels,
                                 int *pitch);
int SDL_SW_UpdateYUVTexture(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                            const void *pixels, int pitch);
int SDL_SW_UpdateYUVTexturePlanar(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                                  const uint8_t *Yplane, int Ypitch,
                                  const uint8_t *Uplane, int Upitch,
                                  const uint8_t *Vplane, int Vpitch);
int SDL_SW_LockYUVTexture(SDL_SW_YUVTexture * swdata, const SDL_Rect * rect,
                          void **pixels, int *pitch);
void SDL_SW_UnlockYUVTexture(SDL_SW_YUVTexture * swdata);
int SDL_SW_CopyYUVToRGB(SDL_SW_YUVTexture * swdata, const SDL_Rect * srcrect,
                        uint32_t target_format, int w, int h, void *pixels,
                        int pitch);
void SDL_SW_DestroyYUVTexture(SDL_SW_YUVTexture * swdata);

/* vi: set ts=4 sw=4 expandtab: */
