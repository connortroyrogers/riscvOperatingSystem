#include <barelib.h>
#if MILESTONE_IMP >= 3
#include <thread.h>
#endif
#if MILESTONE_IMP >= 4
#include <queue.h>
#endif

void t__reset(uint32);

char __real_uart_putc(char);
void t__print(const char* str) {
  while (*str != '\0') __real_uart_putc(*str++);
}

char* t__strcpy(char* str, const char* value) {
  while ((*(str++) = *(value++)) != '\0');
  return str - 1;
}

char* t__hexcpy(char* str, unsigned int v) {
  int len = 0;
  int vals[20];
  for (int i=0; i<20; i++) vals[i] = 0;
  while (v) {
    vals[len++] = v % 16;
    v = v / 16;
  }
  int l = len - 1;
  for (; l >=0; l--) {
    *(str++) = vals[l] <= 9 ? vals[l] + '0' : 'a' + (vals[l] - 10);
  }
  return str;
}

char* t__intcpy(char* str, int v) {
  int len = 0;
  int vals[20];
  if (v < 0)
    *(str++) = '-';
  for (int i=0; i<20; i++) vals[i] = 0;
  while (v) {
    vals[len++] = v % 10;
    v = v / 10;
  }
  int l = len - 1;
  for (; l >=0; l--)
    *(str++) = vals[l] + '0';
  return str;
}

void t__mem_reset(uint32 start) {
#if MILESTONE_IMP >= 3
  for (int i=start; i<NTHREADS; i++) {
    thread_table[i].state = TH_FREE;
    thread_table[i].parent = 0;
    thread_table[i].priority = 0;
    thread_table[i].retval = 0;
    thread_table[i].stackptr = 0;
  }  
#endif
#if MILESTONE_IMP >= 4
  for (int i=start; i<NTHREADS+1; i++) {
    thread_queue[i].qnext = thread_queue[i].qprev = i;
  }
#endif
#if MILESTONE_IMP >= 5
  thread_queue[NTHREADS+1].qnext = thread_queue[NTHREADS+1].qprev = NTHREADS+1;
#endif
}

static uint32 run_idx = 0;
void t__runner(const char* label, uint32 count, void (*runner)(void)) {
  char num[10];
  for (int i=0; i<10; i++) num[i] = '\0';
  t__intcpy(num, (int)count);
  t__print("Running [");
  t__print(num);
  t__print("] ");
  t__print(label);
  t__print(" tests...");
  runner();
  t__print(" DONE\n");
  t__reset(++run_idx);
}

void t__printer(const char* header, char* out[], const char* in[], uint32 count) {
  t__print(header);
  t__print("\n");
  for (int i=0; i<count; i++) {
    t__print(in[i]);
    t__print(out[i]);
    t__print("\n");
  }
}
