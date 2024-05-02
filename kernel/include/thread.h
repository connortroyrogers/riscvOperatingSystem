#ifndef H_THREAD
#define H_THREAD

#include <barelib.h>
#define NTHREADS 20    /*  Maximum number of running threads  */

#define TH_FREE    0   /*                                                 */
#define TH_RUNNING 1   /*  Threads can be in one of several states        */
#define TH_READY   2   /*  This list will be extended as we add features  */
#define TH_SUSPEND 3   /*                                                 */
#define TH_DEFUNCT 4   /*                                                 */
#define TH_SLEEP   5

#define THREAD_STACK_SZ ((mem_end - mem_start) / 2) / NTHREADS  /*  Macro calculates the size of a thread stack  */
#define get_stack(n) mem_end - (n * THREAD_STACK_SZ)            /*  Macro gets start of stack by thread index    */


/*  Each thread has a corresponding 'thread_t' record in the 'thread_table' (see system/thread.c)  */
/*  These entries contain information about the thread                                             */
typedef struct _thread {
  char state;            /*  The current state of the thread                                         */
  uint64* stackptr;      /*  A pointer to the lowest stack address for the thread                    */
  uint32 parent;         /*  The index into the 'thread_table' of the thread's parent                */
  byte retval;           /*  The return value of the function (only valid when state == TH_DEFUNCT)  */
  uint32 priority;       /*  Thread priority (0=highest MAX_UINT32=lowest)                           */
} thread_t;

extern thread_t thread_table[];
extern uint32   current_thread;    /*  The currently running thread  */


/*  thread related prototypes  */
int32 create_thread(void* proc, char* arg, uint32 arglen);
byte join_thread(uint32);
int32 kill_thread(uint32);
int32 suspend_thread(uint32);
int32 resume_thread(uint32);

void ctxsw(uint64**, uint64**);

#endif
