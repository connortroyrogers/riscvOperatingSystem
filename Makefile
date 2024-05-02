GPORT=$(shell python3 -c "import random; print(random.Random(\"$$USER\").randint(5570, 7000));")
#GPORT=                  # Generate a random port based on the current username, uncomment this line to switch to static port
IMG=bareOS.img
ARCH=riscv64-unknown-linux-gnu
CC=$(ARCH)-gcc
LD=$(ARCH)-ld
GDB=$(ARCH)-gdb 
OBJCOPY=$(ARCH)-objcopy
QEMU=qemu-system-riscv64

IDIR=kernel/include
TDIR=kernel/testing
BDIR=.build
MAP=$(BDIR)/kernel.map
ENV=.msfile
milestone?=$(shell cat $(ENV) 2>/dev/null)
milestone_imp=$(shell cat $(ENV) 2>/dev/null)

SRC=$(wildcard kernel/*/*.c)
ASM=$(wildcard kernel/*/*.s) $(wildcard kernel/*/*.S)
OBJ=$(patsubst %.c,$(BDIR)/%.o,$(notdir $(SRC))) $(patsubst %.s,$(BDIR)/%_asm.o,$(patsubst %.S,$(BDIR)/%_asm.o,$(notdir $(ASM))))
OBJ_TEST=$(patsubst %.c,$(BDIR)/%.o,$(notdir $(wildcard $(TDIR)/*.c))) \
         $(patsubst %.s,$(BDIR)/%_asm.o,$(notdir $(wildcard $(TDIR)/*.s)))
OBJ_LINK=$(filter-out $(OBJ_TEST),$(OBJ))
VPATH=$(dir $(ASM)) $(dir $(SRC))

CFLAGS=-Wall -Werror -fno-builtin -nostdlib -march=rv64imac -mabi=lp64 -mcmodel=medany -I $(IDIR) -O0 -g -D MILESTONE=$(milestone) -D MILESTONE_IMP=$(milestone_imp)
SFLAGS= -I $(IDIR) -march=rv64imac -mabi=lp64 -g
DFLAGS= -ex "file $(IMG)" -ex "target remote :$(GPORT)"
EFLAGS= -E -march=rv64imac -mabi=lp64
LDFLAGS=-nostdlib -Map $(MAP)
QFLAGS=-M virt -kernel $(IMG) -bios none -chardev stdio,id=uart0,logfile=.log -serial chardev:uart0 -display none
INJ_FN=shell handle_clk uart_handler ctxload disable_interrupts restore_interrupts initialize resched create_thread resume_thread join_thread uart_putc uart_getc builtin_hello builtin_echo tty_init sem_wait sem_post

STAGE=.setup

.PHONY: all clean qemu qemu-debug gdb dirs pack

all: dirs $(OBJ) $(BDIR)/kernel.elf $(IMG)


# ----------------------  Build OS Image  ----------------------------

$(IMG): $(BDIR)/kernel.elf
	$(OBJCOPY) $(BDIR)/kernel.elf -I binary $(IMG)

$(BDIR)/kernel.elf: $(OBJ_LINK) kernel/kernel.ld
	$(LD) $(LDFLAGS) -T kernel/kernel.ld -o $(BDIR)/kernel.elf $(OBJ_LINK)

%.s: %.S
	$(CC) $(EFLAGS) $< > $@

$(BDIR)/%_asm.o: %.s
	$(CC) $(SFLAGS) -c $< -o $@

$(BDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# ----------------------  Virtualization  ----------------------------

qemu: dirs
ifneq ("$(wildcard $(BDIR)/.force)","")
	$(MAKE) clean
endif
	$(MAKE) .qemu

.qemu: all
	$(QEMU) $(QFLAGS)


# --------------------  Environment Management  ----------------------

dirs: 
	@mkdir -p kernel/lib
	@mkdir -p kernel/app
	@mkdir -p kernel/testing
	@mkdir -p $(BDIR)

pack:
	rm -f bareOS.tar.gz
	tar -czvf bareOS.tar.gz kernel

clean:
	rm -f bareOS.tar.gz
	rm -rf $(BDIR) bareOS.img

checkout: dirs
	@echo -e Checking out milestone $(milestone)...
	@echo
	@cd $(STAGE) && \
		for dir in lib app system include device testing; do \
			for file in *; do if [ "$$file" != "$${file#milestone-$(milestone)-$$dir-}" ]; then echo "  Creating kernel/$$dir/$${file#milestone-$(milestone)-$$dir-}"; cp -i "$$file" ../kernel/$$dir/$${file#milestone-$(milestone)-$$dir-}; fi done \
		done
	@echo $(milestone) > $(ENV)
	@echo
	@echo Checkout complete

# The following is depreciated and should be removed next semester
milestone-%:
	$(MAKE) checkout milestone=$(patsubst milestone-%,%,$@)


# ------------------------  Debugging  -------------------------------

qemu-debug: QFLAGS += -S -gdb tcp::${GPORT}
qemu-debug: .qemu

gdb:
	$(GDB) $(DFLAGS)

test: LDFLAGS += $(addprefix --wrap=,$(INJ_FN))
test: OBJ_LINK += $(OBJ_TEST)
test: clean dirs all
	touch $(BDIR)/.force
	$(MAKE) .qemu

# The following are depreciated and should be removed next semester
test-milestone-1: milestone=1
test-milestone-1: test

test-milestone-2: milestone=2
test-milestone-2: test

test-milestone-3: milestone=3
test-milestone-3: test

test-milestone-4: milestone=4
test-milestone-4: test

test-milestone-5: milestone=5
test-milestone-5: test

test-milestone-6: milestone=6
test-milestone-6: test

test-milestone-7: milestone=7
test-milestone-7: test

test-milestone-8: milestone=8
test-milestone-8: test

test-milestone-9: milestone=9
test-milestone-9: test

test-milestone-10: milestone=10
test-milestone-10: test

FORCE:
