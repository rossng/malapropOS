.global handler_rst
handler_rst:
        bl    table_copy
        msr   cpsr, #0xD2             @ enter IRQ mode, interrupts disabled
        ldr   sp, =stack_top_irq      @ load the IRQ mode stack pointer
        msr   cpsr, #0xD3             @ enter SVC mode, interrupts disabled
        ldr   sp, =stack_top_svc      @ load the SVC mode stack pointer
        sub   sp, sp, #68             @ allocate space on the stack to store the current process context (ctx_t)

        mov   r0, sp                  @ load the stack pointer (i.e. pointer to the ctx_t) as the argument to kernel_handler_rst
        bl    kernel_handler_rst      @ execute the kernel's reset handler

        @ kernel_handler_rst will have initialised a process context at the stack pointer
        ldmia sp!, { r0, lr }         @ load the CPSR and PC of the process into r0 and lr, then advance the sp (by 8 bytes)
        msr   spsr, r0                @ store the loaded CPSR into the SPSR
        ldmia sp, { r0-r12, sp, lr }^ @ load the remainder of the process context directly into user mode (^) registers
        add   sp, sp, #60             @ advance the stack pointer by the remaining 60 bytes so that the stack is empty and back at =stack_top_svc
        movs  pc, lr                  @ do a context switch and resume executing the process from its pc

        b     .

handler_irq:
        sub   lr, lr, #4              @ correct return address - it is set to PC + 4 as per Cortex-A8 Technical Reference Manual s 2.15.1

        sub   sp, sp, #60             @ adjust the IRQ mode stack pointer to make space for the first part of the ctx_t
        stmia sp, { r0-r12, sp, lr }^ @ store the 15 USR mode registers onto stack
        mrs   r0, spsr                @ get the USR mode CSPR (saved into SPSR)
        stmdb sp!, { r0, lr }         @ auto-extend the stack to store r0 (contains the USR mode CPSR) and lr (contains the USR mode PC)
        mov   r0, sp                  @ set first argument to the stack pointer

        bl    kernel_handler_irq      @ invoke C interrupt handler

        @ kernel_handler_irq will have loaded a different process context at the stack pointer
        ldmia sp!, { r0, lr }         @ load the USR mode CPSR into r0 and PC into lr, then adjust the sp
        msr   spsr, r0                @ set the user mode CPSR
        ldmia sp, { r0-r12, sp, lr }^ @ load the remaining registers from the stack into the USR mode registers
        add   sp, sp, #60             @ adjust the IRQ mode sp so the stack is back to =stack_top_irq

        movs  pc, lr                  @ return from interrupt

handler_svc:
        sub   lr, lr, #0              @ correct return address
        sub   sp, sp, #60             @ update SVC mode stack pointer
        															 @ 60 bytes is space for the 15 4-byte registers
        stmia sp, { r0-r12, sp, lr }^ @ store the 15 USR mode registers on stack
        mrs   r0, spsr                @ get the USR mode CSPR (saved into SPSR)
        stmdb sp!, { r0, lr }         @ auto-extend the stack to store r0 (contains
         														 @ the USR mode CPSR) and lr (contains the USR
        														 @ mode PC)
        mov   r0, sp                  @ set    C function arg. = SP
        ldr   r1, [ lr, #-4 ]         @ load                     svc instruction
        bic   r1, r1, #0xFF000000     @ set    C function arg. = svc immediate
        bl    kernel_handler_svc      @ invoke C function
        ldmia sp!, { r0, lr }         @ load   USR mode PC and CPSR
        msr   spsr, r0                @ set    USR mode        CPSR
        ldmia sp, { r0-r12, sp, lr }^ @ load   USR mode registers
        add   sp, sp, #60             @ update SVC mode SP
        movs  pc, lr                  @ return from interrupt

interrupt_vector_table_start:
        ldr   pc, address_rst         @ reset                 vector -> SVC mode
        b     .                       @ undefined instruction vector -> UND mode
        ldr   pc, address_svc         @ supervisor call       vector -> SVC mode
        b     .                       @ abort (prefetch)      vector -> ABT mode
        b     .                       @ abort     (data)      vector -> ABT mode
        b     .                       @ reserved
        ldr   pc, address_irq         @ IRQ                   vector -> IRQ mode
        b     .                       @ FIQ                   vector -> FIQ mode


address_rst: .word handler_rst
address_svc: .word handler_svc
address_irq: .word handler_irq

interrupt_vector_table_end:

table_copy:
        mov   r0, #0                                @ the address at which to store the table
        ldr   r1, =interrupt_vector_table_start     @ the address of the start of the table
        ldr   r2, =interrupt_vector_table_end       @ the address of the end of the table

table_copy_loop:
        ldr  r3, [ r1 ], #4            @ load the current table row pointed to by r1 and increment r1 by a byte
        str  r3, [ r0 ], #4            @ store the row into the destination, then increment r0 by a byte

        cmp  r1, r2                    @ check whether we've reached the end of the table
        bne  table_copy_loop           @ if not, loop again

        mov  pc, lr                    @ else return
