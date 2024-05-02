#include <barelib.h>
#include <malloc.h>
#include <interrupts.h>
#include <fs.h>

fsystem_t* fsd = NULL;
filetable_t oft[NUM_FD];

void* memset(void*, int, int);

void fs_setmaskbit(uint32 x) {                     /*                                           */
  if (fsd == NULL) return;                         /*  Sets the block at index 'x' as used      */
  fsd->freemask[x / 8] |= 0x1 << (x % 8);          /*  in the free bitmask.                     */
}                                                  /*                                           */

void fs_clearmaskbit(uint32 x) {                   /*                                           */
  if (fsd == NULL) return;                         /*  Sets the block at index 'x' as unused    */
  fsd->freemask[x / 8] &= ~(0x1 << (x % 8));       /*  in the free bitmask.                     */
}                                                  /*                                           */

uint32 fs_getmaskbit(uint32 x) {                   /*                                           */
  if (fsd == NULL) return -1;                      /*  Returns the current value of the         */
  return (fsd->freemask[x / 8] >> (x % 8)) & 0x1;  /*  'x'th block in the block device          */
}                                                  /*  0 for unused 1 for used.                 */


/*  Build the file system and save it to a block device.  *
 *  Must be called before the filesystem can be used      */
void fs_mkfs(void) {
  char mask;
  fsystem_t fsd;
  bdev_t device = bs_stats();
  uint32 masksize, i;
  mask = disable_interrupts();
  
  masksize = device.nblocks / 8;                          /*                                             */
  masksize += (device.nblocks % 8 ? 0 : 1);               /*  Construct the 'fsd' variable               */
  fsd.device = device;                                    /*  and set to initial values                  */
  fsd.freemasksz = masksize;                              /*                                             */
  fsd.freemask = malloc(masksize);                        /*  Allocate the free bitmask                  */
  fsd.root_dir.numentries = 0;                            /*                                             */

  for (i=0; i<masksize; i++)                              /*                                             */
    fsd.freemask[i] = 0;                                  /*  Initially clear the free bitmask           */
                                                          /*                                             */
  for (i=0; i<DIR_SIZE; i++) {                            /*  Set up the directory entries as            */
    fsd.root_dir.entry[i].inode_block = EMPTY;            /*  empty.                                     */
    memset(fsd.root_dir.entry[i].name, 0, FILENAME_LEN);  /*                                             */
  }                                                       /*                                             */
  
  fsd.freemask[SB_BIT / 8] |= 0x1 << (SB_BIT % 8);        /*                                             */
  fsd.freemask[BM_BIT / 8] |= 0x1 << (BM_BIT % 8);        /*  Set  the  super  block  and free  bitmask  */
  bs_write(SB_BIT, 0, &fsd, sizeof(fsystem_t));           /*  block  as used  and write  the 'fsd'  and  */
  bs_write(BM_BIT, 0, fsd.freemask, fsd.freemasksz);      /*  bitmask to the 0 and 1 block respectively  */
  free(fsd.freemask);                                     /*                                             */

  restore_interrupts(mask);
  return;
}


/*  Take an initialized block device containing a file system *
 *  and copies it into the 'fsd' to make it the active file   *
 *  system.                                                   */
uint32 fs_mount(void) {
  char mask;
  int i;

  mask = disable_interrupts();
  if ((fsd = (fsystem_t*)malloc(sizeof(fsystem_t))) == (fsystem_t*)-1) {  /*                              */
    restore_interrupts(mask);                                             /*  Allocate space for the fsd  */
    return -1;                                                            /*                              */
  }                                                                       /*  Read the contents of the    */
  bs_read(SB_BIT, 0, fsd, sizeof(fsystem_t));                             /*  superblock into the 'fsd'   */
  if ((fsd->freemask = malloc(fsd->freemasksz)) == (char*)-1) {           /*                              */
    restore_interrupts(mask);                                             /*  Allocate space for the      */
    return -1;                                                            /*  free bitmask and read       */
  }                                                                       /*  the block from the block    */
  bs_read(BM_BIT, 0, fsd->freemask, fsd->freemasksz);                     /*  device.                     */

  for (i=0; i<NUM_FD; i++) {                                              /*                              */
    oft[i].state = FSTATE_CLOSED;                                         /*  Initialize the open file    */
    oft[i].head = 0;                                                      /*  table                       */
    oft[i].direntry = 0;                                                  /*                              */
  }                                                                       /*                              */

  restore_interrupts(mask);
  return 0;
}


/*  Write the current state of the file system to a block device and  *
 *  free the resources for the file system.                           */
uint32 fs_umount(void) {
  char mask = disable_interrupts();

  bs_write(BM_BIT, 0, fsd->freemask, fsd->freemasksz);     /*  Write the bitmask and super blocks to  */
  bs_write(SB_BIT, 0, fsd, sizeof(fsystem_t));             /*  their respective block device blocks   */

  free(fsd->freemask);                                     /*  Free memory used for the filesystem    */
  free((void*)fsd);                                        /*                                         */
  
  restore_interrupts(mask);
  return 0;
}
