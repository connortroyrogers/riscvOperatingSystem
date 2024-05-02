#include <barelib.h>
#include <bareio.h>

#define STDIN_COUNT 0x0
#define STDOUT_COUNT 0x1
#define STDOUT_MATCH 0x2
#define TIMEOUT 0x5
#define status_is(cond) (t__status & (0x1 << cond))
#define test_count(x) sizeof(x) / sizeof(x[0])
#define init_tests(x, c) for (int i=0; i<c; i++) x[i] = "OK"
#define assert(test, ls, err) if (!(test)) ls = err

void  t__print(const char*);
int   t__with_timeout(int32, void*, ...);
void  t__set_io(const char* stdin, int32 stdin_c, int32 stdout_c);
byte  t__check_io(uint32 count, const char* stdout);
char* t__raw_stdout(void);

extern uint32 t__hello_called;
extern uint32 t__echo_called;
extern uint32 t__status;

char builtin_echo(char*);
char builtin_hello(char*);
char __real_shell(char*);

void t__runner(const char* , uint32, void(*)(void));
void t__printer(const char*, char**, const char**, uint32);


static const char* general_prompt[] = {
		       "  Program Compiles:                         ",
		       "  `echo` is callable:                       ",
		       "  `hello` is callable:                      "
};
static const char* echo_prompt[] = {
		       "  Prints the argument text:                 ",
		       "  Echos a line of text:                     ",
		       "  Returns when encountering an empty line:  ",
		       "  Returns 0 when an argument is passed in:  ",
		       "  Returns the number of characters read:    "
};
static const char* hello_prompt[] = {
		       "  Prints error on no argument received:     ",
		       "  Prints the correct string:                ",
		       "  Returns 1 on error:                       ",
		       "  Returns 0 on success:                     "
};
static const char* shell_prompt[] = {
		       "  Prints error on bad command:              ",
		       "  Reads only up to newline each command:    ",
		       "  Sucessfully calls `echo`:                 ",
		       "  Successfully calls `hello`:               ",
		       "  Shell loops after command completes:      ",
		       "  Shell replaces '$?' with previous result: "
};

static char* general_t[test_count(general_prompt)];
static char* echo_t[test_count(echo_prompt)];
static char* hello_t[test_count(echo_prompt)];
static char* shell_t[test_count(shell_prompt)];

static void general_tests(void) {
  t__set_io("", -1, 20);
  t__with_timeout(10, (void (*)(void))builtin_echo, "echo stuff");
  
  assert(!status_is(TIMEOUT), general_t[1], "FAIL - Timeout occured when calling 'builtin_echo'");

  t__set_io("", -1, 20);
  t__with_timeout(10, (void (*)(void))builtin_hello, "hello stuff");

  assert(!status_is(TIMEOUT), general_t[2], "FAIL - Timeout occured when calling 'builtin_hello'");
}

static void echo_tests(void) {
  byte result;
  t__set_io("", -1, 10);
  result = t__with_timeout(10, (void (*)(void))builtin_echo, "echo Echo test");
  t__check_io(0, "Echo test\n");

  assert(status_is(STDOUT_MATCH), echo_t[0], "FAIL - Printed text does not match expected output");
  assert(status_is(STDOUT_COUNT), echo_t[0], "FAIL - Output text is not the expected length");
  assert(result == 0, echo_t[3], "FAIL - Returned non-zero value");
  assert(!status_is(TIMEOUT), echo_t[0], "TIMEOUT -- 'builtin_echo' never returned");
  assert(!status_is(TIMEOUT), echo_t[3], "TIMEOUT -- 'builtin_echo' never returned");

  t__set_io("Echo this string\n", 18, 17);
  result = t__with_timeout(10, (void (*)(void))builtin_echo, "echo");
  t__check_io(17, "Echo this string\n");

  assert(status_is(STDOUT_MATCH), echo_t[1], "FAIL - Did not print line text from stdin");

  t__set_io("differenter text\n\n", -1, 40);
  t__with_timeout(10, (void (*)(void))builtin_echo, "echo");

  assert(!status_is(TIMEOUT), echo_t[2], "TIMEOUT -- 'builtin_echo' never returned after empty newline");

  t__set_io("differenter text\n\n", -1, 40);
  result = t__with_timeout(10, (void (*)(void))builtin_echo, "echo");

  assert(result == 16, echo_t[3], "FAIL - Did not return the correct count");
  assert(!status_is(TIMEOUT), echo_t[3], "TIMEOUT -- 'builtin_echo' never returned");
}

static void hello_tests(void) {
  byte result;
  t__set_io("", -1, 21);
  result = t__with_timeout(10, (void (*)(void))builtin_hello, "hello");
  t__check_io(0, "Error - bad argument\n");
  
  assert(status_is(STDOUT_MATCH), hello_t[0], "FAIL - Text did not match the expected error");
  assert(status_is(STDOUT_COUNT), hello_t[0], "FAIL - Error text not the correct length");
  assert(result == 1, hello_t[2], "FAIL - Return value was not 1 on error");
  assert(!status_is(TIMEOUT), hello_t[0], "TIMEOUT -- 'builtin_hello' did not return");
  assert(!status_is(TIMEOUT), hello_t[2], "TIMEOUT -- 'builtin_hello' did not return");

  t__set_io("", -1, 17);
  result = t__with_timeout(10, (void (*)(void))builtin_hello, "hello username");
  t__check_io(0, "Hello, username!\n");

  assert(status_is(STDOUT_MATCH), hello_t[1], "FAIL - Text did not match the expected output");
  assert(status_is(STDOUT_COUNT), hello_t[1], "FAIL - Output text was not the expected length");
  assert(result == 0, hello_t[3], "FAIL - Return value was non-zero on success");
  assert(!status_is(TIMEOUT), hello_t[1], "TIMEOUT -- 'builtin_hello' did not return");
  assert(!status_is(TIMEOUT), hello_t[3], "TIMEOUT -- 'builtin_hello' did not return");
}

static void shell_tests(void) {
  t__set_io("badcall\n", 9, 32);
  t__with_timeout(10, (void (*)(void))__real_shell, "");
  t__check_io(9, "bareOS$ Unknown command\nbareOS$ ");

  assert(status_is(STDOUT_MATCH), shell_t[0], "FAIL - Error text did not match expected value");
  assert(status_is(STDOUT_COUNT), shell_t[0], "FAIL - Error text was not the expected length");
  assert(status_is(STDIN_COUNT), shell_t[1], "FAIL - Reads from `uart_getc` after '\\n' character");

  t__set_io("echo foobar\n", 13, 15);
  t__echo_called = 0;
  t__with_timeout(10, (void (*)(void))__real_shell, "");
  assert(t__echo_called, shell_t[2], "FAIL - hello not called from shell on 'hello'");

  t__set_io("hello value\n", 13, 23);
  t__hello_called = 0;
  t__with_timeout(10, (void (*)(void))__real_shell, "");

  assert(t__hello_called, shell_t[3], "FAIL - hello not called from shell on 'hello'");

  t__set_io("hello\necho $?\n", 15, 47);
  t__with_timeout(10, (void (*)(void))__real_shell, "");
  t__check_io(15, "bareOS$ Error - bad argument\nbareOS$ 1\nbareOS$ ");

  char* stdout = t__raw_stdout();
  assert(status_is(STDOUT_COUNT), shell_t[4], "FAIL - Shell did not loop after function");
  assert(stdout[37] == '1', shell_t[5], "FAIL - '$?' not replaced with previous return value");
}

void t__ms2(uint32 idx) {
  if (idx == 0) {
    t__print("\n");
    init_tests(general_t, test_count(general_prompt));
    t__runner("general", test_count(general_prompt), general_tests);
  }
  else if (idx == 1) {
    init_tests(echo_t, test_count(echo_prompt));
    t__runner("echo", test_count(echo_prompt), echo_tests);
  }
  else if (idx == 2) {
    init_tests(hello_t, test_count(hello_prompt));
    t__runner("hello", test_count(hello_prompt), hello_tests);
  }
  else if (idx == 3) {
    init_tests(shell_t, test_count(shell_prompt));
    t__runner("shell", test_count(shell_prompt), shell_tests);
  }
  else {
    t__print("\n----------------------------\n");
    t__printer("\nGeneral Tests:", general_t, general_prompt, test_count(general_prompt));
    t__printer("\nEcho Tests:", echo_t, echo_prompt, test_count(echo_prompt));
    t__printer("\nHello Tests:", hello_t, hello_prompt, test_count(hello_prompt));
    t__printer("\nShell Tests:", shell_t, shell_prompt, test_count(shell_prompt));
    t__print("\n");
  }
}
