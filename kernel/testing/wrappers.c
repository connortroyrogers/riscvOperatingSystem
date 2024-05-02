#include <barelib.h>
#if MILESTONE >= 3
#include <thread.h>
#endif

#define IO_LEN 1024
#define validate(x) (x[1] == -1 || x[0] == x[1])
#define STDIN_COUNT 0x0
#define STDOUT_COUNT 0x1
#define STDOUT_MATCH 0x2

extern uint32 boot_complete;
extern uint32 t__status;
extern int32  t__timeout;
void t__break(void);
void t__mem_reset(uint32);

#ifdef MILESTONE_IMP
void t__ms1(uint32);
#endif
#if MILESTONE_IMP > 1
void t__ms2(uint32);
#endif
#if MILESTONE_IMP > 2
void t__ms3(uint32);
#endif
#if MILESTONE_IMP > 3
void t__ms4(uint32);
#endif
#if MILESTONE_IMP > 4
void t__ms5(uint32);
#endif
#if MILESTONE_IMP > 5
void t__ms6(uint32);
#endif
#if MILESTONE_IMP > 6
void t__ms7(uint32);
#endif
#if MILESTONE_IMP > 7
void t__ms8(uint32);
void t__ms8_setup(void);
#endif
#if MILESTONE_IMP > 8
void t__ms9(uint32);
#endif
#if MILESTONE_IMP > 9
void t__ms10(uint32);
#endif

static int t__stdout_expects[2] = {-1, -1};
static int t__stdin_expects[2] = {-1, -1};
static char t__stdin[IO_LEN];
static char t__stdout[IO_LEN];
uint32 t__resched_called = 0;
uint32 t__uart_putc_called = 0;
uint32 t__uart_getc_called = 0;
uint32 t__join_thread_called = 0;
uint32 t__resume_thread_called = 0;
uint32 t__create_thread_called = 0;
uint32 t__ctxload_called = 0;
uint32 t__hello_called = 0;
uint32 t__echo_called = 0;

char* t__raw_stdout(void) {
  return t__stdout;
}

char __real_uart_putc(char);
char __wrap_uart_putc(char ch) {
  t__uart_putc_called += 1;
  t__stdout_expects[0]++;
  t__stdout[(t__uart_putc_called - 1) % IO_LEN] = ch;
  return ch;
}

char __real_uart_getc(void);
char __wrap_uart_getc(void) {
  t__uart_getc_called += 1;
  t__stdin_expects[0] += 1;
  if (t__stdin_expects[1] > 0 && t__stdin_expects[0] >= t__stdin_expects[1])
    while (1);
  return t__stdin[(t__uart_getc_called - 1) % IO_LEN];
}

void t__set_io(const char* stdin, int32 stdin_c, int32 stdout_c) {
  for (int i=0; i<IO_LEN; i++) t__stdout[i] = '\0';
  for (int i=0; i<IO_LEN && stdin[i] != '\0'; i++) t__stdin[i] = stdin[i];
  t__stdout_expects[0] = 0;
  t__stdout_expects[1] = stdout_c;
  t__stdin_expects[0] = 0;
  t__stdin_expects[1] = stdin_c;
  t__uart_getc_called = t__uart_putc_called = 0;
  t__resched_called = 0;
}

byte t__check_io(uint32 stdin_c, const char* stdout) {
  t__status |= 0x1 << STDOUT_MATCH;
  for (int i=0; i<IO_LEN && stdout[i] != '\0'; i++) t__status &= (stdout[i] == t__stdout[i]) << STDOUT_MATCH;
  t__status |= t__uart_getc_called == stdin_c << STDIN_COUNT;
  t__status |= validate(t__stdout_expects) << STDOUT_COUNT;
  return t__status;
}

byte t__wait_called = 0;
byte t__block_wait = 0;
int32 __wrap_sem_wait(uint32 sid) {
  t__wait_called += 1;
  while (t__block_wait);
  return 0;
}

byte t__post_called = 0;
int32 __wrap_sem_post(uint32 sid) {
  t__post_called += 1;
  return 0;
}

byte t__tty_init_called = 0;
void __wrap_tty_init(void) {
  t__tty_init_called += 1;
  return;
}


byte t__enable_uart = 1;
byte t__uart_interrupts = 0;
extern volatile byte* uart;
void set_uart_interrupt(uint32);
void __real_uart_handler(void);
void __wrap_uart_handler(void) {
  if (t__enable_uart)
    __real_uart_handler();
  t__uart_interrupts = uart[0x1];
  set_uart_interrupt(0);
}

extern volatile uint32* clint_timer_addr;
static const uint32 timer_interval = 100000;
byte t__auto_timeout = 1;
byte t__default_timer = 0;
interrupt __real_handle_clk(void);
interrupt __wrap_handle_clk(void) {
  t__timeout -= 1;
  *clint_timer_addr += timer_interval;
  if (t__timeout <= 0 && t__timeout > -2 && t__auto_timeout) {
    t__timeout = -1;
    t__break();
  }
  if (t__default_timer)
    asm volatile(
		 "ld ra,136(sp)\n"
		 "ld t0,128(sp)\n"
		 "ld t1,120(sp)\n"
		 "ld t2,112(sp)\n"
		 "ld s0,104(sp)\n"
		 "ld a0, 96(sp)\n"
		 "ld a1, 88(sp)\n"
		 "ld a2, 80(sp)\n"
		 "ld a3, 72(sp)\n"
		 "ld a4, 64(sp)\n"
		 "ld a5, 56(sp)\n"
		 "ld a6, 48(sp)\n"
		 "ld a7, 40(sp)\n"
		 "ld t3, 32(sp)\n"
		 "ld t4, 24(sp)\n"
		 "ld t5, 16(sp)\n"
		 "ld t6,  8(sp)\n"
		 "add sp,sp,114\n"
		 "j __real_handle_clk"
		 );
}

byte t__default_resched = 0;
byte t__skip_resched = 0;
static int32 t__pinned_thread = -1;
int32 __real_resched(void);
int32 __wrap_resched(void) {
  t__resched_called += 1;

  if (t__skip_resched) return 0;
  if (t__default_resched) {
    int32 v;
    asm volatile(
		 "    .option arch, +zicsr\n"
                 "csrs mstatus, 0x8   \n"    // Temporarily enable machine privilage interrupts
		 "call __real_resched \n"    // Call resched
		 "csrc mstatus, 0x8   \n"    // Disable machine privilage interrupts
		 "mv %0, a0"
		 : "=r" (v)); 
    return v;
  }
  else {
    uint32 i, old = current_thread;
    if (t__pinned_thread == -1) {
      for (i=0; i<NTHREADS; i++)
	if (thread_table[(i + current_thread) % NTHREADS].state == TH_READY)
	  break;
      if (thread_table[current_thread].state == TH_RUNNING)
	thread_table[current_thread].state = TH_READY;
      thread_table[i].state = TH_RUNNING;
      current_thread = i;
      ctxsw(&(thread_table[i].stackptr), &(thread_table[old].stackptr));
    }
    else if (current_thread != t__pinned_thread) {
      if (thread_table[current_thread].state == TH_RUNNING)
	thread_table[current_thread].state = TH_READY;
      thread_table[t__pinned_thread].state = TH_RUNNING;
      current_thread = t__pinned_thread;
      ctxsw(&(thread_table[t__pinned_thread].stackptr), &(thread_table[old].stackptr));
    }
  }
  return 0;
}

int32 __real_join_thread(uint32 threadid);
int32 __wrap_join_thread(uint32 threadid) {
  t__join_thread_called += 1;
#if MILESTONE < 3
  return 1;
#else
  return __real_join_thread(threadid);
#endif
}

int32 __real_resume_thread(uint32 threadid);
int32 __wrap_resume_thread(uint32 threadid) {
  t__resume_thread_called += 1;
#if MILESTONE < 3
  return 1;
#else
  return __real_resume_thread(threadid);
#endif
}

int32 __real_create_thread(void* proc, char* arg, uint32 arglen);
int32 __wrap_create_thread(void* proc, char* arg, uint32 arglen) {
  t__create_thread_called += 1;
#if MILESTONE < 3
  byte (*fn)(char*) = (byte (*)(char*))proc;
  thread_table[1].retval = fn(arg);
  thread_table[1].state = TH_DEFUNCT;
  return 1;
#else
  return __real_create_thread(proc, arg, arglen);
#endif
}

static uint32 run_idx = 0;
byte __wrap_shell(char* arg) {
  t__pinned_thread = current_thread;
#if MILESTONE == 4
  t__ms4(run_idx);
#elif MILESTONE == 5
  t__ms5(run_idx);
#elif MILESTONE == 6
  t__ms6(run_idx);
#elif MILESTONE == 7
  t__ms7(run_idx);
#elif MILESTONE == 8
  t__ms8(run_idx);
#elif MILESTONE == 9
  t__ms9(run_idx);
#elif MILESTONE == 10
  t__ms10(run_idx);
#endif
  while (1);
  return 0;
}

void __real_initialize(void);
void t__run(uint32 idx) {
  run_idx = idx;
  t__default_resched = 0;
  t__default_timer = 0;
  t__skip_resched = 0;
  boot_complete = 0;
  t__mem_reset(0);
#if MILESTONE == 1
  t__ms1(run_idx);
#elif MILESTONE == 2
  t__ms2(run_idx);
#elif MILESTONE == 3
  t__default_resched = 1;
  t__ms3(run_idx);
#elif MILESTONE >= 4
  __real_initialize();
#endif
}

void __wrap_initialize(void) {
  t__timeout = (-1 & ~(0x1 << 31));
#if MILESTONE < 4
  uart_init();
#endif
#if MILESTONE >= 8
  t__ms8_setup();
#endif
  t__run(0);
}

void __real_ctxload(uint64**);
void __wrap_ctxload(uint64** proc) {
  t__ctxload_called += 1;
  __real_ctxload(proc);
}

byte __wrap_disable_interrupts(void) {
  return 0x0;
}

void __wrap_restore_interrupts(byte v) {
  return;
}


byte __real_builtin_hello(char* arg);
byte __wrap_builtin_hello(char* arg) {
  t__hello_called += 1;
  return __real_builtin_hello(arg);
}

byte __real_builtin_echo(char* arg);
byte __wrap_builtin_echo(char* arg) {
  t__echo_called += 1;
  return __real_builtin_echo(arg);
}
