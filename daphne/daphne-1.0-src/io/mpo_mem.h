#ifndef MPO_MEM_H
#define MPO_MEM_H

#include <stdint.h>

// mpo_mem.h
// by Matt Ownby

// some memory functions/macros to make memory management more manageable :)

// does C++ style allocation ...
#define MPO_MALLOC(bytes) new unsigned char[bytes]

// only frees memory if it hasn't already been freed
#define MPO_FREE(ptr) if (ptr != 0) { delete [] ptr; ptr = 0; }

////////////////

// Endian-macros

// LOAD_LIL_SINT16: loads little-endian 16-bit value
//  Usage: int16_t val = LOAD_LIL_SINT16(void *ptr);
#ifndef MSB_FIRST
#define LOAD_LIL_SINT16(ptr) (int16_t) *((int16_t *) (ptr))
#else
#define LOAD_LIL_SINT16(ptr) (int16_t) (*((uint8_t *) (ptr))	| ((*((uint8_t *) (ptr)+1)) << 8))
#endif

// LOAD_LIL_UINT32: loads little-endian 32-bit value
//  Usage: uint32_t val = LOAD_LIL_UINT32(void *ptr);
#ifndef MSB_FIRST
#define LOAD_LIL_UINT32(ptr) (uint32_t) *((uint32_t *) (ptr))
#else
#define LOAD_LIL_UINT32(ptr) (uint32_t) (*((uint8_t *) (ptr)) | ((*((uint8_t *) (ptr)+1)) << 8) | ((*((uint8_t *) (ptr)+2)) << 16) | ((*((uint8_t *) (ptr)+3)) << 24))
#endif

// STORE_LIL_UINT32: stores 32-bit unsigned 'val' to 'ptr' in little-endian format
//  Usage: STORE_LIL_UINT32(void *ptr, uint32_t val);
#ifndef MSB_FIRST
#define STORE_LIL_UINT32(ptr,val) *((uint32_t *) (ptr)) = (val)
#else
#define STORE_LIL_UINT32(ptr,val) *((uint8_t *) (ptr)) = (val) & 0xFF; \
	*(((uint8_t *) (ptr))+1) = ((val) >> 8) & 0xFF; \
	*(((uint8_t *) (ptr))+2) = ((val) >> 16) & 0xFF; \
	*(((uint8_t *) (ptr))+3) = ((val) >> 24) & 0xFF
#endif


#endif // MPO_MEM_H
