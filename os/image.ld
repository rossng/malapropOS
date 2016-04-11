ENTRY(handler_rst)
SECTIONS
{
 . = 0x70010000;
 .text    : { src/startup.o(.text) *(.text .rodata) }
 .data    : { *(.data) }
 .bss     : { *(.bss COMMON) }
 . = ALIGN(8);

 /* Allocate 4KB each for the SVC and IRQ mode stacks */
 . += 0x1000;
 stack_top_svc = .;
 . += 0x1000;
 stack_top_irq = .;

 /* 64KB of heap, for the _sbrk syscall */
 heap_low = .;
 . += 0x10000;
 heap_top = .;

 /* 64KB of stack space for user processes, to be allocated by the kernel */
 . += 0x10000;
 stack_top_usr = .;
}
