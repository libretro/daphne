/*
 * mach3.h
 *
 * Copyright (C) 2001 Warren Ondras
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

// mach3.h

#ifndef MACH3_H
#define MACH3_H

#include <stdint.h>

#include "game.h"

#include <queue>	// for testing, can be replaced with array later

using namespace std;

#define MACH3_CPU_HZ 20000000		// speed of cpu (5 MHz)

#define MACH3_OVERLAY_W 256	// width of overlay
#define MACH3_OVERLAY_H 240 // height of overlay

#define MACH3_COLOR_COUNT 16	// # of colors in palette
#define MACH3_GAMMA 1.0 // brightness adjustment 

#define MACH3_SEARCH_PERIOD 60  // # of display frames of simulated video signal loss during disc searches 
								// required for game hardware to detect acceptance of search commands
								// Adjust as needed.  60 = 1 second, which is probably a little fast compared
								// to a real PR-8210 player, but it's plenty long!

class mach3 : public game
{
public:
	mach3();
   void do_irq(unsigned int);		// does an IRQ tick
	void do_nmi();
	uint8_t cpu_mem_read(uint32_t addr);
	void cpu_mem_write(uint32_t addr, uint8_t value);
	uint8_t cpu_mem_read(uint16_t addr);
	void cpu_mem_write(uint16_t addr, uint8_t value);
	uint8_t port_read(uint16_t);
	void port_write(uint16_t, uint8_t);
	void input_enable(uint8_t);
	void input_disable(uint8_t);
   unsigned get_libretro_button_map(unsigned id);
   const char *get_libretro_button_name(unsigned id);
	bool set_bank(unsigned char, unsigned char);
//	void set_version(int);
//	bool handle_cmdline_arg(const char *arg);
	void patch_roms();
	uint8_t character[0x2000];  //character gfx ROM (8KB)
	uint8_t sprite[0x10000];  //sprite gfx ROM (64KB for UVT, 32KB for MACH3)
   uint8_t m_cpumem2[0x10000]; // memory space for first 6502
   uint8_t m_cpumem3[0x10000]; // memory space for second 6502
	uint8_t targetdata[0x100000]; 
	uint32_t m_current_targetdata; //pointer to active buffer within target data

protected:
	uint8_t m_gamecontrols;
	uint8_t m_serviceswitches;
	uint8_t m_dipswitches;

	bool m_ldvideo_enabled;	// whether laserdisc video is visible or not

	bool m_palette_updated;		// whether our color ram has been written to
	int m_signal_loss_counter;	// to fool the ROM into thinking that the PR-8210 is losing signal during seeks

	void palette_calculate();
	void video_repaint();	// function to repaint video
	
	// these are separated into methods because the order/priority can change:
	void draw_characters();  
	void draw_sprites();

	void draw_8x8(uint8_t character_number, uint8_t *character_set, uint8_t xcoord, uint8_t ycoord);
	void draw_16x16(uint8_t character_number, uint8_t *character_set, uint8_t xcoord, uint8_t ycoord);

	uint8_t m_frame_decoder_select_bit;
	uint8_t m_audio_ready_bit;
	uint16_t m_targetdata_offset;

   uint8_t m_soundchip1_id;
   uint8_t m_soundctrl1;
   uint8_t m_soundchip2_id;
   uint8_t m_soundctrl2;

   uint8_t m_dac_id;
   uint64_t m_dac_last_cycs;
   uint8_t m_dac_last_val;

   bool m_soundchip2_nmi_enabled;
   unsigned int m_last0x4000;
   queue <uint8_t> m_sounddata_latch1;
   queue <uint8_t> m_sounddata_latch2;
   uint8_t m_psg_latch;

private:

};

// Us Vs Them
class uvt : public mach3
{
public:
	uvt();
//	void set_version(int);
};

// Cobra Command on MACH3 hardware
class cobram3 : public mach3
{
public:
	cobram3();
	void patch_roms();
};

#endif
