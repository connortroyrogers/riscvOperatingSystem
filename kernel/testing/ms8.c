#include <barelib.h>
#include <tty.h>

#define UART_RX_INTR 0x4
#define UART_TX_INTR 0x2

#define UART_CHAR   0x0
#define UART_ENABLE 0x1
#define UART_INTR   0x2

#define TIMEOUT 0x5
#define status_is(cond) (t__status & (0x1 << cond))
#define test_count(x) sizeof(x) / sizeof(x[0])
#define init_tests(x, c) for (int i=0; i<c; i++) x[i] = "OK"
#define assert(test, ls, err) if (!(test)) ls = err
#define runner(txt, n) do {				   \
    init_tests(n ## _t, test_count(n ## _prompt));         \
    t__runner(txt, test_count(n ## _prompt), n ## _tests); \
  } while(0)
#define feedback(txt, n) t__printer("\n" txt " Tests:", n ## _t, n ## _prompt, test_count(n ## _prompt))

extern volatile byte* uart;
void uart_init(void);

void t__print(const char*);
void t__runner(const char*, uint32, void(*)(void));
void t__printer(const char*, char**, const char**, uint32);

int t__with_timeout(int32, void*, ...);
void __real_uart_handler(void);

extern byte t__wait_called;
extern byte t__post_called;
extern byte t__tty_init_called;
extern byte t__uart_interrupts;
extern byte t__skip_resched;
extern byte t__block_wait;
extern byte t__enable_uart;
extern int32 t__status;

/*
  NOTES:
  - uart needs to be made non-static for this code to work
  
  UPDATES:
  - ms8.c
  - wrappers.c
  - Makefile
*/

static const char* general_prompt[] = {
				       "  Program Compiles:            ",
				       "  TTY initialized during boot: ",
};
static const char* getc_prompt[] = {
				       "  Thread blocks when no char available:   ",
				       "  Thread does not block when chars exist: ",
				       "  [Bonus] Semephore awaited in getc:      ",
				       "  In buffer count modified:               ",
				       "  In buffer head was modified:            ",
				       "  Returns the correct character:          ",
};
static const char* putc_prompt[] = {
				       "  Thread blocks when buffer is full:             ",
				       "  Thread does not block when buffer is not full: ",
				       "  [Bonus] Semaphore awaited in putc:             ",
				       "  Character is added to the buffer:              ",
				       "  Out buffer count was modified:                 ",
				       "  UART interrupts enabled on call:               ",
};
static const char* rx_prompt[] = {
				       "  RX handler returns if buffer is full: ",
				       "  Character placed into buffer:         ",
				       "  RX handler modifies in buffer count:  ",
				       "  [Bonus] Semaphore notifies getc:      ",
};
static const char* tx_prompt[] = {
				       "  TX handler sends character:           ",
				       "  TX handler modifies out buffer count: ",
				       "  TX handler modifies out buffer head:  ",
				       "  Disables uart interrupts on emtpy:    ",
				       "  [Bonus] Semaphore notifies putc:      ",
};

static char* general_t[test_count(general_prompt)];
static char* getc_t[test_count(getc_prompt)];
static char* putc_t[test_count(putc_prompt)];
static char* rx_t[test_count(rx_prompt)];
static char* tx_t[test_count(tx_prompt)];

static volatile byte* old_uart = 0x0;
static byte uart_emulator[6];
static void uart_setup(void) {
  if (old_uart == 0x0)
    old_uart = uart;
  uart = uart_emulator;
  uart[UART_ENABLE] = 0x01;
}
static void uart_reset(void) {
  uart = old_uart;
}
static void uart_set(char ch, byte intr_enable, byte intr_triggered) {
  uart[UART_CHAR]   = ch;
  uart[UART_ENABLE] = uart[1] | (0x2 * intr_enable);
  uart[UART_INTR]   = intr_triggered;
}

static void general_tests(void) {
  assert(t__tty_init_called > 0, general_t[1], "FAIL - TTY was not initialized");
}

static void getc_tests(void) {
  t__block_wait = 1;
  t__with_timeout(10, tty_getc);
  assert(status_is(TIMEOUT), getc_t[0], "FAIL - Thread did not block");
  assert(t__wait_called > 0, getc_t[2], "FAIL - Semaphore was not awaited");
  t__block_wait = 0;

  tty_in[3] = 'a';
  tty_in[4] = 'p';
  tty_in[5] = 'p';
  tty_in[6] = '\n';
  tty_in_count = 4;
  tty_in_head = 3;
  char result = t__with_timeout(10, tty_getc);
  assert(!status_is(TIMEOUT), getc_t[1], "FAIL - Thread blocked even when character exists");
  assert(result == 'a', getc_t[6],       "FAIL - Received unexpected character from getc");
  assert(tty_in_count == 3, getc_t[4],   "FAIL - Count was not the correct count after getc");
  assert(tty_in_count != 4, getc_t[4],   "FAIL - Count was not changed");
  assert(tty_in_head == 4, getc_t[5],    "FAIL - Head was not adjusted to the correct offset");
  assert(tty_in_head != 3, getc_t[5],    "FAIL - Head was not changed");
}

static void putc_tests(void) {
  t__enable_uart = 0;
  t__block_wait = 1;
  t__wait_called = 0;
  tty_out_count = TTY_BUFFLEN;
  tty_out_head = 0;
  for (int i=0; i<TTY_BUFFLEN; i++)
    tty_out[i] = '\0';
  t__with_timeout(10, tty_putc, 'a');
  for (int i=0; i<TTY_BUFFLEN; i++)
    assert(tty_out[i] != 'a', putc_t[0], "FAIL - character was placed in buffer before blocking");
  assert(status_is(TIMEOUT), putc_t[0], "FAIL - Thread did not block");
  assert(t__wait_called > 0, putc_t[2], "FAIL - Semaphore was not waited");

  t__block_wait = 0;
  tty_out_count = 0;
  tty_out_head = 4;
  for (int i=0; i<TTY_BUFFLEN; i++)
    tty_out[i] = '\0';
  t__with_timeout(10, tty_putc, 'a');
  assert(!status_is(TIMEOUT), putc_t[1], "FAIL - Process blocked when space was available");
  assert(tty_out[tty_out_head] == 'a', putc_t[3], "FAIL - Character not found at head");
  assert(tty_out_count == 1, putc_t[4], "FAIL - Out count was not incremented");

  t__uart_interrupts = 0;
  t__with_timeout(10, tty_putc, 'b');
  assert(tty_out[tty_out_head + 1] == 'b', putc_t[4], "FAIL - Second putc was not placed in ring buffer correctly");
  assert(t__uart_interrupts | 0x2, putc_t[5], "FAIL - Interrupts were not enabled");
  t__enable_uart = 1;
}

static void rx_tests(void) {
  uart_setup();
  
  for (int i=0; i < TTY_BUFFLEN; i++)
    tty_in[i] = '\0';
  tty_in_count = TTY_BUFFLEN;
  tty_in_head = 0;

  uart_set('a', 0, UART_RX_INTR);
  t__with_timeout(10, __real_uart_handler);
  assert(tty_in_count == TTY_BUFFLEN, rx_t[0], "FAIL - In buffer count was changed even when full");
  assert(tty_in_head == 0, rx_t[0], "FAIL - In buffer head changed even when full");
  for (int i=0; i<TTY_BUFFLEN; i++)
    assert(tty_in[i] == '\0', rx_t[0], "FAIL - In buffer content changed even when full");

  for (int i=0; i < TTY_BUFFLEN; i++)
    tty_in[i] = '\0';
  tty_in_count = 0;
  tty_in_head = 2;
  t__post_called = 0;
  uart_set('a', 0, UART_RX_INTR);
  t__with_timeout(10, __real_uart_handler);
  assert(tty_in[2] == 'a', rx_t[1], "FAIL - Written character was not placed at in buffer head");
  assert(tty_in_count == 1, rx_t[1], "FAIL - In buffer count was not correct after character in");
  assert(tty_in_count > 0, rx_t[2], "FAIL - In buffer count was not increased");

  for (int i=0; i < TTY_BUFFLEN; i++)
    tty_in[i] = '\0';
  tty_in_count = 0;
  tty_in_head = 0;
  t__post_called = 0;
  uart_set('\n', 0, UART_RX_INTR);
  t__with_timeout(10, __real_uart_handler);
  assert(t__post_called == 1, rx_t[3], "FAIL - Semaphore was not signaled on character in");
  
  uart_reset();
}

static void tx_tests(void) {
  uart_setup();

  for (int i=0; i < TTY_BUFFLEN; i++)
    tty_out[i] = '\0';
  tty_out_count = 1;
  tty_out_head = 0;
  tty_out[0] = 'c';

  uart_set('a', 1, UART_TX_INTR);
  t__post_called = 0;
  t__with_timeout(10, __real_uart_handler);
  assert(uart[UART_CHAR] == 'c', tx_t[0], "FAIL - Did not receive correct character on uart");
  assert((uart[UART_ENABLE] & 0x2) == 0x0, tx_t[3], "FAIL - Interrupt still enabled after handler completes");
  assert(tty_out_count == 0, tx_t[1], "FAIL - Out buffer count is incorrect");
  assert(tty_out_count < 1, tx_t[1], "FAIL - Out buffer count is too large");
  assert(tty_out_head == 1, tx_t[2], "FAIL - Out buffer head is incorrect");
  assert(tty_out_head != 0, tx_t[2], "FAIL - Out buffer head still points to the same index");
  assert(t__post_called > 0, tx_t[4], "FAIL - Semaphore was not signaled on write");
  
  uart_reset();
}

void t__ms8_setup(void) {
  uart_setup();
}

void t__ms8(uint32 idx) {
  t__skip_resched = 1;
  if (idx == 0) {
    t__print("\n");
    runner("general", general);
  }
  else if (idx == 1)
    runner("getc", getc);
  else if (idx == 2)
    runner("putc", putc);
  else if (idx == 3)
    runner("handler (receive)", rx);
  else if (idx == 4)
    runner("handler (transmit)", tx);
  else {
    t__enable_uart = 1;
    t__print("\n----------------------------\n");
    feedback("General", general);
    feedback("getc", getc);
    feedback("putc", putc);
    feedback("handler (receive)", rx);
    feedback("handler (transmit)", tx);
    t__print("\n");
  }
}
