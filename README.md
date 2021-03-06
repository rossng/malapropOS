# MalapropOS

Adapted from the COMS20001 unit at the University of Bristol.

MalapropOS has a custom libc called `mlibc`.

## Features

### Current

* Communication over UART
* Basic interrupt handling
* Pre-emptive multi-tasking
* Dynamic creation and destruction of process with `fork()` and `exit()`
* An interactive shell, `mμsh`
* Buffered input, support for `Ctrl+C` in the shell
* A scheduler with 'high' and 'low' priority processes
* Basic FAT16 support
* Basic shell utilities: `cd`, `ls`, `cat`, `mkdir`, `pwd`, `rm`, `touch`, `write`
* Asynchronous message-passing IPC


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

## Available system calls

* `void _yield(void)` - deschedule the current process
* `void _exit(procres_t result)` - terminate the current process
* `pid_t _fork(uint32_t fp)` - duplicate the current process and its current stack frame
* `pid_t _getpid(void)` - get the PID of the current process
* `pid_t _waitpid(procevent_t event, pid_t pid, int32_t options)` - wait for an event to be emitted by a specified PID
* `void _exec(proc_ptr function, int32_t argc, char* argv[])` - start executing a different function in this process
* `int32_t _kill(pid_t pid, int32_t sig)` - kill another process
* `int32_t _setpriority(pid_t which, pid_t who, int32_t priority)` - set the priority of a process to high (0) or low (1)
* `int32_t _read(int32_t fd, char* buf, size_t nbytes)` - read some bytes from a file
* `size_t _write(int32_t fd, char* buf, size_t nbytes)` - write some bytes to a file
* `filedesc_t _open(char* pathname, int32_t flags)` - open a file and get its file descriptor
* `int32_t _close(filedesc_t fd)` - close a file by its file descriptor
* `int32_t _unlink(char* pathname)` - delete a file
* `int32_t _lseek(filedesc_t fd, int32_t offset, int32_t whence)` - seek to a position in a file
* `tailq_fat16_dir_head_t* _getdents(filedesc_t fd, int32_t max_num)` - get a list of entries in a directory
* `int32_t _chdir(char* path)` - change the current working directory
* `char* _getcwd(char* buf, size_t nbytes)` - retrieve the path of the current working directory
* `int32_t _mkdir(char* path)` - create a new directory
* `int32_t _stat(char* path, fat16_dir_entry_t* buf)` - get information about a file
* `int32_t _fstat(filedesc_t fd, fat16_dir_entry_t* buf)` - get information about a file

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

### `touch`
Creates a file if it does not already exist. At the moment, only absolute paths are accepted.

Example: `touch /STORY.TXT`

### `write`, `append`
Write the phrase following the filename to the file. At the moment, only absolute paths are accepted. `write` writes from the beginning of the file, `append` from the end.

Example: `write /STORY.TXT Once upon` followed by `append /STORY.TXT a time` will leave the file containing 'Once upon a time'

### `rm`
Remove the specified file. At the moment, only absolute paths are accepted.

Example: `rm /MYFILE1.TXT`

## `writemany`
Write a long example string repeatedly to a file. At the moment, only absolute paths are accepted.

The string "123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789" will be written to the specified file the specfieid number of times.

Example: `writemany /MYFILE1.TXT 15` will write the string 15 times. If `MYFILE1.TXT` currently only occupies one cluster, it will be expanded to fit the text.

### `proc`
Display the list of non-terminated processes.

### `kill`
Terminate the process with the specified PID.

Example: `kill 10`

## IPC

MalapropOS implements asynchronous message passing. A simple demonstration can be seen by running `messenger`. This program forks itself, then the parent sends a message to the queue and the child receives it.

A more complicated demonstration is the 'pipeline'. This consists of three components: a `consumer`, which consumes single-character messages and prints them; a `pipeline`, which consumes single-character messages and sends them on, with the character increments; and a `producer`, which sends single-character messages. These can be attached as so:

```
producer -> pipeline -> ... -> pipeline -> consumer
```

For example:

```
mush> consumer &        // create the consumer in the background
Consumer PID: 2
mush> pipeline 2 &      // attach the first pipeline to the consumer
Pipeline PID: 3
mush> pipeline 3 &      // attach the second pipeline to the consumer
Pipeline PID: 4
mush> producer 4 A      // attach the producer to the second pipeline
Consumer: received C    // output from the consumer
Consumer: received C
```

Here the producer generates the message 'A' repeatedly, and it arrives at the consumer as 'C'.

## Acknowledgements

The implementation of TAILQ is from FreeBSD.

The implementation of `stdarg.h` is from GCC.

Standard type definitions are from http://minirighi.sourceforge.net/html/types_8h-source.html

The `strtol()` implementation in `mlibc` is from Apple XNU.

`stdstring_int_to_str` is based on the K&R implementation, listed at https://en.wikibooks.org/wiki/C_Programming/C_Reference/stdlib.h/itoa

The `stddef.h` errno definitions are from Red Hat, http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
