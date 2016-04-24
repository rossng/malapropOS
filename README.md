# MalapropOS

Adapted from the COMS20001 unit at the University of Bristol.

MalapropOS has a custom libc called `mlibc`.

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
* Basic shell utilities: `cd`, `ls`, `cat`, `mkdir`, `pwd`

### In progress

* Message-passing IPC

### Planned


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

## Available syscalls

## Shell commands

The shell runs as a user process, PID 1. You can type commands at the prompt and press return to run the job described by that command. Backspace is supported.

By default, jobs run in the foreground in low priority. To run a process in the background, add `&` to the end of the command. To run a process in high-priority mode, add `!` to the end of the command. In high priority mode, all high-priority tasks will complete before any low-priority task resumes execution.

While a foreground process is executing, you can terminate it by pressing `Ctrl+C`. When the shell process is rescheduled, it will terminate the schedule. Try running `P0` and press `Ctrl+C` while it is running.

To see high-priority mode in action, try running `P0 !` and pressing `Ctrl+C`. Because the shell is low-priority, execution of `P0` will not be terminated and will complete normally.

### `P0` and `P1`
Print out a series of ascending numbers and then exits. For an example of concurrent process execution, run `P0 &` followed by `P1 &` too see their interleaved output.

### `pwd`
Print the current working directory. This is set to `/` on startup.

### `ls`
List the contents of the current directory.

### `cd`
Change the current working directory. At the moment, only absolute paths are accepted.

Example: `cd /SUBDIR`

### `mkdir`
Create a new directory. At the moment, only absolute paths are accepted.

Example: `mkdir /SUBDIR/NEWDIR`

### `cat`
Display the contents of a file. At the moment, only absolute paths are accepted.

Example: `cat /MYFILE1.TXT`

## Acknowledgements

The implementation of TAILQ is from FreeBSD. The implementation of `stdarg.h` is from GCC.
