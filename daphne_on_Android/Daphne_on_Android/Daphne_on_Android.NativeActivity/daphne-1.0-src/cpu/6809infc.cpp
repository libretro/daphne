// interface for mc6809.c
// by Mark Broadhead

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

// RJS TEMP
#define PRINTEM 0

#if (PRINTEM)
#include "../io/conout.h"
static char s[81] = { 0 };
#endif

						// MATT : sets where the memory begins for the cpu core
void m6809_set_memory(Uint8 *cpumem)
{
	g_cpumem = cpumem;
#if (PRINTEM)
	sprintf(s, "SM: %d", (int)g_cpumem);
	printline(s);
#endif
}

static void FetchInstr(INT_MC addr, UCHAR_MC fetch_buffer[])
{
	memcpy(fetch_buffer, &g_cpumem[addr], MC6809_FETCH_BUFFER_SIZE);

#if (PRINTEM)
	sprintf(s, "FI: %d %d %d %d %d  Addr: %d", *(fetch_buffer + 0), *(fetch_buffer + 1), *(fetch_buffer + 2), *(fetch_buffer + 3), *(fetch_buffer + 4), addr);
	printline(s);
#endif
}

static INT_MC LoadByte(INT_MC addr)
{
#if (PRINTEM)
	Uint8 rc = g_game->cpu_mem_read(static_cast<Uint16>(addr)) & 0xff;

	sprintf(s, "LB: %d  Addr: %d  AddrANDff: %d", rc, static_cast<Uint16>(addr), static_cast<Uint16>(addr) & 0xff);
	printline(s);

	return (rc);
#else
	return (g_game->cpu_mem_read(static_cast<Uint16>(addr)) & 0xff);
#endif
}

static INT_MC LoadWord(INT_MC addr)
{
	UCHAR_MC high_byte = (g_game->cpu_mem_read(static_cast<Uint16>(addr)) & 0xff);
	UCHAR_MC low_byte = (g_game->cpu_mem_read(static_cast<Uint16>(addr + 1)) & 0xff);

#if (PRINTEM)
	sprintf(s, "LW: hb: %d  lb: %d  combine: %d  Addr: %d  AddrANDff: %d", high_byte, low_byte, (high_byte << 8) | low_byte, static_cast<Uint16>(addr), static_cast<Uint16>(addr) & 0xff);
	printline(s);
#endif

	return ((high_byte << 8) | low_byte);
}

static void StoreByte(INT_MC addr, INT_MC value)
{
	g_game->cpu_mem_write(static_cast<Uint16>(addr & 0xffff), (value & 0xff));

#if (PRINTEM)
	sprintf(s, "SB: %d  ValueANDff: %d  Addr: %d  AddrANDffff: %d", value, (value & 0xff), static_cast<Uint16>(addr), static_cast<Uint16>(addr & 0xffff));
	printline(s);
#endif
}

static void StoreWord(INT_MC addr, INT_MC value)
{
	g_game->cpu_mem_write(static_cast<Uint16>(addr & 0xffff), ((value >> 8) & 0xff));
	g_game->cpu_mem_write(static_cast<Uint16>((addr + 1) & 0xffff), (value & 0xff));

#if (PRINTEM)
	sprintf(s, "SW: hb: %d  lb: %d  Addr: %d  AddrANDffff: %d", ((value >> 8) & 0xff), (value & 0xff), static_cast<Uint16>(addr), static_cast<Uint16>(addr & 0xffff));
	printline(s);
#endif
}

// I don't know if we'll need this...
static INT_MC BiosCall(struct MC6809_REGS *regs)
{
#if (PRINTEM)
	sprintf(s, "BIOSCall: 0x12");
	printline(s);
#endif

	return 0x12;  /* NOP */
}

void initialize_m6809(void)
{
#if (PRINTEM)
	sprintf(s, "initialize_m6809: Now");
	printline(s);
#endif

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
#if (PRINTEM)
	sprintf(s, "m6809_reset: Now");
	printline(s);
#endif

	mc6809_Reset();
}

