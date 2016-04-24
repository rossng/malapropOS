# MalapropOS

Adapted from the COMS20001 unit at the University of Bristol.

MalapropOS has a custom libc called `mlibc`. The implementation of TAILQ is from FreeBSD. The implementation of `stdarg.h` is from GCC.

## Features

### Current

* Communication over UART
* Basic interrupt handling
* Pre-emptive multi-tasking with a fixed process list and fixed timeslices
* Dynamic creation and destruction of process with `fork()` and `exit()`
* A very basic interactive shell, `mμsh`
* Buffered input, support for `Ctrl+C` in the shell
* A priority-based scheduling algorithm
* Basic FAT16 support

### In progress

* Basic shell utilities: cd, ls, cat

### Planned

* Message-passing IPC

## Supported platforms

* RealView Platform Cortex-A8 (tested in QEMU)

## Build

* `vagrant up`
* `vagrant ssh`
* `cd /vagrant/mlibc`
* `make build`
* `cd /vagrant/os`
* `make build`

## Run

* `make launch-qemu` launches malapropOS in QEMU, paused before execution
* `make launch-gdb` launches a GDB session connected to QEMU. Use `c` to continue execution.
* `make init-disk` initialises the disk, `make launch-disk` launches the disk emulator (which communicates with the OS over UART)
* `Ctrl+C q Return y Return` will quit GDB
* `make kill-qemu` will kill QEMU
