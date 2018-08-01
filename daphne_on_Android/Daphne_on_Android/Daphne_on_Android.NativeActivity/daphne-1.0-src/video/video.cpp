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
#include "SDL_Console.h"
#include "../game/game.h"
#include "../ldp-out/ldp.h"
#include "../ldp-out/ldp-vldp-gl.h"
// RJS - ADD - needed for home directory
#include "../io/homedir.h"

using namespace std;

// RJS ADD
#include "../../main_android.h"
#ifdef __ANDROID__
#include "android\asset_manager.h"
#endif

#ifndef GP2X
unsigned int g_vid_width = 640, g_vid_height = 480;	// default video width and video height
#ifdef DEBUG
const Uint16 cg_normalwidths[] = { 320, 640, 800, 1024, 1280, 1280, 1600 };
const Uint16 cg_normalheights[]= { 240, 480, 600, 768, 960, 1024, 1200 };
#else
const Uint16 cg_normalwidths[] = { 640, 800, 1024, 1280, 1280, 1600 };
const Uint16 cg_normalheights[]= { 480, 600, 768, 960, 1024, 1200 };
#endif // DEBUG
#else
unsigned int g_vid_width = 320, g_vid_height = 240;	// default for gp2x
const Uint16 cg_normalwidths[] = { 320 };
const Uint16 cg_normalheights[]= { 240 };
#endif

// the dimensions that we draw (may differ from g_vid_width/height if aspect ratio is enforced)
unsigned int g_draw_width = 640, g_draw_height = 480;

// RJS START - MAIN THREAD back to surface, rendering happen in the LDP thread
SDL_Surface *g_led_bmps[LED_RANGE] = { 0 };
SDL_Surface *g_other_bmps[B_EMPTY] = { 0 };
// SDL_Texture *g_led_bmps[LED_RANGE] = { 0 };
// SDL_Texture *g_other_bmps[B_EMPTY] = { 0 };
// RJS END

SDL_Surface *g_screen = NULL;	// our primary display
SDL_Surface *g_screen_blitter = NULL;	// the surface we blit to (we don't blit directly to g_screen because opengl doesn't like that)

// RJS ADD - this renderer isn't needed anymore
SDL_Renderer *g_renderer = NULL;

bool g_console_initialized = false;	// 1 once console is initialized
bool g_fullscreen = false;	// whether we should initialize video in fullscreen mode or not
int sboverlay_characterset = 1;

// whether we will try to force a 4:3 aspect ratio regardless of window size
// (this is probably a good idea as a default option)
bool g_bForceAspectRatio = true;

// the # of degrees to rotate counter-clockwise in opengl mode
float g_fRotateDegrees = 0.0;

////////////////////////////////////////////////////////////////////////////////////////////////////

// initializes the window in which we will draw our BMP's
// returns true if successful, false if failure
bool init_display()
{
	LOGI("daphne-libretro: In init_display, top of function.");

	bool result = false;	// whether video initialization is successful or not
	bool abnormalscreensize = true; // assume abnormal
	// RJS START
	// const SDL_VideoInfo *vidinfo = NULL;
	// Uint8 suggested_bpp = 0;
	// RJS END
	Uint32 sdl_flags = 0;	// the default for this depends on whether we are using HW accelerated YUV overlays or not
	
	// RJS REMOVED
	//char *hw_env = getenv("SDL_VIDEO_YUV_HWACCEL");

	// RJS START - SDL2 handles where a surface will be, SW or HW. This isn't needed.
	/*
	// if HW acceleration has been disabled, we need to use a SW surface due to some oddities with crashing and fullscreen
	if (hw_env && (hw_env[0] == '0'))
	{
		sdl_flags = SDL_SWSURFACE;
	}

	// else if HW acceleration hasn't been explicitely disabled ...
	else
	{

		sdl_flags = SDL_HWSURFACE;
		// Win32 notes (may not apply to linux) :
		// After digging around in the SDL source code, I've discovered a few things ...
		// When using fullscreen mode, you should always use SDL_HWSURFACE because otherwise
		// you can't use YUV hardware overlays due to SDL creating an extra surface without
		// your consent (which seems retarded to me hehe).
		// SDL_SWSURFACE + hw accelerated YUV overlays will work in windowed mode, but might
		// as well just use SDL_HWSURFACE for everything.
	}
	*/
	// RJS END

	char s[250] = { 0 };
	Uint32 x = 0;	// temporary index

	LOGI("daphne-libretro: In init_display, before SDL_InitSubSystem.");

	// if we were able to initialize the video properly
	if ( SDL_InitSubSystem(SDL_INIT_VIDEO) >=0 )
	{

		LOGI("daphne-libretro: In init_display, after positive SDL_InitSubSystem.");
	
		// RJS START - This is handled differently in SDL2, I think through textures.
		/*
		vidinfo = SDL_GetVideoInfo();
		suggested_bpp = vidinfo->vfmt->BitsPerPixel;

		// if we were in 8-bit mode, try to get at least 16-bit color
		// because we definitely use more than 256 colors in daphne
		if (suggested_bpp < 16)
		{
			suggested_bpp = 32;
		}
		*/
		// RJS END

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
		// RJS CHANGE
		// ORIG g_screen_blitter = SDL_CreateRGBSurface(SDL_SWSURFACE,
        //                                g_vid_width,
        //                                g_vid_height,
		//								32,
		//								0xff, 0xFF00, 0xFF0000, 0xFF000000);

		// RJS START - SDL2 now requires a renderer, this is an abstraction for what
		// SDL2 might use D3D, OGL, OGLes, etc.  That's why it's needed.
		// if (g_screen) g_renderer = SDL_CreateRenderer(g_screen, -1, 0);
		// RJS END

		// TEX g_screen_blitter = SDL_CreateTexture(g_renderer, SDL_BITSPERPIXEL(32), SDL_TEXTUREACCESS_STREAMING,
		//	g_vid_width, g_vid_height);

		LOGI("daphne-libretro: In init_display, before SDL_CreateRGBSurface.");

		g_screen_blitter = SDL_CreateRGBSurface(0, g_vid_width, g_vid_height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
		result = true;

		LOGI("daphne-libretro: In init_display, after SDL_CreateRGBSurface.  g_screen: %d  g_screen_blitter: %d", (int) g_screen, (int) g_screen_blitter);

		if (g_screen && g_screen_blitter)
		{
			// RJS START - removed, lazy
			// sprintf(s, "Set %dx%d at %d bpp with flags: %x", g_screen->w, g_screen->h, g_screen->format->BitsPerPixel, g_screen->flags);
			// printline(s);
			// RJS END

			// initialize SDL console in the background
			if (ConsoleInit("pics/ConsoleFont.bmp", g_screen_blitter, 100)==0)
			{
				AddCommand(g_cpu_break, "break");
				g_console_initialized = true;
				result = true;
			}
			else
			{
				printerror("Error initializing SDL console =(");			
			}

			// sometimes the screen initializes to white, so this attempts to prevent that
			vid_blank();
			vid_flip();
			vid_blank();
			vid_flip();
		}
	}

	LOGI("daphne-libretro: In init_display, after negative SDL_InitSubSystem or through sucessful positive block, see above log.");

	if (result == 0)
	{
		sprintf(s, "Could not initialize video display: %s", SDL_GetError());
		printerror(s);
	}

	LOGI("daphne-libretro: In init_display, bottom of function. result: %d", result);

	return(result);

}

// shuts down video display
void shutdown_display()
{
	printline("Shutting down video display...");

	if (g_console_initialized)
	{
		ConsoleShutdown();
		g_console_initialized = false;
	}

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void vid_flip()
{
	// RJS NOTE - this is probably where we need to set flag or something
	// for the LDP thread to pick up the surface . . . maybe (still early in the process)
	// if we're not using OpenGL, then just use the regular SDL Flip ...
	{
		// RJS CHANGE
		// orig SDL_Flip(g_screen);
		// new  SDL_RenderPresent(g_renderer);
		printline("vid_flip not supported in this thread.");
	}
}

void vid_blank()
{
   // RJS START - Render clear from SDL1 to 2
   // SDL_FillRect(g_screen, NULL, 0);
   SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
   SDL_RenderClear(g_renderer);
   // RJS END
}

// RJS CHANGE - MAIN THREAD back to surface, we'll need to "blit" this to the overlay/UI surface (g_screen)
void vid_blit(SDL_Surface *srf, int x, int y)
{
	if (g_ldp->is_blitting_allowed())
   {
      SDL_Rect dest;
      dest.x = (short) x;
      dest.y = (short) y;
      dest.w = (unsigned short) srf->w;
      dest.h = (unsigned short) srf->h;
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

// loads all the .bmp's used by DAPHNE
// returns true if they were all successfully loaded, or a false if they weren't
bool load_bmps()
{
	// 2017.02.01 - RJS ADD - Logging.
	LOGI("daphne-libretro: In load_bmps, top of routine.");

	bool result = true;	// assume success unless we hear otherwise
	int index = 0;
#ifdef __ANDROID__
	char filename[PATH_MAX];
#else
	char filename[_MAX_PATH];
#endif

	// RJS - ADD - Need the place downloaded bmps are since we are not 
	// embedding them in .so.
#ifdef __ANDROID__
	char strBMPhomedir[PATH_MAX];
#else
	char strBMPhomedir[_MAX_PATH];
#endif
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
		{
			result = false;
		}
	}

	// 2017.02.01 - RJS ADD - Logging.
	LOGI("daphne-libretro: In load_bmps, bottom of routine.");
	return(result);
}


// RJS REMOVE - MAIN THREAD back to surface, keeping old routine in case need to go back again
/*
SDL_Texture *load_one_bmp(const char *filename)
{
	SDL_Surface *result = SDL_LoadBMP(filename);
	if (!result)
	{
		string err = "Could not load bitmap : ";
		err = err + filename;
		printerror(err.c_str());
		return(NULL);
	}

	SDL_Texture * sdlTexture = SDL_CreateTextureFromSurface(g_renderer, result);
	SDL_FreeSurface(result);

	Uint32 textFormat;
	int textAccess;
	int textW;
	int textH;
	SDL_QueryTexture(sdlTexture, &textFormat, &textAccess, &textW, &textH);

	return(sdlTexture);
}
*/

// 2017.06.22 - RJS ADD - Since files are saved into the .so which is a zip and therefore we don't have a
// typical path to them we need to copy them out.  This routine does that.  A later consideration should
// be to output all files at once.
bool copy_android_asset_to_directory(const char *filename)
{
	/*
	AAssetManager * mgr = app->activity->assetManager;
	AAssetDir* assetDir = AAssetManager_openDir(mgr, "");
	const char* filename = (const char*)NULL;

	while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
		AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_STREAMING);
		char buf[BUFSIZ];
		int nb_read = 0;
		FILE* out = fopen(filename, "w");
		while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
			fwrite(buf, nb_read, 1, out);
		fclose(out);
		AAsset_close(asset);
	}

	AAssetDir_close(assetDir);
	*/
	return true;
}
// RJS END

// 2017.07.14 - RJS ADD (below includes) - see JavaVM stuff in below routine (load_one_bmp).
/*
#include <dlfcn.h>
#include "android\asset_manager.h"
#include "android\asset_manager_jni.h"
#include "jni.h"
*/

SDL_Surface *load_one_bmp(const char *filename)
{
	// 2017.02.01 - RJS ADD - Logging.
	LOGI("daphne-libretro: In load_one_bmp, top of routine.  File: %s", filename);
	
	// 2017.07.14 - RJS - Adding asset loading in Android.  If asset doesn't exist, then just falls
	// through.  Problems are we don;t have a JavaVM here.  I've been looking into how to do it, there
	// is no official way to do this without it comming from JNI.  Going to try to use what is in the
	// Android frameworks DdmConnection.cpp, DdmConnection::start(const char* name).  This is clearly
	// not efficient in the long run, just hacking in here for ease of debugging.
	// 2017.07.20 - RJS - So this just won't work.  First off, there is no concept of assets in just
	// a .so file, duh.  They are kept in the apk itself.  The only real way to embed a data file
	// in a .so is to create an offline tool that will read the file and create the char[] or
	// some such data structure.  Punting on this and will require support files to be downloaded
	// sort of like needing a BIOS file for an emulator to work.  The code trying to get a JavaVM
	// has been copied to SDL_android.c . . . hope it works.
	/*
	JavaVM * vm		= NULL;
	JNIEnv * env	= NULL;

	JavaVMInitArgs	args;
	JavaVMOption	opt;
	opt.optionString		= "-agentlib:jdwp=transport=dt_android_adb,suspend=n,server=y";
	args.version			= JNI_VERSION_1_4;
	args.options			= &opt;
	args.nOptions			= 1;
	args.ignoreUnrecognized	= JNI_FALSE;

	void * libdvm_dso = dlopen("libdvm.so", RTLD_NOW);
	if (!libdvm_dso)
	{
		LOGI("daphne-libretro: In load_one_bmp, libdvm, couldn't open: %s", dlerror());
	}

	void * libandroid_runtime_dso = dlopen("libandroid_runtime.so", RTLD_NOW);
	if (!libandroid_runtime_dso)
	{
		LOGI("daphne-libretro: In load_one_bmp, libandroid_runtime, couldn't open: %s", dlerror());
	}

	if ((!libdvm_dso) || (!libandroid_runtime_dso))
	{
		if (libandroid_runtime_dso) dlclose(libandroid_runtime_dso);
		if (libdvm_dso) dlclose(libdvm_dso);
	} else {
		jint(*JNI_CreateJavaVM)(JavaVM** p_vm, JNIEnv** p_env, void* vm_args);
		JNI_CreateJavaVM = (typeof JNI_CreateJavaVM)dlsym(libdvm_dso, "JNI_CreateJavaVM");
		if (!JNI_CreateJavaVM)
		{
			LOGI("daphne-libretro: In load_one_bmp, JNI_CreateJavaVM, couldn't find symbol: %s", dlerror());
		}

		jint(*registerNatives)(JNIEnv* env, jclass clazz);
		registerNatives = (typeof registerNatives)dlsym(libandroid_runtime_dso, "Java_com_android_internal_util_WithFramework_registerNatives");
		if (!registerNatives)
		{
			LOGI("daphne-libretro: In load_one_bmp, Java_com_android_internal_util_WithFramework_registerNatives, couldn't find symbol: %s", dlerror());
		}

		if ((!JNI_CreateJavaVM) || (!registerNatives)) {
			if (libandroid_runtime_dso) dlclose(libandroid_runtime_dso);
			if (libdvm_dso) dlclose(libdvm_dso);
		} else {
			if (JNI_CreateJavaVM(&vm, &env, &args) == 0)
			{
				if (registerNatives(env, 0) == 0)
				{
					env.
					AAssetManager * assetMgr = AAssetManager_fromJava(env, )
				}
			}
		}
	}
	*/
	// 2017.07.14 - RJS END
	
	SDL_Surface *result = SDL_LoadBMP(filename);
	if (!result)
	{
		string err = "Could not load bitmap : ";
		// RJS - CHANGE - Adding essentially a TODO to this error.
		// err = err + filename;
		err = err + filename + " - Need to report back to LR that there is an error to display or shutdown.";
		printerror(err.c_str());
	}

	// 2017.02.01 - RJS ADD - Logging.
	LOGI("daphne-libretro: In load_one_bmp, bottom of routine.");

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
		if (SCOREBOARD_OVERLAY_DEFAULT)
		{
			if (src.x == (0x0F * OVERLAY_LED_WIDTH)) src.x = 0x00;
		}

		// RJS CHANGE BACK - MAIN THREAD to surfaces
		/*
		if (i == 0)
		{
			LOGI("overlay surface: BEFORE");
			for (int j = 0; j < OVERLAY_LED_HEIGHT; j++)
			{
				uint8_t * temp = ((uint8_t *) overlay->pixels) + dest.x + (overlay->pitch * j);
				LOGI("overlay surface: %02x %02x %02x %02x %02x %02x %02x %02x", *temp, *(temp + 1), *(temp + 2), *(temp + 3), *(temp + 4), *(temp + 5), *(temp + 6), *(temp + 7));
			}
			LOGI("source bmp");
			for (int j = 0; j < OVERLAY_LED_HEIGHT; j++)
			{
				uint8_t * temp = ((uint8_t *)g_other_bmps[B_OVERLAY_LEDS]->pixels) + src.x + (g_other_bmps[B_OVERLAY_LEDS]->pitch * j);
				LOGI("        surface: %02x %02x %02x %02x %02x %02x %02x %02x", *temp, *(temp + 1), *(temp + 2), *(temp + 3), *(temp + 4), *(temp + 5), *(temp + 6), *(temp + 7));
			}

			LOGI("overlay format: format: %d  paladdr: %d  bpp: %d  Bpp: %d Rm: %d  Gm: %d  Bm: %d  Am: %d", overlay->format->format, (int)overlay->format->palette, overlay->format->BitsPerPixel, overlay->format->BytesPerPixel, overlay->format->Rmask, overlay->format->Gmask, overlay->format->Bmask, overlay->format->Amask);
			LOGI("overlay format: Rl: %d  Gl: %d  Bl: %d  Al: %d   Rs: %d  Gs: %d  Bs: %d  As: %d", overlay->format->Rloss, overlay->format->Gloss, overlay->format->Bloss, overlay->format->Aloss, overlay->format->Rshift, overlay->format->Gshift, overlay->format->Bshift, overlay->format->Ashift);
			LOGI(":overlay palette:");
			for (int j = 0; j < overlay->format->palette->ncolors; j++)
			{
				SDL_Color * curr_color = (overlay->format->palette->colors) + j;
				LOGI("r: %d  g: %d  b: %d  a: %d", curr_color->r, curr_color->g, curr_color->b, curr_color->a);
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
			LOGI("overlay surface: AFTER");
			for (int j = 0; j < OVERLAY_LED_HEIGHT; j++)
			{
				uint8_t * temp = ((uint8_t *)overlay->pixels) + dest.x + (overlay->pitch * j);
				LOGI("overlay surface: %02x %02x %02x %02x %02x %02x %02x %02x", *temp, *(temp + 1), *(temp + 2), *(temp + 3), *(temp + 4), *(temp + 5), *(temp + 6), *(temp + 7));
			}
			LOGI("x");
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
	dest.x = (short) x;
	dest.y = (short) y;
	dest.w = (unsigned short)srf->w;
	dest.h = (unsigned short)srf->h;

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

// de-allocates all of the .bmps that we have allocated
void free_bmps()
{

	int nuke_index = 0;

	// get rid of all the LED's
	for (; nuke_index < LED_RANGE; nuke_index++)
	{
		free_one_bmp(g_led_bmps[nuke_index]);
	}
	for (nuke_index = 0; nuke_index < B_EMPTY; nuke_index++)
	{
		// check to make sure it exists before we try to free
		if (g_other_bmps[nuke_index])
		{
			free_one_bmp(g_other_bmps[nuke_index]);
		}
	}
}

void free_one_bmp(SDL_Surface *candidate)
{
	SDL_FreeSurface(candidate);
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

int get_console_initialized()
{
	return g_console_initialized;
}

bool get_fullscreen()
{
	return g_fullscreen;
}

// sets our g_fullscreen bool (determines whether will be in fullscreen mode or not)
void set_fullscreen(bool value)
{
	g_fullscreen = value;
}

void set_rotate_degrees(float fDegrees)
{
	g_fRotateDegrees = fDegrees;
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

// converts YUV to RGB
// Use this only when you don't care about speed =]
// NOTE : it is important for y, u, and v to be signed
void yuv2rgb(SDL_Color *result, int y, int u, int v)
{
	// NOTE : Visual C++ 7.0 apparently has a bug
	// in its floating point optimizations because this function
	// will return incorrect results when compiled as a Release
	// Possible workaround: use integer math instead of float? or don't use VC++ 7? hehe

	int b = (int) (1.164*(y - 16)                   + 2.018*(u - 128));
	int g = (int) (1.164*(y - 16) - 0.813*(v - 128) - 0.391*(u - 128));
	int r = (int) (1.164*(y - 16) + 1.596*(v - 128));

	// clamp values (not sure if this is necessary, but we aren't worried about speed)
	if (b > 255) b = 255;
	if (b < 0) b = 0;
	if (g > 255) g = 255;
	if (g < 0) g = 0;
	if (r > 255) r = 255;
	if (r < 0) r = 0;
	
	result->r = (unsigned char) r;
	result->g = (unsigned char) g;
	result->b = (unsigned char) b;
}

void draw_string(const char* t, int col, int row, SDL_Surface* overlay)
{
	SDL_Rect dest;

	dest.x = (short) ((col*6));
	dest.y = (short) ((row*13));
	dest.w = (unsigned short) (6 * strlen(t)); // width of rectangle area to draw (width of font * length of string)
	dest.h = 13;	// height of area (height of font)
	
	SDL_FillRect(overlay, &dest, 0); // erase anything at our destination before we print new text
	SDLDrawText(t,  overlay, FONT_SMALL, dest.x, dest.y);
	// RJS REMOVE - MAIN THREAD this shouldn't be needed since we're just copying to render in the LDP thread
	// SDL_UpdateRects(overlay, 1, &dest);
}

// toggles fullscreen mode
void vid_toggle_fullscreen()
{
	// Commented out because it creates major problems with YUV overlays and causing segfaults..
	// The real way to toggle fullscreen is to kill overlay, kill surface, make surface, make overlay.
	/*
	g_screen = SDL_SetVideoMode(g_screen->w,
		g_screen->h,
		g_screen->format->BitsPerPixel,
		g_screen->flags ^ SDL_FULLSCREEN);
		*/
}

// NOTE : put into a separate function to make autotesting easier
void set_yuv_hwaccel(bool enabled)
{
	const char *val = "0";
	if (enabled) val = "1";
#ifdef WIN32
	SetEnvironmentVariable("SDL_VIDEO_YUV_HWACCEL", val);
	string sEnv = "SDL_VIDEO_YUV_HWACCEL=";
	sEnv += val;
	putenv(sEnv.c_str());
#else
	setenv("SDL_VIDEO_YUV_HWACCEL", val, 1);
#endif
}

bool get_yuv_hwaccel()
{
	bool result = true;	// it is enabled by default
#ifdef WIN32
	char buf[30];
	ZeroMemory(buf, sizeof(buf));
	DWORD res = GetEnvironmentVariable("SDL_VIDEO_YUV_HWACCEL", buf, sizeof(buf));
	if (buf[0] == '0') result = false;
#else
	char *hw_env = getenv("SDL_VIDEO_YUV_HWACCEL");

	// if HW acceleration has been disabled
	if (hw_env && (hw_env[0] == '0')) result = false;
#endif
	return result;
}

void set_force_aspect_ratio(bool bEnabled)
{
	g_bForceAspectRatio = bEnabled;
}

bool get_force_aspect_ratio()
{
	return g_bForceAspectRatio;
}
