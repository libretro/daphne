/*
 * mach3.h
 *
 * Copyright (C) 2005 Mark Broadhead
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

// timetrav.h

#ifndef TIMETRAV_H
#define TIMETRAV_H

#include <stdint.h>
#include "game.h"

#define TIMETRAV_CPU_HZ 5000000		// speed of cpu (5 MHz ?)

class timetrav : public game
{
public:
	timetrav();
	void do_nmi();
	uint8_t cpu_mem_read(uint32_t addr);
	void cpu_mem_write(uint32_t addr, uint8_t value);
	uint8_t port_read(uint16_t);
	void port_write(uint16_t, uint8_t);
	void input_enable(uint8_t);
	void input_disable(uint8_t);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);

protected:
};

#endif
