/**************************************************************************************************
***************************************************************************************************
*
* Copyright (C) 2016 Seismal
*
* This file is part of the Libretro Daphne Core.
*
* Please see the LISCENSE file in the root of this project for distribution and
* modification terms.
*
* TODO: Fill in the rest of this block as needed.
* TODO: Remove all excluded files, mostly in game folder.
* TODO: Remove any notion of a console - this is SDL functionality that's not being porting.
*
***************************************************************************************************
**************************************************************************************************/
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#include <ctype.h>

#include "libretro.h"
#include "libretro_daphne.h"
#include "../daphne-1.0-src/io/input.h"
#include "../daphne-1.0-src/daphne.h"
#include "../daphne-1.0-src/game/game.h"
#include "../main_android.h"
#include "../include/SDL_render.h"


/**************************************************************************************************
***************************************************************************************************
*
* Defines, macros, enums, etc.
*
***************************************************************************************************
**************************************************************************************************/


/**************************************************************************************************
***************************************************************************************************
*
* Variables.
*
***************************************************************************************************
**************************************************************************************************/
#define RETRO_MAX_PLAYERS 2
#define RETRO_MAX_BUTTONS (RETRO_DEVICE_ID_JOYPAD_R3 + 1)

static int button_ids[RETRO_MAX_BUTTONS] = {SWITCH_NOTHING};

// File and path of loaded rom.
bool gf_isThayers = false;
char gstr_rom_path[1024];

char gstr_rom_name[DAPHNE_MAX_ROMNAME];
char gstr_rom_extension[sizeof(DAPHNE_ROM_EXTENSION)];

/**************************************************************************************************
* Callbacks.
* retro_log_printf_t			Logging function, takes enum level argurment.
* retro_video_refresh_t			Render a frame, see .h for more info.
* retro_input_poll_t			Polls (preps) input, but gives you nothing.
* retro_input_state_t			Gets the specific input values, A, B, etc.
* retro_environment_t			Doc says, give implimentation a way to perform
*								uncommon tasks.  Called before retro_init. It
*								allows core to ask questions of the user.
* retro_audio_sample_t			Renders single audio frame, following cb renders
* retro_audio_sample_batch_t	multiple audio frames.  Use only one of these.
**************************************************************************************************/
static	retro_log_printf_t			log_cb			   = NULL;
		retro_video_refresh_t		   video_cb          = NULL;
static	retro_input_poll_t			input_poll_cb    	= NULL;
static	retro_input_state_t			input_state_cb		= NULL;
static	retro_environment_t			environ_cb		   = NULL;
static	retro_audio_sample_t		   audio_cb		      = NULL;
retro_audio_sample_batch_t	         audio_batch_cb	   = NULL;


/**************************************************************************************************
***************************************************************************************************
*
* Set Callbacks.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Called before retro_init.
**************************************************************************************************/
void retro_set_environment(retro_environment_t in_environment)
{
	if (! in_environment) return;

	environ_cb = in_environment;
	
	static const struct retro_variable t_environmentvariables[] =
	{
		{ "daphne_invertctrl",		"Invert controls (if supported); enable|disable" },
		{ "daphne_useoverlaysb",	"Overlay scoreboard (if supported); enable|disable" },
		{ "daphne_emulate_seek",	"Emulate LaserDisc seeks; enable|disable" },
		{ "daphne_cheat",			"Supported Cheats; disable|enable" },
		{ NULL, NULL }
	};

	environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)t_environmentvariables);
}

/**************************************************************************************************
*************************************************************************************************/
int retro_get_variable(char * strVariable)
{
	struct retro_variable var = { 0 };
	var.key = strVariable;
	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
	{
		if (strncmp(var.value, "disable", 8) == 0)	return 0;
		if (strncmp(var.value, "enable", 7) == 0)	return 1;
	}

	return -1;
}

/**************************************************************************************************
* Render a frame callback.  Will be called, at least once, before retro_run.
*************************************************************************************************/
void retro_set_video_refresh(retro_video_refresh_t in_videorefresh)
{
	if (! in_videorefresh) return;
	video_cb = in_videorefresh;
}

/**************************************************************************************************
* Render an audio frame, only use one.  Will be called, at least once, before
* retro_run.
*************************************************************************************************/
void retro_set_audio_sample(retro_audio_sample_t in_audiosample)
{
	if (! in_audiosample) return;
	audio_cb = in_audiosample;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t in_audiosamplebatch)
{
	if (! in_audiosamplebatch) return;
	audio_batch_cb = in_audiosamplebatch;
}

/**************************************************************************************************
* Poll input. Will be called, at least once, before retro_run.
**************************************************************************************************/
void retro_set_input_poll(retro_input_poll_t in_inputpoll)
{
	if (! in_inputpoll) return;
	input_poll_cb = in_inputpoll;
}

/**************************************************************************************************
* Get specific input, usually, after a poll. Will be called, at least once,
* before retro_run.
**************************************************************************************************/
void retro_set_input_state(retro_input_state_t in_inputstate)
{
	if (! in_inputstate) return;
	input_state_cb = in_inputstate;
}


/**************************************************************************************************
***************************************************************************************************
*
* Initialization.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Basic init.  Called only once.
**************************************************************************************************/
void retro_init(void)
{
	// Set the internal pixel format.
	if (environ_cb)
	{
		// RGB565 picked here as that is what is recommended.  However, Daphne
		// itself uses YUY2 (16 bpp).
		enum retro_pixel_format n_pixelformat = RETRO_PIXEL_FORMAT_RGB565;
		environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &n_pixelformat);
	}

	// Set the available buttons for the user.  Not doing analogs for right now.
	struct retro_input_descriptor t_inputdescriptors[] =
	{
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_A,		"A"			},	// 0
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_B,		"B"			},	// 1
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_X,		"X"			},	// 2
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_Y,		"Y"			},	// 3
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_SELECT,	"Select"	},	// 4
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_START,	"Start"		},	// 5
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_RIGHT,	"Right"		},	// 6
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_LEFT,		"Left"		},	// 7
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_UP,		"Up"		},	// 8
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_DOWN,		"Down"		},	// 9
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_R,		"R"			},	// 10
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_L,		"L"			},	// 11
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_R3,		"RSB"		},	// 12
		{ 0, RETRO_DEVICE_JOYPAD,	0, RETRO_DEVICE_ID_JOYPAD_L3,		"LSB"		},	// 13
		{ 0, RETRO_DEVICE_NONE,		0, RETRO_DEVICE_ID_JOYPAD_L3,		NULL		}	// 14
	};
	environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &t_inputdescriptors);

	// Setup cross platform logging.
	struct retro_log_callback t_logcb;
	if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &t_logcb))
	{
		log_cb = t_logcb.log;
		log_cb(RETRO_LOG_INFO, "daphne-libretro: Logging initialized.\n");
	}

	if (log_cb)
      log_cb(RETRO_LOG_INFO, "daphne-libretro: In retro_init.\n");

    // Set the performance level, not sure what "4" means
    unsigned int n_perflevel = 4;
    environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &n_perflevel);

	// Clear the rom paths.
	gf_isThayers			= false;
	gstr_rom_extension[0]	= '\0';
	gstr_rom_name[0]		= '\0';
	gstr_rom_path[0]		= '\0';

	// Call Daphne startup code.
	// daphne_retro_init();
}

/**************************************************************************************************
* Core tear down.  Essentially de-retro_init.  Called only once.
**************************************************************************************************/
void retro_deinit(void)
{
	if (log_cb)
      log_cb(RETRO_LOG_INFO, "daphne-libretro: In retro_deinit.\n");

	main_daphne_shutdown();
}


/**************************************************************************************************
***************************************************************************************************
*
* Information for frontend.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Function always returns this.  Allows engine to know what version the API is
* so it can make various determinations.
**************************************************************************************************/
unsigned int retro_api_version(void)
{
	return RETRO_API_VERSION;
}

/**************************************************************************************************
* Information for the frontend.  Call can be made anytime.
**************************************************************************************************/
void retro_get_system_info(struct retro_system_info *out_systeminfo)
{
	memset(out_systeminfo, 0, sizeof(*out_systeminfo));
    out_systeminfo->library_name		= "Daphne";
    out_systeminfo->library_version		= "v0.04";
    out_systeminfo->need_fullpath		= true;
	out_systeminfo->block_extract		= true;
    out_systeminfo->valid_extensions	= DAPHNE_ROM_EXTENSION;
}

/**************************************************************************************************
* Gets information about system audio/video timings and geometry.  Can be
* called only after retro_load_game() has successfully completed.
**************************************************************************************************/
void retro_get_system_av_info(struct retro_system_av_info *out_avinfo)
{
    memset(out_avinfo, 0, sizeof(*out_avinfo));
    
	out_avinfo->geometry.base_width   = DAPHNE_VIDEO_W;
    out_avinfo->geometry.base_height  = DAPHNE_VIDEO_H;
    out_avinfo->geometry.max_width    = DAPHNE_VIDEO_W;
    out_avinfo->geometry.max_height   = DAPHNE_VIDEO_H;
    out_avinfo->geometry.aspect_ratio = DAPHNE_VIDEO_Wf / DAPHNE_VIDEO_Hf;
	
	out_avinfo->timing.fps = DAPHNE_TIMING_FPS;
	// RJS HERE - sample rate input to RA
	out_avinfo->timing.sample_rate = DAPHNE_AUDIO_SAMPLE_RATE;
}


/**************************************************************************************************
***************************************************************************************************
*
* Runtime.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Usually comes from the frontend telling you a particular controller port
* has changed to some other device, RETRO_DEVICE_*.  RETRO_DEVICE_JOYPAD is
* assumed to be connected to all ports.
**************************************************************************************************/
void retro_set_controller_port_device(unsigned in_port, unsigned in_device)
{
	UNUSED(in_port);
	UNUSED(in_device);
}

/**************************************************************************************************
* Resets the current game.  Think reset button pushed on console.
**************************************************************************************************/
void retro_reset(void)
{
   if (g_game)
      g_game->reset();

	// Clear the rom paths.
	gstr_rom_extension[0]	= '\0';
	gstr_rom_name[0]		= '\0';
	gstr_rom_path[0]		= '\0';
}

/**************************************************************************************************
* Checks if the given port and button's input state has changed.  Save new state if needed.
**************************************************************************************************/

bool retro_has_inputstate_changed(int in_port, int in_button, uint16_t in_key)
{
	typedef struct retro_input_button_record
	{
		uint16_t	n_state;
	} retro_input_button_record;

	static retro_input_button_record retro_input_button_table[RETRO_MAX_PLAYERS][RETRO_MAX_BUTTONS] = { { { 0 } } };

	if (retro_input_button_table[in_port][in_button].n_state != in_key)
	{
		retro_input_button_table[in_port][in_button].n_state = in_key;
		return true;
	}

	return false;
}

/**************************************************************************************************
* Runs the game for one video frame.
**************************************************************************************************/
extern SDL_SW_YUVTexture * get_vb_waiting(int * vb_ndx);
extern bool input_pause(bool fPaused);
extern void set_vb_rendering_done(int vb_ndx);
extern int retro_run_frames_delta;

int retro_run_frames	= 0;

bool retro_run_once = false;

void retro_run(void)
{
	if (retro_run_frames_delta >= RETRO_RUN_FRAMES_PAUSED_THRESHOLD)
	{
		retro_run_frames_delta = 0;
		input_pause(false);
	}
	retro_run_frames++;

	// Some of the back end needs to know when certain systems have been init'd.  When
	// retro_run has run once, all relavent system have been initialized.
	retro_run_once = true;

	// Poll input.
	input_poll_cb();


	// Notice numplayers starts at 1.  The playercontrollerid_list points to the player ordinal
	// to the actual device id (port).  Hardcoded here for later proper updating.
	int n_playercontrollerid_list[RETRO_MAX_PLAYERS] = { 0, 1 };

	for (int n_numplayers = 0; n_numplayers < RETRO_MAX_PLAYERS; n_numplayers++)
	{
		int n_port = n_playercontrollerid_list[n_numplayers];

		for (int n_button_ndx = 0; n_button_ndx < RETRO_MAX_BUTTONS; n_button_ndx++)
		{
			uint16_t n_key;
			n_key = input_state_cb(n_port, RETRO_DEVICE_JOYPAD, 0, n_button_ndx);

			if (retro_has_inputstate_changed(n_port, n_button_ndx, n_key))
			{
            if (button_ids[n_button_ndx] != SWITCH_NOTHING)
            {
               if (n_key)
                  input_enable(button_ids[n_button_ndx]);
               else
                  input_disable(button_ids[n_button_ndx]);
            }
			}
		}
		// float analogX = (float)input_state_cb(n_port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) / 32768.0f;
	}

#if 0
	int ab_ndx = -1;
	int ab_frames = 0;
	int16_t * ab_buffer	= NULL;
   ab_buffer = get_ab_waiting(&ab_ndx, &ab_frames);
   if (audio_batch_cb)
      audio_batch_cb(ab_buffer, ab_frames);
   set_ab_streaming_done(ab_ndx);
#endif

	// Total hack for clearing some buffers.
	/*
	int testhacklimit;
	testhacklimit = 40;
	ab_buffer = get_ab_waiting(&ab_ndx, &ab_frames);
	while (ab_buffer && testhacklimit)
	{
		set_ab_streaming_done(ab_ndx);
		testhacklimit--;
		if (testhacklimit) ab_buffer = get_ab_waiting(&ab_ndx, &ab_frames);
	}
	*/


	// Does:		g_game->start()
	// Which is:	cpu_execute()
	// Which does this:
	//				cpu_execute_one_cycle_init() // Only done once.
	//				cpu_execute_one_cycle()
	// main_daphne_mainloop();
	
	// struct VIDEO_BUFFER tVB[4];
	int vb_ndx						= -1;
	SDL_SW_YUVTexture * sw_overlay	= NULL;

	sw_overlay = get_vb_waiting(&vb_ndx);
	if (sw_overlay && video_cb) 
	{

      	video_cb(sw_overlay->pixels, sw_overlay->w, sw_overlay->h, sw_overlay->w * DAPHNE_VIDEO_ByPP);
	set_vb_rendering_done(vb_ndx);
	}

	
}


/**************************************************************************************************
***************************************************************************************************
*
* Serialization - for emulator state save.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Frontend calls this to determine size of serialization.
**************************************************************************************************/
size_t retro_serialize_size(void)
{
	// TODO: Attempt to state save.
	// Returning 0 means this is not implimented.
	return 0;
}

/**************************************************************************************************
* Serialize state using the given buffer.
**************************************************************************************************/
bool retro_serialize(void *out_data, size_t in_size)
{
	// TODO: Attempt to save state.
	UNUSED(out_data);
	UNUSED(in_size);
	return false;
}

/**************************************************************************************************
* Unserialize state using the given buffer.
**************************************************************************************************/
bool retro_unserialize(const void *in_data, size_t in_size)
{
	// TODO: Attempt to load state.
	UNUSED(in_data);
	UNUSED(in_size);
	return false;
}


/**************************************************************************************************
***************************************************************************************************
*
* Cheats.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Reset (or init) any cheat code.
**************************************************************************************************/
void retro_cheat_reset(void)
{
	// TODO: Add needed cheats.
}

/**************************************************************************************************
* Cheat has been activated from frontend.
**************************************************************************************************/
void retro_cheat_set(unsigned in_index, bool in_enabled, const char *in_code)
{
	// TODO: Add needed cheats.
	UNUSED(in_index);
	UNUSED(in_enabled);
	UNUSED(in_code);
}


/**************************************************************************************************
***************************************************************************************************
*
* Load/Unload games.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Strips out the path from LR and loads the globals.  True if OK, false if bad.
**************************************************************************************************/
bool retro_load_game_get_path(const struct retro_game_info *in_game)
{
	// Make sure we only have a path and no data was loaded.
	if (in_game->data != NULL)
	{
		if (log_cb)
         log_cb(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, data buffer was loaded.\n");
		return false;
	}

	// Double check that we have a path.  Inputs from non-pathed sources will not be valid.
	if (in_game->path == NULL)
	{
		if (log_cb)
         log_cb(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, path was NULL, should never be.\n");
		return false;
	}

	if (log_cb)
      log_cb(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, full path from LR. Path: %s\n", in_game->path);

	// Strip out the file name.
	// Make sure the path is long enough.
	int n_pathsize = strlen(in_game->path);
	if (n_pathsize <= (sizeof(DAPHNE_ROM_EXTENSION) + sizeof(".")))
	{
		if (log_cb)
         log_cb(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, path filename doesn't seem to have a valid format. Pathsize: %d  Extsize: %d", n_pathsize, (sizeof(DAPHNE_ROM_EXTENSION) + sizeof(".")));
		return false;
	}

	if (log_cb)
      log_cb(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, full path size from LR. Path: %d\n", n_pathsize);

	// Look for the last slash in the path.
	const char * pstr_filename = strrchr(in_game->path, '/');
		if (pstr_filename == NULL) pstr_filename = strrchr(in_game->path, '\\'); //windows
	if (pstr_filename == NULL)
	{
		// We could have the case of just a filename.
		pstr_filename = in_game->path;
	} else {
		// Get past the slash.
		pstr_filename++;
	}

	if (log_cb)
      log_cb(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, filename and extension. Filename: %s\n", pstr_filename);

	// Split the filename from extension.
	const char * pstr_fileextension = strrchr((char *) pstr_filename, '.');
	pstr_fileextension++;
	if (strcmp(DAPHNE_ROM_EXTENSION, pstr_fileextension) != 0)
	{
		if (log_cb)
         log_cb(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, filename doesn't seem to have a valid format. Ext: %s  Fileext: %s\n", DAPHNE_ROM_EXTENSION, pstr_fileextension);
		return false;
	}

	if (log_cb)
      log_cb(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, extension. Extension: %s\n", pstr_fileextension);

	// Load globals with path, filename, and extension.  Assumption is everything is OK.
	strcpy(gstr_rom_extension, pstr_fileextension);

	if ((pstr_fileextension - pstr_filename - 1) > sizeof(gstr_rom_name))
	{
		if (log_cb)
         log_cb(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, filename doesn't seem to have a valid format.\n");
		return false;
	}
	memcpy(gstr_rom_name, pstr_filename, pstr_fileextension - pstr_filename - 1);

	if ((pstr_filename - in_game->path - 1) > sizeof(gstr_rom_path))
	{
		if (log_cb)
         log_cb(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, path doesn't seem to have a valid format.\n");
		return false;
	}
	memcpy(gstr_rom_path, in_game->path, pstr_filename - in_game->path - 1); 

	// Failsafe.  We're so careful.
	gstr_rom_extension[sizeof(gstr_rom_extension) - 1]	= '\0';
	gstr_rom_name[sizeof(gstr_rom_name) - 1]			= '\0';
	gstr_rom_path[sizeof(gstr_rom_path) - 1]			= '\0';

	for (int i = 0; gstr_rom_name[i]; i++) {
		gstr_rom_name[i] = tolower(gstr_rom_name[i]);
	}

	if (log_cb)
      log_cb(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, final file. Path: %s  Name: %s  Ext: %s\n", gstr_rom_path, gstr_rom_name, gstr_rom_extension);

	// All is good.
	return true;
}

/**************************************************************************************************
* Get game type enum.  Only added ones that I've tested for now.
**************************************************************************************************/
// Build out the command line v01.
/*
DAPHNE_GAME_TYPE retro_load_game_get_game_type(char * pstr_shortgamename)
{
	if (strcmp(pstr_shortgamename, "lair") == 0)			return(DAPHNE_GAME_TYPE_LAIR);
	if (strcmp(pstr_shortgamename, "lair2") == 0)			return(DAPHNE_GAME_TYPE_LAIR2);
	if (strcmp(pstr_shortgamename, "ace") == 0)				return(DAPHNE_GAME_TYPE_ACE);
	if (strcmp(pstr_shortgamename, "ace") == 0)				return(DAPHNE_GAME_TYPE_ACE91);
	if (strcmp(pstr_shortgamename, "cliff") == 0)			return(DAPHNE_GAME_TYPE_CLIFF);
	if (strcmp(pstr_shortgamename, "gtg") == 0)				return(DAPHNE_GAME_TYPE_GTG);
	if (strcmp(pstr_shortgamename, "sdq") == 0)				return(DAPHNE_GAME_TYPE_SUPERD);
	if (strcmp(pstr_shortgamename, "tq") == 0)				return(DAPHNE_GAME_TYPE_THAYERS);
	if (strcmp(pstr_shortgamename, "astron") == 0)			return(DAPHNE_GAME_TYPE_ASTRON);
	if (strcmp(pstr_shortgamename, "galaxy") == 0)			return(DAPHNE_GAME_TYPE_GALAXY);
	if (strcmp(pstr_shortgamename, "esh") == 0)				return(DAPHNE_GAME_TYPE_ESH);
	if (strcmp(pstr_shortgamename, "laireuro") == 0)		return(DAPHNE_GAME_TYPE_LAIREURO);
	if (strcmp(pstr_shortgamename, "badlands") == 0)		return(DAPHNE_GAME_TYPE_BADLANDS);
	if (strcmp(pstr_shortgamename, "starrider") == 0)		return(DAPHNE_GAME_TYPE_STARRIDER);
	if (strcmp(pstr_shortgamename, "bega") == 0)			return(DAPHNE_GAME_TYPE_BEGA);
	if (strcmp(pstr_shortgamename, "interstellar") == 0)	return(DAPHNE_GAME_TYPE_INTERSTELLAR);
	if (strcmp(pstr_shortgamename, "sae") == 0)				return(DAPHNE_GAME_TYPE_SAE);
	if (strcmp(pstr_shortgamename, "mach3") == 0)			return(DAPHNE_GAME_TYPE_MACH3);
	if (strcmp(pstr_shortgamename, "uvt") == 0)				return(DAPHNE_GAME_TYPE_UVT);
	if (strcmp(pstr_shortgamename, "badlands") == 0)		return(DAPHNE_GAME_TYPE_BADLANDP);
	if (strcmp(pstr_shortgamename, "dle11") == 0)			return(DAPHNE_GAME_TYPE_DLE1);
	if (strcmp(pstr_shortgamename, "dle21") == 0)			return(DAPHNE_GAME_TYPE_DLE2);
	if (strcmp(pstr_shortgamename, "gpworld") == 0)			return(DAPHNE_GAME_TYPE_GPWORLD);
	if (strcmp(pstr_shortgamename, "cobraab") == 0)			return(DAPHNE_GAME_SUBTYPE_ASTRON_COBRA);
	if (strcmp(pstr_shortgamename, "roadblaster") == 0)		return(DAPHNE_GAME_SUBTYPE_BEGA_ROADBLASTER);

	return DAPHNE_GAME_TYPE_UNDEFINED;
}
*/

bool retro_load_game_fill_framefile(char * pstr_framefile, int str_framefile_sz)
{
	// A resonable default.
	char str_default_framefile[DAPHNE_MAX_ROMNAME + 4];
	strcpy(str_default_framefile, gstr_rom_name);
	strcat(str_default_framefile, ".txt");
	strcpy(pstr_framefile, str_default_framefile);

	if ((strcmp(gstr_rom_name, "ace")		== 0) ||
		(strcmp(gstr_rom_name, "ace_a2")	== 0) ||
		(strcmp(gstr_rom_name, "ace_a")		== 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "ace91")			== 0) ||
		(strcmp(gstr_rom_name, "ace91_euro")	== 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "aceeuro") == 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "astron")	== 0) ||
		(strcmp(gstr_rom_name, "astronp")	== 0))
	{
		strcpy(pstr_framefile, "astron.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "badlandp") == 0) ||
		(strcmp(gstr_rom_name, "badlands") == 0))
	{
		strcpy(pstr_framefile, "badlands.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "bega")		== 0) ||
		(strcmp(gstr_rom_name, "begar1")	== 0))
	{
		strcpy(pstr_framefile, "bega.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "benchmark") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcmp(gstr_rom_name, "blazer") == 0))
	{
		strcpy(pstr_framefile, "blazer.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "cliff")		== 0) ||
		(strcmp(gstr_rom_name, "cliffalt")	== 0) ||
		(strcmp(gstr_rom_name, "cliffalt2")	== 0))
	{
		strcpy(pstr_framefile, "cliff.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "cobra")		== 0) ||
		(strcmp(gstr_rom_name, "cobraab")	== 0) ||
		(strcmp(gstr_rom_name, "cobram3")	== 0))
	{
		strcpy(pstr_framefile, "cobra.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "cobraconv") == 0))
	{
		strcpy(pstr_framefile, "cobraconv.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "cputest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcmp(gstr_rom_name, "dle11")	== 0) ||
		(strcmp(gstr_rom_name, "dle2")	== 0) ||
		(strcmp(gstr_rom_name, "dle20")	== 0) ||
		(strcmp(gstr_rom_name, "dle21")	== 0))
	{
		strcpy(pstr_framefile, "lair.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "esh")		== 0) ||
		(strcmp(gstr_rom_name, "eshalt")	== 0) ||
		(strcmp(gstr_rom_name, "eshalt2")	== 0))
	{
		strcpy(pstr_framefile, "esh.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "firefox")	== 0) ||
		(strcmp(gstr_rom_name, "firefoxa")	== 0))
	{
		strcpy(pstr_framefile, "firefox.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "ffr") == 0))
	{
		strcpy(pstr_framefile, "ffr.txt");
		return false;
	}

	if ((strcmp(gstr_rom_name, "galaxy")	== 0) ||
		(strcmp(gstr_rom_name, "galaxyp")	== 0))
	{
		strcpy(pstr_framefile, "galaxy.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "gpworld") == 0))
	{
		strcpy(pstr_framefile, "gpworld.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "gtg") == 0))
	{
		strcpy(pstr_framefile, "gtg.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "interstellar") == 0))
	{
		strcpy(pstr_framefile, "interstellar.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "lair")		== 0) ||
		(strcmp(gstr_rom_name, "lair_f")	== 0) ||
		(strcmp(gstr_rom_name, "lair_e")	== 0) ||
		(strcmp(gstr_rom_name, "lair_d")	== 0) ||
		(strcmp(gstr_rom_name, "lair_c")	== 0) ||
		(strcmp(gstr_rom_name, "lair_b")	== 0) ||
		(strcmp(gstr_rom_name, "lair_a")	== 0) ||
		(strcmp(gstr_rom_name, "lair_n1")	== 0) ||
		(strcmp(gstr_rom_name, "lair_x")	== 0) ||
		(strcmp(gstr_rom_name, "lair_alt")	== 0) ||
		(strcmp(gstr_rom_name, "laireuro")	== 0) ||
		(strcmp(gstr_rom_name, "lair_ita")	== 0) ||
		(strcmp(gstr_rom_name, "lair_d2")	== 0))
	{
		strcpy(pstr_framefile, "lair.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "lair2")				== 0) ||
		(strcmp(gstr_rom_name, "lair2_319_euro")	== 0) ||
		(strcmp(gstr_rom_name, "lair2_319_span")	== 0) ||
		(strcmp(gstr_rom_name, "lair2_318")			== 0) ||
		(strcmp(gstr_rom_name, "lair2_316_euro")	== 0) ||
		(strcmp(gstr_rom_name, "lair2_315")			== 0) ||
		(strcmp(gstr_rom_name, "lair2_314")			== 0) ||
		(strcmp(gstr_rom_name, "lair2_300")			== 0) ||
		(strcmp(gstr_rom_name, "lair2_211")			== 0))
	{
		strcpy(pstr_framefile, "lair2.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "lgp") == 0))
	{
		strcpy(pstr_framefile, "lgp.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "mach3") == 0))
	{
		strcpy(pstr_framefile, "mach3.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "mcputest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcmp(gstr_rom_name, "releasetest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcmp(gstr_rom_name, "roadblaster") == 0))
	{
		strcpy(pstr_framefile, "roadblaster.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "sae") == 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "seektest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcmp(gstr_rom_name, "singe") == 0))
	{
		strcpy(pstr_framefile, "singe.txt");  // ??
		return true;
	}

	if ((strcmp(gstr_rom_name, "speedtest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcmp(gstr_rom_name, "sdq")			== 0) ||
		(strcmp(gstr_rom_name, "sdqshort")		== 0) ||
		(strcmp(gstr_rom_name, "sdqshortalt")	== 0))
	{
		strcpy(pstr_framefile, "sdq.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "starrider") == 0))
	{
		strcpy(pstr_framefile, "starrider.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "superdon") == 0))
	{
		strcpy(pstr_framefile, "superdon.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "timetrav") == 0))
	{
		strcpy(pstr_framefile, "timetrav.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "test_sb") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcmp(gstr_rom_name, "tq")		== 0) ||
		(strcmp(gstr_rom_name, "tq_alt")	== 0) ||
		(strcmp(gstr_rom_name, "tq_swear")	== 0))
	{
		gf_isThayers = true;
		strcpy(pstr_framefile, "tq.txt");
		return true;
	}

	if ((strcmp(gstr_rom_name, "uvt") == 0))
	{
		strcpy(pstr_framefile, "uvt.txt");
		return true;
	}

	return true;
}

/**************************************************************************************************
* Loads a game.  For Daphne, and the complex loading, only the path is sent
* here which means nothing is loaded by the frontend.
**************************************************************************************************/
bool retro_load_game(const struct retro_game_info *in_game)
{
	if (log_cb)
      log_cb(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game, path is: %s\n", in_game->path);

	// Strip out the path.
	if (! retro_load_game_get_path(in_game)) return false;

	// Build out the command line v02.
	#define DAPHNE_MAX_COMMANDLINE_ARGS	30
	#define DAPHNE_MAX_ARG_LEN	20

	int num_args = 0;
	static char str_args[DAPHNE_MAX_COMMANDLINE_ARGS][DAPHNE_MAX_ARG_LEN];
	static char * pstr_args[DAPHNE_MAX_COMMANDLINE_ARGS];

	strcpy(str_args[num_args], "daphne");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], gstr_rom_name);
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "vldp");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "-framefile");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	retro_load_game_fill_framefile(str_args[num_args], DAPHNE_MAX_ARG_LEN);
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "-fullscreen");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "-seek_frames_per_ms");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "20");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "-min_seek_delay");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "1000");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "-fastboot");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	strcpy(str_args[num_args], "-cheat");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	if (((strncmp(gstr_rom_name, "lair2", 5)	!= 0) &&
		 (strncmp(gstr_rom_name, "aceeuro", 7)	!= 0))
		&&
		((strncmp(gstr_rom_name, "lair", 4)	== 0) ||
		(strncmp(gstr_rom_name, "dle", 3)	== 0) ||
		(strncmp(gstr_rom_name, "ace", 3)	== 0) ||
		(strncmp(gstr_rom_name, "sae", 3)	== 0) ||
		(strncmp(gstr_rom_name, "tq", 2)	== 0)))
	{
		strcpy(str_args[num_args], "-useoverlaysb");
		pstr_args[num_args] = str_args[num_args];
		num_args++;

		strcpy(str_args[num_args], "0");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
	}

	if ((strncmp(gstr_rom_name, "cobra", 5)		== 0) ||
		(strncmp(gstr_rom_name, "cobraab", 7)	== 0) ||
		(strncmp(gstr_rom_name, "cobraconv", 9)	== 0) ||
		(strncmp(gstr_rom_name, "cobram3", 7)	== 0) ||
		(strncmp(gstr_rom_name, "astron", 6)	== 0) ||
		(strncmp(gstr_rom_name, "astronp", 7)	== 0) ||
		(strncmp(gstr_rom_name, "galaxy", 6)	== 0) ||
		(strncmp(gstr_rom_name, "galaxyp", 7)	== 0) ||
		(strncmp(gstr_rom_name, "mach3", 7)		== 0) ||
		(strncmp(gstr_rom_name, "uvt", 3)		== 0))
	{
		strcpy(str_args[num_args], "-invertctrl");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
	}

	if ((strncmp(gstr_rom_name, "badlands", 8) == 0) ||
		(strncmp(gstr_rom_name, "badlandp", 8) == 0))
	{
		strcpy(str_args[num_args], "-bank");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "0");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "00000000");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "-bank");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "1");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "10000001");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
	}
			/*
	if (strncmp(gstr_rom_name, "aceeuro", 7) == 0)
	{
		strcpy(str_args[num_args], "-x");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "640");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "-y");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
		strcpy(str_args[num_args], "480");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
	}
	*/

	strcpy(str_args[num_args], "-homedir");
	pstr_args[num_args] = str_args[num_args];
	num_args++;

	pstr_args[num_args]  = strcat(gstr_rom_path, "/..");
	num_args++;

	// 2017.11.28 - RJS - Can be removed.  Printing the commandline.
	char strCommandline[1024] = "";

	for (int i = 0; i < num_args; i++)
	{
		strcat(strCommandline, pstr_args[i]);
		strcat(strCommandline, " ");
	}

	if (main_daphne(num_args, pstr_args) != 0)
      return false;

   if (g_game)
   {
      for (int n_button_ndx = 0; n_button_ndx < RETRO_MAX_BUTTONS; n_button_ndx++)
         button_ids[n_button_ndx]    = g_game->get_libretro_button_map(n_button_ndx);
   }
	
	return true;

	/*
	// Build out the command line v01.
	// Loading game type.
	DAPHNE_GAME_TYPE e_gametype = retro_load_game_get_game_type(gstr_rom_name);
	if (e_gametype == DAPHNE_GAME_TYPE_UNDEFINED)
	{
		retro_log(3, "daphne-libretro: In daphne_retro_load_game, game %s unavailable.", gstr_rom_name);
		return false;
	}

	// **************************************************************************************************
	// * **** Game Notes
	// * - Sega GP World (gpworld) does work, has hiccups.
	// * - Thayers Quest (tq) does work but is a bit unstable, can't get a good repro.  Putting that off
	// *   for right now.  You must have a keyboard attached.
	// **************************************************************************************************
	
	// Arguements to mimic command line.
	#define DAPHNE_NUM_COMMANDLINE_ARGS	13
	static char str_daphne_arguements_table[DAPHNE_GAME_TYPE_MAX][DAPHNE_NUM_COMMANDLINE_ARGS][20] =
	{
		{ "undefined",	"",				"",		"",				"",					"",				"",						"",						"",						"",					"",					"",				""			},	// 00 DAPHNE_GAME_TYPE_UNDEFINED
		{ "daphne",		"lair",			"vldp",	"-framefile",	"lair.txt",			"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot"	},	// 01 DAPHNE_GAME_TYPE_LAIR
		{ "daphne",		"lair2",		"vldp",	"-framefile",	"lair2.txt",		"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot"	},	// 02 DAPHNE_GAME_TYPE_LAIR2
		{ "daphne",		"ace",			"vldp",	"-framefile",	"ace.txt",			"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot" },	// 03 DAPHNE_GAME_TYPE_ACE
		{ "daphne",		"ace",			"vldp",	"-framefile",	"ace.txt",			"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot"	},	// 04 DAPHNE_GAME_TYPE_ACE91
		{ "daphne",		"cliff",		"vldp",	"-framefile",	"cliff.txt",		"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 05 DAPHNE_GAME_TYPE_CLIFF
		{ "daphne",		"gtg",			"vldp",	"-framefile",	"gtg.txt",			"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 06 DAPHNE_GAME_TYPE_GTG
		{ "daphne",		"sdq",			"vldp",	"-framefile",	"sdq.txt",			"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 07 DAPHNE_GAME_TYPE_SUPERD
		{ "daphne",		"tq",			"vldp",	"-framefile",	"tq.txt",			"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 08 DAPHNE_GAME_TYPE_THAYERS
		{ "daphne",		"astron",		"vldp",	"-framefile",	"astron.txt",		"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 09 DAPHNE_GAME_TYPE_ASTRON
		{ "daphne",		"galaxy",		"vldp",	"-framefile",	"galaxy.txt",		"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 10 DAPHNE_GAME_TYPE_GALAXY
		{ "daphne",		"esh",			"vldp",	"-frameile",	"esh.txt",			"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 11 DAPHNE_GAME_TYPE_ESH
		{ "daphne",		"laireuro",		"vldp",	"-framefile",	"lair.txt",			"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot"	},	// 12 DAPHNE_GAME_TYPE_LAIREURO
		{ "daphne",		"badlands",		"vldp",	"-framefile",	"badlands.txt",		"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 13 DAPHNE_GAME_TYPE_BADLANDS
		{ "daphne",		"starrider",	"vldp",	"-framefile",	"starrider.txt",	"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 14 DAPHNE_GAME_TYPE_STARRIDER
		{ "daphne",		"bega",			"vldp",	"-framefile",	"bega.txt",			"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 15 DAPHNE_GAME_TYPE_BEGA
		{ "daphne",		"interstellar",	"vldp",	"-framefile",	"interstellar.txt",	"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 16 DAPHNE_GAME_TYPE_INTERSTELLAR
		{ "daphne",		"sae",			"vldp",	"-framefile",	"ace.txt",			"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot" },	// 17 DAPHNE_GAME_TYPE_SAE
		{ "daphne",		"mach3",		"vldp",	"-framefile",	"mach3.txt",		"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 18 DAPHNE_GAME_TYPE_MACH3
		{ "daphne",		"uvt",			"vldp",	"-framefile",	"uvt.txt",			"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 19 DAPHNE_GAME_TYPE_UVT
		{ "daphne",		"badlands",		"vldp",	"-framefile",	"badlands.txt",		"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 20 DAPHNE_GAME_TYPE_BADLANDP
		{ "daphne",		"dle11",		"vldp",	"-framefile",	"lair.txt",			"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot" },	// 21 DAPHNE_GAME_TYPE_DLE1
		{ "daphne",		"dle21",		"vldp",	"-framefile",	"lair.txt",			"-fullscreen",	"-useoverlaysb",		"0",					"-seek_frames_per_ms",	"20",				"-min_seek_delay",	"1000",			"-fastboot" },	// 22 DAPHNE_GAME_TYPE_DLE2
		{ "daphne",		"gpworld",		"vldp",	"-framefile",	"gpworld.txt",		"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			},	// 23 DAPHNE_GAME_TYPE_GPWORLD
		{ "daphne",		"cobraab",		"vldp",	"-framefile",	"cobra.txt",		"-fullscreen",	"-invertctrl",			"-seek_frames_per_ms",	"20",					"-min_seek_delay",	"1000",				"-fastboot",	""			},	// 24 DAPHNE_GAME_SUBTYPE_ASTRON_COBRA
		{ "daphne",		"roadblaster",	"vldp",	"-framefile",	"roadblaster.txt",	"-fullscreen",	"-seek_frames_per_ms",	"20",					"-min_seek_delay",		"1000",				"-fastboot",		"",				""			}	// 25 DAPHNE_GAME_SUBTYPE_BEGA_ROADBLASTER
	};
	
	// Get and load the proper arguements.
	#define DAPHNE_NUM_GLOBAL_COMMANDLINE_ARGS	2
	int i;
	char * pstr_daphne_arguements[DAPHNE_NUM_COMMANDLINE_ARGS + DAPHNE_NUM_GLOBAL_COMMANDLINE_ARGS];
	for (i = 0; i < DAPHNE_NUM_COMMANDLINE_ARGS; i++)
	{
		char * pstr_arguement = &str_daphne_arguements_table[e_gametype][i][0];
		if (*pstr_arguement == '\0') break;
		pstr_daphne_arguements[i] = pstr_arguement;
	}
	static char strHomedir[9] = "-homedir";
	pstr_daphne_arguements[i]		= strHomedir;
	pstr_daphne_arguements[i + 1]	= strcat(gstr_rom_path, "/..");

	if (main_daphne(i + DAPHNE_NUM_GLOBAL_COMMANDLINE_ARGS, pstr_daphne_arguements) == 0) return true;
	else return false;
	*/
}

/**************************************************************************************************
* Loads special case games.  For instance, some systems require certain games
* to load BIOS and ROM, and other games just a ROM.  Not needed for Daphne.
**************************************************************************************************/
bool retro_load_game_special(unsigned in_game_type,	const struct retro_game_info *in_info, size_t in_num_info)
{
	UNUSED(in_game_type);
	UNUSED(in_info);
	UNUSED(in_num_info);
	return false;
}

/**************************************************************************************************
* Unloads a game and allows a fresh call to retro_load_game.
**************************************************************************************************/
void retro_unload_game(void)
{
	// 2017.09.20 - RJS - Daphne was never written to be reentrant, so we need to quit.
	// retro_deinit();
	// 2018.03.05 - RJS - While retro_unload_game and retro_deinit are usually called one after
	// the other this isn't a good idea long term.  Some systems may not be shut down yet when
	// unload is called that deinit relies on being shutdown.
}

/**************************************************************************************************
* Why is this routine here?  It should be in the information section above
* but just following libretro.h.  Makes it easier to see changes.
**************************************************************************************************/
unsigned int retro_get_region(void)
{
	// Returning NTSC not sure if important to return PAL at any point.
	return RETRO_REGION_NTSC;
}


/**************************************************************************************************
***************************************************************************************************
*
* Memory routines called from frontend.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
* Return memory area asked for by frontend.
**************************************************************************************************/
void *retro_get_memory_data(unsigned in_id)
{
	// TODO: Return memory, not sure if needed.
	UNUSED(in_id);
	return NULL;
}

/**************************************************************************************************
* Retrun size of memory asked for by frontend.
**************************************************************************************************/
size_t retro_get_memory_size(unsigned in_id)
{
	// TODO: Return memory sizes, not sure if needed.
	UNUSED(in_id);
	return 0;
}

/**************************************************************************************************
***************************************************************************************************
*
* Utility functions.
*
***************************************************************************************************
**************************************************************************************************/

/**************************************************************************************************
**************************************************************************************************/
void retro_log(int in_debug_info_warn_error, const char *in_fmt, ...)
{
	if (!log_cb)
      return;

	enum retro_log_level e_log_level = (enum retro_log_level) in_debug_info_warn_error;
	if (e_log_level > RETRO_LOG_ERROR) e_log_level = RETRO_LOG_ERROR;

	va_list va_arguements;
	va_start(va_arguements, in_fmt);
	log_cb(e_log_level, in_fmt, va_arguements);
	va_end(va_arguements);
}
