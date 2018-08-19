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
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS 1
#pragma warning (disable:4996)
#endif

#include <stdio.h>
#include <stdlib.h>	// for lousy random number generation
#include <sys/types.h>
#include <string.h>

#ifdef __APPLE__
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/host_priv.h>
#include <mach/machine.h>
#endif


#ifdef __unix__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef __linux__
#include <sys/utsname.h>	// MATT : I'm not sure if this is good for UNIX in general so I put it here
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>	// for write
#endif

#include <zlib.h>	// for crc32 calculation
#include "../io/error.h"
#include "../daphne.h"
#include "network.h"

////////////////////

// gets the cpu's memory, rounds to nearest 64 megs of RAM
unsigned int get_sys_mem()
{
	unsigned int result = 0;
	unsigned int mod = 0;
	unsigned int mem = 0;
#ifdef __linux__
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

#if defined(__unix__) || defined(__APPLE__)
	size_t len = sizeof(mem);
	sysctlbyname("hw.physmem", &mem, &len, NULL, NULL);
#endif

#ifdef _WIN32
	MEMORYSTATUS memstat;
	GlobalMemoryStatus(&memstat);
	mem = memstat.dwTotalPhys;
#endif

	result = (mem / (1024*1024)) + 32;	// for rounding
	mod = result % 64;
	result -= mod;

	return result;
}
