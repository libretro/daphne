/*
 * video.cpp
 *
 * Copyright (C) 2001 Matt Ownby
 *
 * This file is part of DAPHNE, a laserdisc arcade game emulator
 *
 * DAPHNE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DAPHNE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// video.cpp
// Part of the DAPHNE emulator
// This code started by Matt Ownby, May 2000
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>	// for some error messages
#include "video.h"
#include "palette.h"
#include "SDL_DrawText.h"
#include "../io/conout.h"
#include "../io/error.h"
#include "../io/mpo_fileio.h"
#include "../io/mpo_mem.h"
#include "../game/game.h"
#include "../ldp-out/ldp.h"
// RJS - ADD - needed for home directory
#include "../io/homedir.h"

using namespace std;

unsigned int g_vid_width = 640, g_vid_height = 480;	// default video width and video height
#ifdef DEBUG
const Uint16 cg_normalwidths[] = { 320, 640, 800, 1024, 1280, 1280, 1600 };
const Uint16 cg_normalheights[]= { 240, 480, 600, 768, 960, 1024, 1200 };
#else
const Uint16 cg_normalwidths[] = { 640, 800, 1024, 1280, 1280, 1600 };
const Uint16 cg_normalheights[]= { 480, 600, 768, 960, 1024, 1200 };
#endif // DEBUG

// the dimensions that we draw (may differ from g_vid_width/height if aspect ratio is enforced)
unsigned int g_draw_width = 640, g_draw_height = 480;

SDL_Surface *g_led_bmps[LED_RANGE] = { 0 };
SDL_Surface *g_other_bmps[B_EMPTY] = { 0 };

SDL_Surface *g_screen = NULL;	// our primary display
SDL_Surface *g_screen_blitter = NULL;	// the surface we blit to (we don't blit directly to g_screen because opengl doesn't like that)

int sboverlay_characterset = 1;

// whether we will try to force a 4:3 aspect ratio regardless of window size
// (this is probably a good idea as a default option)
bool g_bForceAspectRatio = true;

////////////////////////////////////////////////////////////////////////////////////////////////////

// initializes the window in which we will draw our BMP's
// returns true if successful, false if failure
bool init_display()
{
	bool result = false;	// whether video initialization is successful or not
	bool abnormalscreensize = true; // assume abnormal
	Uint32 sdl_flags = 0;	// the default for this depends on whether we are using HW accelerated YUV overlays or not
	
	char s[250] = { 0 };
	Uint32 x = 0;	// temporary index

	// if we were able to initialize the video properly
	{
		// go through each standard resolution size to see if we are using a standard resolution
		for (x=0; x < (sizeof(cg_normalwidths) / sizeof(Uint16)); x++)
		{
			// if we find a match, we know we're using a standard res
			if ((g_vid_width == cg_normalwidths[x]) && (g_vid_height == cg_normalheights[x]))
			{
				abnormalscreensize = false;
			}
		}

		// if we're using a non-standard resolution
		if (abnormalscreensize)
		{
			printline("WARNING : You have specified an abnormal screen resolution! Normal screen resolutions are:");
			// print each standard resolution
			for (x=0; x < (sizeof(cg_normalwidths) / sizeof(Uint16)); x++)
			{	
				sprintf(s,"%d x %d", cg_normalwidths[x], cg_normalheights[x]);
				printline(s);
			}
			newline();
		}

		g_draw_width = g_vid_width;
		g_draw_height = g_vid_height;

		// if we're supposed to enforce the aspect ratio ...
		if (g_bForceAspectRatio)
		{
			double dCurAspectRatio = (double) g_vid_width / g_vid_height;
			const double dTARGET_ASPECT_RATIO = 4.0/3.0;

			// if current aspect ratio is less than 1.3333
			if (dCurAspectRatio < dTARGET_ASPECT_RATIO)
			{
				g_draw_height = (g_draw_width * 3) / 4;
			}
			// else if current aspect ratio is greater than 1.3333
			else if (dCurAspectRatio > dTARGET_ASPECT_RATIO)
			{
				g_draw_width = (g_draw_height * 4) / 3;
			}
			// else the aspect ratio is already correct, so don't change anything
		}

		// create a 32-bit surface
		g_screen_blitter = SDL_CreateRGBSurface(0, g_vid_width, g_vid_height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		result = true;

		if (g_screen && g_screen_blitter)
		{
         // NOTE: SDL Console was initialized here.
         result                = true;

			// sometimes the screen initializes to white, so this attempts to prevent that
			vid_blank();
			vid_flip();
			vid_blank();
			vid_flip();
		}
	}

	if (result == 0)
	{
		sprintf(s, "Could not initialize video display: %s", SDL_GetError());
		printerror(s);
	}

	return(result);

}

// shuts down video display
void shutdown_display()
{
	printline("Shutting down video display...");
}

void vid_flip()
{
	// RJS NOTE - this is probably where we need to set flag or something
	// for the LDP thread to pick up the surface . . . maybe (still early in the process)
	// if we're not using OpenGL, then just use the regular SDL Flip ...
	{
		// RJS CHANGE
		// orig SDL_Flip(g_screen);
		printline("vid_flip not supported in this thread.");
	}
}

void vid_blank()
{
   SDL_FillRect(g_screen, NULL, 0);
}

// RJS CHANGE - MAIN THREAD back to surface, we'll need to "blit" this to the overlay/UI surface (g_screen)
void vid_blit(SDL_Surface *srf, int x, int y)
{
	if (g_ldp->is_blitting_allowed())
   {
      SDL_Rect dest;
      dest.x = (int16_t) x;
      dest.y = (int16_t) y;
      dest.w = (uint16_t) srf->w;
      dest.h = (uint16_t) srf->h;
      SDL_BlitSurface(srf, NULL, g_screen, &dest);
   }
	// else blitting isn't allowed, so just ignore
}

// redraws the proper display (Scoreboard, etc) on the screen, after first clearing the screen
// call this every time you want the display to return to normal
void display_repaint()
{
	vid_blank();
	vid_flip();
	g_game->video_force_blit();
}

static SDL_Surface *load_one_bmp(const char *filename);

// loads all the .bmp's used by DAPHNE
// returns true if they were all successfully loaded, or a false if they weren't
bool load_bmps()
{
	// 2017.02.01 - RJS ADD - Logging.

	bool result = true;	// assume success unless we hear otherwise
	int index = 0;
	char filename[1024];

	// RJS - ADD - Need the place downloaded bmps are since we are not 
	// embedding them in .so.
	char strBMPhomedir[1024];
	sprintf(strBMPhomedir, "%s/", g_homedir.get_homedir().c_str());

	for (; index < LED_RANGE; index++)
	{
		// RJS TODO - this will be fun
		sprintf(filename, "%spics/led%d.bmp", strBMPhomedir, index);

		g_led_bmps[index] = load_one_bmp(filename);

		// If the bit map did not successfully load
		if (g_led_bmps[index] == 0)
		{
			result = false;
		}
	}

	// RJS - ADD - the sprintfs to handle the homedir, literal string no longer passed in beacuse of this
	sprintf(filename, "%spics/player1.bmp", strBMPhomedir);
	g_other_bmps[B_DL_PLAYER1] = load_one_bmp(filename);

	sprintf(filename, "%spics/player2.bmp", strBMPhomedir);
	g_other_bmps[B_DL_PLAYER2] = load_one_bmp(filename);

	sprintf(filename, "%spics/lives.bmp", strBMPhomedir);
	g_other_bmps[B_DL_LIVES] = load_one_bmp(filename);

	sprintf(filename, "%spics/credits.bmp", strBMPhomedir);
	g_other_bmps[B_DL_CREDITS] = load_one_bmp(filename);

	sprintf(filename, "%spics/saveme.bmp", strBMPhomedir);
	g_other_bmps[B_DAPHNE_SAVEME] = load_one_bmp(filename);

	sprintf(filename, "%spics/gamenowook.bmp", strBMPhomedir);
	g_other_bmps[B_GAMENOWOOK] = load_one_bmp(filename);

    if (sboverlay_characterset != 2)
		sprintf(filename, "%spics/overlayleds1.bmp", strBMPhomedir);
	else
		sprintf(filename, "%spics/overlayleds2.bmp", strBMPhomedir);
	g_other_bmps[B_OVERLAY_LEDS] = load_one_bmp(filename);

	sprintf(filename, "%spics/ldp1450font.bmp", strBMPhomedir);
	g_other_bmps[B_OVERLAY_LDP1450] = load_one_bmp(filename);

	// check to make sure they all loaded
	for (index = 0; index < B_EMPTY; index++)
	{
		if (g_other_bmps[index] == NULL)
			result = false;
	}

	return(result);
}

static SDL_Surface *load_one_bmp(const char *filename)
{
	SDL_Surface *result = SDL_LoadBMP(filename);
	if (!result)
	{
		string err = "Could not load bitmap : ";
		// RJS - CHANGE - Adding essentially a TODO to this error.
		// err = err + filename;
		err = err + filename + " - Need to report back to LR that there is an error to display or shutdown.";
		printerror(err.c_str());
	}

	return(result);
}

// Draw's one of our LED's to the screen
// value contains the bitmap to draw (0-9 is valid)
// x and y contain the coordinates on the screen
// This function is called from scoreboard.cpp
// 1 is returned on success, 0 on failure
bool draw_led(int value, int x, int y)
{
	vid_blit(g_led_bmps[value], x, y);
	return true;
}

// Draw overlay digits to the screen
// RJS CHANGE BACK - MAIN THREAD to surfaces
void draw_overlay_leds(unsigned int values[], int num_digits, int start_x, int y, SDL_Surface *overlay)
{
	SDL_Rect src, dest;

	dest.x = start_x;
	dest.y = y;
	dest.w = OVERLAY_LED_WIDTH;
	dest.h = OVERLAY_LED_HEIGHT;

    src.y = 0;
    src.w = OVERLAY_LED_WIDTH;
    src.h = OVERLAY_LED_HEIGHT;
		
	/* Draw the digit(s) */
	for(int i = 0; i < num_digits; i++)
	{
		src.x = values[i] * OVERLAY_LED_WIDTH;

		// 2017.11.09 - RJS - Added ability for space to be replaced with 0.
      if (src.x == (0x0F * OVERLAY_LED_WIDTH)) src.x = 0x00;

		// RJS CHANGE BACK - MAIN THREAD to surfaces
		/*
		if (i == 0)
		{
			for (int j = 0; j < OVERLAY_LED_HEIGHT; j++)
			{
				uint8_t * temp = ((uint8_t *) overlay->pixels) + dest.x + (overlay->pitch * j);
			}
			for (int j = 0; j < OVERLAY_LED_HEIGHT; j++)
			{
				uint8_t * temp = ((uint8_t *)g_other_bmps[B_OVERLAY_LEDS]->pixels) + src.x + (g_other_bmps[B_OVERLAY_LEDS]->pitch * j);
			}

			for (int j = 0; j < overlay->format->palette->ncolors; j++)
			{
				SDL_Color * curr_color = (overlay->format->palette->colors) + j;
			}
		}
		*/

		// 2017.11.09 - RJS - Sometime the palette's alpha component is 0 sometimes not 0.  This seem to change the way the blit works.
		// That is my current theory, until I'm certain, hacking this bit here.
		if (overlay->format->palette->colors->a != 126)
		{
			overlay->format->palette->colors->a = 126;
		}

		SDL_BlitSurface(g_other_bmps[B_OVERLAY_LEDS], &src, overlay, &dest);
		
		/*
		if (i == 0)
		{
			for (int j = 0; j < OVERLAY_LED_HEIGHT; j++)
			{
				uint8_t * temp = ((uint8_t *)overlay->pixels) + dest.x + (overlay->pitch * j);
			}
		}
		*/
		
		dest.x += OVERLAY_LED_WIDTH;
	}

	// RJS REMOVE - UpdateRects not in SDL2, should not be need here since
	// we are just updating the given surface (not texture) and will render it later
	/*
    dest.x = start_x;
    dest.w = num_digits * OVERLAY_LED_WIDTH;
    SDL_UpdateRects(overlay, 1, &dest);
	*/
}

// Draw LDP1450 overlay characters to the screen (added by Brad O.)
void draw_singleline_LDP1450(char *LDP1450_String, int start_x, int y, SDL_Surface *overlay)
{
	SDL_Rect src, dest;

	int i = 0;
	int value = 0;
	int LDP1450_strlen;
//	char s[81]="";

	dest.x = start_x;
	dest.y = y;
	dest.w = OVERLAY_LDP1450_WIDTH;
	dest.h = OVERLAY_LDP1450_HEIGHT;

	src.y = 0;
	src.w = OVERLAY_LDP1450_WIDTH;
	src.h = OVERLAY_LDP1450_WIDTH;

	LDP1450_strlen = strlen(LDP1450_String);

	if (!LDP1450_strlen)              // if a blank line is sent, we must blank out the entire line
	{
		strcpy(LDP1450_String,"           ");
		LDP1450_strlen = strlen(LDP1450_String);
	}
	else 
	{
		if (LDP1450_strlen <= 11)	 // pad end of string with spaces (in case previous line was not cleared)
		{
			for (i=LDP1450_strlen; i<=11; i++)
				LDP1450_String[i] = 32;
			LDP1450_strlen = strlen(LDP1450_String);
		}
	}

	for (i=0; i<LDP1450_strlen; i++)
	{
		value = LDP1450_String[i];

		if (value >= 0x26 && value <= 0x39)       // numbers and symbols
			value -= 0x25;
		else if (value >= 0x41 && value <= 0x5a)  // alpha
			value -= 0x2a;
		else if (value == 0x13)                   // special LDP-1450 character (inversed space)
			value = 0x32;
		else
			value = 0x31;			              // if not a number, symbol, or alpha, recognize as a space

		src.x = value * OVERLAY_LDP1450_WIDTH;

		SDL_BlitSurface(g_other_bmps[B_OVERLAY_LDP1450], &src, overlay, &dest);

		dest.x += OVERLAY_LDP1450_CHARACTER_SPACING;
	}
	dest.x = start_x;
	dest.w = LDP1450_strlen * OVERLAY_LDP1450_CHARACTER_SPACING;

	// MPO : calling UpdateRects probably isn't necessary and may be harmful
	//SDL_UpdateRects(overlay, 1, &dest);
}

//  used to draw non LED stuff like scoreboard text
//  'which' corresponds to enumerated values
bool draw_othergfx(int which, int x, int y, bool bSendToScreenBlitter)
{
	// NOTE : this is drawn to g_screen_blitter, not to g_screen,
	//  to be more friendly to our opengl implementation!
	SDL_Surface *srf = g_other_bmps[which];
	SDL_Rect dest;
	dest.x = (int16_t) x;
	dest.y = (int16_t) y;
	dest.w = (uint16_t)srf->w;
	dest.h = (uint16_t)srf->h;

	// if we should blit this to the screen blitter for later use ...
	if (bSendToScreenBlitter)
	{
		SDL_BlitSurface(srf, NULL, g_screen_blitter, &dest);
	}
	// else blit it now
	else
	{
		vid_blit(g_other_bmps[which], x, y);
	}
	return true;
}

static void free_one_bmp(SDL_Surface *candidate)
{
	SDL_FreeSurface(candidate);
}

// de-allocates all of the .bmps that we have allocated
void free_bmps(void)
{
	int nuke_index = 0;

	// get rid of all the LED's
	for (; nuke_index < LED_RANGE; nuke_index++)
		free_one_bmp(g_led_bmps[nuke_index]);
	for (nuke_index = 0; nuke_index < B_EMPTY; nuke_index++)
	{
		// check to make sure it exists before we try to free
		if (g_other_bmps[nuke_index])
			free_one_bmp(g_other_bmps[nuke_index]);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////

SDL_Surface *get_screen()
{
	return g_screen;
}

SDL_Surface *get_screen_blitter()
{
	return g_screen_blitter;
}

void set_sboverlay_characterset(int value)
{
	sboverlay_characterset = value;
}

// returns video width
Uint16 get_video_width()
{
	return g_vid_width;
}

// sets g_vid_width
void set_video_width(Uint16 width)
{
	// Let the user specify whatever width s/he wants (and suffer the consequences)
	// We need to support arbitrary resolution to accomodate stuff like screen rotation
	g_vid_width = width;
}

// returns video height
Uint16 get_video_height()
{
	return g_vid_height;
}

// sets g_vid_height
void set_video_height(Uint16 height)
{
	// Let the user specify whatever height s/he wants (and suffer the consequences)
	// We need to support arbitrary resolution to accomodate stuff like screen rotation
	g_vid_height = height;
}

///////////////////////////////////////////////////////////////////////////////////////////

void draw_string(const char* t, int col, int row, SDL_Surface* overlay)
{
	SDL_Rect dest;

	dest.x = (int16_t) ((col*6));
	dest.y = (int16_t) ((row*13));
	dest.w = (uint16_t) (6 * strlen(t)); // width of rectangle area to draw (width of font * length of string)
	dest.h = 13;	// height of area (height of font)
	
	SDL_FillRect(overlay, &dest, 0); // erase anything at our destination before we print new text
	SDLDrawText(t,  overlay, FONT_SMALL, dest.x, dest.y);
	// RJS REMOVE - MAIN THREAD this shouldn't be needed since we're just copying to render in the LDP thread
	// SDL_UpdateRects(overlay, 1, &dest);
}

void set_force_aspect_ratio(bool bEnabled)
{
	g_bForceAspectRatio = bEnabled;
}

bool get_force_aspect_ratio()
{
	return g_bForceAspectRatio;
}
