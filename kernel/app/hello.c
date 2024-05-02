#include <bareio.h>
#include <barelib.h>


/*
 * 'builtin_hello' prints "Hello, <text>!\n" where <text> is the contents 
 * following "builtin_hello " in the argument and returns 0.  
 * If no text exists, print and error and return 1 instead.
 */
byte builtin_hello(char* arg) {
  char buff[1024] = {'\0'};
  if(arg[5] == '\0' || arg[6] == '\0'){
    printf("Error - bad argument\n");
    return 1;
  }
  buff[0] = 'H';
  buff[1] = 'e';
  buff[2] = 'l';
  buff[3] = 'l';
  buff[4] = 'o';
  buff[5] = ',';
  buff[6] = ' ';
  int i = 6;
  int j = 7;
  while(arg[i] != '\0'){
    buff[j++] = arg[i++];
  }
  buff[j] = '!';
  printf("%s\n", buff);
  return 0;
}
