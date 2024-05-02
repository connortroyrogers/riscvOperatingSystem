#include <barelib.h>
#include <fs.h>

extern fsystem_t* fsd;

/*  Search for 'filename' in the root directory.  If the  *
 *  file exists,  returns an error.  Otherwise, create a  *
 *  new file  entry in the  root directory, a llocate an  *
 *  unused  block in the block  device and  assign it to  *
 *  the new file.                                         */
// Custom function to compare two strings

void* memset(void*, int, int);

int32 fs_create(char* filename) {
//check for potential errors -----------------

    //if directory is empty
    if(fsd->root_dir.numentries == DIR_SIZE){
        return -1;
    }

    //find unused directory
    int32 entry_index = fsd->root_dir.numentries;
    for (int32 i = 0; i < DIR_SIZE; i++) {
        if (fsd->root_dir.entry[i].inode_block == EMPTY) {
            entry_index = i;
            break;
        }
    }


    //check if filename is already used
    for (int32 i = 0; i < fsd->root_dir.numentries; i++) {
        if (fsd->root_dir.entry[i].inode_block != EMPTY &&
            fs_strcmp(fsd->root_dir.entry[i].name, filename) == 0) {
            return -1;
        }
    }

    //look for free block
    uint32 inode_block_index = EMPTY;
    for (uint32 i = 0; i < fsd->device.nblocks; i++) {
        if (fs_getmaskbit(i) == 0) {
            inode_block_index = i;
            fs_setmaskbit(i);
            break;
        }
    }

    //return -1 if no block found
    if (inode_block_index == EMPTY){
        return -1;
    }


//create file TODO -----------------------
    //populate directory entry
    fsd->root_dir.entry[entry_index].inode_block = inode_block_index;
    fsd->root_dir.numentries++;
    fs_strcpy(fsd->root_dir.entry[entry_index].name, filename);

    //write inode to block
    inode_t new_inode;
    new_inode.id = inode_block_index;
    new_inode.size = 0;
    for(int i = 0; i <INODE_BLOCKS; i++){
        new_inode.blocks[i] = EMPTY;
    }

    bs_write(inode_block_index, 0, &new_inode, sizeof(inode_t));

    //write freemask
    bs_write(BM_BIT, 0, fsd->freemask, fsd->freemasksz);

    return 0;
}