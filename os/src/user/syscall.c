#include "syscall.h"

void _yield() {
        asm volatile("svc #158\n");
}

void _exit(int status) {
        asm volatile(
                "mov r0, %0 \n"
                "svc #0     \n"
                :
                : "r" (status)
                : "r0");
}

pid_t _fork() {
        pid_t r;
        asm volatile(
                "svc #2 \n"
                "mov %0, r0 \n"
                : "=r" (r)
                :
                : "r0"
        );
        return r;
}

int32_t _read(int32_t fd, char *buf, size_t nbytes) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #3     \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fd), "r" (buf), "r" (nbytes)
                : "r0", "r1", "r2");
        return r;
}

char* current_break = 0;
void* _sbrk(int32_t incr) {
        extern char heap_low; // Defined by the linker
        extern char heap_top; // Defined by the linker
        char *prev_break;

        // If this is the first time, set the current break
        // location to the start of the unmapped region
        if (current_break == 0) {
                current_break = &heap_low;
        }

        // Record the location of the current break before
        // we map more of the unallocated space
        prev_break = current_break;

        // If we cannot move the break by the increment without
        // exceeding the available space, return (void*)-1
        if (current_break + incr > &heap_top) {
                return (void*)-1;
        }

        // Else move the break forward and return the position
        // at the start of the increment
        current_break += incr;
        return (void*)prev_break;
 }

size_t _write(int32_t fd, char *buf, size_t nbytes) {
        int r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #4     \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fd), "r" (buf), "r" (nbytes)
                : "r0", "r1", "r2");
        return r;
}

pid_t _getpid() {
        pid_t r;
        asm volatile(
                "svc #20 \n"
                "mov %0, r0 \n"
                : "=r" (r)
                :
                : "r0"
        );
        return r;
}

pid_t _waitpid(procevent_t event, pid_t pid, int32_t options) {
        // This isn't quite the same as the traditional waitpid syscall
        pid_t r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #7     \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (event), "r" (pid), "r" (options)
                : "r0", "r1", "r2");
        return r;
}

void _exec(void (*function)()) {
        // A dead-simple exec syscall that just starts the function passed as a process
        asm volatile(
                "mov r0, %0 \n"
                "svc #1000  \n"
                :
                : "r" (function)
                : "r0");
}
