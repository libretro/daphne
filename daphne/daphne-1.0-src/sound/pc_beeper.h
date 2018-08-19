/*
 * pc_beeper.h
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

#ifndef PC_BEEPER_H
#define PC_BEEPER_H

#include <stdint.h>

// init callback
int beeper_init(uint32_t unused);

// should be called from the game driver to control beeper
void beeper_ctrl_data(unsigned int uPort, unsigned int uByte, int internal_id);

// called from sound mixer to get audio stream
void beeper_get_stream(uint8_t *stream, int length, int internal_id);

#endif // PC_BEEPER_H
