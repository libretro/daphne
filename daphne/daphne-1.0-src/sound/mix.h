/*
 * mix.h
 *
 * Copyright (C) 2007 Matt Ownby
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

// mix.h

#ifndef MIX_H
#define MIX_H

#include <stdint.h>

struct mix_s
{
	void *pMixBuf;
	struct mix_s *pNext;
};

// TO USE THE MIX FUNCTIONS:
// 1 - set g_pMixBufs to a pointer to a populated mix_s struct (that contains all your streams)
// 2 - set g_pSampleDst to the destination stream
// 3 - set g_uBytesToMix to how many bytes long all lines are (they all must be the same length).
//	IMPORTANT : g_uBytesToMix must be a multiple of 8, and must be >= 8!
// 4 - run g_mix_func() and you're done!

// we always want this function defined for the purpose of testing (releasetest.cpp)
void mix_c();

extern mix_s *g_pMixBufs;
extern uint8_t *g_pSampleDst;
extern uint32_t g_uBytesToMix;
#define g_mix_func mix_c

/////////////////////////////

#endif
