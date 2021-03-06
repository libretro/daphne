/*
 * superd.h
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


// superd.h
// by Mark Broadhead

#ifndef SUPERD_H
#define SUPERD_H

#include <stdint.h>

#include "game.h"

#define SUPERD_CPU_HZ	5000000	// speed of cpu
#define SUPERD_IRQ_PERIOD (1000.0/60.0)	// # of milliseconds per frame
					// based on the strobe signal from the ldv-1000

#define SUPERD_OVERLAY_W 256	// width of overlay
#define SUPERD_OVERLAY_H 256 // height of overlay
#define SUPERD_COLOR_COUNT 32

enum { S_SD_COIN, S_SD_SUCCEED, S_SD_FAIL, S_SDA_SUCCESS_LO, S_SDA_SUCCESS_HI };

class superd : public game
{
public:
	superd();
	bool init();
	void do_irq(unsigned int);
	void cpu_mem_write(uint16_t, uint8_t);
	uint8_t port_read(uint16_t);
	void port_write(uint16_t, uint8_t);
	void input_enable(uint8_t);
	void input_disable(uint8_t);
	void OnVblank();
	void OnLDV1000LineChange(bool bIsStatus, bool bIsEnabled);
	bool set_bank(unsigned char, unsigned char);
	void video_repaint();
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);
	void palette_calculate();
	
protected:
    uint8_t m_soundchip_id;
    uint8_t ldp_output_latch;	// holds data to be sent to the LDV1000
	uint8_t ldp_input_latch;	// holds data that was retrieved from the LDV1000
	uint8_t character[0x2000];	// character ram
	uint8_t color_prom[0x20];
	uint8_t banks[4];				// superdon's banks
		// bank 1 is joystick
		// bank 2 is buttons
		// bank 3 is dip switch 1
		// bank 4 is dip switch 2
};

class sdqshort : public superd
{
public:
	sdqshort();
};

class sdqshortalt : public superd
{
public:
	sdqshortalt();
};

#endif
