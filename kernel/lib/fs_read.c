#include <barelib.h>
#include <fs.h>

/* fs_read - Takes a file descriptor index into the 'oft', a  pointer to a  *
 *           buffer that the function writes data to and a number of bytes  *
 *           to read.                                                       *
 *                                                                          *
 *           'fs_read' reads data starting at the open file's 'head' until  *
 *           it has  copied either 'len' bytes  from the file's  blocks or  *
 *           the 'head' reaches the end of the file.                        *
 *                                                                          *
 * returns - 'fs_read' should return the number of bytes read (either 'len' *
 *           or the  number of bytes  remaining in the file,  whichever is  *
 *           smaller).                                                      */
uint32 fs_read(uint32 fd, char* buff, uint32 len) {
    inode_t* fileinode = &oft[fd].inode;
    uint32 bytes_read = 0;
    uint32 remaining = oft[fd].inode.size - oft[fd].head; 
    uint32 offset = oft[fd].head;

    //loop until either len bytes are read or offset reaches file size
    while (bytes_read < len && offset < fileinode->size) {
        //calculate index for block
        uint32 block_index = offset / MDEV_BLOCK_SIZE;
        //calculate offset for block
        uint32 block_offset = offset % MDEV_BLOCK_SIZE;
        //initialize variable for number of bytes to read
        uint32 bytes_to_read;

        //declare variable for number of bytes to read depending on case
        if (MDEV_BLOCK_SIZE - block_offset < len - bytes_read) {
            bytes_to_read = MDEV_BLOCK_SIZE - block_offset;
        } else {
            bytes_to_read = len - bytes_read;
        }

        //read data from block to buff plus number of bytes that have been read already
        bs_read(fileinode->blocks[block_index], block_offset, buff + bytes_read, bytes_to_read);
        
        //update offset and bytes_read
        offset += bytes_to_read;
        bytes_read += bytes_to_read;
    }

    //update head position
    oft[fd].head = offset;

    //change head variable to end of file if read beyond length of file
    if (bytes_read > remaining) {
        oft[fd].head = fileinode->size;
    }
    //return either remaining bytes or bytes read, depending on what is less
    return (remaining < bytes_read) ? remaining : bytes_read;
}
