#define TTY_BUFFLEN 128

extern uint32 tty_in_sem;         /* Semaphore used to lock `tty_getc` if waiting for data                                            */
extern uint32 tty_out_sem;        /* Semaphore used to lock `tty_putc` if waiting for space in the queue                              */
extern char tty_in[TTY_BUFFLEN];  /* Circular buffer for storing characters read from the UART until requested by a thread            */
extern char tty_out[TTY_BUFFLEN]; /* Circular buffer for storing character to be written to the UART while waiting for it to be ready */
extern uint32 tty_in_head;        /* Index of the first character in `tty_in`                                                         */
extern uint32 tty_in_count;       /* Number of characters in `tty_in`                                                                 */
extern uint32 tty_out_head;       /* Index of the first character in `tty_out`                                                        */
extern uint32 tty_out_count;      /* Number of characters in `tty_out`                                                                */

void tty_init(void);
char tty_getc(void);
void tty_putc(char);
