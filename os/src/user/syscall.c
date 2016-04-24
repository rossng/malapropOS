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

pid_t _fork(uint32_t fp) {
        pid_t r;
        asm volatile(
                "mov r0, sp \n"
                "mov r1, %1 \n"
                "svc #2 \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fp)
                : "r0", "r1"
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

void _exec(proc_ptr function, int32_t argc, char* argv[]) {
        // A dead-simple exec syscall that just starts the function passed as a process
        asm volatile(
                "mov r0, %0 \n"
                "mov r1, %1 \n"
                "mov r2, %2 \n"
                "svc #1000  \n"
                :
                : "r" (function), "r" (argc), "r" (argv)
                : "r0", "r1", "r2");
}

int32_t _kill(pid_t pid, int32_t sig) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #37    \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (pid), "r" (sig)
                : "r0", "r1");
        return r;
}

int32_t _setpriority(pid_t which, pid_t who, int32_t priority) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #97    \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (which), "r" (who), "r" (priority)
                : "r0", "r1", "r2");
        return r;
}

filedesc_t _open(char* pathname, int32_t flags) {
        filedesc_t r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #5     \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (pathname), "r" (flags)
                : "r0", "r1");
        return r;
}

int32_t _close(filedesc_t fd) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "svc #6     \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fd)
                : "r0");
        return r;
}

int32_t _unlink(char* pathname) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "svc #10    \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (pathname)
                : "r0");
        return r;
}

tailq_fat16_dir_head_t* _getdents(filedesc_t fd, int32_t max_num) {
        tailq_fat16_dir_head_t* r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #141     \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fd), "r" (max_num)
                : "r0", "r1");
        return r;
}

int32_t _chdir(char* path) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "svc #12    \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (path)
                : "r0", "r1");
        return r;
}

char* _getcwd(char* buf, size_t nbytes) {
        char* r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #183   \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (buf), "r" (nbytes)
                : "r0", "r1");
        return r;
}

int32_t _stat(char* path, fat16_dir_entry_t* buf) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #18    \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (path), "r" (buf)
                : "r0", "r1");
        return r;
}

int32_t _fstat(filedesc_t fd, fat16_dir_entry_t* buf) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "mov r1, %2 \n"
                "svc #28    \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (fd), "r" (buf)
                : "r0", "r1");
        return r;
}

int32_t _mkdir(char* path) {
        int32_t r;
        asm volatile(
                "mov r0, %1 \n"
                "svc #39    \n"
                "mov %0, r0 \n"
                : "=r" (r)
                : "r" (path)
                : "r0", "r1");
        return r;
}
