#include <queue.h>
#include <bareio.h>

/*  Queues in bareOS are all contained in the 'thread_queue' array.  Each queue has a "root"
 *  that contains the index of the first and last elements in that respective queue.  These
 *  roots are  found at the end  of the 'thread_queue'  array.  Following the 'qnext' index 
 *  of each element, starting at the "root" should always eventually lead back to the "root".
 *  The same should be true in reverse using 'qprev'.                                          */

queue_t thread_queue[NTHREADS+1];   /*  Array of queue elements, one per thread plus one for the read_queue root  */
uint32 ready_list = NTHREADS + 0;   /*  Index of the read_list root  */
uint32 sleep_list = NTHREADS + 1;   /*  Index of the sleep_list root */


/*  'thread_enqueue' takes an index into the thread_queue  associated with a queue "root"  *
 *  and a threadid of a thread to add to the queue.  The thread will be added to the tail  *
 *  of the queue,  ensuring that the  previous tail of the queue is correctly threaded to  *
 *  maintain the queue.                                                                    */
void thread_enqueue(uint32 queue, uint32 threadid) {
    int curr = thread_queue[queue].qnext;
    int buff = curr;
    int key = thread_table[threadid].priority;

    while (thread_queue[buff].key != thread_queue[thread_queue[queue].qnext].key) {
        if (buff == threadid) {
            return; 
        }
        buff = thread_queue[buff].qnext;
    }

    while (thread_queue[curr].key >= thread_queue[threadid].key &&
        thread_queue[curr].qnext != queue) {
        curr = thread_queue[curr].qnext;
    }

    thread_queue[threadid].key = key;  
    thread_queue[threadid].qnext = thread_queue[curr].qnext;
    thread_queue[threadid].qprev = curr;
    thread_queue[thread_queue[curr].qnext].qprev = threadid;
    thread_queue[curr].qnext = threadid;

}

void thread_sleep(uint32 queue, uint32 threadid){
    int key = thread_queue[threadid].key;
    int curr = queue;
    int buff = thread_queue[curr].qnext;
    if(key > thread_queue[buff].key){
        //check if thread is already queued
        while (buff != queue) {
            if (buff == threadid) {
                return; 
            }
            buff = thread_queue[buff].qnext;
        }
        
        //iterate while current node key is less than inserted key
        //and havent reached end of queue
        //decrement key by value of current key
        //go to next node
        while (thread_queue[curr].key <= key &&
            thread_queue[curr].qnext != queue) {
            key -= thread_queue[curr].key;
            curr = thread_queue[curr].qnext;
        }
        
        //if reached tail of queue, decrement current nodes key from key
        if(thread_queue[curr].qnext == queue && curr != queue){
            key -= thread_queue[curr].key;
        }
    }
    thread_queue[threadid].key = key;
    thread_queue[threadid].qnext = thread_queue[curr].qnext;
    thread_queue[threadid].qprev = curr;
    thread_queue[thread_queue[curr].qnext].qprev = threadid;
    thread_queue[curr].qnext = threadid;
}


/*  'thread_dequeue' takes a queue index associated with a queue "root" and removes the  *
 *  thread at the head of the queue and returns the index of that thread, ensuring that  *
 *  the queue  maintains its structure and the head correctly points to the next thread  *
 *  (if any).                                                                            */
uint32 thread_dequeue(uint32 queue) {
    if(thread_queue[queue].qnext == queue){
        return NTHREADS;
    }
    
    int poppedThread = thread_queue[queue].qnext;

    thread_queue[queue].qnext = thread_queue[poppedThread].qnext;
    thread_queue[thread_queue[poppedThread].qnext].qprev = thread_queue[poppedThread].qprev;
    
    return poppedThread;
}
