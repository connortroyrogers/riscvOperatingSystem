#include <barelib.h>
#include <interrupts.h>
#include <tty.h>

#define UART_PRIO_ADDR        0xc000028   /*  These  values and  addresses  are used to  setup  */
#define UART_ENABLE           0x400       /*  the UART on the PLIC.  The addresses must be set  */
#define EXTERNAL_ENABLED_ADDR 0xc002000   /*  to certain  values to enable the UART  hardware.  */

#define UART0_BAUD 115200                 /* Configuration parameters for the UART.  BAUD and   */
#define UART0_FREQ 1843200                /* FREQ are factors for the rate to send characters   */

#define UART0_CFG_REG 0x10000000          /*  This addresses are for the NS16550  UART module  */
#define UART0_RW_REG   0x0                /*  Each device on the PLIC has dedicated addresses  */
#define UART0_INTR_REG 0x1                /*  tied to hardware registers.  These are the ones  */
#define UART0_RW_H_REG 0x2                /*  used by the UART.                                */
#define UART0_INT_STAT 0x2                /*                                                   */
#define UART0_CTRL_REG 0x3                /*                                                   */
#define UART0_MODEM    0x4                /*  For  example, RW_REG  reads and  writes a  byte  */
#define UART0_STAT_REG 0x5                /*  to/from the UART.                                */

#define UART_RX_ON    0x01                /*                                                   */
#define UART_TX_ON    0x02                /*                                                   */
#define UART_TRIGGER  0x08                /*  These are common values sent to UART registers   */
#define UART_CFG_ON   0x80                /*  during normal operation                          */
#define UART_8BIT     0x03                /*                                                   */
#define UART_PARITY   0x08                /*                                                   */
#define UART_IDLE     0x20                /*                                                   */

#define UART_RX_INTR  0x4                 /*  UART interrupt code for "received data ready"    */
#define UART_TX_INTR  0x2                 /*  UART interrupt code for "transmit reg empty"     */
#define UART_INT_MASK 0xE                 /*  Mask for extracting interrupt data from reg      */

static char readchar = NULL;
volatile byte* uart;


/* DO NOT Call these functions directly, ever.  Use `uart_putc` and `uart_getc` instead */
char __sys_putc(char ch) {
  while ((uart[UART0_STAT_REG] & UART_IDLE) == 0);     /*  Wait for the UART to be idle  */
  return uart[UART0_RW_REG] = (ch & 0xff);             /*  Send character to the UART    */
}
char __sys_getc(void) {
  char ch;                                             /*  This function synchronously waits for      */
  while ((ch = readchar) == NULL);                     /*  a value to be placed  into 'readchar'      */
  readchar = NULL;                                     /*      (see 'uart_handler')                   */
  ch = (ch == '\r' ? '\n' : ch);                       /*  Replace the CR character with newline      */
  __sys_putc(ch);                                      /*                                             */
  return ch;                                           /*  Return the character read.                 */
}


char (*sys_putc_hook)(char) = __sys_putc;
char (*sys_getc_hook)(void) = __sys_getc;
char uart_putc(char ch) {
  return sys_putc_hook(ch);                            /*  Hook for monkeypatching grading scripts    */
}

char uart_getc(void) {
  return sys_getc_hook();
}

/*  This function is used to enable or disable interrupt generation on the NS16550  *
 *  It is NOT modifying the behavior of the OS level interrupt handling.  This will *
 *  not - for instance - disable timer interrupts, only UART TX interrupts          */
void set_uart_interrupt(byte enabled) {
  char mask = disable_interrupts();                                                /*  Save and disable global interrupts           */
  byte state = uart[UART0_INTR_REG];
  uart[UART0_INTR_REG] = (enabled ? (state | UART_TX_ON) : (state & ~UART_TX_ON));  /*  Set the "write ready" interrupt on the UART  */
  restore_interrupts(mask);                                                        /*  Restore the state of the global interrupts   */
}

/*
 *  This function is automatically called in response to an external interrupt on the PLIC
 *     (see '__traps' in bootstrap.s)
 */
void uart_handler(void) {
  byte code = uart[UART0_INT_STAT] & UART_INT_MASK;
  if (code == UART_RX_INTR){
    //if buffer count is currently less than buffer length proceed
    if(tty_in_count < TTY_BUFFLEN){
      //read character
      readchar = uart[UART0_RW_REG];
      //insert character into tty_in
      tty_in[(tty_in_head + tty_in_count) % TTY_BUFFLEN] = readchar;
      //increment tty counter
      tty_in_count++;
    }
  }
  else if (code == UART_TX_INTR){
    //if out buffer is not empty proceed
    if (tty_out_count > 0) {
      //get character from buffer
      char ch = tty_out[tty_out_head];
      //set character in to uart buffer
      uart[UART0_RW_REG] = ch;
      //increment buffer head
      tty_out_head = (tty_out_head + 1) % TTY_BUFFLEN; // Move head pointer in tty_out buffer
      //decrement buffer count
      tty_out_count--;
      set_uart_interrupt(0);
    }else{
      set_uart_interrupt(0);
    }
  }
}

/*
 *  This function must be called to initially set up the UART hardware.
 */
void uart_init(void) {
  uint32* plic_prio_addr = (uint32*)UART_PRIO_ADDR;
  uint32* plic_enabled_addr = (uint32*)EXTERNAL_ENABLED_ADDR;

  uint32 divisor = UART0_FREQ / (16 * UART0_BAUD);     /*  Calculate divisor factor for UART speed    */
  *plic_prio_addr = *plic_prio_addr | 0x7;                  /*                                   */
  *plic_enabled_addr = *plic_enabled_addr | UART_ENABLE;    /*    Enable the UART on the PLIC    */

  uart = (byte*)UART0_CFG_REG;                         /*  Set the location of the UART in memory     */
  uart[UART0_CTRL_REG] = UART_CFG_ON;                  /*  Switch  UART  to  configuration   mode     */
  uart[UART0_RW_REG]   = divisor & 0xff;               /*  Set the UART speed to the high and low     */
  uart[UART0_RW_H_REG] = (divisor >> 8) & 0xff;        /*  registers.                                 */
  uart[UART0_CTRL_REG] = UART_PARITY | UART_8BIT;      /*  Enable parity and 8-bit mode and start     */
  uart[UART0_INTR_REG] = UART_RX_ON;                   /*  listening for characters.                  */
  uart[UART0_MODEM]   |= UART_TRIGGER;                 /*                                             */
}
