	.file "msutil.s"
	.option arch, +zicsr
	.equ REGSZ, 8
	.equ TIMEOUT, 0x5

recovery_space:
	.dword 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

	.globl 	t__status
t__status:
	.word 0

	.globl t__timeout
t__timeout:
	.word -1

	.globl t__break
t__break:
	la	t0,rcv_ptr
	csrw	mepc,t0
	mret

	.globl t__reset
t__reset:
	la sp, _mmap_kstack_top
	call t__run
	j    __noop
	
	.globl t__with_timeout
t__with_timeout:
	la 	t0,recovery_space
	sd	ra,0*REGSZ(t0)
	sd	s0,1*REGSZ(t0)
	sd 	s1,2*REGSZ(t0)
	sd	s2,3*REGSZ(t0)
	sd	s3,4*REGSZ(t0)
	sd	s4,5*REGSZ(t0)
	sd	s5,6*REGSZ(t0)
	sd	s6,7*REGSZ(t0)
	sd	s7,8*REGSZ(t0)
	sd	s8,9*REGSZ(t0)
	sd	s9,10*REGSZ(t0)
	sd	s10,11*REGSZ(t0)
	sd	s11,12*REGSZ(t0)
	sd	a0,13*REGSZ(t0)
	sd	a1,14*REGSZ(t0)
	sd	a2,15*REGSZ(t0)
	sd	a3,16*REGSZ(t0)
	sd	a4,17*REGSZ(t0)
	sd	a5,18*REGSZ(t0)
	sd	a6,19*REGSZ(t0)
	sd	a7,20*REGSZ(t0)
	sd 	sp,21*REGSZ(t0)

	li      t0,0x0
	sw      t0,t__status,t1
	sw      a0,t__timeout,t1 # --
	addi    t0,a1,0          #  |
	addi	a0,a2,0          #  |
	addi 	a1,a3,0          #  |
	addi	a2,a4,0          #  |  Shift arguments into position for the protected call
	addi	a3,a5,0          #  |
	addi	a4,a6,0          #  |
	addi	a5,a7,0          # --

	li	t1, 0x0            # --
	sw	t1,t__status,t2    #  |  Call the protected function and jump to `__ms_complete_recover` when done
	jalr 	t0                 #  |
	j	complete_recovery  # --
rcv_ptr:
	addi 	a0,zero,-1            # -- Fix stack pointer after long jump
complete_recovery:
	lw      t1,t__timeout
	slti    t1,t1,0x0
	slli    t1,t1,TIMEOUT
	lw      t0,t__status
	or      t0,t0,t1
	sw      t0,t__status,t1
	li      t0, 0xFFFF
	sw      t0,t__timeout,t1
	la	t0,recovery_space
	ld	ra,0*REGSZ(t0)
	ld	s0,1*REGSZ(t0)
	ld 	s1,2*REGSZ(t0)
	ld	s2,3*REGSZ(t0)
	ld	s3,4*REGSZ(t0)
	ld	s4,5*REGSZ(t0)
	ld	s5,6*REGSZ(t0)
	ld	s6,7*REGSZ(t0)
	ld	s7,8*REGSZ(t0)
	ld	s8,9*REGSZ(t0)
	ld	s9,10*REGSZ(t0)
	ld	s10,11*REGSZ(t0)
	ld	s11,12*REGSZ(t0)
	ld	a1,14*REGSZ(t0)
	ld	a2,15*REGSZ(t0)
	ld	a3,16*REGSZ(t0)
	ld	a4,17*REGSZ(t0)
	ld	a5,18*REGSZ(t0)
	ld	a6,19*REGSZ(t0)
	ld	a7,20*REGSZ(t0)
	ld 	sp,21*REGSZ(t0)
	ret
