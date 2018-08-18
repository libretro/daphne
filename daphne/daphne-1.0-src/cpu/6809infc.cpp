// interface for mc6809.c
// by Mark Broadhead

#include <stdint.h>
#include <string.h>
#include "mc6809.h"
#include "6809infc.h"
#include "cpu.h"
#include "../game/game.h"

#ifdef WIN32
#pragma warning (disable:4244)	// disable the warning about possible loss of data
#pragma warning (disable:4100) // disable warning about unreferenced parameter
#endif

Uint8 *g_cpumem = NULL;	// where this cpu's memory begins

// MATT : sets where the memory begins for the cpu core
void m6809_set_memory(Uint8 *cpumem)
{
	g_cpumem = cpumem;
}

static void FetchInstr(INT_MC addr, UCHAR_MC fetch_buffer[])
{
	memcpy(fetch_buffer, &g_cpumem[addr], MC6809_FETCH_BUFFER_SIZE);
}

static INT_MC LoadByte(INT_MC addr)
{
	return (g_game->cpu_mem_read(static_cast<uint16_t>(addr)) & 0xff);
}

static INT_MC LoadWord(INT_MC addr)
{
	UCHAR_MC high_byte = (g_game->cpu_mem_read(static_cast<uint16_t>(addr)) & 0xff);
	UCHAR_MC low_byte = (g_game->cpu_mem_read(static_cast<uint16_t>(addr + 1)) & 0xff);

	return ((high_byte << 8) | low_byte);
}

static void StoreByte(INT_MC addr, INT_MC value)
{
	g_game->cpu_mem_write(static_cast<uint16_t>(addr & 0xffff), (value & 0xff));
}

static void StoreWord(INT_MC addr, INT_MC value)
{
	g_game->cpu_mem_write(static_cast<uint16_t>(addr & 0xffff), ((value >> 8) & 0xff));
	g_game->cpu_mem_write(static_cast<uint16_t>((addr + 1) & 0xffff), (value & 0xff));
}

// I don't know if we'll need this...
static INT_MC BiosCall(struct MC6809_REGS *regs)
{
	return 0x12;  /* NOP */
}

void initialize_m6809(void)
{
	MC6809_INTERFACE interface = {
		FetchInstr,
		LoadByte,
		LoadWord,
		StoreByte,
		StoreWord,
		BiosCall };
	mc6809_Init(&interface);
	mc6809_Reset();
}

void m6809_reset(void)
{
	mc6809_Reset();
}

