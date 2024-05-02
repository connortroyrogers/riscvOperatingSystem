#include <barelib.h>
#include <syscall.h>
#include <queue.h>
#include <interrupts.h>

#define TIMEOUT 0x5
#define status_is(cond) (t__status & (0x1 << cond))
#define test_count(x) sizeof(x) / sizeof(x[0])
#define init_tests(x, c) for (int i=0; i<c; i++) x[i] = "OK"
#define assert(test, ls, err) if (!(test)) ls = err

void t__print(const char*);
void t__runner(const char* , uint32, void(*)(void));
void t__printer(const char*, char**, const char**, uint32);
void t__mem_reset(uint32);

int  t__with_timeout(int32, void*, ...);

extern byte t__default_resched;
extern byte t__auto_timeout;
extern int32 t__timeout;
extern uint32 t__status;


static const char* general_prompt[] = {
			     "  Program Compiles:             "
};
static const char* init_prompt[] = {
			     "    List initialization:        ",
};
static const char* enqueue_prompt[] = {
			     "    List integrity [1-thread]:  ",
			     "    List integrity [2-threads]: ",
			     "    List integrity [3-threads]: ",
};
static const char* dequeue_prompt[] = {
			     "    First pop:                  ",
			     "    First return:               ",
			     "    Second pop:                 ",
			     "    Second return:              ",
			     "    Third pop:                  ",
			     "    Third return:               ",
};
static const char* order_prompt[] = {
			     "    Enqueue first:              ",
			     "    Enqueue second:             ",
			     "    Enqueue third:              ",
			     "    Dequeue first:              ",
			     "    Dequeue second:             ",
			     "    Dequeue third:              ",
};
static const char* resched_prompt[] = {
			     "  Enqueue on resume:            ",
			     "  Dequeue on suspend:           ",
			     "  Resched dequeues/enqueues:    ",
};
static char* general_t[test_count(general_prompt)];
static char* init_t[test_count(init_prompt)];
static char* enqueue_t[test_count(enqueue_prompt)];
static char* dequeue_t[test_count(dequeue_prompt)];
static char* order_t[test_count(order_prompt)];
static char* resched_t[test_count(resched_prompt)];

static byte dummy_thread(char* arg) {
  while (1)
    raise_syscall(RESCHED);
  return 0;
}

static byte resched_thread(char* arg) {
  assert(thread_queue[ready_list].qnext == 0, resched_t[2], "FAIL - 'ready_list.qnext' does not point to shell after resched");
  assert(thread_queue[ready_list].qprev == 0, resched_t[2], "FAIL - 'ready_list.qprev' does not point to shell after resched");
  return 0;
}

static byte join_thread_local(uint32 threadid) {
  char mask = disable_interrupts();
  byte result;

  if (thread_table[threadid].state == TH_FREE) {
    restore_interrupts(mask);
    return NTHREADS;
  }
  
  while (thread_table[threadid].state != TH_DEFUNCT && t__timeout > 0) {
    raise_syscall(RESCHED);
  }
  thread_table[threadid].state = TH_FREE;
  result = thread_table[threadid].retval;

  restore_interrupts(mask);
  return result;
}

static void general_tests(void) {
  return;
}

static void initialize_tests(void) {
  assert(thread_queue[ready_list].qnext == ready_list, init_t[0], "FAIL - 'ready_list.qnext' does not point to itself");
  assert(thread_queue[ready_list].qprev == ready_list, init_t[0], "FAIL - 'ready_list.qprev' does not point to itself");
  t__mem_reset(1);
}

static void enqueue_tests(void) {
  int32 tid1, tid2, tid3;
  tid1 = create_thread(dummy_thread, "", 0);
  thread_enqueue(ready_list, tid1);

  assert(thread_queue[ready_list].qnext == tid1, enqueue_t[0], "FAIL - 'ready_list.qnext' does not point to new thread");
  assert(thread_queue[ready_list].qprev == tid1, enqueue_t[0], "FAIL - 'ready_list.qprev' does not point to new thread");
  assert(thread_queue[tid1].qprev == ready_list, enqueue_t[0], "FAIL - new thread's 'qprev' does not point to 'ready_list'");
  assert(thread_queue[tid1].qnext == ready_list, enqueue_t[0], "FAIL - new thread's 'qnext' does not point to 'ready_list'");

  tid2 = create_thread(dummy_thread, "", 0);
  thread_enqueue(ready_list, tid2);

  assert(thread_queue[ready_list].qnext == tid1, enqueue_t[1], "FAIL - 'ready_list.qnext' does not point to first thread");
  assert(thread_queue[ready_list].qprev == tid2, enqueue_t[1], "FAIL - 'ready_list.qprev' does not point to new thread");
  assert(thread_queue[tid1].qnext == tid2, enqueue_t[1], "FAIL - first thread's 'qnext' does not point to new thread");
  assert(thread_queue[tid1].qprev == ready_list, enqueue_t[1], "FAIL - first thread's 'qprev' does not point to 'ready_list'");
  assert(thread_queue[tid2].qnext == ready_list, enqueue_t[1], "FAIL - new thread's 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid2].qprev == tid1, enqueue_t[1], "FAIL - new thread's 'qprev' does not point to first thread");

  tid3 = create_thread(dummy_thread, "", 0);
  thread_enqueue(ready_list, tid3);

  assert(thread_queue[ready_list].qnext == tid1, enqueue_t[2], "FAIL - 'ready_list.qnext' does not point to first thread");
  assert(thread_queue[ready_list].qprev == tid3, enqueue_t[2], "FAIL - 'ready_list.qprev' does not point to new thread");
  assert(thread_queue[tid1].qnext == tid2, enqueue_t[2], "FAIL - first thread's 'qnext' does not point to second thread");
  assert(thread_queue[tid1].qprev == ready_list, enqueue_t[2], "FAIL - first thread's 'qprev' does not point to 'ready_list'");
  assert(thread_queue[tid2].qnext == tid3, enqueue_t[2], "FAIL - second thread's 'qnext' does not point to new thread");
  assert(thread_queue[tid2].qprev == tid1, enqueue_t[2], "FAIL - second thread's 'qprev' does not point to first thread");
  assert(thread_queue[tid3].qnext == ready_list, enqueue_t[2], "FAIL - new thread's 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid3].qprev == tid2, enqueue_t[2], "FAIL - new thread's 'qprev' does not point to second thread");

  t__mem_reset(1);
}

static void dequeue_tests(void) {
  int32 tid1, tid2, tid3;
  int32 ret1, ret2, ret3;
  tid1 = create_thread(dummy_thread, "", 0);
  tid2 = create_thread(dummy_thread, "", 0);
  tid3 = create_thread(dummy_thread, "", 0);
  thread_enqueue(ready_list, tid1);
  thread_enqueue(ready_list, tid2);
  thread_enqueue(ready_list, tid3);

  ret1 = thread_dequeue(ready_list);
  assert(thread_queue[ready_list].qnext == tid2, dequeue_t[0], "FAIL - 'ready_list.qnext' does not point to second thread");
  assert(thread_queue[ready_list].qprev == tid3, dequeue_t[0], "FAIL - 'ready_list.qprev' does not point to third thread");
  assert(thread_queue[tid2].qnext == tid3, dequeue_t[0], "FAIL - second thread's 'qnext' does not point to third thread");
  assert(thread_queue[tid2].qprev == ready_list, dequeue_t[0], "FAIL - second thread's 'qprev' does not point to 'ready_list'");
  assert(thread_queue[tid3].qnext == ready_list, dequeue_t[0], "FAIL - third thread's 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid3].qprev == tid2, dequeue_t[0], "FAIL - third thread's 'qprev' does not point to first thread");
  assert(ret1 = tid1, dequeue_t[1], "FAIL - return value is not the first enqueued thread'");

  ret2 = thread_dequeue(ready_list);
  assert(thread_queue[ready_list].qnext == tid3, dequeue_t[2], "FAIL - 'ready_list.qnext' does not point to third thread");
  assert(thread_queue[ready_list].qprev == tid3, dequeue_t[2], "FAIL - 'ready_list.qprev' does not point to third thread");
  assert(thread_queue[tid3].qnext == ready_list, dequeue_t[2], "FAIL - third thread's 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid3].qprev == ready_list, dequeue_t[2], "FAIL - third thread's 'qprev' does not point to 'ready_list'");
  assert(ret2 = tid2, dequeue_t[3], "FAIL - return value is not the second enqueued thread'");

  ret3 = thread_dequeue(ready_list);
  assert(thread_queue[ready_list].qnext == ready_list, dequeue_t[4], "FAIL - 'ready_list.qnext' does not point to 'ready_list'");
  assert(thread_queue[ready_list].qprev == ready_list, dequeue_t[4], "FAIL - 'ready_list.qprev' does not point to 'ready_list'");
  assert(ret3 = tid3, dequeue_t[5], "FAIL - return value is not the third enqueued thread'");

  t__mem_reset(1);
}

static void order_tests(void) {
  int32 tid1, tid2, tid3;
  tid1 = create_thread(dummy_thread, "", 0);
  tid2 = create_thread(dummy_thread, "", 0);
  tid3 = create_thread(dummy_thread, "", 0);
  
  thread_enqueue(ready_list, tid3);
  assert(thread_queue[ready_list].qnext == tid3, order_t[0], "FAIL - 'ready_list.qnext' does not point to out-of-order entry 1");
  assert(thread_queue[ready_list].qprev == tid3, order_t[0], "FAIL - 'ready_list.qprev' does not point to out-of-order entry 1");
  
  thread_enqueue(ready_list, tid1);
  assert(thread_queue[ready_list].qnext == tid3, order_t[1], "FAIL - 'ready_list.qnext' does not point to out-of-order entry 1");
  assert(thread_queue[ready_list].qprev == tid1, order_t[1], "FAIL - 'ready_list.qprev' does not point to out-of-order entry 2");
  assert(thread_queue[tid3].qnext == tid1,       order_t[1], "FAIL - out-of-order entry 1 'qnext' does not point to out-of-order entry 2");
  assert(thread_queue[tid3].qprev == ready_list, order_t[1], "FAIL - out-of-order entry 1 'qprev' does not point to 'ready_list'");
  assert(thread_queue[tid1].qnext == ready_list, order_t[1], "FAIL - out-of-order entry 2 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid1].qprev == tid3,       order_t[1], "FAIL - out-of-order entry 2 'qprev' does not point to out-of-order entry 1");

  thread_enqueue(ready_list, tid2);
  assert(thread_queue[ready_list].qnext == tid3, order_t[2], "FAIL - 'ready_list.qnext' does not point to out-of-order entry 1");
  assert(thread_queue[ready_list].qprev == tid2, order_t[2], "FAIL - 'ready_list.qprev' does not point to out-of-order entry 3");
  assert(thread_queue[tid3].qnext == tid1,       order_t[2], "FAIL - out-of-order entry 1 'qnext' does not point to out-of-order entry 2");
  assert(thread_queue[tid3].qprev == ready_list, order_t[2], "FAIL - out-of-order entry 1 'qprev' does not point to 'ready_list'");
  assert(thread_queue[tid1].qnext == tid2,       order_t[2], "FAIL - out-of-order entry 2 'qnext' does not point to out-of-order entry 3");
  assert(thread_queue[tid1].qprev == tid3,       order_t[2], "FAIL - out-of-order entry 2 'qprev' does not point to out-of-order entry 1");
  assert(thread_queue[tid2].qnext == ready_list, order_t[2], "FAIL - out-of-order entry 3 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid2].qprev == tid1,       order_t[2], "FAIL - out-of-order entry 3 'qprev' does not point to out-of-order entry 2");

  thread_dequeue(ready_list);
  assert(thread_queue[ready_list].qnext == tid1, order_t[3], "FAIL - 'ready_list.qnext' does not point to out-of-order entry 2");
  assert(thread_queue[ready_list].qprev == tid2, order_t[3], "FAIL - 'ready_list.qprev' does not point to out-of-order entry 3");
  assert(thread_queue[tid1].qnext == tid2,       order_t[3], "FAIL - out-of-order entry 2 'qnext' does not point to out-of-order entry 3");
  assert(thread_queue[tid1].qprev == ready_list, order_t[3], "FAIL - out-of-order entry 2 'qprev' does not point to 'ready_list'");
  assert(thread_queue[tid2].qnext == ready_list, order_t[3], "FAIL - out-of-order entry 3 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid2].qprev == tid1,       order_t[3], "FAIL - out-of-order entry 3 'qprev' does not point to out-of-order entry 2");

  thread_dequeue(ready_list);
  assert(thread_queue[ready_list].qnext == tid2, order_t[4], "FAIL - 'ready_list.qnext' does not point to out-of-order entry 3");
  assert(thread_queue[ready_list].qprev == tid2, order_t[4], "FAIL - 'ready_list.qprev' does not point to out-of-order entry 3");
  assert(thread_queue[tid2].qnext == ready_list, order_t[4], "FAIL - out-of-order entry 3 'qnext' does not point to 'ready_list'");
  assert(thread_queue[tid2].qprev == ready_list, order_t[4], "FAIL - out-of-order entry 3 'qprev' does not point to 'ready_list'");

  thread_dequeue(ready_list);
  assert(thread_queue[ready_list].qnext == ready_list, order_t[5], "FAIL - 'ready_list.qnext' does not point to 'ready_list'");
  assert(thread_queue[ready_list].qprev == ready_list, order_t[5], "FAIL - 'ready_list.qprev does not point to 'ready_list'");

  t__mem_reset(1);
}

static void resched_tests(void) {
  int32 tid;
  t__auto_timeout = 1;
  tid = create_thread(dummy_thread, "", 0);

  t__with_timeout(10, (void (*)(void))resume_thread, tid);
  assert(thread_queue[ready_list].qnext == tid, resched_t[0], "FAIL - 'ready_list.qnext' does not point to new thread");
  assert(thread_queue[ready_list].qprev == tid, resched_t[0], "FAIL - 'ready_list.qprev' does not point to new thread");
  assert(!status_is(TIMEOUT), resched_t[0], "TIMEOUT -- 'resume_thread' timed out while testing resched");

  t__with_timeout(10, (void (*)(void))suspend_thread, tid);
  assert(thread_queue[ready_list].qnext == ready_list, resched_t[1], "FAIL - 'ready_list.qnext' does not point to 'ready_list'");
  assert(thread_queue[ready_list].qprev == ready_list, resched_t[1], "FAIL - 'ready_list.qnext' does not point to 'ready_list'");
  assert(!status_is(TIMEOUT), resched_t[1], "TIMEOUT -- 'suspend_thread' timed out while testing resched");

  t__mem_reset(1);
  
  tid = create_thread(resched_thread, "", 0);
  t__default_resched = 1;
  t__with_timeout(10, (void (*)(void))resume_thread, tid);
  assert(!status_is(TIMEOUT), resched_t[2], "TIMEOUT -- 'resume_thread' timed out while testing resched");
  t__auto_timeout = 0;
  t__timeout = 10;
  join_thread_local(tid);
  assert(t__timeout > 0, resched_t[2], "FAIL - Timout after resuming queued thread");
}

void t__ms4(uint32 idx) {
  t__auto_timeout = 0;
  if (idx == 0) {
    t__print("\n");
    init_tests(general_t, test_count(general_prompt));
    t__runner("general", test_count(general_prompt), general_tests);
  }
  else if (idx == 1) {
    init_tests(init_t, test_count(init_prompt));
    t__runner("initialization", test_count(init_prompt), initialize_tests);
  }
  else if (idx == 2) {
    init_tests(enqueue_t, test_count(enqueue_prompt));
    t__runner("enqueue", test_count(enqueue_prompt), enqueue_tests);
  }
  else if (idx == 3) {
    init_tests(dequeue_t, test_count(dequeue_prompt));
    t__runner("dequeue", test_count(dequeue_prompt), dequeue_tests);
  }
  else if (idx == 4) {
    init_tests(order_t, test_count(order_prompt));
    t__runner("order", test_count(order_prompt), order_tests);
  }
  else if (idx == 5) {
    init_tests(resched_t, test_count(resched_prompt));
    t__runner("resched", test_count(resched_prompt), resched_tests);
  }
  else {
    t__print("\n----------------------------\n");
    t__printer("\nGeneral Tests:", general_t, general_prompt, test_count(general_prompt));
    t__print("\nQueue Tests:\n");
    t__printer("  Initialization:", init_t, init_prompt, test_count(init_prompt));
    t__printer("  Enqueue:", enqueue_t, enqueue_prompt, test_count(enqueue_prompt));
    t__printer("  Dequeue:", dequeue_t, dequeue_prompt, test_count(dequeue_prompt));
    t__printer("  Out of Order:", order_t, order_prompt, test_count(order_prompt));
    t__printer("\nScheduling Tests:", resched_t, resched_prompt, test_count(resched_prompt));
    t__print("\n");
  }
}
