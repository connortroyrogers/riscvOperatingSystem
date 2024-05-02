#include <interrupts.h>
#include <malloc.h>
#include <barelib.h>
#include <fs.h>

void* memcpy(void*, void*, int);

static bdev_t ramdisk;             /* After initialization, contains metadata about the block device */
static char* ramfs_blocks = NULL;  /* A pointer to the actual memory used as the block device        */

uint32 bs_mk_ramdisk(uint32 blocksize, uint32 numblocks) {                    /*                               */
  char mask = disable_interrupts();                                           /*  Initialize the block device  */
  ramdisk.blocksz = (blocksize == NULL ? MDEV_BLOCK_SIZE : blocksize);        /*  This  sets  the block  size  */
  ramdisk.nblocks = (numblocks == NULL ? MDEV_NUM_BLOCKS : numblocks);        /*  and block count for  future  */
  ramfs_blocks = malloc(ramdisk.blocksz * ramdisk.nblocks);                   /*  reference.   And  allocates  */
  restore_interrupts(mask);                                                   /*  the memory  for the  device  */
  return (ramfs_blocks == (void*)-1 ? -1 : 0);                                /*  itself.                      */
}                                                                             /*                               */

bdev_t bs_stats(void) {   /*                                                            */
  return ramdisk;         /*  External accessor function for the block device metadata  */
}                         /*                                                            */

uint32 bs_free_ramdisk(void) {  /*                                        */
  char mask;                    /*                                        */
  if (ramfs_blocks == NULL) {   /*                                        */
    return -1;                  /*                                        */
  }                             /*  Free memory used by the block device  */
  mask = disable_interrupts();  /*                                        */
  free(ramfs_blocks);           /*                                        */
  restore_interrupts(mask);     /*                                        */
  return 0;                     /*                                        */
}

uint32 bs_read(uint32 block, uint32 offset, void* buf, uint32 len) {    /*                                   */
  char mask = disable_interrupts();                                     /*                                   */
  if (offset < 0 || offset + len > ramdisk.blocksz ||                   /*  Check if the block is valid      */
      block < 0 || block >= ramdisk.nblocks ||                          /*  if not valid, restore interrupts */
      ramfs_blocks == NULL) {                                           /*  and return error.                */
    restore_interrupts(mask);                                           /*                                   */
    return -1;                                                          /*                                   */
  }                                                                     /*                                   */
  memcpy(buf, &(ramfs_blocks[block * ramdisk.blocksz]) + offset, len);  /*  Copy the data from the block to  */
  restore_interrupts(mask);                                             /*  the output buffer.               */
  return 0;                                                             /*                                   */
}                                                                       /*                                   */

uint32 bs_write(uint32 block, uint32 offset, void* buf, uint32 len) {   /*                                   */
  char mask = disable_interrupts();                                     /*                                   */
  if (offset < 0 || offset + len > ramdisk.blocksz ||                   /*  Check if the block is valid      */
      block < 0 || block >= ramdisk.nblocks ||                          /*  if not valid, restore interrupts */
      ramfs_blocks == NULL) {                                           /*  and return error.                */
    restore_interrupts(mask);                                           /*                                   */
    return -1;                                                          /*                                   */
  }                                                                     /*                                   */
  memcpy(&(ramfs_blocks[block * ramdisk.blocksz]) + offset, buf, len);  /*  Copy the data from the buffer    */
  restore_interrupts(mask);                                             /*  to the block.                    */
  return 0;                                                             /*                                   */
}                                                                       /*                                   */
