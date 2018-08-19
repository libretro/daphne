/*
 * badlands.h
 *
 * Copyright (C) 2001-2005 Mark Broadhead
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

// badlands.h
// by Mark Broadhead

#include <stdint.h>
#include "game.h"

#define BADLANDS_CPU_HZ 14318180

#define BADLANDS_OVERLAY_W 320	// width of overlay
#define BADLANDS_OVERLAY_H 256 // height of overlay

#define BADLANDS_COLOR_COUNT 16

#define BADLANDS_GAMMA 1.0	// don't adjust colors 

enum
{
	S_BL_SHOT
};

class badlands : public game
{
public:
	badlands();
	void do_nmi();		// does an NMI tick
	void do_irq(unsigned int);		// does an IRQ/FIRQ tick
	uint8_t cpu_mem_read(uint16_t addr);			// memory read routine
	void cpu_mem_write(uint16_t addr, uint8_t value);		// memory write routine
	void reset();
	void set_preset(int);
	void input_enable(uint8_t);
	void input_disable(uint8_t);
	bool set_bank(unsigned char, unsigned char);
	void palette_calculate();
	void video_repaint();	// function to repaint video
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);

protected:
	void update_shoot_led(uint8_t);
	uint8_t charx_offset;
	uint8_t chary_offset;
	uint16_t char_base;
	uint8_t m_soundchip_id;
   bool shoot_led_overlay;
	bool shoot_led_numlock;
	bool shoot_led;
	bool firq_on;
	bool irq_on;
	bool nmi_on;
	uint8_t character[0x2000];		
	uint8_t color_prom[0x20];
	uint8_t banks[3];				// badlands's banks
		// bank 1 is switches
		// bank 2 is dip switch 1
		// bank 3 is dip switch 2
};

class badlandp : public badlands
{
public:
   badlandp();
	uint8_t cpu_mem_read(uint16_t addr);			// memory read routine
	void cpu_mem_write(uint16_t addr, uint8_t value);		// memory write routine
};

