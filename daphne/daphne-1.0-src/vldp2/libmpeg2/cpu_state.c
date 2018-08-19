/*
 * cpu_state.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mpeg2dec; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdlib.h>
#include <inttypes.h>

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"
#if defined(ARCH_X86) || defined(ARCH_X86_64)
#include "mmx.h"
#endif

void (* mpeg2_cpu_state_save) (cpu_state_t * state) = NULL;
void (* mpeg2_cpu_state_restore) (cpu_state_t * state) = NULL;

#if defined(ARCH_X86) || defined(ARCH_X86_64)
static void state_restore_mmx (cpu_state_t * state)
{
    emms ();
}
#endif

void mpeg2_cpu_state_init (uint32_t accel)
{
#if defined(ARCH_X86) || defined(ARCH_X86_64)
    if (accel & MPEG2_ACCEL_X86_MMX) {
	mpeg2_cpu_state_restore = state_restore_mmx;
    }
#endif
}
