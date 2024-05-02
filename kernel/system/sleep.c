#include <interrupts.h>
#include <queue.h>
#include <thread.h>
#include <syscall.h>
#include <bareio.h>

/*  Places the thread into a sleep state and inserts it into the  *
 *  sleep delta list.                                             */
int32 sleep(uint32 threadid, uint32 delay) {
  char mask;
  mask = disable_interrupts();
  if(delay == 0){
    raise_syscall(RESCHED);
    restore_interrupts(mask);
    return -1;
  }
  //dequeue if process is already queued
  thread_dequeue(thread_queue[threadid].qprev);
  //set key to delay
  thread_queue[threadid].key = delay;
  //set state to sleep
  thread_table[threadid].state = TH_SLEEP;
  //enqueue in sleep list
  thread_sleep(sleep_list, threadid);
  if(thread_queue[thread_queue[threadid].qnext].key > 0){
    thread_queue[thread_queue[threadid].qnext].key = thread_queue[thread_queue[threadid].qnext].key - thread_queue[threadid].key;
  }
  //raise syscall
  raise_syscall(RESCHED);
  restore_interrupts(mask);

  return 0;
}

/*  If the thread is in the sleep state, remove the thread from the  *
 *  sleep queue and resumes it.                                      */
int32 unsleep(uint32 threadid) {
  char mask;
  mask = disable_interrupts();
  //if state is not sleep, cannot unsleep
  if(thread_table[threadid].state != TH_SLEEP){
    return -1;
  }
  //dequeue thread from sleep list
  thread_dequeue(thread_queue[threadid].qprev);
  //increment key after dequeue
  thread_queue[thread_queue[threadid].qnext].key = thread_queue[thread_queue[threadid].qnext].key + thread_queue[threadid].key;
  //resume thread

  resume_thread(threadid);

  restore_interrupts(mask);
  return 0;
}
