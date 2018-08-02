#pragma once

// RJS START - config.h is an autoheader created .h from config.h.in.
// Since Android doesn't typically come with the autoheader tool and
// I'm not 100% confident in it, I'll mimic what's need here.

// For alloc.c
#undef HAVE_MEMALIGN

// For cpu_accel.c
#undef ACCEL_DETECT

// For cpu_state.c, idct_altivec.c, idct_mmx.c, motion_comp.c, motion_comp_altivec.c, motion_comp_mmx.c
// For yuv2rgb_mmx.c
#undef ARCH_X86
#undef ARCH_PPC

// For idct.c, idct_alpha.c, idct_mlib.c, motion_comp.c, motion_comp_alpha.c, motion_comp_mlib.c
#undef ARCH_ALPHA
#undef LIBMPEG2_MLIB

// For video_out_dx.c
#undef LIBVO_DX

// RJS NOTE - This should likely be defined . . . guessing.
// For video_out_sdl.c 
#define LIBVO_SDL

// For video_out_x11.c
#undef LIBVO_X11

// For yuv2rgb_mlib.c
#undef LIBVO_MLIB

// For extract_mpeg2.c, mpeg2dec.c
#undef HAVE_IO_H

// RJS END
