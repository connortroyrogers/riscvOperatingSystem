#include <barelib.h>
#include <fs.h>

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD];

/*  Modify  the state  of the open  file table to  close  *
 *  the 'fd' index and write the inode back to the block  *
    device.  If the  entry is already closed,  return an  *
 *  error.                                                */
int32 fs_close(int32 fd) {
  //check if file is already closed
  if(oft[fd].state == FSTATE_CLOSED){
    return -1;
  }
  //write inode to ramdisk
  bs_write(oft[fd].inode.id, 0, &oft[fd].inode, sizeof(inode_t));
  //set state to closed
  oft[fd].state = FSTATE_CLOSED;
  return 0;
}
