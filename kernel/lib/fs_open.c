#include <barelib.h>
#include <fs.h>

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD];

/*  Search for a filename  in the directory, if the file doesn't exist  *
 *  or it is  already  open, return  an error.   Otherwise find a free  *
 *  slot in the open file table and initialize it to the corresponding  *
 *  inode in the root directory.                                        *
 *  'head' is initialized to the start of the file.                     */


int32 fs_open(char* filename) {
  int i;
  //iterate through directory
  for(i = 0; i < fsd->root_dir.numentries; i++){
    //check for filename
    if(fs_strcmp(fsd->root_dir.entry[i].name, filename) == 0){
      //check if file is already open
      for(int j = 0; j < NUM_FD; j++){
        if(oft[j].state == FSTATE_OPEN && oft[j].direntry == i){
          return -1;
        }
      }
      //check for open index in file table
      for(int j = 0; j < NUM_FD; j++){
        //populate element and read inode
        if(oft[j].state == FSTATE_CLOSED){
          oft[j].state = FSTATE_OPEN;
          oft[j].direntry = i;
          oft[j].head = 0;
          bs_read(fsd->root_dir.entry[i].inode_block, 0, &(oft[j].inode), sizeof(inode_t));
          return j;
        }
      }
      
      return -1;
    }
  }
  return -1;
}
