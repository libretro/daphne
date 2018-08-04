/*
 * input.h
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

#ifndef INPUT_H
#define INPUT_H

enum
{
	SWITCH_UP,
	SWITCH_LEFT,
	SWITCH_DOWN,
	SWITCH_RIGHT,
	SWITCH_START1,
	SWITCH_START2,
	SWITCH_BUTTON1,
	SWITCH_BUTTON2,
	SWITCH_BUTTON3,
	SWITCH_COIN1,
	SWITCH_COIN2,
	SWITCH_SKILL1,
	SWITCH_SKILL2, 
	SWITCH_SKILL3,
	SWITCH_SERVICE,
	SWITCH_TEST,
	SWITCH_RESET,
	SWITCH_SCREENSHOT,
	SWITCH_QUIT,
	SWITCH_PAUSE,
	SWITCH_CONSOLE,
	SWITCH_TILT,
	// RJS ADD - coin start switch
	SWITCH_COINSTART,
	SWITCH_COUNT,
	// RJS ADD - for switch clairity
	SWITCH_NOTHING
}; // daphne inputs for arcade and additional controls, leave SWITCH_COUNT at the end

///////////////////////

#include <SDL.h>

// to be passed into the coin queue
struct coin_input
{
	bool coin_enabled;	//	whether the coin was enabled or disabled
	Uint8 coin_val;	// either SWITCH_COIN1 or SWITCH_COIN2
	Uint64 cycles_when_to_enable;	// the cycle count that we must have surpassed in order to be able to enable the coin
};

////////////////////////

int SDL_input_init();
int SDL_input_shutdown();

// Filters out mouse events if 'bFilteredOut' is true.
// The purpose is so that games that don't use the mouse don't get a bunch of extra mouse
//  events which can hurt performance.
void FilterMouseEvents(bool bFilteredOut);

void SDL_check_input();

void process_event(SDL_Event *event);
// RJS CHANGE START
//void process_keydown(SDLKey key);
//void process_keyup(SDLKey key);
void process_keydown(SDL_Keycode key);
void process_keyup(SDL_Keycode key);
void input_invert_controls(bool fInvert);
bool input_isinverted();
// RJS CHANGE END
void process_joystick_motion(SDL_Event *event);
void process_joystick_hat_motion(SDL_Event *event);
bool input_pause(bool fPaused);
void input_enable(Uint8);
void input_disable(Uint8);
void reset_idle(void); // added by JFA

#endif // INPUT_H

