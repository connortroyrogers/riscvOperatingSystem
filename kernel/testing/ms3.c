#include <barelib.h>
#include <bareio.h>
#include <thread.h>
#include <syscall.h>
#if MILESTONE_IMP >= 4
#include <queue.h>
#endif

#define STDIN_COUNT 0x0
#define STDOUT_COUNT 0x1
#define STDOUT_MATCH 0x2
#define TIMEOUT 0x5
#define status_is(cond) (t__status & (0x1 << cond))
#define test_count(x) sizeof(x) / sizeof(x[0])
#define init_tests(x, c) for (int i=0; i<c; i++) x[i] = "OK"
#define assert(test, ls, err) if (!(test)) ls = err

void __real_ctxload(uint64**);
void enable_interrupts(void);

void t__print(const char*);
void t__set_io(const char* stdin, int32 stdin_c, int32 stdout_c);
byte t__check_io(uint32 count, const char* stdout);
int  t__with_timeout(int32, void*, ...);
void t__mem_reset(uint32);

char __real_initialize(char*);
char __real_shell(char*);

void t__runner(const char* , uint32, void(*)(void));
void t__printer(const char*, char**, const char**, uint32);

extern uint32 t__resched_called;
extern uint32 t__join_thread_called;
extern uint32 t__create_thread_called;
extern uint32 t__resume_thread_called;
extern uint32 t__ctxload_called;
extern byte t__skip_resched;
extern byte t__default_resched;
extern uint32 t__status;

static const char* general_prompt[] = {
			    "  Program Compiles:                   "
};
static const char* suspend_prompt[] = {
			    "  Indicated thread was suspended:     ",
			    "  Invalid threads not suspended:      ",
			    "  Raises RESCHED signal:              ",
};
static const char* resume_prompt[] = {
			    "  Indicated thread was readied:       ",
			    "  Invalid threads are ignored:        ",
			    "  Raises RESCHED signal:              "
};
static const char* join_prompt[] = {
			    "  Frees awaited thread:               ",
			    "  Returns the thread's return value:  ",
			    "  Blocks until thread is defunct:     ",
};
static const char* resched_prompt[] = {
			    "  Alternates between two threads:     ",
			    "  Cycles between three threads:       ",
			    "  Sets old thread as ready:           ",
			    "  Sets the current thread:            ",
			    "  Sets the new thread to running:     ",
};
static const char* shell_prompt[] = {
			    "  Creates applications with threads:  ",
			    "  Calls `create_thread`:              ",
			    "  Calls `resume_thread`:              ",
			    "  Calls `join_thread`:                ",
};
static const char* init_prompt[] = {
			    "  Started with ctxload:               ",
};

static char* general_t[test_count(general_prompt)];
static char* suspend_t[test_count(suspend_prompt)];
static char* resume_t[test_count(resume_prompt)];
static char* join_t[test_count(join_prompt)];
static char* resched_t[test_count(resched_prompt)];
static char* shell_t[test_count(shell_prompt)];
static char* init_t[test_count(init_prompt)];

static void resume(uint32 tid) {
  thread_table[tid].state = TH_READY;
#if MILESTONE_IMP >= 4
  thread_enqueue(ready_list, tid);
#endif
}

static char thread_1(char* arg) {
  return 0;
}

static byte t2_val = 0;
static char add_by_two(char* arg) {
  byte v = t2_val;
  t2_val += 1;
  raise_syscall(RESCHED);
  assert(t2_val >= v + 2, resched_t[0], "FAIL - Thread ran again before all other threads ran");
  t2_val += 1;
  raise_syscall(RESCHED);
  assert(t2_val >= v + 4, resched_t[0], "FAIL - Thread ran again before all other threads ran");
  return 0;
}

static byte t3_val = 0;
char add_by_three(char* arg) {
  byte v = t3_val;
  t3_val += 1;
  raise_syscall(RESCHED);
  assert(t3_val >= v + 3, resched_t[1], "FAIL - Thread ran again before all other threads ran");
  t3_val += 1;
  raise_syscall(RESCHED);
  assert(t3_val >= v + 6, resched_t[1], "FAIL - Thread ran again before all other threads ran");
  t3_val += 1;
  return 0;
}

char state_tester(char* arg) {
  assert(thread_table[0].state != TH_FREE,                 resched_t[2], "FAIL - Previous thread was set to FREE");
  assert(thread_table[0].state != TH_RUNNING,              resched_t[2], "FAIL - Previous thread set to RUNNING");
  assert(thread_table[0].state != TH_SUSPEND,              resched_t[2], "FAIL - Previous thread set to SUSPEND");
  assert(thread_table[0].state != TH_DEFUNCT,              resched_t[2], "FAIL - Previous thread set to DEFUNCT");
  assert(current_thread != 0,                              resched_t[3], "FAIL - 'current_thread' is not set to the newly running thread");
  assert(thread_table[current_thread].state != TH_FREE,    resched_t[4], "FAIL - 'current_thread' is set to FREE");
  assert(thread_table[current_thread].state != TH_READY,   resched_t[4], "FAIL - 'current_thread' is set to READY");
  assert(thread_table[current_thread].state != TH_SUSPEND, resched_t[4], "FAIL - 'current_thread' is set to SUSPEND");
  assert(thread_table[current_thread].state != TH_DEFUNCT, resched_t[4], "FAIL - 'current_thread' is set to DEFUNCT");
  return 0;
}

static void general_tests(void) {
  return;
}

static void suspend_tests(void) {
  int32 tid = create_thread(thread_1, "", 0);
  resume(tid);
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))suspend_thread, tid);

  assert(thread_table[tid].state != TH_FREE, suspend_t[0], "FAIL - Thread set to FREE after suspend call");
  assert(thread_table[tid].state != TH_RUNNING, suspend_t[0], "FAIL - Thread set to RUNNING after suspend call");
  assert(thread_table[tid].state != TH_READY, suspend_t[0], "FAIL - Thread set to READY after suspend call");
  assert(thread_table[tid].state != TH_DEFUNCT, suspend_t[0], "FAIL - Thread set to DEFUNCT after suspend call");
  assert(t__resched_called, suspend_t[2], "FAIL - RESCHED system call was not raised");

  thread_table[tid].state = TH_FREE;
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))suspend_thread, tid);
  
  assert(t__resched_called == 0, suspend_t[1], "FAIL - RESCHED called when thread was FREE");
  
  thread_table[tid].state = TH_SUSPEND;
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))suspend_thread, tid);
  
  assert(t__resched_called == 0, suspend_t[1], "FAIL - RESCHED called when thread was SUSPEND");

  thread_table[tid].state = TH_DEFUNCT;
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))suspend_thread, tid);
  
  assert(t__resched_called == 0, suspend_t[1], "FAIL - RESCHED called when thread was DEFUNCT");
}

static void resume_tests(void) {
  int32 tid = create_thread(thread_1, "", 0);
  t__with_timeout(10, (void (*)(void))resume_thread, tid);

  assert(thread_table[tid].state != TH_FREE, resume_t[0], "FAIL - Thread set to FREE after resume call");
  assert(thread_table[tid].state != TH_RUNNING, resume_t[0], "FAIL - Thread set to RUNNING after resume call");
  assert(thread_table[tid].state != TH_SUSPEND, resume_t[0], "FAIL - Thread set to SUSPEND after resume call");
  assert(thread_table[tid].state != TH_DEFUNCT, resume_t[0], "FAIL - Thread set to DEFUNCT after resume call");
  assert(t__resched_called, resume_t[2], "FAIL - RESCHED system call was not raise");
  
  thread_table[tid].state = TH_FREE;
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))resume_thread, tid);
  
  assert(t__resched_called == 0, resume_t[1], "FAIL - RESCHED called when thread was FREE");
  
  thread_table[tid].state = TH_RUNNING;
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))resume_thread, tid);

  assert(t__resched_called == 0, resume_t[1], "FAIL - RESCHED called when thread was RUNNING");

  resume(tid);
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))resume_thread, tid);

  assert(t__resched_called == 0, resume_t[1], "FAIL - RESCHED called when thread was ready");

  thread_table[tid].state = TH_DEFUNCT;
  t__resched_called = 0;
  t__with_timeout(10, (void (*)(void))resume_thread, tid);

  assert(t__resched_called == 0, resume_t[1], "FAIL - RESCHED called when thread was DEFUNCT");
}

static void join_tests(void) {
  byte result;
  int32 tid = create_thread(thread_1, "", 0);
  thread_table[tid].state = TH_DEFUNCT;
  thread_table[tid].retval = 12;

  t__resched_called = 0;
  result = (byte)t__with_timeout(10, (void (*)(void))join_thread, tid);

  assert(t__resched_called == 0, join_t[0], "FAIL - RESCHED raised when thread was already DEFUNCT");
  assert(thread_table[tid].state != TH_RUNNING, join_t[0], "FAIL - Joined thread's state was RUNNING after join");
  assert(thread_table[tid].state != TH_READY, join_t[0], "FAIL - Joined thread's state was READY after join");
  assert(thread_table[tid].state != TH_SUSPEND, join_t[0], "FAIL - Joined thread's state was SUSPEND after join");
  assert(thread_table[tid].state != TH_DEFUNCT, join_t[0], "FAIL - Joined thread's state was still DEFUNCT after join");
  assert(result == 12, join_t[1], "FAIL - 'join_thread' did not return the thread's result");

  t__resched_called = 0;
  resume(tid);
  t__with_timeout(10, (void (*)(void))join_thread, tid);

  assert(t__resched_called > 100, join_t[2], "FAIL - 'join_thread' not busy waiting on RESCHED when thread is READY");
  assert(t__resched_called, join_t[2], "FAIL - RESCHED not raised when joined thread was READY");

  t__resched_called = 0;
  thread_table[tid].state = TH_FREE;
  t__with_timeout(10, (void (*)(void))join_thread, tid);

  assert(t__resched_called == 0, join_t[2], "FAIL - RESCHED raised when joined thread was FREE");

  t__resched_called = 0;
  thread_table[tid].state = TH_RUNNING;
  t__with_timeout(10, (void (*)(void))join_thread, tid);

  assert(t__resched_called > 100, join_t[2], "FAIL - 'join_thread' not busy waiting on RESCHED when thread is RUNNING");
  assert(t__resched_called, join_t[2], "FAIL - RESCHED not raised when joined thread was RUNNING");

  t__resched_called = 0;
  thread_table[tid].state = TH_SUSPEND;
  t__with_timeout(10, (void (*)(void))join_thread, tid);
  
  assert(t__resched_called > 100, join_t[2], "FAIL - 'join_thread' not busy waiting on RESCHED when thread is SUSPEND");
  assert(t__resched_called, join_t[2], "FAIL - RESCHED not raised when joined thread was SUSPEND");
}

static void resched_01(void) {
  int32 tid1;
  tid1 = create_thread(state_tester, "", 0);
  resume(tid1);
  raise_syscall(RESCHED);
}

static void resched_02(void) {
  int32 tid1;
  tid1 = create_thread(add_by_two, "", 0);
  resume(tid1);
  raise_syscall(RESCHED);
  t2_val += 1;
  raise_syscall(RESCHED);
  t2_val += 1;
  raise_syscall(RESCHED);
  assert(t2_val > 2, resched_t[0], "FAIL - Test thread was not scheduled");
}

static void resched_03(void) {
  int32 tid1, tid2;
  tid1 = create_thread(add_by_three, "", 0);
  tid2 = create_thread(add_by_three, "", 0);
  resume(tid1);
  resume(tid2);
  raise_syscall(RESCHED);
  t3_val += 1;
  raise_syscall(RESCHED);
  t3_val += 1;
  raise_syscall(RESCHED);
  assert(t3_val > 2, resched_t[2], "FAIL - Test thread was not scheduled");
}

static void resched_tests(void) {
  t__skip_resched = 0;
  t__default_resched = 1;

  t__with_timeout(10, (void (*)(void))resched_01);
  assert(!status_is(TIMEOUT), resched_t[2], "TIMEOUT -- Test was stuck in infinite loop");
  assert(!status_is(TIMEOUT), resched_t[3], "TIMEOUT -- Test was stuck in infinite loop");
  assert(!status_is(TIMEOUT), resched_t[4], "TIMEOUT -- Test was stuck in infinite loop");

  t__mem_reset(1);
  enable_interrupts();
  t__with_timeout(10, (void (*)(void))resched_02);
  assert(!status_is(TIMEOUT), resched_t[0], "TIMEOUT -- Test was stuck in infinite loop");

  t__mem_reset(1);
  enable_interrupts();
  t__with_timeout(10, (void (*)(void))resched_03);
  assert(!status_is(TIMEOUT), resched_t[1], "TIMEOUT -- Test was stuck in infinite loop");
}

static void shell_tests(void) {
  t__set_io("hello v\n", 9, 100);
  t__resched_called = t__create_thread_called = t__resume_thread_called = t__join_thread_called = 0;
  t__with_timeout(10, (void (*)(void))__real_shell);

  assert(t__resched_called, shell_t[0], "FAIL - RESCHED not called when starting a thread from the shell");
  assert(t__create_thread_called == 1, shell_t[1], "FAIL - 'create_thread' not called from shell on new thread");
  assert(t__resume_thread_called == 1, shell_t[2], "FAIL - 'resume_thread' not called from shell on new thread");
  assert(t__join_thread_called == 1, shell_t[3], "FAIL - 'join_thread' not called from shell on new thread");
}

static void initialize_tests(void) {
  t__ctxload_called = 0;
  t__with_timeout(10, (void (*)(void))__real_initialize);

  assert(t__ctxload_called == 1, init_t[0], "FAIL - 'ctxload' not called during initializiation");
}

static byte runner(char* arg) {
  uint32 idx = *(int*)arg;
  if (idx == 0) {
    t__print("\n");
    init_tests(general_t, test_count(general_prompt));
    t__runner("general", test_count(general_prompt), general_tests);
  }
  else if (idx == 1) {
    init_tests(suspend_t, test_count(suspend_prompt));
    t__runner("suspend", test_count(suspend_prompt), suspend_tests);
  }
  else if (idx == 2) {
    init_tests(resume_t, test_count(resume_prompt));
    t__runner("resume", test_count(resume_prompt), resume_tests);
  }
  else if (idx == 3) {
    init_tests(join_t, test_count(join_prompt));
    t__runner("join", test_count(join_prompt), join_tests);
  }
  else if (idx == 4) {
    init_tests(resched_t, test_count(resched_prompt));
    t__runner("resched", test_count(resched_prompt), resched_tests);
  }
  else if (idx == 5) {
    init_tests(shell_t, test_count(shell_prompt));
    t__runner("shell", test_count(shell_prompt), shell_tests);
  }
  return 0;
}

void t__ms3(uint32 idx) {
  t__skip_resched = 1;
  if (idx < 6) {
    int32 tid = create_thread(runner, (char*)(&idx), sizeof(uint32));
    thread_table[tid].state = TH_RUNNING;
    __real_ctxload(&(thread_table[tid].stackptr));
  }
  else if  (idx == 6) {
    init_tests(init_t, test_count(init_prompt));
    t__runner("init", test_count(init_prompt), initialize_tests);
  }
  else {
    t__print("\n----------------------------\n");
    t__printer("\nGeneral Tests:", general_t, general_prompt, test_count(general_prompt));
    t__printer("\nSuspend Tests:", suspend_t, suspend_prompt, test_count(suspend_prompt));
    t__printer("\nResume Tests:", resume_t, resume_prompt, test_count(resume_prompt));
    t__printer("\nJoin Tests:", join_t, join_prompt, test_count(join_prompt));
    t__printer("\nResched Tests:", resched_t, resched_prompt, test_count(resched_prompt));
    t__printer("\nShell Tests:", shell_t, shell_prompt, test_count(shell_prompt));
    t__printer("\nInit Tests:", init_t, init_prompt, test_count(init_prompt));
    t__print("\n");
  }
}
