#ifndef H_QUEUE
#define H_QUEUE

#include <thread.h>

/*  Certain  OS  features  require  threads  to  be  queued.  *
 *  Because each  thread can  only belong  to one queue at a  *
 *  time, all  queues are  stored  in a single  thread_queue  *
 *  list (see system/queue.c) which uses indices to point to  *
 *  the next and previous element in a given queue.           */
typedef struct _queue {
  uint32 key;            /*  An arbitrary key value for the thread, meaning depends on which queue it is in  */
  uint32 qprev;          /*  The next element in the queue                                                   */
  uint32 qnext;          /*  The previous element in the queue                                               */
} queue_t;

extern queue_t thread_queue[];
extern uint32 ready_list;
extern uint32 sleep_list;

/*  thread related prototypes  */
void thread_enqueue(uint32, uint32);
void thread_sleep(uint32, uint32);
uint32 thread_dequeue(uint32);

#endif
