# MalapropOS

Adapted from the COMS20001 unit at the University of Bristol.

Compiles against Red Hat Newlib instead of GCC libc.

## Features

### Current

* Communication over UART
* Basic interrupt handling
* Pre-emptive multi-tasking with a fixed process list and fixed timeslices

### Planned

* Dynamic creation and destruction of process with `fork()` and `exit()`
* A better scheduling algorithm
* IPC
* A simple file system

## Supported platforms

* RealView Platform Cortex-A8 (tested in QEMU)
