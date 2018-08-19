/*
 * blend.h
 *
 * Copyright (C) 2005 Matt Ownby
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

// blend.h


#ifndef BLEND_H
#define BLEND_H

#include <stdint.h>
#include <SDL.h>	// for datatype defs

// TO USE THE BLEND FUNCTIONS:
// 1 - set g_blend_line1 to the first line of bytes to be averaged
// 2 - set g_blend_line2 to the second line of bytes to be averaged
// 3 - set g_blend_dest to the line of bytes that will be the destination
// 4 - set g_blend_iterations to how many bytes long all lines are (they all must be the same length).
//	IMPORTANT : g_blend_iterations must be a multiple of 8, and must be >= 8!
// 5 - run g_blend_func() and you're done!

// we always want this function defined for the purpose of testing (releasetest.cpp)
void blend_c();

extern uint8_t *g_blend_line1;
extern uint8_t *g_blend_line2;
extern uint8_t *g_blend_dest;
extern Uint32 g_blend_iterations;
#define g_blend_func blend_c

/////////////////////////////

#endif
