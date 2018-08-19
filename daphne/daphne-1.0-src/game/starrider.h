/*
 * starrider.h
 *
 * Copyright (C) 2001 Mark Broadhead
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

// starrider.h
// by Mark Broadhead

#include <stdint.h>

#include "game.h"

#define STARRIDER_OVERLAY_W 320	// width of overlay
#define STARRIDER_OVERLAY_H 256 // height of overlay

class starrider : public game
{
public:
	starrider();
	void do_nmi();		// does an NMI tick
	void do_irq();		// does an IRQ tick
	void do_firq();		// does a FIRQ tick
	uint8_t cpu_mem_read(uint16_t addr);			// memory read routine
	void cpu_mem_write(uint16_t addr, uint8_t value);		// memory write routine
	void input_enable(uint8_t);
	void input_disable(uint8_t);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();
	void video_repaint();	// function to repaint video
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);

private:
	int current_bank;
	bool firq_on;
	bool irq_on;
	bool nmi_on;
	uint8_t character[0x2000];		
	uint8_t rombank1[0xa000];		
	uint8_t rombank2[0x4000];		
//	SDL_Color colors[16];	// color palette
	uint8_t banks[3];				// starrider's banks
		// bank 1 is switches
		// bank 2 is dip switch 1
		// bank 3 is dip switch 2
};

