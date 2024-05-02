#include <barelib.h>
#include <fs.h>

/* fs_write - Takes a file descriptor index into the 'oft', a  pointer to a  *
 *            buffer  that the  function reads data  from and the number of  *
 *            bytes to copy from the buffer to the file.                     *
 *                                                                           *
 *            'fs_write' reads data from the 'buff' and copies it into the   *
 *            file  'blocks' starting  at the 'head'.  The  function  will   *
 *            allocate new blocks from the block device as needed to write   *
 *            data to the file and assign them to the file's inode.          *
 *                                                                           *
 *  returns - 'fs_write' should return the number of bytes written to the    *
 *            file.                                                          */

uint32 fs_write(uint32 fd, char* buff, uint32 len) {
    //initialize inode, bytes_written and offset to track and perform write
    inode_t* fileinode = &oft[fd].inode;
    uint32 bytes_written = 0;
    uint32 offset = oft[fd].head;

    //write until 'len' bytes are written
    while (bytes_written < len) {
        //calculate index for block
        uint32 block_index = offset / MDEV_BLOCK_SIZE;
        //calculate offset for block
        uint32 block_offset = offset % MDEV_BLOCK_SIZE;

        //declare variable to keep track of bytes left to write
        uint32 bytes_to_write;

        //ensure writing to block wont surpass len
        if (MDEV_BLOCK_SIZE - block_offset < len - bytes_written) {
            bytes_to_write = MDEV_BLOCK_SIZE - block_offset;
        } else {
            bytes_to_write = len - bytes_written;
        }

        //check if a block has not already been allocated
        if (fileinode->blocks[block_index] == EMPTY) {
            //find new block to allocate
            uint32 new_block = -1;
            for (uint32 i = 0; i < fsd->device.nblocks; i++) {
              if (fs_getmaskbit(i) == 0) {
                  fs_setmaskbit(i);
                  new_block = i;
                  break;
              }
            }
            if (new_block == -1) {
                return -1;
            }
            fileinode->blocks[block_index] = new_block;
            fileinode->size += MDEV_BLOCK_SIZE;
        }

        //write data from buffer to allocated block
        bs_write(fileinode->blocks[block_index], block_offset, buff + bytes_written, bytes_to_write);

        //update offset and written variable with bytes that were written
        offset += bytes_to_write;
        bytes_written += bytes_to_write;
    }

    //update file head to new offset
    oft[fd].head = offset;

    //update inode size if new offset exceeds previous size
    if (oft[fd].head > fileinode->size) {
        fileinode->size = oft[fd].head;
    }

    //write updated inode info to disk
    bs_write(fileinode->id, 0, fileinode, sizeof(inode_t));
    //write freemask after allocations and writes
    bs_write(BM_BIT, 0, fsd->freemask, fsd->freemasksz);

    return bytes_written;
}
