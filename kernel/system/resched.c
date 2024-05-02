#include <barelib.h>
#include <syscall.h>
#include <thread.h>
#include <queue.h>
#include <bareio.h>
/*  'resched' places the current running thread into the ready state  *
 *  and  places it onto  the tail of the  ready queue.  Then it gets  *
 *  the head  of the ready  queue  and sets this  new thread  as the  *
 *  'current_thread'.  Finally,  'resched' uses 'ctxsw' to swap from  *
 *  the old thread to the new thread.                                 */
int32 resched(void) {
  int buff = current_thread;

  static uint32 cnt = 0;
  int new = thread_dequeue(ready_list);
  for(int i = 0; i < NTHREADS; i++){
    if(thread_table[new].state == TH_READY){
      if(thread_table[buff].state == TH_RUNNING || thread_table[buff].state == TH_READY){
        thread_table[buff].state = TH_READY;
        thread_enqueue(ready_list, buff);
      }
      thread_table[new].state = TH_RUNNING;
      current_thread = new;
      ctxsw(&(thread_table[new].stackptr), &(thread_table[buff].stackptr));
      return 0;
    }
    cnt++;
  }
  return 0;
}


//shrechan@iu.edu