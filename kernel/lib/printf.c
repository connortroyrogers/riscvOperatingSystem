#include <bareio.h>
#include <barelib.h>

/*
 *  In this file, we will write a 'printf' function used for the rest of
 *  the semester.  In is recommended  you  try to make your  function as
 *  flexible as possible to allow you to implement  new features  in the
 *  future.
 *  Use "va_arg(ap, int)" to get the next argument in a variable length
 *  argument list.
 */

void printf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);

  while(*format != '\0'){
    if(*format == '%'){

      switch(*(format+1)){
        //case for decimal formatting
        case 'd':{
          unsigned long val = va_arg(ap, unsigned long);
          char buff[64] = {0};
          int i = 0;
          if(val < 0){
            uart_putc('-');
            val = -val;
          }
          while(val>0){
            buff[i++] = '0' + val % 10;
            val /= 10;
          }

          while(--i >= 0){
            uart_putc(buff[i]);
          }

          format++;
          break;
        //case for hexadecimal formatting
        }case 'x':{
          unsigned long val = va_arg(ap, unsigned long);
          char buff[64] = {0};
          int i = 0;
          //add 0 to buffer if argument is 0
          if(val == 0){
            buff[i++] = '0';
          }
          /*if val is negative, convert to unsigned long of inverse and
           perform ones complement*/
          if (val < 0) {
            val = (unsigned long)(-val);
            val = ~val + 1;
          }
          /*while val > 0, get mod of val with 16, and add character to buffer
          depending on value of remainder*/
          while (val > 0) {
            int rem = val % 16;
            buff[i++] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
            val /= 16;
          }

          buff[i++] = 'x';
          buff[i++] = '0';

          while (--i >= 0) {
            uart_putc(buff[i]);
          }
          
          format++;
          break;
        //case for string formatting
        }case 's':{
          char* str = va_arg(ap, char*);
          int i = 0;
          while(str[i] != '\0'){
            uart_putc(str[i++]);
          }
          format++;
          break;
        }default:{
          uart_putc(*(format+1));
          format++;
        }
      }
    }else{
      uart_putc(*format); 
    }
    format++;
  }
  va_end(ap);
  return;
}
