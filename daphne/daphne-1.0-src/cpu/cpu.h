/*
 * cpu.h
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

#ifndef CPU_H
#define CPU_H

#include <stdint.h>

enum { CPU_UNDEFINED, CPU_Z80, CPU_X86, CPU_M6809, CPU_M6502, CPU_COP421, CPU_I88, CPU_COUNT };	// cpu's supported by daphne now, leave CPU_COUNT at the end

#define CPU_MEM_SIZE	0x100000	// 1 meg for I86
#define MAX_CONTEXT_SIZE	100	/* max # of bytes that a cpu context can have */
#define MAX_IRQS	4	/* how many IRQs we will support per CPU */

struct cpudef;

// structure that defines parameters for each cpu daphne uses
struct cpudef
{
	// these may all be modified externally
	int type;	// which kind of cpu it is
	uint32_t hz;	// how many cycles per second the cpu is to run
	uint32_t initial_pc;	// the initial program counter for the cpu
	bool must_copy_context;	// whether THIS core will be used to emulate two or more cpu's OF THIS SAME TYPE
	double nmi_period;	// how often the NMI ticks (in milliseconds, not seconds)
	double irq_period[MAX_IRQS];	// how often the IRQs tick (in milliseconds, not seconds)
	uint8_t *mem;	// where the cpu's memory begins

	// these should not be modified externally
	uint8_t id;	// which we are adding
	void (*init_callback)();	// callback to initialize the cpu
	void (*shutdown_callback)();	// callback to shutdown the cpu
	void (*setmemory_callback)(uint8_t *);	// the callback to set the location of the RAM/ROM for the CPU core
	uint32_t (*execute_callback)(uint32_t);	// callback to execute cycles for this particular cpu
	uint32_t (*getcontext_callback)(void *);	// callback to get a cpu's context
	void (*setcontext_callback)(void *);	// callback to set a cpu's context
	uint32_t (*getpc_callback)();	// callback to get the program counter
	void (*setpc_callback)(uint32_t);	// callback to set the program counter
	uint32_t (*elapsedcycles_callback)();	// callback to get the # of elapsed cycles
	void (*reset_callback)();	// callback to reset the CPU
	const char *(*ascii_info_callback)(void *context, int regnum);	// callback to get CPU info (for debugging purposes), returns empty string if no info is available
	unsigned int (*dasm_callback)( char *buffer, unsigned pc );	// callback to disassemble code at a specified location

	// how many cycles per interleave (Hz / g_uInterleavePerMs / 1000), rough calculation, pre-calculated for speed
	unsigned int uCyclesPerInterleave;
	unsigned int uNMIMicroPeriod;	// NMI ticks every 'this many' micro seconds (to avoid using floats for gp2x's sake)
	unsigned int uNMITickCount;	// how many NMI's have ticked from the timer (so we know when the next one will take place)
	unsigned int uNMITickBoundaryMs;	// how many ms need to elapse before the next NMI will tick
	unsigned int uIRQMicroPeriod[MAX_IRQS];	// IRQ ticks every 'this many' micro seconds (to avoid using floats for gp2x's sake)
	unsigned int uIRQTickCount[MAX_IRQS];	// same as NMI
	unsigned int uIRQTickBoundaryMs[MAX_IRQS];	// same as NMI
	unsigned pending_nmi_count;	// how many NMI's we have queued up to do
	unsigned int pending_irq_count[MAX_IRQS];	// how many IRQ's we have queued up to do
	uint64_t total_cycles_executed;	// any cycles we've tracked so far
	unsigned int uEventCyclesExecuted;	// how many cycles we've executed since the optional event tracking started
	unsigned int uEventCyclesEnd;	// when event tracking ends and optional event fires (0 if no event)
	void (*event_callback)(void *data);	// callback we call when optional event fires
	void *event_data;	// whatever data we are supposed to pass back to the event callback
	uint8_t context[MAX_CONTEXT_SIZE];	// the cpu's context (in case we were forced to copy it out)
	struct cpudef *next_cpu;	// pointer to the next cpu in this linked list
};

void add_cpu(struct cpudef *);	// add a new cpu
void del_all_cpus();	// delete all cpus that have been added (for shutting down daphne)
void cpu_init();	// initialize one cpu
void cpu_shutdown();	// shutdown all cpus
int cpu_execute(void * in_Data);
void cpu_reset();

// Creates an precisely timed 'event'. After 'uCyclesTilEvent' elapses, event_callback will be called.
// Each even is just a one-shot deal, it doesn't loop.
void cpu_set_event(unsigned int uCpuID, unsigned int uCyclesTilEvent, void (*event_callback)(void *data), void *event_data);

void cpu_pause();
void cpu_unpause();
uint32_t get_cpu_timer();
uint64_t get_total_cycles_executed(uint8_t cpu_id);
struct cpudef * get_cpu_struct(uint8_t cpu_id);
unsigned char cpu_getactivecpu();
uint8_t *get_cpu_mem(uint8_t cpu_id);
uint32_t get_cpu_hz(uint8_t cpu_id);

void cpu_change_nmi(uint8_t cpu_id, double new_period);

// Generates an NMI at the next possible opportunity for the indicated cpu.
// Used when a CPU does not have a regularly timed NMI (such as in multi-cpu situations).
void cpu_generate_nmi(uint8_t cpu_id);

void cpu_change_irq(uint8_t cpu_id, unsigned int which_irq, double new_period);

// Generates an IRQ (indicated by 'which_irq') at the next possible opportunity for the indicated cpu.
// Used when a CPU does not have a regularly timed IRQ (such as in multi-cpu situations).
void cpu_generate_irq(uint8_t cpu_id, unsigned int which_irq);
void cpu_change_interleave(uint32_t);

void generic_6502_init();
void generic_6502_shutdown();
void generic_6502_reset();
void generic_6502_setmemory(uint8_t *buf);
uint32_t generic_6502_getcontext(void *context_buf);
void generic_6502_setcontext(void *context_buf);
const char *generic_6502_info(void *context, int regnum);
uint32_t generic_cpu_elapsedcycles_stub();
const char *generic_ascii_info_stub(void *, int);
unsigned int generic_dasm_stub( char *buffer, unsigned pc );
void reset_cpu_globals();

#endif
