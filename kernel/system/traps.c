#include <barelib.h>

int32 resched(void);

/*
 *  This file contains code for handling exceptions generated
 *  by the hardware   (see '__traps' in bootstrap.s)
 *
 *  We can add a new syscall by writing a function and adding
 *  it to the 'syscall_table' below.
 */

int32 (*syscall_table[]) (void) = {
                                   resched
};

void __sys_capture_syscall(void) {
  return;
}

void (*sys_syscall_hook)(void) = __sys_capture_syscall;
static int32 __exception_result = 0, __exception_signal = -1;
int32 raise_syscall(uint32 sig) {
  sys_syscall_hook();
  __exception_signal = sig;
  asm volatile("ecall");
  __exception_signal = -1;
  return __exception_result;
}

interrupt handle_exception(void) {
  if (__exception_signal < sizeof(syscall_table) / sizeof(void*))
    __exception_result = syscall_table[__exception_signal]();
}
