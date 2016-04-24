#include <stdproc.h>
#include <syscall.h>

void stdproc_exit(procres_t result) {
        _exit(result);
}

pid_t stdproc_fork() {
        uint32_t fp;
        asm volatile(
                "mov %0, r11 \n"
                : "=r" (fp)
                :
                : "r0"
        );
        return _fork(fp);
}

pid_t stdproc_getpid(void) {
        return _getpid();
}
