#include <barelib.h>

uint32 set_interrupt(uint32);    /*  Turn on an interrupt for a specific signal  */
void restore_interrupts(char);   /*  Return the interrupts to a given state      */
void enable_interrupts(void);    /*  Turn on all interrupts                      */
char disable_interrupts(void);   /*  Turn off all interrupts                     */
char is_interrupting(void);      /*  Returns whether interrupts are enabled      */
