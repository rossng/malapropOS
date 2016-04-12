#include <stdmem.h>
#include <stddef.h>
#include <syscall.h>

void* stdmem_allocate(size_t size) {
        // A very simple memory allocator
        void *p = _sbrk(0);
        if (_sbrk(size) == (void*)-1) {
                return NULL;
        } else {
                return p;
        }
}

void* stdmem_copy(void* destination, const void* source, size_t numbytes) {
        char* src = (char*)src;
        char* dest = (char*)dest;

        for (int i = 0 ; i < numbytes ; i++) {
                dest[i] = src[i];
        }

        return destination;
}

void stdmem_free(void* ptr) {
        // At the moment, just don't bother
        return;
}
