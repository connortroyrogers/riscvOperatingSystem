#include <bareio.h>
#include <barelib.h>
#include <shell.h>
#include <thread.h>
#include <queue.h>
#include <syscall.h>
#define PROMPT "bareOS$ "  /*  Prompt printed by the shell to the user  */


/*
 * 'shell' loops forever, prompting the user for input, then calling a function based
 * on the text read in from the user.
 */
byte shell(char* arg) {
  unsigned char ret = '0';
  while(1){
    printf("%s", PROMPT);
    int i = 0;
    int j;
    int k;
    int l;
    int inputBuffLength = 0;
    char input[1024];
    char inputBuff[1024];
    char command[32];
    char c;
    while(1){
      c = uart_getc();
      if(c == '\n'){
        input[i] = '\0';
        break;
      }
      else if(c == '\r'){
        input[i] = '\0';
        break;
      }
      else if(c == 8){
        if(i > 0){
          //printf("backspacing");
          i--;
          input[i] = ' ';
          printf("\b \b");
        }
      }
      else{
        input[i++] = c;
        if(i >= sizeof(input) - 1){
          input[1023] = '\0';
          break;
        }
      }
    }
    l = 0;
    //populate inputBuffer with proper formatting for $?
    while(input[l] != '\0'){
      if(input[l] == '$'){
        if(input[l+1] == '?'){
          if(ret <=9){
            inputBuff[l++] = '0' + ret;
            inputBuffLength++;
          }
          else{
            int cnt = 0;
            char buff[16];
            while(ret>0){
              buff[cnt++] = '0' + ret % 10;
              ret /= 10;
            }
            while(--cnt >= 0){
              inputBuff[l++] = buff[cnt];
              inputBuffLength++;
            }
          }
          l+=2;
        }else{
          inputBuff[l] = input[l];
          l++;
          inputBuffLength++;
        }
        //if no formatting necessary, copy directly to inputBuff array
      }else{
        inputBuff[l] = input[l];
        l++;
        inputBuffLength++;
      }
      //printf("populating inputBuff\n");
    }
    j = 0;
    while(input[j] != ' ' && input[j] != '\0'){
      //printf("putting command into array\nCurrent progress: %s\n", command);
      command[j] = input[j];
      j++;
    }
    command[j] = '\0'; 

    if((command[0] == 'h' || command[0] == 'H') && command[1] == 'e' && command[2] == 'l' 
              && command[3] == 'l' && command[4] == 'o' && (command[5] == ' ' || command[5] == '\0')){
      //uint32 tid = resume_thread(create_thread(&builtin_hello, inputBuff, inputBuffLength));
      //ret = join_thread(tid);

      uint32 tid = resume_thread(create_thread(&builtin_hello, inputBuff, inputBuffLength));
      ret = join_thread(tid);

    }
    else if((command[0] == 'e' || command[0] == 'E') && command[1] == 'c' && command[2] == 'h' 
              && command[3] == 'o' && (command[4] == ' ' || command[4] == '\0')){
      //uint32 tid = resume_thread(create_thread(&builtin_echo, inputBuff, inputBuffLength));
      uint32 tid = resume_thread(create_thread(&builtin_echo, inputBuff, inputBuffLength));
      ret = join_thread(tid);

    }
    else{
      printf("Unknown command\n");
    }
    k = 0;
    while(inputBuff[k] != '\0'){
      inputBuff[k++] = '\0';
    }
  }
  return 0;
}
