/*
 * error.cpp
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


// error.cpp
// daphne error handling

#ifdef WIN32
#include <windows.h>
#endif

#include <SDL.h>
#include <string.h>
#include "../daphne.h"
#include "conout.h"
#include "input.h"
#include "../game/game.h"

// notifies the user of an error that has occurred
void printerror(const char *s)
{
   static const char CRLF[3] = { 13, 10, 0 };	// carriage return / linefeed combo, for the addlog statements in this file
	addlog(s);
	addlog(CRLF);
}

// prints a notice to the screen
void printnotice(const char *s)
{
   printerror(s);
}
