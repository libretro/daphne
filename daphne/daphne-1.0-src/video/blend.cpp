/*
 * blend.cpp
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

// blend.cpp

#include <stdint.h>
#include "blend.h"

uint8_t *g_blend_line1 = 0;
uint8_t *g_blend_line2 = 0;
uint8_t *g_blend_dest = 0;
uint32_t g_blend_iterations = 0;

void blend_c()
{
	register uint8_t *ptr1 = g_blend_line1;
	register uint8_t *ptr2 = g_blend_line2;
	
	for (unsigned int col = 0; col < g_blend_iterations; col++)
	{
		g_blend_dest[col] = (uint8_t) ((*ptr1 + *ptr2) >> 1);	// average fields together
		ptr1++;
		ptr2++;
	}
}
