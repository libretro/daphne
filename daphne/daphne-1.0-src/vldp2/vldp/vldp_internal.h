/*
 * vldp_internal.h
 *
 * Copyright (C) 2001 Matt Ownby
 *
 * This file is part of VLDP, a virtual laserdisc player.
 *
 * VLDP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * VLDP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// should only be used by the vldp private thread!

#ifndef VLDP_INTERNAL_H
#define VLDP_INTERNAL_H

#include <stdint.h>

#include "vldp.h"	// for the VLDP_BOOL definition and SDL.h

// this is which version of the .dat file format we are using
#define DAT_VERSION 2

// header for the .DAT files that are generated
struct dat_header
{
	uint8_t version;	// which version of the DAT file this is
	uint8_t finished;	// whether the parse finished parsing or was interrupted
	uint8_t uses_fields;	// whether the stream uses fields or frames
	uint32_t length;	// length of the m2v stream
};

struct precache_entry_s
{
	void *ptrBuf;	// buffer that holds precached file
	unsigned int uLength;	// length (in bytes) of the buffer
	unsigned int uPos;	// our current position within the stream
};

int idle_handler(void *surface);

///////////////////////////////////////
extern uint8_t s_old_req_cmdORcount;	// the last value of the command byte we received
extern int s_paused;	// whether the video is to be paused
extern int s_blanked;	// whether the mpeg video is to be blanked
extern int s_frames_to_skip;	// how many frames to skip before rendering the next frame (used for P and B frames seeking)
extern int s_frames_to_skip_with_inc;	// how many frames to skip while increasing the frame number (for multi-speed playback)
extern int s_skip_all;	// skip all subsequent frames.  Used to bail out of the middle of libmpeg2, back to vldp
extern unsigned int s_uSkipAllCount;	// how many frames we've skipped when s_skip_all is enabled.
extern int s_step_forward;	// if this is set, we step forward 1 frame
extern uint32_t s_timer;	// FPS timer used by the blitting code to run at the right speed
extern uint32_t s_extra_delay_ms;	// any extra delay that null_draw_frame() will use before drawing a frame (intended for laserdisc seek delay simulation)
extern uint32_t s_uFramesShownSinceTimer;	// how many frames should've been rendered (relative to s_timer) before we advance
extern int s_overlay_allocated;	// whether the SDL overlays have been allocated

// Which frame we've skipped to (0 if we haven't skipped)
// Used in order to maintain the current frame number until the skip actually occurs.
extern unsigned int s_uPendingSkipFrame;

extern unsigned int s_skip_per_frame;	// how many frames to skip per frame (for playing at 2X for example)
extern unsigned int s_stall_per_frame;	// how many frames to stall per frame (for playing at 1/2X for example)

#endif
