#ifndef STDMEM_H_
#define STDMEM_H_

#include <stdtypes.h>

void* stdmem_allocate(size_t size);

void* stdmem_copy(void* destination, const void* source, size_t numbytes);

void stdmem_free(void* ptr);

#endif
