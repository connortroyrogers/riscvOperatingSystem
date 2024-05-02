OUTPUT_ARCH(riscv)
ENTRY(_start)

/*
 * This file tells the compiler what address in memory to place each function
 * and global variable.  It also allows us to save certain memory addresses
 * into variables for use in C code (see _mmap_text_start)
 */


/*
  MEMORY MAP Globals
  
  _mmap_text_start  - Lowest address for the text segment
  _mmap_text_end    - Address after text segment

  _mmap_data_start  - Lowest address for the data segment
  _mmap_write_start - Addresses below this point should be READ ONLY
  _mmap_global_ptr  - Address to initialize the RISC-V 'gp' register
  _mmap_data_end    - Address after the data segment

  _mmap_bss_start   - Lowest address for the bss segment
  _mmap_bss_end     - Address after the bss segment

  _mmap_kstack_top  - Top of the Kernel stack
  _mmap_mem_start   - Lowest address of the global heap/stack segment
  _mmap_mem_end     - Last address of the heap/stack segment (This differs from text/dat/bss_end)
*/

MEMORY {
	_sysmem(WXA!RI) : ORIGIN =0x80000000, LENGTH =128M
}

SECTIONS {
	.text : {
	    PROVIDE(_mmap_text_start = ABSOLUTE(.));
	    *(.text.entry)
	    *(.text .text.*)
	    . = ALIGN(4);
	    PROVIDE(_mmap_text_end = ABSOLUTE(.));
	} > _sysmem

	.rodata BLOCK(0x1000) : {
	    PROVIDE(_mmap_data_start = ABSOLUTE(.));
	    *(.srodata .srodata.*)
	    *(.rodata .rodata.*)
	} > _sysmem

	.data : {
	    PROVIDE(_mmap_write_start = ABSOLUTE(.));
	    *(.sdata .sdata.*)
	    *(.data .data.*)
	    PROVIDE(_mmap_data_end = ABSOLUTE(.));
	} > _sysmem

	.bss : {
	    PROVIDE(_mmap_bss_start = ABSOLUTE(.));
	    *(.sbss .sbss.*)
	    *(.bss .bss.*)
	    PROVIDE(_mmap_bss_end = ABSOLUTE(.));
	} > _sysmem

	. += 0x8000;
	PROVIDE(_mmap_kstack_top = ABSOLUTE(.));
	. += 0x1000;

	PROVIDE(_mmap_global_ptr = _mmap_data_start + 0x0800);
	PROVIDE(_mmap_mem_start = ABSOLUTE(.));
	PROVIDE(_mmap_mem_end = ORIGIN(_sysmem) + (LENGTH(_sysmem)) - 1);
}
