/*
* This file contains functions for initializing and handling interrupts
* from the hardware timer.
*/
#include <barelib.h>
#include <interrupts.h>
#include <queue.h>
#include <sleep.h>

#define TRAP_TIMER_ENABLE 0x80
volatile uint32* clint_timer_addr = (uint32*)0x2004000;
const uint32 timer_interval = 100000;
int64 resched(void);
/*
* This function is called as part of the bootstrapping sequence
* to enable the timer. (see bootstrap.s)
*/
void clk_init(void) {
*clint_timer_addr = timer_interval;
set_interrupt(TRAP_TIMER_ENABLE);
}
/*
* This function is triggered every 'timer_interval' microseconds
* automatically. (see '__traps' in bootstrap.s)
*/
interrupt handle_clk(void) {
    *clint_timer_addr += timer_interval;
    if (boot_complete && is_interrupting()) {
        char mask = disable_interrupts();
        uint32 buff = sleep_list;
        thread_queue[thread_queue[sleep_list].qnext].key--;
        while (thread_queue[buff].qnext != sleep_list && thread_queue[thread_queue[buff].qnext].key == 0){
            unsleep(thread_queue[buff].qnext);
            buff = thread_queue[buff].qnext;
        }
        resched();
        restore_interrupts(mask);
    }
}
