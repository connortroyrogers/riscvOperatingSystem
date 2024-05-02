#include <bareio.h>
#include <barelib.h>


/*
 * 'builtin_echo' reads in a line of text from the UART
 * and prints it.  It will continue to do this until the
 * line read from the UART is empty (indicated with a \n
 * followed immediately by another \n).
 */
byte builtin_echo(char* arg) {
  char buff[1024] = {'\0'};
  char buffArg[1024] = {'\0'};
  char c;
  int i = 0;
  int j = 5;
  int k = 0;
  int count = 0;
  //if contains arguments, write to buffer then print
  if(arg[5] != '\0' || arg[6] != '\0'){
    //printf("printing argument in echo\n");
    while(arg[j] != '\0'){
      buffArg[i++] = arg[j++];
    }
    printf("%s\n", buffArg);
    return 0;
  }else{
    //while loop to continue to print until empty line is passed
    while(1){
      //printf("Entering while loop in echo\n");
      //while loop to populate string buffer
      i = 0;
      while(1){
        
        c = uart_getc();
        if(c == '\n'){
          buff[i] = '\0';
          break;
        }
        else if(c == '\r'){
          buff[i] = '\0';
          break;
        }
        else if(c == 8){
          if(i > 0){
            i--;
            buff[i] = ' ';
            printf("\b \b");
          }
        }
        else{
          buff[i++] = c;
          count++;
          if(i >= sizeof(buff) - 1){
            buff[1023] = '\0';
            break;
          }
        }
      }
      if(buff[0] == '\0'){
        break;
      }
      else{
        printf("%s\n", buff);
      }
      while(buff[k] != '\0'){
        buff[k] = '\0';
        k++;
      }
    }
    return count;
  }
}
