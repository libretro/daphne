/*
 * laireuro.h
 *
 * Copyright (C) 2001-2007 Mark Broadhead
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

#ifndef LAIREURO_H
#define LAIREURO_H

#include <stdint.h>
#include "game.h"

#define LAIREURO_CPU_HZ	3579545	// speed of cpu - from schematics

#define LAIREURO_OVERLAY_W 360	// width of overlay
#define LAIREURO_OVERLAY_H 288 // height of overlay
#define LAIREURO_COLOR_COUNT 8 // 8 colors total

// we need our own callback since laireuro uses IM2
int32_t laireuro_irq_callback(int);

// CTC Stuff
void ctc_init(double, double, double, double, double);
void ctc_write(uint8_t, uint8_t);
uint8_t ctc_read(uint8_t);
void dart_write(bool b, bool command, uint8_t data);
void ctc_update_period(uint8_t channel);

#define COUNTER true
#define TIMER false

struct ctc_channel
{
	double trig; // period of the trigger input
	uint8_t control;
	uint8_t time_const;
	bool load_const;
	bool time;
	bool time_trig;
	bool clk_trig_section;
	uint16_t prescaler;
	bool mode;
	bool interrupt;
};

struct ctc_chip
{
	uint8_t int_vector;
	ctc_channel channels[4];
	double clock; // period of the clock
};

struct dart_chip
{
	uint8_t next_reg;
	uint8_t int_vector;
	bool transmit_int;
	bool ext_int;
};


class laireuro : public game
{
public:
	laireuro();
	void do_irq(Uint32);
	void do_nmi();
	uint8_t cpu_mem_read(uint16_t);
	void cpu_mem_write(uint16_t, uint8_t);
	uint8_t port_read(uint16_t);
	void port_write(uint16_t, uint8_t);
	void input_enable(uint8_t);
	void input_disable(uint8_t);
	void palette_calculate();
	void video_repaint();
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);
	void set_version(int);
	bool set_bank(uint8_t, uint8_t);

protected:
	uint8_t m_wt_misc;
	uint8_t m_character[0x2000];	
	SDL_Color m_colors[LAIREURO_COLOR_COUNT];		
	uint8_t m_banks[4];				
};

class aceeuro : public laireuro
{
public:
	aceeuro();
};

#endif
