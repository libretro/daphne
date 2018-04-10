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
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory>
#ifdef WIN32
#include <ctype.h>
#endif

#include "libretro.h"
#include "libretro_daphne.h"
#include "../daphne-1.0-src/daphne.h"
#include "../main_android.h"
#include "..\src\core\android\SDL_android.h"
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

// File and path of loaded rom.
bool gf_isThayers = false;
#ifdef __ANDROID__
char gstr_rom_path[PATH_MAX];
#else
char gstr_rom_path[_MAX_PATH];
#endif

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
static	retro_log_printf_t			cb_logprint			= NULL;
		retro_video_refresh_t		cb_videorefresh		= NULL;
static	retro_input_poll_t			cb_inputpoll		= NULL;
static	retro_input_state_t			cb_inputstate		= NULL;
static	retro_environment_t			cb_environment		= NULL;
static	retro_audio_sample_t		cb_audiosample		= NULL;
		retro_audio_sample_batch_t	cb_audiosamplebatch	= NULL;


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
void retro_keyboard_input_callback(bool in_down, unsigned in_keycode, uint32_t in_character, uint16_t in_key_modifiers);
			
void retro_set_environment(retro_environment_t in_environment)
{
	LOGI("daphne-libretro: In retro_set_environment, top of function.");

	if (! in_environment) return;

	cb_environment = in_environment;
	
	static const struct retro_variable t_environmentvariables[] =
	{
		{ "daphne_invertctrl",		"Invert controls (if supported); enable|disable" },
		{ "daphne_useoverlaysb",	"Overlay scoreboard (if supported); enable|disable" },
		{ "daphne_emulate_seek",	"Emulate LaserDisc seeks; enable|disable" },
		{ "daphne_cheat",			"Supported Cheats; disable|enable" },
		{ NULL, NULL }
	};

	cb_environment(RETRO_ENVIRONMENT_SET_VARIABLES, (void *)t_environmentvariables);

	struct retro_keyboard_callback t_keyboardcb;
	t_keyboardcb.callback = retro_keyboard_input_callback;
	cb_environment(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, (void *)&t_keyboardcb);

	LOGI("daphne-libretro: In retro_set_environment, bottom of function.");
}

/**************************************************************************************************
*************************************************************************************************/
int retro_get_variable(char * strVariable)
{
	struct retro_variable var = { 0 };
	var.key = strVariable;
	if (cb_environment(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
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
	LOGI("daphne-libretro: In retro_set_video_refresh, top of function.");

	if (! in_videorefresh) return;
	cb_videorefresh = in_videorefresh;
	
	LOGI("daphne-libretro: In retro_set_video_refresh, bottom of function.");
}

/**************************************************************************************************
* Render an audio frame, only use one.  Will be called, at least once, before
* retro_run.
*************************************************************************************************/
void retro_set_audio_sample(retro_audio_sample_t in_audiosample)
{
	LOGI("daphne-libretro: In retro_set_audio_sample, top of function.");

	if (! in_audiosample) return;
	cb_audiosample = in_audiosample;

	LOGI("daphne-libretro: In retro_set_audio_sample, bottom of function.");
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t in_audiosamplebatch)
{
	LOGI("daphne-libretro: In retro_set_audio_sample_batch, top of function.");

	if (! in_audiosamplebatch) return;
	cb_audiosamplebatch = in_audiosamplebatch;

	LOGI("daphne-libretro: In retro_set_audio_sample_batch, bottom of function.");
}

/**************************************************************************************************
* Poll input. Will be called, at least once, before retro_run.
**************************************************************************************************/
void retro_set_input_poll(retro_input_poll_t in_inputpoll)
{
	LOGI("daphne-libretro: In retro_set_input_poll, top of function.");

	if (! in_inputpoll) return;
	cb_inputpoll = in_inputpoll;

	LOGI("daphne-libretro: In retro_set_input_poll, bottom of function.");
}

/**************************************************************************************************
* Get specific input, usually, after a poll. Will be called, at least once,
* before retro_run.
**************************************************************************************************/
void retro_set_input_state(retro_input_state_t in_inputstate)
{
	LOGI("daphne-libretro: In retro_set_input_state, top of function.");

	if (! in_inputstate) return;
	cb_inputstate = in_inputstate;

	LOGI("daphne-libretro: In retro_set_input_state, bottom of function.");
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
	LOGI("daphne-libretro: In retro_init, top of function.");

	// Set the internal pixel format.
	if (cb_environment)
	{
		// RGB565 picked here as that is what is recommended.  However, Daphne
		// itself uses YUY2 (16 bpp).
		enum retro_pixel_format n_pixelformat = RETRO_PIXEL_FORMAT_RGB565;
		cb_environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &n_pixelformat);
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
	cb_environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &t_inputdescriptors);

	// Setup cross platform logging.
	struct retro_log_callback t_logcb;
	if (cb_environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &t_logcb))
	{
		cb_logprint = t_logcb.log;
		cb_logprint(RETRO_LOG_INFO, "daphne-libretro: Logging initialized.");
	}

	if (cb_logprint) cb_logprint(RETRO_LOG_INFO, "daphne-libretro: In retro_init.");

    // Set the performance level, not sure what "4" means
    unsigned int n_perflevel = 4;
    cb_environment(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &n_perflevel);

	// Get system specific information.
	struct retro_system_specific_info t_system_specific;
	cb_environment(RETRO_ENVIRONMENT_GET_SYSTEM_SPECIFIC_INFO, &t_system_specific);

	// Clear the rom paths.
	gf_isThayers			= false;
	gstr_rom_extension[0]	= '\0';
	gstr_rom_name[0]		= '\0';
	gstr_rom_path[0]		= '\0';

	// Call Daphne startup code.
	// daphne_retro_init();

	LOGI("daphne-libretro: In retro_init, bottom of function.");
}

/**************************************************************************************************
* Core tear down.  Essentially de-retro_init.  Called only once.
**************************************************************************************************/
void retro_deinit(void)
{
	LOGI("daphne-libretro: In retro_deinit, top of function.");
	if (cb_logprint) cb_logprint(RETRO_LOG_INFO, "daphne-libretro: In retro_deinit.");

#ifdef __ANDROID__
	// 2017.09.20 - RJS - this might be overkill, was just mimicing APK version
	Android_nativeQuit();
#endif

	main_daphne_shutdown();

	LOGI("daphne-libretro: In retro_deinit, bottom of function.");
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
	LOGI("daphne-libretro: In retro_get_system_info, top of function.");

	memset(out_systeminfo, 0, sizeof(*out_systeminfo));
    out_systeminfo->library_name		= "Daphne";
    out_systeminfo->library_version		= "v0.04";
    out_systeminfo->need_fullpath		= true;
	out_systeminfo->block_extract		= true;
    out_systeminfo->valid_extensions	= DAPHNE_ROM_EXTENSION;

	LOGI("daphne-libretro: In retro_get_system_info, bottom of function.");
}

/**************************************************************************************************
* Gets information about system audio/video timings and geometry.  Can be
* called only after retro_load_game() has successfully completed.
**************************************************************************************************/
void retro_get_system_av_info(struct retro_system_av_info *out_avinfo)
{
	LOGI("daphne-libretro: In retro_get_system_av_info, top of function.");

    memset(out_avinfo, 0, sizeof(*out_avinfo));
    
	out_avinfo->geometry.base_width   = DAPHNE_VIDEO_W;
    out_avinfo->geometry.base_height  = DAPHNE_VIDEO_H;
    out_avinfo->geometry.max_width    = DAPHNE_VIDEO_W;
    out_avinfo->geometry.max_height   = DAPHNE_VIDEO_H;
    out_avinfo->geometry.aspect_ratio = DAPHNE_VIDEO_Wf / DAPHNE_VIDEO_Hf;
	
	out_avinfo->timing.fps = DAPHNE_TIMING_FPS;
	// RJS HERE - sample rate input to RA
	out_avinfo->timing.sample_rate = DAPHNE_AUDIO_SAMPLE_RATE;

	LOGI("daphne-libretro: In retro_get_system_av_info, bottom of function.");
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
	// TODO: Put Daphne reset code here.

	// Clear the rom paths.
	gstr_rom_extension[0]	= '\0';
	gstr_rom_name[0]		= '\0';
	gstr_rom_path[0]		= '\0';
}

/**************************************************************************************************
* Handles keyboard input.
**************************************************************************************************/
extern "C" {
	extern int Android_OnKeyDown(int keycode);
	extern int Android_OnKeyUp(int keycode);
}

void retro_keyboard_input_callback(bool in_down, unsigned in_keycode, uint32_t in_character, uint16_t in_key_modifiers)
{
	LOGI("daphne-libretro: In retro_keyboard_input_callback, top of function.  down: %d  keycode: %d  character: %d  mods: %d", in_down, in_keycode, in_character, in_key_modifiers);

	if (gf_isThayers == false)		return;
	if (in_keycode >= RETROK_LAST)	return;

#ifdef __ANDROID__
	// 2017.12.04 - RJS - Doesn't handle key modifiers.  Add if needed.  Table is a monster, may need some rethought to make sparse.
	static int n_RAtoAndroid_Keycode[] = {
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 0
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 1
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 2
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 3
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 4
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 5
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 6
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 7
		AKEYCODE_UNKNOWN,						// RETROK_BACKSPACE			= 8
		AKEYCODE_TAB,							// RETROK_TAB				= 9
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 10
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 11
		AKEYCODE_CLEAR,							// RETROK_CLEAR				= 12
		AKEYCODE_ENTER,							// RETROK_RETURN			= 13
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 14
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 15
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 16
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 17
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 18
		AKEYCODE_UNKNOWN,						// RETROK_PAUSE				= 19
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 20
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 21
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 22
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 23
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 24
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 25
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 26
		AKEYCODE_ESCAPE,						// RETROK_ESCAPE			= 27
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 28
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 29
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 30
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 31
		AKEYCODE_SPACE,							// RETROK_SPACE				= 32
		AKEYCODE_UNKNOWN,						// RETROK_EXCLAIM			= 33
		AKEYCODE_UNKNOWN,						// RETROK_QUOTEDBL			= 34
		AKEYCODE_POUND,							// RETROK_HASH				= 35
		AKEYCODE_UNKNOWN,						// RETROK_DOLLAR			= 36
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 37
		AKEYCODE_STAR,							// RETROK_AMPERSAND			= 38
		AKEYCODE_APOSTROPHE,					// RETROK_QUOTE				= 39
		AKEYCODE_NUMPAD_LEFT_PAREN,				// RETROK_LEFTPAREN			= 40
		AKEYCODE_NUMPAD_RIGHT_PAREN,			// RETROK_RIGHTPAREN		= 41
		AKEYCODE_STAR,							// RETROK_ASTERISK			= 42
		AKEYCODE_PLUS,							// RETROK_PLUS				= 43
		AKEYCODE_COMMA,							// RETROK_COMMA				= 44
		AKEYCODE_MINUS,							// RETROK_MINUS				= 45
		AKEYCODE_PERIOD,						// RETROK_PERIOD			= 46
		AKEYCODE_SLASH,							// RETROK_SLASH				= 47
		AKEYCODE_0,								// RETROK_0					= 48
		AKEYCODE_1,								// RETROK_1					= 49
		AKEYCODE_2,								// RETROK_2					= 50
		AKEYCODE_3,								// RETROK_3					= 51
		AKEYCODE_4,								// RETROK_4					= 52
		AKEYCODE_5,								// RETROK_5					= 53
		AKEYCODE_6,								// RETROK_6					= 54
		AKEYCODE_7,								// RETROK_7					= 55
		AKEYCODE_8,								// RETROK_8					= 56
		AKEYCODE_9,								// RETROK_9					= 57
		AKEYCODE_UNKNOWN,						// RETROK_COLON				= 58
		AKEYCODE_SEMICOLON,						// RETROK_SEMICOLON			= 59
		AKEYCODE_UNKNOWN,						// RETROK_LESS				= 60
		AKEYCODE_EQUALS,						// RETROK_EQUALS			= 61
		AKEYCODE_UNKNOWN,						// RETROK_GREATER			= 62
		AKEYCODE_UNKNOWN,						// RETROK_QUESTION			= 63
		AKEYCODE_AT,							// RETROK_AT				= 64
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 65
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 66
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 67
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 68
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 69
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 70
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 71
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 72
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 73
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 74
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 75
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 76
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 77
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 78
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 79
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 80
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 81
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 82
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 83
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 84
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 85
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 86
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 87
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 88
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 89
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 90
		AKEYCODE_LEFT_BRACKET,					// RETROK_LEFTBRACKET		= 91
		AKEYCODE_BACKSLASH,						// RETROK_BACKSLASH			= 92
		AKEYCODE_RIGHT_BRACKET,					// RETROK_RIGHTBRACKET		= 93
		AKEYCODE_UNKNOWN,						// RETROK_CARET				= 94
		AKEYCODE_UNKNOWN,						// RETROK_UNDERSCORE		= 95
		AKEYCODE_UNKNOWN,						// RETROK_BACKQUOTE			= 96
		AKEYCODE_A,								// RETROK_a					= 97
		AKEYCODE_B,								// RETROK_b					= 98
		AKEYCODE_C,								// RETROK_c					= 99
		AKEYCODE_D,								// RETROK_d					= 100
		AKEYCODE_E,								// RETROK_e					= 101
		AKEYCODE_F,								// RETROK_f					= 102
		AKEYCODE_G,								// RETROK_g					= 103
		AKEYCODE_H,								// RETROK_h					= 104
		AKEYCODE_I,								// RETROK_i					= 105
		AKEYCODE_J,								// RETROK_j					= 106
		AKEYCODE_K,								// RETROK_k					= 107
		AKEYCODE_L,								// RETROK_l					= 108
		AKEYCODE_M,								// RETROK_m					= 109
		AKEYCODE_N,								// RETROK_n					= 110
		AKEYCODE_O,								// RETROK_o					= 111
		AKEYCODE_P,								// RETROK_p					= 112
		AKEYCODE_Q,								// RETROK_q					= 113
		AKEYCODE_R,								// RETROK_r					= 114
		AKEYCODE_S,								// RETROK_s					= 115
		AKEYCODE_T,								// RETROK_t					= 116
		AKEYCODE_U,								// RETROK_u					= 117
		AKEYCODE_V,								// RETROK_v					= 118
		AKEYCODE_W,								// RETROK_w					= 119
		AKEYCODE_X,								// RETROK_x					= 120
		AKEYCODE_Y,								// RETROK_y					= 121
		AKEYCODE_Z,								// RETROK_z					= 122
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 123
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 124
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 125
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 126
		AKEYCODE_DEL,							// RETROK_DELETE			= 127
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 128
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 129
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 130
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 131
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 132
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 133
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 134
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 135
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 136
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 137
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 138
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 139
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 140
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 141
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 142
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 143
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 144
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 145
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 146
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 147
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 148
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 149
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 150
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 151
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 152
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 153
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 154
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 155
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 156
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 157
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 158
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 159
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 160
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 161
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 162
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 163
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 164
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 165
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 166
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 167
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 168
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 169
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 170
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 171
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 172
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 173
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 174
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 175
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 176
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 177
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 178
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 179
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 180
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 181
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 182
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 183
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 184
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 185
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 186
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 187
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 188
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 189
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 190
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 191
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 192
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 193
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 194
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 195
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 197
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 198
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 199
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 190
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 200
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 201
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 202
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 203
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 204
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 205
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 206
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 207
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 208
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 209
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 210
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 211
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 212
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 213
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 214
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 215
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 216
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 217
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 218
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 219
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 220
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 221
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 222
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 223
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 224
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 225
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 226
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 227
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 228
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 229
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 230
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 231
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 232
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 233
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 234
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 235
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 236
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 237
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 238
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 239
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 240
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 241
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 242
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 243
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 244
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 245
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 246
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 247
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 248
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 249
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 250
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 251
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 252
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 253
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 254
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 255
		AKEYCODE_NUMPAD_0,						// RETROK_KP0				= 256
		AKEYCODE_NUMPAD_1,						// RETROK_KP1				= 257
		AKEYCODE_NUMPAD_2,						// RETROK_KP2				= 258
		AKEYCODE_NUMPAD_3,						// RETROK_KP3				= 259
		AKEYCODE_NUMPAD_4,						// RETROK_KP4				= 260
		AKEYCODE_NUMPAD_5,						// RETROK_KP5				= 261
		AKEYCODE_NUMPAD_6,						// RETROK_KP6				= 262
		AKEYCODE_NUMPAD_7,						// RETROK_KP7				= 263
		AKEYCODE_NUMPAD_8,						// RETROK_KP8				= 264
		AKEYCODE_NUMPAD_9,						// RETROK_KP9				= 265
		AKEYCODE_NUMPAD_DOT,					// RETROK_KP_PERIOD			= 266
		AKEYCODE_NUMPAD_DIVIDE,					// RETROK_KP_DIVIDE			= 267
		AKEYCODE_NUMPAD_MULTIPLY,				// RETROK_KP_MULTIPLY		= 268
		AKEYCODE_NUMPAD_SUBTRACT,				// RETROK_KP_MINUS			= 269
		AKEYCODE_NUMPAD_ADD,					// RETROK_KP_PLUS			= 270
		AKEYCODE_NUMPAD_ENTER,					// RETROK_KP_ENTER			= 271
		AKEYCODE_NUMPAD_EQUALS,					// RETROK_KP_EQUALS			= 272
		AKEYCODE_DPAD_UP,						// RETROK_UP				= 273
		AKEYCODE_DPAD_DOWN,						// RETROK_DOWN				= 274
		AKEYCODE_DPAD_RIGHT,					// RETROK_RIGHT				= 275
		AKEYCODE_DPAD_LEFT,						// RETROK_LEFT				= 276
		AKEYCODE_INSERT,						// RETROK_INSERT			= 277
		AKEYCODE_MOVE_HOME,						// RETROK_HOME				= 278
		AKEYCODE_MOVE_END,						// RETROK_END				= 279
		AKEYCODE_PAGE_UP,						// RETROK_PAGEUP			= 280
		AKEYCODE_PAGE_DOWN,						// RETROK_PAGEDOWN			= 281
		AKEYCODE_F1,							// RETROK_F1				= 282
		AKEYCODE_F2,							// RETROK_F2				= 283
		AKEYCODE_F3,							// RETROK_F3				= 284
		AKEYCODE_F4,							// RETROK_F4				= 285
		AKEYCODE_F5,							// RETROK_F5				= 286
		AKEYCODE_F6,							// RETROK_F6				= 287
		AKEYCODE_F7,							// RETROK_F7				= 288
		AKEYCODE_F8,							// RETROK_F8				= 289
		AKEYCODE_F9,							// RETROK_F9				= 290
		AKEYCODE_F10,							// RETROK_F10				= 291
		AKEYCODE_F11,							// RETROK_F11				= 292
		AKEYCODE_F12,							// RETROK_F12				= 293
		AKEYCODE_UNKNOWN,						// RETROK_F13				= 294
		AKEYCODE_UNKNOWN,						// RETROK_F14				= 295
		AKEYCODE_UNKNOWN,						// RETROK_F15				= 296
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 297
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 298
		AKEYCODE_UNKNOWN,						// RETROK_UNKNOWN			= 299
		AKEYCODE_NUM_LOCK,						// RETROK_NUMLOCK			= 300
		AKEYCODE_CAPS_LOCK,						// RETROK_CAPSLOCK			= 301
		AKEYCODE_SCROLL_LOCK,					// RETROK_SCROLLOCK			= 302
		AKEYCODE_SHIFT_RIGHT,					// RETROK_RSHIFT			= 303
		AKEYCODE_SHIFT_LEFT,					// RETROK_LSHIFT			= 304
		AKEYCODE_CTRL_RIGHT,					// RETROK_RCTRL				= 305
		AKEYCODE_CTRL_LEFT,						// RETROK_LCTRL				= 306
		AKEYCODE_ALT_RIGHT,						// RETROK_RALT				= 307
		AKEYCODE_ALT_LEFT,						// RETROK_LALT				= 308
		AKEYCODE_META_RIGHT,					// RETROK_RMETA				= 309
		AKEYCODE_META_LEFT,						// RETROK_LMETA				= 310
		AKEYCODE_UNKNOWN,						// RETROK_LSUPER			= 311
		AKEYCODE_UNKNOWN,						// RETROK_RSUPER			= 312
		AKEYCODE_BUTTON_MODE,					// RETROK_MODE				= 313
		AKEYCODE_UNKNOWN,						// RETROK_COMPOSE			= 314
		AKEYCODE_HELP,							// RETROK_HELP				= 315
		AKEYCODE_UNKNOWN,						// RETROK_PRINT				= 316
		AKEYCODE_SYSRQ,							// RETROK_SYSREQ			= 317
		AKEYCODE_BREAK,							// RETROK_BREAK				= 318
		AKEYCODE_MENU,							// RETROK_MENU				= 319
		AKEYCODE_STB_POWER,						// RETROK_POWER				= 320
		AKEYCODE_UNKNOWN,						// RETROK_EURO				= 321
		AKEYCODE_UNKNOWN						// RETROK_UNDO				= 322
	};


	if (in_down)	Android_OnKeyDown(n_RAtoAndroid_Keycode[in_keycode]);
	else			Android_OnKeyUp(n_RAtoAndroid_Keycode[in_keycode]);
#endif // __ANDROID__
}

/**************************************************************************************************
* Checks if the given port and button's input state has changed.  Save new state if needed.
**************************************************************************************************/
#define RETRO_MAX_PLAYERS 2
#define RETRO_MAX_BUTTONS (RETRO_DEVICE_ID_JOYPAD_R3 + 1)

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
extern "C" {
	extern int Android_OnPadDown(int n_port, int keycode);
	extern int Android_OnPadUp(int n_port, int keycode);
	extern int16_t * get_ab_waiting(int * ab_ndx, int * ab_frames);
	extern void set_ab_streaming_done(int ab_ndx);
}
extern SDL_SW_YUVTexture * get_vb_waiting(int * vb_ndx);
extern bool input_pause(bool fPaused);
extern void set_vb_rendering_done(int vb_ndx);
extern int retro_run_frames_delta;

// 2017.11.06 - RJS - This should be the same amount as the VIDEO_BUFFER_AMOUNT define.  Hard coding in for ease.
// 2018.04.02 - RJS - Ignore above comment.  Left for legacy.
/*
struct VIDEO_BUFFER
{
	int					vb_ndx;
	SDL_SW_YUVTexture *	sw_overlay;
};
struct VIDEO_BUFFER tVB[4] = { { -1, NULL },{ -1, NULL },{ -1, NULL },{ -1, NULL } };
*/

int retro_run_frames	= 0;

#ifdef __ANDROID__
jboolean retro_run_once	= JNI_FALSE;
#else
#define JNI_FALSE false
#define JNI_TRUE true
bool retro_run_once = JNI_FALSE;
#endif

void retro_run(void)
{
	// LOGI("*** RETRO RUN BEAT ***");
	// LOGI("daphne-libretro: In retro_run, top of function.");

	// 2017.11.08 - RJS - Counter to tell when retro_run is paused.
	// LOGI("rrf: %d  rrf_xxxx: %d  rrf_delta: %d", retro_run_frames, retro_run_frames, retro_run_frames_delta);
	if (retro_run_frames_delta >= RETRO_RUN_FRAMES_PAUSED_THRESHOLD)
	{
		retro_run_frames_delta = 0;
		LOGI("rrf: %d  rrf_xxxx: %d  rrf_delta: %d  -  Should be UNPAUSEing.", retro_run_frames, retro_run_frames, retro_run_frames_delta);
		input_pause(false);
	}
	retro_run_frames++;

	// Some of the back end needs to know when certain systems have been init'd.  When
	// retro_run has run once, all relavent system have been initialized.
	retro_run_once = JNI_TRUE;

	// Poll input.
	cb_inputpoll();

	// Notice numplayers starts at 1.  The playercontrollerid_list points to the player ordinal
	// to the actual device id (port).  Hardcoded here for later proper updating.
	int n_playercontrollerid_list[RETRO_MAX_PLAYERS] = { 0, 1 };
	for (int n_numplayers = 0; n_numplayers < RETRO_MAX_PLAYERS; n_numplayers++)
	{
		int n_port = n_playercontrollerid_list[n_numplayers];

		for (int n_button_ndx = 0; n_button_ndx < RETRO_MAX_BUTTONS; n_button_ndx++)
		{
			uint16_t n_key;
			n_key = cb_inputstate(n_port, RETRO_DEVICE_JOYPAD, 0, n_button_ndx);
			// RJS REMOVE LOGI("daphne-libretro: In retro_run, before check of input state.  keycode: %d  port: %d  key: %d.", n_button_ndx, n_port, n_key);
			// 2017.09.18 - RJS - I may not need this debouncer as this is kept track in SDL_PrivateJoystickButton.		Nope, you need it.
			if (retro_has_inputstate_changed(n_port, n_button_ndx, n_key))
			{
			#ifdef __ANDROID__
				// LOGI("daphne-libretro: In retro_run, input state has changed.  keycode: %d  port: %d  key: %d.", n_button_ndx, n_port, n_key);
				if (n_button_ndx == 0)	n_button_ndx = AKEYCODE_BUTTON_A;
				if (n_button_ndx == 1)	n_button_ndx = AKEYCODE_BUTTON_X;
				if (n_button_ndx == 8)	n_button_ndx = AKEYCODE_BUTTON_B;
				if (n_button_ndx == 9)	n_button_ndx = AKEYCODE_BUTTON_Y;

				if (n_button_ndx == 2)	n_button_ndx = AKEYCODE_BACK;
				if (n_button_ndx == 3)	n_button_ndx = AKEYCODE_BUTTON_START;

				if (n_button_ndx == 4)	n_button_ndx = AKEYCODE_DPAD_UP;
				if (n_button_ndx == 5)	n_button_ndx = AKEYCODE_DPAD_DOWN;
				if (n_button_ndx == 6)	n_button_ndx = AKEYCODE_DPAD_LEFT;
				if (n_button_ndx == 7)	n_button_ndx = AKEYCODE_DPAD_RIGHT;

				if (n_button_ndx == 10)	n_button_ndx = AKEYCODE_BUTTON_L1;
				if (n_button_ndx == 11)	n_button_ndx = AKEYCODE_BUTTON_R1;

				if (n_button_ndx == 12)	n_button_ndx = AKEYCODE_BUTTON_L2;
				if (n_button_ndx == 13)	n_button_ndx = AKEYCODE_BUTTON_R2;

				if (n_button_ndx == 14)	n_button_ndx = AKEYCODE_BUTTON_THUMBL;
				if (n_button_ndx == 15)	n_button_ndx = AKEYCODE_BUTTON_THUMBR;

				if (n_key)	Android_OnPadDown(n_port, n_button_ndx);
				else		Android_OnPadUp(n_port, n_button_ndx);
			#else
				// 2018.01.23 - RJS - Need Windows here
			#endif
			}
		}
		// float analogX = (float)cb_inputstate(n_port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X) / 32768.0f;
	}

	int ab_ndx = -1;
	int ab_frames = 0;
	int16_t * ab_buffer	= NULL;

	ab_buffer = get_ab_waiting(&ab_ndx, &ab_frames);
	LOGI("In retro_run, ab_buffer: %d  ab_frames: %d  cd_audiosamplebatch: %d", (int)ab_buffer, ab_frames, (int)cb_audiosamplebatch);
	if (cb_audiosamplebatch) cb_audiosamplebatch(ab_buffer, ab_frames);
	set_ab_streaming_done(ab_ndx);

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
	if (sw_overlay && cb_videorefresh) cb_videorefresh(sw_overlay->pixels, sw_overlay->w, sw_overlay->h, sw_overlay->w * DAPHNE_VIDEO_ByPP);
	set_vb_rendering_done(vb_ndx);

	// LOGI("daphne-libretro: In retro_run, bottom of function.");
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
		if (cb_logprint) cb_logprint(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, data buffer was loaded.");
		return false;
	}

	// Double check that we have a path.  Inputs from non-pathed sources will not be valid.
	if (in_game->path == NULL)
	{
		if (cb_logprint) cb_logprint(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, path was NULL, should never be.");
		return false;
	}

	if (cb_logprint) cb_logprint(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, full path from LR. Path: %s", in_game->path);

	// Strip out the file name.
	// Make sure the path is long enough.
	int n_pathsize = strlen(in_game->path);
	if (n_pathsize <= (sizeof(DAPHNE_ROM_EXTENSION) + sizeof(".")))
	{
		if (cb_logprint) cb_logprint(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, path filename doesn't seem to have a valid format. Pathsize: %d  Extsize: %d", n_pathsize, (sizeof(DAPHNE_ROM_EXTENSION) + sizeof(".")));
		return false;
	}

	if (cb_logprint) cb_logprint(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, full path size from LR. Path: %d", n_pathsize);

	// Look for the last slash in the path.
	const char * pstr_filename = strrchr(in_game->path, '/');
	if (pstr_filename == NULL)
	{
		// We could have the case of just a filename.
		pstr_filename = in_game->path;
	} else {
		// Get past the slash.
		pstr_filename++;
	}

	if (cb_logprint) cb_logprint(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, filename and extension. Filename: %s", pstr_filename);

	// Split the filename from extension.
	const char * pstr_fileextension = strrchr((char *) pstr_filename, '.');
	pstr_fileextension++;
	if (strcmp(DAPHNE_ROM_EXTENSION, pstr_fileextension) != 0)
	{
		if (cb_logprint) cb_logprint(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, filename doesn't seem to have a valid format. Ext: %s  Fileext: %s", DAPHNE_ROM_EXTENSION, pstr_fileextension);
		return false;
	}

	if (cb_logprint) cb_logprint(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, extension. Extension: %s", pstr_fileextension);

	// Load globals with path, filename, and extension.  Assumption is everything is OK.
#ifdef __ANDROID__
	strncpy(gstr_rom_extension, pstr_fileextension, sizeof(gstr_rom_extension));
#else
	strncpy_s(gstr_rom_extension, pstr_fileextension, sizeof(gstr_rom_extension));
#endif

	if ((pstr_fileextension - pstr_filename - 1) > sizeof(gstr_rom_name))
	{
		if (cb_logprint) cb_logprint(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, filename doesn't seem to have a valid format.");
		return false;
	}
	memcpy(gstr_rom_name, pstr_filename, pstr_fileextension - pstr_filename - 1);

	if ((pstr_filename - in_game->path - 1) > sizeof(gstr_rom_path))
	{
		if (cb_logprint) cb_logprint(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game_get_path, path doesn't seem to have a valid format.");
		return false;
	}
	memcpy(gstr_rom_path, in_game->path, pstr_filename - in_game->path - 1); 

	// Failsafe.  We're so careful.
	gstr_rom_extension[sizeof(gstr_rom_extension) - 1]	= '\0';
	gstr_rom_name[sizeof(gstr_rom_name) - 1]			= '\0';
	gstr_rom_path[sizeof(gstr_rom_path) - 1]			= '\0';

#ifndef __ANDROID__
	for (int i = 0; gstr_rom_name[i]; i++) {
		gstr_rom_name[i] = tolower(gstr_rom_name[i]);
	}
#endif

	if (cb_logprint) cb_logprint(RETRO_LOG_INFO, "daphne-libretro: In retro_load_game_get_path, final file. Path: %s  Name: %s  Ext: %s", gstr_rom_path, gstr_rom_name, gstr_rom_extension);

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
	if (strcasecmp(pstr_shortgamename, "lair") == 0)			return(DAPHNE_GAME_TYPE_LAIR);
	if (strcasecmp(pstr_shortgamename, "lair2") == 0)			return(DAPHNE_GAME_TYPE_LAIR2);
	if (strcasecmp(pstr_shortgamename, "ace") == 0)				return(DAPHNE_GAME_TYPE_ACE);
	if (strcasecmp(pstr_shortgamename, "ace") == 0)				return(DAPHNE_GAME_TYPE_ACE91);
	if (strcasecmp(pstr_shortgamename, "cliff") == 0)			return(DAPHNE_GAME_TYPE_CLIFF);
	if (strcasecmp(pstr_shortgamename, "gtg") == 0)				return(DAPHNE_GAME_TYPE_GTG);
	if (strcasecmp(pstr_shortgamename, "sdq") == 0)				return(DAPHNE_GAME_TYPE_SUPERD);
	if (strcasecmp(pstr_shortgamename, "tq") == 0)				return(DAPHNE_GAME_TYPE_THAYERS);
	if (strcasecmp(pstr_shortgamename, "astron") == 0)			return(DAPHNE_GAME_TYPE_ASTRON);
	if (strcasecmp(pstr_shortgamename, "galaxy") == 0)			return(DAPHNE_GAME_TYPE_GALAXY);
	if (strcasecmp(pstr_shortgamename, "esh") == 0)				return(DAPHNE_GAME_TYPE_ESH);
	if (strcasecmp(pstr_shortgamename, "laireuro") == 0)		return(DAPHNE_GAME_TYPE_LAIREURO);
	if (strcasecmp(pstr_shortgamename, "badlands") == 0)		return(DAPHNE_GAME_TYPE_BADLANDS);
	if (strcasecmp(pstr_shortgamename, "starrider") == 0)		return(DAPHNE_GAME_TYPE_STARRIDER);
	if (strcasecmp(pstr_shortgamename, "bega") == 0)			return(DAPHNE_GAME_TYPE_BEGA);
	if (strcasecmp(pstr_shortgamename, "interstellar") == 0)	return(DAPHNE_GAME_TYPE_INTERSTELLAR);
	if (strcasecmp(pstr_shortgamename, "sae") == 0)				return(DAPHNE_GAME_TYPE_SAE);
	if (strcasecmp(pstr_shortgamename, "mach3") == 0)			return(DAPHNE_GAME_TYPE_MACH3);
	if (strcasecmp(pstr_shortgamename, "uvt") == 0)				return(DAPHNE_GAME_TYPE_UVT);
	if (strcasecmp(pstr_shortgamename, "badlands") == 0)		return(DAPHNE_GAME_TYPE_BADLANDP);
	if (strcasecmp(pstr_shortgamename, "dle11") == 0)			return(DAPHNE_GAME_TYPE_DLE1);
	if (strcasecmp(pstr_shortgamename, "dle21") == 0)			return(DAPHNE_GAME_TYPE_DLE2);
	if (strcasecmp(pstr_shortgamename, "gpworld") == 0)			return(DAPHNE_GAME_TYPE_GPWORLD);
	if (strcasecmp(pstr_shortgamename, "cobraab") == 0)			return(DAPHNE_GAME_SUBTYPE_ASTRON_COBRA);
	if (strcasecmp(pstr_shortgamename, "roadblaster") == 0)		return(DAPHNE_GAME_SUBTYPE_BEGA_ROADBLASTER);

	return DAPHNE_GAME_TYPE_UNDEFINED;
}
*/

#ifndef __ANDROID__
#define strcasecmp strcmp
#define strncasecmp strncmp
#endif

bool retro_load_game_fill_framefile(char * pstr_framefile, int str_framefile_sz)
{
	// A resonable default.
	char str_default_framefile[DAPHNE_MAX_ROMNAME + 4];
	strcpy(str_default_framefile, gstr_rom_name);
	strcat(str_default_framefile, ".txt");
	strcpy(pstr_framefile, str_default_framefile);

	LOGI("daphne-libretro: In retro_load_game_fill_framefile, rom name:  %s", gstr_rom_name);

	if ((strcasecmp(gstr_rom_name, "ace")		== 0) ||
		(strcasecmp(gstr_rom_name, "ace_a2")	== 0) ||
		(strcasecmp(gstr_rom_name, "ace_a")		== 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "ace91")			== 0) ||
		(strcasecmp(gstr_rom_name, "ace91_euro")	== 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "aceeuro") == 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "astron")	== 0) ||
		(strcasecmp(gstr_rom_name, "astronp")	== 0))
	{
		strcpy(pstr_framefile, "astron.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "badlandp") == 0) ||
		(strcasecmp(gstr_rom_name, "badlands") == 0))
	{
		strcpy(pstr_framefile, "badlands.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "bega")		== 0) ||
		(strcasecmp(gstr_rom_name, "begar1")	== 0))
	{
		strcpy(pstr_framefile, "bega.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "benchmark") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "blazer") == 0))
	{
		strcpy(pstr_framefile, "blazer.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "cliff")		== 0) ||
		(strcasecmp(gstr_rom_name, "cliffalt")	== 0) ||
		(strcasecmp(gstr_rom_name, "cliffalt2")	== 0))
	{
		strcpy(pstr_framefile, "cliff.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "cobra")		== 0) ||
		(strcasecmp(gstr_rom_name, "cobraab")	== 0) ||
		(strcasecmp(gstr_rom_name, "cobram3")	== 0))
	{
		strcpy(pstr_framefile, "cobra.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "cobraconv") == 0))
	{
		strcpy(pstr_framefile, "cobraconv.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "cputest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "dle11")	== 0) ||
		(strcasecmp(gstr_rom_name, "dle2")	== 0) ||
		(strcasecmp(gstr_rom_name, "dle20")	== 0) ||
		(strcasecmp(gstr_rom_name, "dle21")	== 0))
	{
		strcpy(pstr_framefile, "lair.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "esh")		== 0) ||
		(strcasecmp(gstr_rom_name, "eshalt")	== 0) ||
		(strcasecmp(gstr_rom_name, "eshalt2")	== 0))
	{
		strcpy(pstr_framefile, "esh.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "firefox")	== 0) ||
		(strcasecmp(gstr_rom_name, "firefoxa")	== 0))
	{
		strcpy(pstr_framefile, "firefox.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "ffr") == 0))
	{
		strcpy(pstr_framefile, "ffr.txt");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "galaxy")	== 0) ||
		(strcasecmp(gstr_rom_name, "galaxyp")	== 0))
	{
		strcpy(pstr_framefile, "galaxy.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "gpworld") == 0))
	{
		strcpy(pstr_framefile, "gpworld.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "gtg") == 0))
	{
		strcpy(pstr_framefile, "gtg.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "interstellar") == 0))
	{
		strcpy(pstr_framefile, "interstellar.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "lair")		== 0) ||
		(strcasecmp(gstr_rom_name, "lair_f")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_e")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_d")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_c")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_b")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_a")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_n1")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_x")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_alt")	== 0) ||
		(strcasecmp(gstr_rom_name, "laireuro")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_ita")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair_d2")	== 0))
	{
		strcpy(pstr_framefile, "lair.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "lair2")				== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_319_euro")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_319_span")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_318")			== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_316_euro")	== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_315")			== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_314")			== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_300")			== 0) ||
		(strcasecmp(gstr_rom_name, "lair2_211")			== 0))
	{
		strcpy(pstr_framefile, "lair2.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "lgp") == 0))
	{
		strcpy(pstr_framefile, "lgp.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "mach3") == 0))
	{
		strcpy(pstr_framefile, "mach3.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "mcputest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "releasetest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "roadblaster") == 0))
	{
		strcpy(pstr_framefile, "roadblaster.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "sae") == 0))
	{
		strcpy(pstr_framefile, "ace.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "seektest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "singe") == 0))
	{
		strcpy(pstr_framefile, "singe.txt");  // ??
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "speedtest") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "sdq")			== 0) ||
		(strcasecmp(gstr_rom_name, "sdqshort")		== 0) ||
		(strcasecmp(gstr_rom_name, "sdqshortalt")	== 0))
	{
		strcpy(pstr_framefile, "sdq.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "starrider") == 0))
	{
		strcpy(pstr_framefile, "starrider.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "superdon") == 0))
	{
		strcpy(pstr_framefile, "superdon.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "timetrav") == 0))
	{
		strcpy(pstr_framefile, "timetrav.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "test_sb") == 0))
	{
		strcpy(pstr_framefile, "");
		return false;
	}

	if ((strcasecmp(gstr_rom_name, "tq")		== 0) ||
		(strcasecmp(gstr_rom_name, "tq_alt")	== 0) ||
		(strcasecmp(gstr_rom_name, "tq_swear")	== 0))
	{
		gf_isThayers = true;
		strcpy(pstr_framefile, "tq.txt");
		return true;
	}

	if ((strcasecmp(gstr_rom_name, "uvt") == 0))
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
	LOGI("daphne-libretro: In retro_load_game, top of function.");

	if (cb_logprint) cb_logprint(RETRO_LOG_ERROR, "daphne-libretro: In retro_load_game, path is: %s", in_game->path);

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

	if (((strncasecmp(gstr_rom_name, "lair2", 5)	!= 0) &&
		 (strncasecmp(gstr_rom_name, "aceeuro", 7)	!= 0))
		&&
		((strncasecmp(gstr_rom_name, "lair", 4)	== 0) ||
		(strncasecmp(gstr_rom_name, "dle", 3)	== 0) ||
		(strncasecmp(gstr_rom_name, "ace", 3)	== 0) ||
		(strncasecmp(gstr_rom_name, "sae", 3)	== 0) ||
		(strncasecmp(gstr_rom_name, "tq", 2)	== 0)))
	{
		strcpy(str_args[num_args], "-useoverlaysb");
		pstr_args[num_args] = str_args[num_args];
		num_args++;

		strcpy(str_args[num_args], "0");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
	}

	if ((strncasecmp(gstr_rom_name, "cobra", 5)		== 0) ||
		(strncasecmp(gstr_rom_name, "cobraab", 7)	== 0) ||
		(strncasecmp(gstr_rom_name, "cobraconv", 9)	== 0) ||
		(strncasecmp(gstr_rom_name, "cobram3", 7)	== 0) ||
		(strncasecmp(gstr_rom_name, "astron", 6)	== 0) ||
		(strncasecmp(gstr_rom_name, "astronp", 7)	== 0) ||
		(strncasecmp(gstr_rom_name, "galaxy", 6)	== 0) ||
		(strncasecmp(gstr_rom_name, "galaxyp", 7)	== 0) ||
		(strncasecmp(gstr_rom_name, "mach3", 7)		== 0) ||
		(strncasecmp(gstr_rom_name, "uvt", 3)		== 0))
	{
		strcpy(str_args[num_args], "-invertctrl");
		pstr_args[num_args] = str_args[num_args];
		num_args++;
	}

	if ((strncasecmp(gstr_rom_name, "badlands", 8) == 0) ||
		(strncasecmp(gstr_rom_name, "badlandp", 8) == 0))
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
	if (strncasecmp(gstr_rom_name, "aceeuro", 7) == 0)
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
#ifdef __ANDROID__
	char strCommandline[PATH_MAX] = "";
#else
	char strCommandline[_MAX_PATH] = "";
#endif

	for (int i = 0; i < num_args; i++)
	{
		strcat(strCommandline, pstr_args[i]);
		strcat(strCommandline, " ");
	}
	LOGI("daphne-libretro: In retro_load_game, commandline to main_daphne: %d  %s", num_args, strCommandline);

	if (main_daphne(num_args, pstr_args) == 0) return true;
	
	return false;
	

	/*
	// Build out the command line v01.
	// Loading game type.
	DAPHNE_GAME_TYPE e_gametype = retro_load_game_get_game_type(gstr_rom_name);
	LOGI("daphne-libretro: In retro_load_game, game type: %d  path: %s  rom name: %s", e_gametype, gstr_rom_path, gstr_rom_name);
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

	LOGI("daphne-libretro: In retro_load_game, bottom of function - going to main_daphne.  path: %s", pstr_daphne_arguements[i + 1]);

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
	if (!cb_logprint) return;

	enum retro_log_level e_log_level = (enum retro_log_level) in_debug_info_warn_error;
	if (e_log_level > RETRO_LOG_ERROR) e_log_level = RETRO_LOG_ERROR;

	va_list va_arguements;
	va_start(va_arguements, in_fmt);
	cb_logprint(e_log_level, in_fmt, va_arguements);
	va_end(va_arguements);
}

//**************************************************************************************************
//**************************************************************************************************
/* 2017.10.10 - RJS - JavaVM removal.
JavaVM *  retro_get_javavm()
{
	if (!cb_environment) return NULL;

	struct retro_system_specific_info info;
	memset(&info, 0, sizeof(struct retro_system_specific_info));
	cb_environment(RETRO_ENVIRONMENT_GET_SYSTEM_SPECIFIC_INFO, &info);

	return info.javavm;
}

// **************************************************************************************************
// **************************************************************************************************
jobject retro_get_nativeinstance()
{
	if (!cb_environment) return NULL;

	struct retro_system_specific_info info;
	memset(&info, 0, sizeof(struct retro_system_specific_info));
	cb_environment(RETRO_ENVIRONMENT_GET_SYSTEM_SPECIFIC_INFO, &info);

	return info.clazz;
}
*/