ENTRY(handler_rst)
SECTIONS
{
 . = 0x00010000;
 .text    : { src/startup.o(.text) *(.text .rodata) }
 .data    : { *(.data) }
 .bss     : { *(.bss COMMON) }
 . = ALIGN(8);
 /* Allocate 4KB for the SVC and IRQ mode stacks */
 . += 0x1000;
 stack_top_svc = .;
 . += 0x1000;
 stack_top_irq = .;
 . += 0x1000;
 /* Beginning of process memory, to be allocated by kernel */
 process_memory_start = .;

 
 .       = . + 0x00001000;
 tos_P0  = .;
 /* allocate stack for P1       */
 .       = . + 0x00001000;
 tos_P1  = .;
}
