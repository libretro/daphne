/*
 * network.cpp
 *
 * Copyright (C) 2002 Matt Ownby
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
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning (disable:4996)
#endif

#include <stdio.h>
#include <stdlib.h>	// for lousy random number generation
#include <sys/types.h>
#include <string.h>

#ifdef MAC_OSX
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/host_priv.h>
#include <mach/machine.h>
#include <carbon/carbon.h>
#endif


#ifdef FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef LINUX
#include <sys/utsname.h>	// MATT : I'm not sure if this is good for UNIX in general so I put it here
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>	// for DNS
#include <sys/time.h>
#include <unistd.h>	// for write
#endif

#include <zlib.h>	// for crc32 calculation
#include "../io/error.h"
#include "../daphne.h"
#include "network.h"

// arbitrary port I've chosen to send incoming data
#define NET_PORT 7733

// ip address to send data to
// I changed this to 'stats' in case I ever want to change the IP address of the stats
// server without changing the location of the web server.
#define NET_IP "stats.daphne-emu.com"

////////////////////

#ifdef WIN32
// some code I found to calculate cpu mhz
_inline unsigned __int64 GetCycleCount(void)
{
#ifdef __GNUC__
   /* TODO/FIXME - very nonportable these days - we need to check if we even need this and if not, just remove this along with querying the MHz of the CPU in general. */
   return 0;
#else
    _asm    _emit 0x0F
    _asm    _emit 0x31
#endif
}
#endif

// gets the cpu's mhz (rounds to nearest 50 MHz)
unsigned int get_cpu_mhz()
{
	unsigned int result = 0;
	unsigned int mod = 0;
#ifdef LINUX
#ifdef NATIVE_CPU_X86
	FILE *F;
	double mhz;
	int iRes = 0;
	const char *s = "cat /proc/cpuinfo | grep MHz | sed -e 's/^.*: //'";
	F = popen(s, "r");
	if (F)
	{
		iRes = fscanf(F, "%lf", &mhz);
		pclose(F);
	}
	result = (unsigned int) mhz;
#endif // NATIVE_CPU_X86
#ifdef NATIVE_CPU_MIPS
	result = 294;	// assume playstation 2 for now :)
#endif // NATIVE_CPU_MIPS
#endif // LINUX

#ifdef FREEBSD
	FILE *F;
	double mhz;
	char command[128]="dmesg | grep CPU | perl -e 'while (<>) {if ($_ =~ /\\D+(\\d+).+-MHz.+/) {print \"$1\n\"}}' > /tmp/result.txt"; 
	system(command);
	F = fopen("/tmp/result.txt", "rb");
	if (F)
	{
		fscanf(F, "%lf", &mhz);
		fclose(F);
		unlink("/tmp/result.txt");
	}
	result = (unsigned int) mhz;
#endif
	

#ifdef WIN32
	unsigned __int64  m_startcycle;
	unsigned __int64  m_res;

	m_startcycle = GetCycleCount();
	Sleep(1000); 
	m_res = GetCycleCount()-m_startcycle;

	result = (unsigned int)(m_res / 1000000);	// convert Hz to MHz
#endif

#ifdef MAC_OSX
	long cpuSpeed = 0;
	Gestalt(gestaltProcClkSpeed, &cpuSpeed);
	result = (unsigned int)(cpuSpeed / 1000000);
	return result;
#endif

	// round to nearest 50 MHz
	result += 25;	// for rounding
	mod = result % 50;
	result -= mod;
	
	return result;
}

// gets the cpu's memory, rounds to nearest 64 megs of RAM
unsigned int get_sys_mem()
{
	unsigned int result = 0;
	unsigned int mod = 0;
	unsigned int mem = 0;
#ifdef LINUX
	FILE *F;
	int iRes = 0;
	const char *s = "ls -l /proc/kcore | awk '{print $5}'";
	F = popen(s, "r");
	if (F)
	{
		iRes = fscanf(F, "%u", &mem);	// this breaks if they have over 2 gigs of ram :)
		pclose(F);
	}

#endif

#ifdef FREEBSD
	size_t len;
	len = sizeof(mem);
	sysctlbyname("hw.physmem", &mem, &len, NULL, NULL);
	
#endif

#ifdef WIN32
	MEMORYSTATUS memstat;
	GlobalMemoryStatus(&memstat);
	mem = memstat.dwTotalPhys;
#endif

	result = (mem / (1024*1024)) + 32;	// for rounding
	mod = result % 64;
	result -= mod;

	return result;
}

char *get_cpu_name()
{
	static char result[NET_LONGSTRSIZE] = { 0 };
	strcpy(result, "UnknownCPU");	// default ...

#ifdef NATIVE_CPU_X86
	unsigned int reg_ebx, reg_ecx, reg_edx;
#ifdef WIN32
	_asm
	{
		xor eax, eax
		cpuid
		mov reg_ebx, ebx
		mov reg_ecx, ecx
		mov reg_edx, edx
	}
#else
    asm
    (
    	"xor %%eax, %%eax\n\t"
    	"cpuid\n\t"
		: "=b" (reg_ebx), "=c" (reg_ecx), "=d" (reg_edx)
		: /* no inputs */
		: "cc", "eax"	/* a is clobbered upon completion */
	);
#endif

	result[0] = (char) ((reg_ebx) & 0xFF);
	result[1] = (char) ((reg_ebx >> 8) & 0xFF);
	result[2] = (char) ((reg_ebx >> 16) & 0xFF);
	result[3] = (char) (reg_ebx >> 24);

	result[4] = (char) ((reg_edx) & 0xFF);
	result[5] = (char) ((reg_edx >> 8) & 0xFF);
	result[6] = (char) ((reg_edx >> 16) & 0xFF);
	result[7] = (char) ((reg_edx >> 24) & 0xFF);

	result[8] = (char) ((reg_ecx) & 0xFF);
	result[9] = (char) ((reg_ecx >> 8) & 0xFF);
	result[10] = (char) ((reg_ecx >> 16) & 0xFF);
	result[11] = (char) ((reg_ecx >> 24) & 0xFF);
#endif // NATIVE_CPU_X86

#ifdef NATIVE_CPU_MIPS
	strcpy(result, "MIPS R5900 V2.0");	// assume playstation 2 for now
#endif	// NATIVE_CPU_MIPS

#ifdef NATIVE_CPU_SPARC
	strcpy(result, "Sparc");
#endif	// NATIVE_CPU_SPARC

//On Mac, we can tell what type of CPU by simply checking the ifdefs thanks to the universal binary.
#ifdef MAC_OSX
#ifdef __PPC__	
	strcpy(result, "PowerPC");
#else
	strcpy(result, "GenuineIntel");
#endif
#endif

	return result;
}
