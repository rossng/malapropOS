.global irq_enable
.global irq_unable

irq_enable:  mrs   r0,   cpsr              @  enable IRQ interrupts
             bic   r0, r0, #0x80
             msr   cpsr_c, r0

             mov   pc, lr

irq_unable:  mrs   r0,   cpsr              @ disable IRQ interrupts
             orr   r0, r0, #0x80
             msr   cpsr_c, r0

             mov   pc, lr
