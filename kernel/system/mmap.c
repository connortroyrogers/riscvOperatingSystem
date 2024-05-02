#include <barelib.h>

/*
 *  This file formalizes the symbols created in kernel.ld into variables
 *  we can use elsewhere in our code.
 */

/*
  _mmap_text_start  - Lowest address for the text segment
  _mmap_text_end    - Address after text segment

  _mmap_data_start  - Lowest address for the data segment
  _mmap_data_end    - Address after the data segment

  _mmap_bss_start   - Lowest address for the bss segment
  _mmap_bss_end     - Address after the bss segment

  _mmap_mem_start   - Lowest address of the global heap/stack segment
  _mmap_mem_end     - Last address of the heap/stack segment (This differs from text/dat/bss_end)

*/

extern uint32 _mmap_text_start;
extern uint32 _mmap_text_end;
extern uint32 _mmap_data_start;
extern uint32 _mmap_data_end;
extern uint32 _mmap_bss_start;
extern uint32 _mmap_bss_end;
extern uint32 _mmap_mem_start;
extern uint32 _mmap_mem_end;

uint32* text_start = (uint32*)&_mmap_text_start;
uint32* text_end   = (uint32*)&_mmap_text_end;
uint32* data_start = (uint32*)&_mmap_data_start;
uint32* data_end   = (uint32*)&_mmap_data_end;
uint32* bss_start  = (uint32*)&_mmap_bss_start;
uint32* mem_start  = (uint32*)&_mmap_mem_start;
uint32* mem_end    = (uint32*)&_mmap_mem_end;
