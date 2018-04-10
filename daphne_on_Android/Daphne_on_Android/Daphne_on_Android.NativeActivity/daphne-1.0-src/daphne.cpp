/*
* daphne.cpp
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

// DAPHNE : Dragon's Lair / Space Ace Emulator
// Started by Matt Ownby, many people have contributed since then
// Begun August 3rd, 1999

// ORIGINAL GOAL (we've obviously done much more than this now hehe):

// To emulate the original Dragon's Lair / Space Ace game such that the original ROM's are executed
// properly, the joystick and buttons are emulated, and the Laserdisc Player is properly
// controlled
// In short, so that the original game may be played with the following requirements:
// - Original Dragon's Lair Laserdisc
// - Original Dragon's Lair ROM's (disk files, not the physical ROM's heh)
// - A laserdisc player
// - A common modern computer (i386 Win32, i386 Linux most likely)
// No other original hardware (motherboard, power supply, harness) required!

// RJS ADD - we aren't precompiling any headers but still need what would have been compiled
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif

#include "../pch.h"

#include <string>
#include "io/my_stdio.h"

using namespace std;

#ifdef MAC_OSX
#include "mmxdefs.h"
#endif

#ifndef FREEBSD
#include <SDL_main.h>
#else
#include "/usr/local/include/SDL11/SDL_main.h"
#endif

#ifdef WIN32
// win32 doesn't have regular chdir apparently
#define chdir _chdir
#include <direct.h>
#endif

#ifdef UNIX
#include <unistd.h>	// for chdir
#endif

#include "io/homedir.h"
#include "io/input.h"
#include "daphne.h"
#include "timer/timer.h"
#include "io/serial.h"
#include "sound/sound.h"
#include "io/conout.h"
#include "io/conin.h"
#include "io/cmdline.h"
#include "io/network.h"
#include "video/video.h"
#include "video/led.h"
#include "ldp-out/ldp.h"
#include "video/SDL_Console.h"
#include "io/error.h"
#include "cpu/cpu-debug.h"
#include "cpu/cpu.h"
#include "game/game.h"

#include "globals.h"
// some global data is stored in this file

#include "video\SDL_DrawText.h"

#include "..\main_android.h"
#ifdef __ANDROID__
extern "C" {
	extern void sdl_init();
}
#endif


// -------------------------------------------------------------------------------------------------

////////////////////////////////////////

const char *get_daphne_version()
{
	return "1.0.7";
}

unsigned char get_filename(char *s, unsigned char n)
// prepares a filename using any wildcards we may have
// returns filename in s
// returns 1 if this filename contains wild characters
// returns 0 if this filename does not contain any wild characters
{

	unsigned int i = 0;
	unsigned char result = 0;

	for (i = 0; i < strlen(s); i++)
	{
		if (s[i] == '+')
		{
			result = 1;
			s[i] = (char) (n + 0x30);	// convert wildcard to number
		}
	}

	return(result);

}

// RJS ADD START - for resetting flag
void set_quitflag(int nFlag)
{
	quitflag = nFlag;
}
// RJS ADD END

void set_quitflag()
// sets the quit flag
{

	quitflag = 1;

}

unsigned char get_quitflag()
// returns the quit flag
{

	return(quitflag);

}

bool change_dir(const char *cpszNewDir)
{
	int i = chdir(cpszNewDir);
	return (i == 0);
}

// sets current directory based on the full path of the executable
// This solves the problem of someone running daphne without first being in the daphne directory
void set_cur_dir(const char *exe_loc)
{
	int index = strlen(exe_loc) - 1;	// start on last character
	string path = "";


	// locate the preceeding / or \ character
	while ( (index >= 0) &&
			(exe_loc[index] != '/') &&
			(exe_loc[index] != '\\'))
	{
		index--;
	}

	// if we found a leading / or \ character
	if (index >= 0)
	{
		path = exe_loc;
		path.erase(index);	// erase from here to the end
		change_dir(path.c_str());
	}
}

/////////////////////// MAIN /////////////////////

// the main function for both Windows and Linux <grin>
// RJS CHANGE
// int main(int argc, char **argv)
int main_daphne(int argc, char **argv)
{
	// RJS LOG
	LOGI("daphne-libretro: In main_daphne, top of routine.");

	int result_code = 1;	// assume an error unless we find otherwise

	set_cur_dir(argv[0]);	// set active directory

	// RJS ADD START - on the c side, reinitialization of variables
	input_invert_controls(false);
	set_quitflag(0);
	// RJS ADD END

	reset_logfile(argc, argv);

	// initialize SDL without any subsystems but with the no parachute option so
	// 1 - we can initialize either audio or video first
	// 2 - we can trace segfaults using a debugger
	LOGI("daphne-libretro: In main_daphne, before SDL_Init.");

#ifdef __ANDROID__
	sdl_init();
#endif

	if (SDL_Init(SDL_INIT_NOPARACHUTE) < 0)
	{
		printerror("Could not initialize SDL!");
		exit(1);
	}

	LOGI("daphne-libretro: In main_daphne, after SDL_Init.");

	// parse the command line (which allocates game and ldp) and continue if no errors
	// this is important!  if game_type or ldp_type fails to allocate g_game and g_ldp,
	// then the program will segfault and daphne must NEVER segfault!  hehe
	if (parse_cmd_line(argc, argv))
	{
		LOGI("daphne-libretro: In main_daphne, after parse_cmd_line.");
	
		// MATT : we have to wait until after the command line is parsed before we do anything with the LEDs in case the
		// user does not enable them
		remember_leds(); // memorizes the status of keyboard leds
		change_led(false, false, false); // turns all keyboard leds off

		// if the display initialized properly
		// RJS CHANGE - need a renderer to be able to make bitmaps into textures.
		// if (load_bmps() && init_display())
		// RJS RENDER - need to load bmps, change to surfaces, quitflag checked added here since starting ldp early
		// if (init_display() && load_bmps()) took this out for a hack to check moving rendering to another thread
		
		LOGI("daphne-libretro: In main_daphne, before init_display.");

		if ((get_quitflag() == 0) && (init_display()))
		{
			// RJS RENDER ADD - try to start the ldp first so we can draw, ldp features are not needed yet
			LOGI("daphne-libretro: In main_daphne, before LDP pre_init and load_bmps.");
			if (g_ldp->pre_init() && load_bmps())
			{
				// RJS ADD START - loading the only font we have, not sure where it is ever loaded in the first place, maybe latent code,
				// this font must be loaded first since there is an enum making the "small" font number 0
				//			if (ConsoleInit("pics/ConsoleFont.bmp", g_screen_blitter, 100)==0)
				LoadFont("pics/ConsoleFont.bmp", TRANS_FONT, get_screen_blitter()->format);

				LOGI("daphne-libretro: In main_daphne, before sound_init.");
				LOGI("sound_bringup: In main_daphne, calling sound_init.");
				if (sound_init())
				{
					LOGI("daphne-libretro: In main_daphne, before SDL_input_init.");
					if (SDL_input_init())
					{
						// if the roms were loaded successfully
						LOGI("daphne-libretro: In main_daphne, before load_roms.");
						if (g_game->load_roms())
						{
							// if the video was initialized successfully
							LOGI("daphne-libretro: In main_daphne, before video_init.");
							if (g_game->video_init())
							{
								// if the game has some problems, notify the user before the user plays the game
								LOGI("daphne-libretro: In main_daphne, before get_issues.");
								if (g_game->get_issues())
								{
									printnowookin(g_game->get_issues());
								}
								SDL_Delay(1000);
								// delay for a bit before the LDP is intialized to make sure
								// all video is done getting drawn before VLDP is initialized

								// if the laserdisc player was initialized properly
								// RJS RENDER REMOVE - if and else
								// if (g_ldp->pre_init())
								{
									LOGI("daphne-libretro: In main_daphne, most SDL subsystems initialized, before game pre_init.");
									if (g_game->pre_init())     // initialize all cpu's
									{
										// 2017.08.18 - RJS - See main_daphne_* routines below.
										// printline("Booting ROM ...");
										// LOGI("daphne-libretro: In main_daphne, firing up cpus and therfore the main loop, before game start.");
										// RJS HERE - main loop start function called here
										g_game->start();	// HERE IS THE MAIN LOOP RIGHT HERE
										// LOGI("daphne-libretro: In main_daphne, game is over for some reason, before game pre_shutdown.");
										// g_game->pre_shutdown();

										// Send our game/ldp type to server to create stats.
										// This was moved to after the game has finished running because it creates a slight delay (like 1 second),
										//  which throws off the think_delay function in the LDP class.
										// net_send_data_to_server();

										result_code = 0;	// daphne will exit without any errors
									}
									else
									{
										//exit if returns an error but don't print error message to avoid repetition
										result_code = 1;
									}
									// g_ldp->pre_shutdown();
								}
								// RJS RENDER REMOVE - else error
								// else
								// {
								// 	if (RJStest != 0) printerror("Could not initialize laserdisc player!");
								// }
								// g_game->video_shutdown();
							} // end if game video was initted properly
							else
							{
								printerror("Game-specific video initialization failed!");
								result_code = 1;
							}
						} // end if roms were loaded properly
						else
						{
							printerror("Could not load ROM images! You must supply these.");
							result_code = 1;
						}
						// SDL_input_shutdown();
					}
					else
					{
						printerror("Could not initialize input!");
						result_code = 1;
					}
					// sound_shutdown();
				}
				else
				{
					printerror("Sound initialization failed!");
					result_code = 1;
				}
			}
			else
			{
				// RJS ADD - this whole else block
				printerror("Could not initialize laserdisc player!");
				result_code = 1;
			}
			// shutdown_display();	// shut down the display
		} // end init display
		else
		{
			printerror("Video initialization failed!");
			result_code = 1;
		}
	} // end game class allocated properly
	// if command line was bogus, quit
	else
	{
		printerror("Bad command line or initialization problem (see daphne_log.txt for details). \n"
			"To run DAPHNE, you must specify which game to run and which laserdisc player you are using. \n"
			"For example, try 'daphne lair noldp' to run Dragon's Lair in testing mode.");
		result_code = 1;
	}

	/*
	// if our g_game class was allocated
	if (g_game)
	{
		delete(g_game);
		// RJS ADD - for reentrance
		g_game = NULL;
	}

	// if g_ldp was allocated, de-allocate it
	if (g_ldp)
	{
		delete(g_ldp);
		// RJS ADD - for reentrance
		g_ldp = NULL;
	}

	free_bmps();	// always do this no matter what

	restore_leds();  // sets keyboard leds back how they were (this is safe even if we have the led's disabled)

	// RJS CHANGE
	// SDL_Quit();
	atexit(SDL_Quit);

	// RJS CHANGE - no need to force exit here, let's be nice
	// exit(result_code);
	*/
	LOGI("daphne-libretro: In main_daphne, bottom of routine.  Returning: %d", result_code);
	return(result_code);
}

// 2017.08.18 - RJS - branched out main loop and shutdown functions here since LR
// has its own run methods that are incompatible.  If this core also needs to be 
// compileable for standalone use, then these routines need to be added back to main_daphne
// and surrounded by #if statements for cross platform use.
int main_daphne_mainloop()
{
	LOGI("daphne-libretro: In main_daphne_mainloop, firing up cpus and therfore the main loop.");
	g_game->start();
	return 0;
}

int main_daphne_shutdown()
{
	LOGI("daphne-libretro: In main_daphne_shutdown, top of routine.");
	g_game->pre_shutdown();

	LOGI("daphne-libretro: In main_daphne_shutdown, after g_game->pre_shutdown.");
	net_send_data_to_server();

	int result_code = 0;	// daphne will exit without any errors

	LOGI("daphne-libretro: In main_daphne_shutdown, after net_send_data_to_server.");
	g_ldp->pre_shutdown();
	LOGI("daphne-libretro: In main_daphne_shutdown, after g_ldp->pre_shutdown.");
	g_game->video_shutdown();
	LOGI("daphne-libretro: In main_daphne_shutdown, after g_game->video_shutdown.");

	SDL_input_shutdown();
	LOGI("daphne-libretro: In main_daphne_shutdown, after SDL_input_shutdown.");

	sound_shutdown();
	LOGI("daphne-libretro: In main_daphne_shutdown, after sound_shutdown.");
	shutdown_display();
	LOGI("daphne-libretro: In main_daphne_shutdown, after shutdown_display. g_game: %d  g_ldp: %d", (int) g_game, (int) g_ldp);

	if (g_game)
	{
		delete(g_game);
		g_game = NULL;
	}

	if (g_ldp)
	{
		delete(g_ldp);
		g_ldp = NULL;
	}
	// 2018.02.06 - RJS - g_game had better have existed.
	LOGI("daphne-libretro: In main_daphne_shutdown, after delete of g_game and g_ldp.");

	free_bmps();
	LOGI("daphne-libretro: In main_daphne_shutdown, after free_bmps.");
	restore_leds();
	LOGI("daphne-libretro: In main_daphne_shutdown, after restore_leds.");
	atexit(SDL_Quit);
	LOGI("daphne-libretro: In main_daphne_shutdown, after atexit.");

	LOGI("daphne-libretro: In main_daphne_shutdown, bottom of routine. Result: %d", result_code);
	return(result_code);
}

// sets the serial port to be used to control LDP
void set_serial_port(unsigned char i)
{
	serial_port = i;
}


unsigned char get_serial_port()
{
	return(serial_port);
}

void set_baud_rate(int i)
{
	baud_rate = i;
}

int get_baud_rate()
{
	return(baud_rate);
}

void set_search_offset(int i)
{
	search_offset = i;
}

int get_search_offset()
{
	return(search_offset);
}

unsigned char get_frame_modifier()
{
	return(frame_modifier);
}

void set_frame_modifier(unsigned char value)
{
	frame_modifier = value;
}

void set_scoreboard(unsigned char value)
{
	realscoreboard = value;
}

unsigned char get_scoreboard()
{
	return (realscoreboard);
}

void set_scoreboard_port(unsigned int value)
{
	rsb_port = value;
}

unsigned int get_scoreboard_port()
{
	return(rsb_port);
}

void reset_logfile(int argc, char **argv)
{
	int i = 0;
	char s[160];
	string str;

	snprintf(s, sizeof(s), "--DAPHNE version %s", get_daphne_version());
	printline(s);
	str = "--Command line is: ";
	for (i = 0; i < argc; i++)
	{
		str = str + argv[i] + " ";
	}
	printline(str.c_str());
	snprintf(s, sizeof(s), "--CPU : %s %d MHz || Mem : %d megs", get_cpu_name(), get_cpu_mhz(), get_sys_mem());
	printline(s);
	snprintf(s, sizeof(s), "--OS : %s || Video : %s", get_os_description(), get_video_description());
	printline(s);
	outstr("--OpenGL: ");
#ifdef USE_OPENGL
	printline("Compiled In");
#else
	printline("Not Compiled In");
#endif // USE_OPENGL

	outstr("--RGB2YUV Function: ");
#ifdef USE_MMX
	printline("MMX");
#else
	printline("C");
#endif
	outstr("--Line Blending Function: ");
#ifdef USE_MMX
	printline("MMX");
#else
	printline("C");
#endif // blend MMX

	outstr("--Audio Mixing Function: ");
#ifdef USE_MMX
	printline("MMX");
#else
	printline("C");
#endif // blend MMX
}

// added by JFA for -idleexit
void set_idleexit(unsigned int value)
{
	idleexit = value;
}

unsigned int get_idleexit()
{
	return(idleexit);
}
// end edit

// added by JFA for -startsilent
void set_startsilent(unsigned char value)
{
	startsilent = value;
}

unsigned char get_startsilent()
{
	return(startsilent);
}
// end edit
