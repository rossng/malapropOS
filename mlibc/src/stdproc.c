#include <stdproc.h>
#include <syscall.h>

void stdproc_exit(procres_t result) {
        _exit(result);
}

pid_t stdproc_fork(void) {
        // TODO: implement fork
        return _fork();
}
