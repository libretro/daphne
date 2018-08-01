/*
 * cpu-debug.cpp
 *
 * Copyright (C) 2001 Matt Ownby
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

// cpu-debugger
// by Matt Ownby
// designed to interface with MAME cpu cores
// some of the code in here was inspired by and/or taken from Marat Fayzullin's z80 debugger

#ifdef CPU_DEBUG

#include <stdio.h>
#include <ctype.h>
#include "mamewrap.h"
#include "../io/conout.h"
#include "../io/conin.h"
#include "../daphne.h"
#include "../daphne.h"
#include "../game/game.h"
#include "../ldp-out/ldp.h"
#include "../ldp-in/ldv1000.h"
#include "cpu.h"
#include "cpu-debug.h"

unsigned char g_cpu_trace = 0;	// whether we are stopping at each instruction
UINT32	g_breakpoint = 0;	// address to break at
unsigned char g_break = 0;	// whether to break at breakpoint or not
unsigned int g_which_cpu = 0;	// which CPU to debug (since we now support multiple CPU's)

// function used to set the value of g_cpu_trace to avoid extern statements
void set_cpu_trace(unsigned char value)
{
	g_cpu_trace = value;
}

// function used to set which cpu to be debugged
void set_debug_cpu(unsigned int which_cpu)
{
	g_which_cpu = which_cpu;
}

// I called this MAME_Debug so that I could test mame cpu cores with daphne (obviously I can't ship mame cpu cores with
// daphne due to licensing issues)
void MAME_Debug(void)
{	
	// if we are in trace mode OR if we've got our desired breakpoint
	if (g_cpu_trace || (g_break && (get_cpu_struct(g_which_cpu)->getpc_callback() == g_breakpoint)))
	{
		// if the active cpu is the one to be debugged
		if (cpu_getactivecpu() == g_which_cpu)
		{
			// since we may be at the debug prompt for a long time, we pause the cpu timer here
			cpu_pause();
			cpu_unpause();
		}
	}
}

// called by debugger to give us the opportunity to translate a hex address into a label
// (for easier reading)
// Actually, this is a rather brilliant idea, props go to Juergen or whoever thought it up
// 'what' is either 0 or 1, not sure the difference
// size corresponds to enumerations 
// acc (access) also corresponds to enumerations
const char *set_ea_info( int what, unsigned address, int size, int acc )
{
	static char addrstr[160] = { 0 };	// must be static so it isn't de-allocated upon returning
	const char *name = NULL;

	switch(acc)
	{
		case EA_NONE:
		case EA_VALUE:
			sprintf(addrstr, "%x", address);
			break;
		case EA_ABS_PC:
		case EA_MEM_RD:
		case EA_MEM_WR:
			name = g_game->get_address_name (address);
			// if memory location has a name, print that too
			if (name)
			{
				sprintf(addrstr, "%s ($%x)", name, address);
			}
			else
			{
				sprintf(addrstr, "$%x", address);
			}
			break;
		case EA_REL_PC:
			address += size;	// relative offset, address address
			name = g_game->get_address_name (address);

			if (name)
			{
				sprintf(addrstr, "%s ($%x) (PC + %d)", name, address, size);
			}
			else
			{
				sprintf(addrstr, "$%x (PC + %d)", address, size);
			}
			break;
		default:
			sprintf(addrstr, "Unknown: acc %d, size %d, address %u, what %d", acc, size, address, what);
			break;
	}

	return(addrstr);
}

void debug_menu()
{
	newline();
	printline("CPU Debugger Commands");
	printline("---------------------");
	printline("<CR>            : break at next instruction");
	printline("c               : continue without breaking");
	printline("d <addr>        : disassemble at address");
	printline("f               : display current laserdisc frame");
	printline("m <addr>        : memory dump at address");
	printline("n               : next CPU (only for multi-cpu configs)");
	printline("= <addr>        : break at address");
	printline("q               : quit emulator");
	printline("w <addr> <byte> : write byte to address");
	printline("?               : this menu");
}

// print a disassembly starting from addr
void debug_disassemble(unsigned int addr)
{
	char s[160] = { 0 };
	int line = 0;
	const char *name = NULL;

	// print this many lines because that's how many our console can show at a time
	while (line < 13)
	{
		name = g_game->get_address_name(addr); // check to see if this address has a name
		// if so, it's a label name, so print it
		if (name)
		{
			outstr(name);
			printline(":");
			line++;
		}
		sprintf(s, "%04x: ", addr);
		outstr(s);

		addr += get_cpu_struct(g_which_cpu)->dasm_callback(s, addr);

		printline(s);
		line++;
	}

}

// prints cpu registers and flags (keep it on 2 lines)
void print_cpu_context()
{

	char tmpstr[160] = { 0 };
	char nextinstr[160];
	cpudef *cpu = get_cpu_struct(g_which_cpu);
	const char *s = NULL;	// results returned by CPU ascii callback
	int reg = CPU_INFO_REG;	// base register
	int x = 0;	// our X position so we can apply word wrap

	// fill nextinstr with the disassembly of the next instruction
	cpu->dasm_callback(nextinstr, cpu->getpc_callback());

	// print all registers we can
	for (reg = CPU_INFO_REG; reg < MAX_REGS; reg++)
	{
		s = cpu->ascii_info_callback(NULL, reg);

		// if we got something back ...
		if (s[0] != 0)
		{
			outstr(s);
			outstr(" ");	// spacer after register info
			x += strlen(s) + 1;	// +1 because we take into account space

			// if it's time to do a line feed ...
			if (x > 76)
			{
				newline();
				x = 0;
			}
		}
	};
	newline();

	sprintf(tmpstr,
	    "AT PC: [%02X - %s]   FLAGS: [%s] ",
		cpu->getpc_callback(),
		nextinstr,
		cpu->ascii_info_callback(NULL, CPU_INFO_FLAGS)
	);
	printline(tmpstr);
}

// print a memory dump (duh)
void print_memory_dump(unsigned int addr)
{
	int i = 0, j = 0;	// temporary indices
	char tmpstr[160] = { 0 };
	const char lines = 13;	// # of lines in the memory dump

	newline();

	for(j=0 ; j < lines ; j++)
	{
		sprintf(tmpstr, "%04X: ", addr);
		outstr(tmpstr);

		for(i=0; i < 16; i++,addr++)
		{
			sprintf(tmpstr, "%02X ", g_game->cpu_mem_read(addr));
			outstr(tmpstr);
		}

		outstr(" | ");

		addr-=16;
		for(i=0;i < 16;i++,addr++)
		{
			char ch = g_game->cpu_mem_read(addr);
			outchr(isprint(ch) ? ch :'.');
		}

		newline();
	}
}

#endif
// end #ifdef CPU_DEBUG
