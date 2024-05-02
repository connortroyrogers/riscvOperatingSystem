       .option arch, +zicsr

.globl set_interrupt
set_interrupt:
	csrrs a0, mie, a0
	ret

.globl disable_interrupts
disable_interrupts:
       csrrci a0, sstatus, 0x2
       ret

.globl enable_interrupts
enable_interrupts:
	csrs sstatus, 0x2
	ret

.globl restore_interrupts
restore_interrupts:
       csrw sstatus, a0
       ret

.global is_interrupting
is_interrupting:
       csrr a0, sstatus
       and a0, a0, 0x2
       ret
