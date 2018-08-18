// interface for mc6809.c
// by Mark Broadhead
#ifndef _6809INFC_H
#define _6809INFC_H

#include <stdint.h>
#include "mc6809.h"

void m6809_set_memory(uint8_t *);
void initialize_m6809(void);
void m6809_reset(void);

#endif
