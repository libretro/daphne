/*
 * bega.h
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

// badlands.h
// by Mark Broadhead

#include <stdint.h>
#include "game.h"

#define BEGA_OVERLAY_W 256	// width of overlay
#define BEGA_OVERLAY_H 256 // height of overlay

// which color index will be transparent
   // wdo 2005-04-11:  Bega's, Road Blaster, and Cobra Command all set every 8th color to black (7,15,23, etc), so it's
   // not clear which is the correct transparent color.  (It is not 0, because this causes Road Blaster to mask the mpeg
   // video, and it is not 63, since Bega's uses this for the black shadows on text.
   // Colors 0-7 aren't set by Road Blaster at all.)  Color 7 is probably a safe choice, and appears
   // to be unused by any of the games for visible black.
   //
   // wdo 2005-4-13:  color 7 doesn't work on linux unless the -nosound parameter is used (????)
   // so we will use 15 instead.
#define BEGA_TRANSPARENT_COLOR 15

#define BEGA_COLOR_COUNT 0x38
#define BEGA_CPU_HZ 15000000 // unverified

class bega : public game
{
public:
	bega();
	void do_nmi();		// does an NMI tick
	void do_irq(unsigned int);		// does an IRQ tick
	uint8_t cpu_mem_read(uint16_t addr);			// memory read routine
	void cpu_mem_write(uint16_t addr, uint8_t value);		// memory write routine
	void input_enable(uint8_t);
	void input_disable(uint8_t);
	void palette_calculate();
	void video_repaint();	// function to repaint video
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);
	void set_version(int);
	bool set_bank(unsigned char which_bank, unsigned char value);

protected:
   uint8_t m_soundchip1_id;   
   uint8_t m_soundchip2_id;
   uint8_t m_soundchip1_address_latch;
   uint8_t m_soundchip2_address_latch;
   uint8_t m_sounddata_latch;
   void draw_8x8(int, uint8_t *, int, int, int, int, int);
	void draw_16x16(int, uint8_t *, int, int, int, int, int);
	void draw_sprites(int, uint8_t *);
	void write_m6850_control(uint8_t);
	uint8_t read_m6850_status();
	void write_m6850_data(uint8_t);
	uint8_t read_m6850_data();
	uint8_t ldp_status;
	bool vblank;
	void recalc_palette();
	uint8_t m_cpumem2[0x10000];
   uint8_t mc6850_status;
	uint8_t character1[0x6000];		
	uint8_t character2[0x6000];		
	uint8_t banks[3];				// bega's banks
		// bank 1 is switches
		// bank 2 is dip switch 1
		// bank 3 is dip switch 2
};

class cobra : public bega
{
public:
	cobra();
	void set_version(int);
};

// Roadblaster, using Bega's Battle PCB
class roadblaster : public bega
{
public:
	roadblaster();
	void patch_roms();
};
