#include <barelib.h>
#include <interrupts.h>
#include <syscall.h>
#include <thread.h>
#include <queue.h>

/*  Takes a index into the thread table of a thread to suspend.  If the thread is  *
 *  not in the  running or  ready state,  returns an error.   Otherwise, sets the  *
 *  thread's  state  to  suspended  and  raises a  RESCHED  syscall to schedule a  *
 *  different thread.  Returns the threadid to confirm suspension.                 */
int32 suspend_thread(uint32 threadid) {
  char mask;
  mask = disable_interrupts();
  if(thread_table[threadid].state != TH_RUNNING && thread_table[threadid].state != TH_READY){
    return 0;
  }else{
    thread_dequeue(thread_queue[threadid].qprev);
    thread_table[threadid].state = TH_SUSPEND;
    raise_syscall(RESCHED);
    restore_interrupts(mask);
    return threadid;
  }
}
