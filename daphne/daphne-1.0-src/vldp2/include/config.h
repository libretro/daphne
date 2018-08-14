#pragma once

// RJS START - config.h is an autoheader created .h from config.h.in.
// Since Android doesn't typically come with the autoheader tool and
// I'm not 100% confident in it, I'll mimic what's need here.

// For alloc.c
#undef HAVE_MEMALIGN

// For cpu_accel.c
#undef ACCEL_DETECT

// For extract_mpeg2.c, mpeg2dec.c
#undef HAVE_IO_H

// RJS END
