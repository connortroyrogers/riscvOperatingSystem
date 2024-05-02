#include <barelib.h>
#include <thread.h>
#include <queue.h>
#include <sleep.h>

#define TIMEOUT 0x5
#define status_is(cond) (t__status & (0x1 << cond))
#define test_count(x) sizeof(x) / sizeof(x[0])
#define init_tests(x, c) for (int i=0; i<c; i++) x[i] = "OK"
#define assert(test, ls, err) if (!(test)) ls = err

extern uint32 sleep_list;

extern byte t__auto_timeout;
extern byte t__skip_resched;
extern byte t__default_resched;
extern byte t__default_timer;
extern uint32 t__status;

void t__print(const char*);
void t__runner(const char*, uint32, void(*)(void));
void t__printer(const char*, char**, const char**, uint32);

int t__with_timeout(int32, void*, ...);
void t__mem_reset(uint32);

static const char* general_prompt[] = {
				       "  Program Compiles:                ",
};
static const char* sleep_prompt[] = {
				       "  Sleep queue is initialized:      ",
				       "  Sets thread's state:             ",
				       "  Adds thread to queue:            ",
				       "  Key set to sleep time [first]:   ",
				       "  Thread removed from ready queue: ",
				       "  Longer sleep added in place:     ",
				       "  Longer sleep key is difference:  ",
				       "  Shorter sleep added at head:     ",
				       "  Queue adjusted for new head:     ",
				       "  Intermediate sleep inserted:     ",
				       "  Intermediate sleep adjusts keys: ",
};
static const char* unsleep_prompt[] = {
				       "  Returns -1 when empty:           ",
				       "  Removed from head:               ",
				       "  Adjusts next key [head]:         ",
				       "  Remove from middle:              ",
				       "  Adjusts next key [middle]:       ",
};
static const char* resched_prompt[] = {
				       "  Clock decrements head:           ",
				       "  Zero delay threads removed:      ",
				       "  Removed multiple zero threads:   ",
				       "  Removed threads readied:         ",
};

static char* general_t[test_count(general_prompt)];
static char* sleep_t[test_count(sleep_prompt)];
static char* unsleep_t[test_count(unsleep_prompt)];
static char* resched_t[test_count(resched_prompt)];

static void resume(uint32 tid) {
  thread_table[tid].state = TH_READY;
  thread_enqueue(ready_list, tid);
}

static byte dummy_thread(char* arg) { while(1); return 0; }

static void general_tests(void) { return; }
static void sleep_tests(void) {
  t__skip_resched = 1;
  
  assert(thread_queue[sleep_list].qnext == sleep_list, sleep_t[0], "FAIL - 'sleep_list.qnext' does not point to itself");
  assert(thread_queue[sleep_list].qprev == sleep_list, sleep_t[0], "FAIL - 'sleep_list.qprev' does not point to itself");
  
  int32 tid1 = create_thread(dummy_thread, "", 0);
  resume(tid1);
  t__with_timeout(10, sleep, tid1, 120);
  
  assert(thread_table[tid1].state != TH_FREE, sleep_t[1], "FAIL - Sleeping thread's state set to FREE");
  assert(thread_table[tid1].state != TH_READY, sleep_t[1], "FAIL - Sleeping thread's state still READY");
  assert(thread_table[tid1].state != TH_RUNNING, sleep_t[1], "FAIL - Sleeping thread's state set to RUNNING");
  assert(thread_table[tid1].state != TH_SUSPEND, sleep_t[1], "FAIL - Sleeping thread's state set to SUSPEND");
  assert(thread_table[tid1].state != TH_DEFUNCT, sleep_t[1], "FAIL - Sleeping thread's state set to DEFUNCT");

  assert(thread_queue[sleep_list].qnext == tid1, sleep_t[2], "FAIL - 'sleep_list.qnext' does not point to slept thread");
  assert(thread_queue[sleep_list].qprev == tid1, sleep_t[2], "FAIL - 'sleed_list.qprev' does not point to slept thread");
  assert(thread_queue[tid1].qnext == sleep_list, sleep_t[2], "FAIL - Slept thread's 'qnext' does not point to sleep_list");
  assert(thread_queue[tid1].qprev == sleep_list, sleep_t[2], "FAIL - Slept thread's 'qprev' does not point to the sleep_list");
  assert(thread_queue[tid1].key == 120, sleep_t[3], "FAIL - Thread's key does not match sleep delay value");

  assert(thread_queue[ready_list].qnext != tid1, sleep_t[4], "FAIL - Thread still listed in ready_list");
  assert(thread_queue[ready_list].qprev != tid1, sleep_t[4], "FAIL - Thread still listed in ready_list");
  assert(!status_is(TIMEOUT), sleep_t[1], "TIMEOUT -- 'sleep' timed out during testing");
  assert(!status_is(TIMEOUT), sleep_t[2], "TIMEOUT -- 'sleep' timed out during testing");
  assert(!status_is(TIMEOUT), sleep_t[3], "TIMEOUT -- 'sleep' timed out during testing");
  assert(!status_is(TIMEOUT), sleep_t[4], "TIMEOUT -- 'sleep' timed out during testing");

  int32 tid2 = create_thread(dummy_thread, "", 0);
  resume(tid2);
  t__with_timeout(10, sleep, tid2, 140);
  
  assert(thread_queue[sleep_list].qnext == tid1, sleep_t[5], "FAIL - Sleepier thread inserted before dozing thread");
  assert(thread_queue[tid1].qprev == sleep_list, sleep_t[5], "FAIL - Sleepier thread inserted before dozing thread");
  assert(thread_queue[tid1].qnext == tid2, sleep_t[5], "FAIL - Second thread not found in sleep queue");
  assert(thread_queue[sleep_list].qprev == tid2, sleep_t[5], "FAIL - Second thread not found in sleep queue");
  assert(thread_queue[tid2].qnext == sleep_list, sleep_t[5], "FAIL - Second thread not found in sleep queue");
  assert(thread_queue[tid2].qprev == tid1, sleep_t[5], "FAIL - Second thread not found in sleep queue");

  assert(thread_queue[tid2].key == 20, sleep_t[6], "FAIL - Added thread's key is not the remaining sleep time");
  assert(thread_queue[tid2].key != 120, sleep_t[6], "FAIL - Added thread's key not adjusted by existing keys");
  assert(thread_queue[tid1].key == 120, sleep_t[6], "FAIL - Sleep adjusted the delay of a preceeding thread");
  assert(!status_is(TIMEOUT), sleep_t[5], "TIMEOUT -- 'sleep' timed out during testing");
  assert(!status_is(TIMEOUT), sleep_t[6], "TIMEOUT -- 'sleep' timed out during testing");

  int32 tid3 = create_thread(dummy_thread, "", 0);
  resume(tid3);
  t__with_timeout(10, sleep, tid3, 88);

  assert(thread_queue[sleep_list].qnext == tid3, sleep_t[7], "FAIL - Short sleep not placed at head of list");
  assert(thread_queue[tid1].qprev == tid3, sleep_t[7], "FAIL - Short sleep not placed at head of list");
  assert(thread_queue[tid1].qnext == tid2, sleep_t[7], "FAIL - Inserting short sleep broke queue threading");
  assert(thread_queue[tid2].qprev == tid1, sleep_t[7], "FAIL - Inserting short sleep broke queue threading");
  assert(thread_queue[tid2].qnext == sleep_list, sleep_t[7], "FAIL - Inserting short sleep broke queue threading");
  assert(thread_queue[tid3].qnext == tid1, sleep_t[7], "FAIL - New thread not pointing to previous head");
  assert(thread_queue[tid3].qprev == sleep_list, sleep_t[7], "FAIL - New thread does not point back to sleep_list");

  assert(thread_queue[tid3].key == 88, sleep_t[8], "FAIL - New thread's key is not set to sleep duration");
  assert(thread_queue[tid1].key == 32, sleep_t[8], "FAIL - Next thread's key was not adjusted on insert");
  assert(thread_queue[tid2].key == 20, sleep_t[8], "FAIL - Sleep adjusted the wrong thread's delay value");
  assert(!status_is(TIMEOUT), sleep_t[7], "TIMEOUT -- 'sleep' timed out during testing");
  assert(!status_is(TIMEOUT), sleep_t[8], "TIMEOUT -- 'sleep' timed out during testing");

  int32 tid4 = create_thread(dummy_thread, "", 0);
  resume(tid4);
  t__with_timeout(10, sleep, tid4, 100);

  assert(thread_queue[tid3].qnext == tid4, sleep_t[9], "FAIL - Thread was not inserted in the correct position");
  assert(thread_queue[tid1].qprev == tid4, sleep_t[9], "FAIL - Thread was not inserted in the correct position");
  assert(thread_queue[tid4].qnext == tid1, sleep_t[9], "FAIL - Thread was not inserted in the correct position");
  assert(thread_queue[tid4].qprev == tid3, sleep_t[9], "FAIL - Thread was not inserted in the correct position");

  assert(thread_queue[tid3].key == 88, sleep_t[10], "FAIL - Key of previous thread was adjusted");
  assert(thread_queue[tid4].key == 12, sleep_t[10], "FAIL - Key was not adjusted for previous delays");
  assert(thread_queue[tid1].key == 20, sleep_t[10], "FAIL - Key of the next thread was not adjusted");
  assert(!status_is(TIMEOUT), sleep_t[9], "TIMEOUT -- 'sleep' timed out during testing");
  assert(!status_is(TIMEOUT), sleep_t[10], "TIMEOUT -- 'sleep' timed out during testing");
}

static void unsleep_tests(void) {
  t__skip_resched = 1;
  t__default_resched = 1;

  uint32 timeout = 0;
  int32 result = t__with_timeout(10, unsleep, 1);

  assert(result == -1, unsleep_t[0], "FAIL - Did not return -1 when emtpy");
  assert(!status_is(TIMEOUT), unsleep_t[0], "TIMEOUT -- 'unsleep' timed out during testing");

  t__mem_reset(1);
  int32 tid1 = create_thread(dummy_thread, "", 0), tid2 = create_thread(dummy_thread, "", 0);
  int32 tid3 = create_thread(dummy_thread, "", 0), tid4 = create_thread(dummy_thread, "", 0);
  resume(tid1);
  resume(tid2);
  resume(tid3);
  resume(tid4);

  t__with_timeout(10, sleep, tid1, 5);
  timeout |= status_is(TIMEOUT);
  t__with_timeout(10, sleep, tid2, 10);
  timeout |= status_is(TIMEOUT);
  t__with_timeout(10, sleep, tid3, 15);
  timeout |= status_is(TIMEOUT);
  t__with_timeout(10, sleep, tid4, 20);
  timeout |= status_is(TIMEOUT);

  t__with_timeout(10, unsleep, tid1);
  timeout |= status_is(TIMEOUT);

  assert(thread_queue[sleep_list].qnext == tid2, unsleep_t[1], "FAIL - Unexpected thread at head of sleep_list");
  assert(thread_queue[tid2].qprev == sleep_list, unsleep_t[1], "FAIL - Unexpected thread at head of sleep_list");
  assert(thread_queue[tid2].key == 10, unsleep_t[2], "FAIL - Key for new head was not expected value");
  assert(thread_queue[tid2].key != 5, unsleep_t[2], "FAIL - Key for new head was not adjusted");
  assert(!timeout, unsleep_t[1], "TIMEOUT -- 'unsleep' timed out during testing");
  assert(!timeout, unsleep_t[2], "TIMEOUT -- 'unsleep' timed out during testing");

  t__with_timeout(10, unsleep, tid3);

  assert(thread_queue[sleep_list].qnext == tid2, unsleep_t[3], "FAIL - remove from middle of list changed head");
  assert(thread_queue[tid2].qnext == tid4, unsleep_t[3], "FAIL - Threading of list wrong after removal");
  assert(thread_queue[tid2].qnext != tid3, unsleep_t[3], "FAIL - Removed thread still in list");
  assert(thread_queue[tid2].key == 10, unsleep_t[4], "FAIL - Head's key value changed");
  assert(thread_queue[tid4].key == 10, unsleep_t[4], "FAIL - Key for tail was not expected value");
  assert(thread_queue[tid4].key != 5, unsleep_t[4], "FAIL - Key for tail was not adjusted");
}

static void wait_for_tick(int32 tid, int32 target) {
  while (thread_queue[tid].key > target)
    continue;
}

static void resched_tests(void) {
  t__skip_resched = 1;
  t__default_timer = 1;
  int32 tid, tid2;
  tid = create_thread(dummy_thread, "", 0);
  resume(tid);
  t__with_timeout(10, sleep, tid, 10);
  t__with_timeout(20, wait_for_tick, tid, 9);

  assert(!status_is(TIMEOUT), resched_t[0], "FAIL - Thread was not decremented on clock tick");

  t__mem_reset(1);

  tid = create_thread(dummy_thread, "", 0);
  resume(tid);
  t__with_timeout(10, sleep, tid, 10);
  t__with_timeout(20, wait_for_tick, tid, 0);

  assert(thread_queue[sleep_list].qnext == sleep_list, resched_t[1], "FAIL - Sleep list still contains thread");
  assert(thread_table[tid].state == TH_READY, resched_t[3], "FAIL - Thread with 0 delay was not readied");
  assert(!status_is(TIMEOUT), resched_t[1], "TIMEOUT -- Test timed out while waiting for thread to wake");
  assert(!status_is(TIMEOUT), resched_t[3], "TIMEOUT -- Test timed out while waiting for thread to wake");

  t__mem_reset(1);
  tid = create_thread(dummy_thread, "", 0);
  tid2 = create_thread(dummy_thread, "", 0);
  resume(tid);
  resume(tid2);
  t__with_timeout(10, sleep, tid, 1);
  t__with_timeout(10, sleep, tid2, 1);
  t__with_timeout(20, wait_for_tick, tid, 0);

  assert(thread_queue[sleep_list].qnext == sleep_list, resched_t[2], "FAIL - Sleep list still contains threads");
  assert(!status_is(TIMEOUT), resched_t[2], "TIMEOUT -- Test timed out while waiting for thread to wake");
  t__default_timer = 0;
}

void t__ms5(uint32 idx) {
  if (idx == 0) {
    t__print("\n");
    init_tests(general_t, test_count(general_prompt));
    t__runner("general", test_count(general_prompt), general_tests);
  }
  else if (idx == 1) {
    init_tests(sleep_t, test_count(sleep_prompt));
    t__runner("sleep", test_count(sleep_prompt), sleep_tests);
  }
  else if (idx == 2) {
    init_tests(unsleep_t, test_count(unsleep_prompt));
    t__runner("unsleep", test_count(unsleep_prompt), unsleep_tests);
  }
  else if (idx == 3) {
    init_tests(resched_t, test_count(resched_prompt));
    t__runner("resched", test_count(resched_prompt), resched_tests);
  }
  else {
    t__print("\n----------------------------\n");
    t__printer("\nGeneral Tests:", general_t, general_prompt, test_count(general_prompt));
    t__printer("\nSleep Tests:", sleep_t, sleep_prompt, test_count(sleep_prompt));
    t__printer("\nUnsleep Tests:", unsleep_t, unsleep_prompt, test_count(unsleep_prompt));
    t__printer("\nResched Tests:", resched_t, resched_prompt, test_count(resched_prompt));
    t__print("\n");
  }
}
