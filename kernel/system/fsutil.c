#include <barelib.h>
#include <bareio.h>
#include <fs.h>

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD]; 

/*  Print the contents of the filesystem struct  */
void fs_print_fsd(void) {
  printf("\n    fsd\n");
  printf("nblocks:  %d\n", fsd->device.nblocks);
  printf("blocksz:  %d\n", fsd->device.blocksz);
  printf("masksz:   %d\n", fsd->freemasksz);
  printf("numfiles: %d\n", fsd->root_dir.numentries);
}

/*  Print the contents of the free bitmask display which  *
 *  blocks are marked as utilized.                        */
void fs_print_mask(void) {
  int i, j;

  printf("\n    free mask\n");
  for (i=0; i<fsd->freemasksz; i++) {
    for (j=0; j<8; j++)
      if (i==0 && j<2)
	printf("%c", (fs_getmaskbit((i*8)+j) ? 's' : '0'));
      else
	printf("%d", fs_getmaskbit((i*8)+j));
    printf("%c", ((i+1) % 8 ? ' ' : '\n'));
  }
  if ((i+1) % 8)
    printf("\n");
}

/*  Print the contents of the open file table  */
void fs_print_oft(void) {
  int i;

  printf("\n    oft\n");
  printf("Num  state  fileptr  in.id  name\n");
  for (i=0; i<NUM_FD; i++) {
    printf("%3d  %5s  %7d  %5d  %s 0x%x\n", i,
	    (oft[i].state == FSTATE_OPEN ? "OPEN" : "CLOSE"),
	    oft[i].head,
	    fsd->root_dir.entry[oft[i].direntry].name,
	    &oft);
  }
}

/*  Print the entries in the root directory  */
void fs_print_root(void) {
  int i;
  printf("\n    root directory [%d entries]\n", fsd->root_dir.numentries);
  printf("ID  block  name          blocks\n");
  for (i=0; i<fsd->root_dir.numentries; i++) {
    printf("%2d %5d %s \n", i,
	   fsd->root_dir.entry[i].inode_block,
	   fsd->root_dir.entry[i].name);
  }
}

/*  Print the status of a entry in the open file table  */
void fs_print_fd(int32 fd) {
  int sz=0,i=0;

  printf("\n    file descriptor [%d]\n", fd);
  printf("Name:    %s\n", fsd->root_dir.entry[oft[fd].direntry].name);
  printf("State:   %d\n", oft[fd].state);
  printf("Fileptr: %d\n", oft[fd].head);
  printf("Size:    %d\n", oft[fd].inode.size);
  printf("\nblocks: ");
  while (sz < oft[fd].inode.size && i <= INODE_BLOCKS) {
    int bsize = oft[fd].inode.size - i;
    bsize = (bsize > fsd->device.blocksz ? fsd->device.blocksz : bsize);
    sz += bsize;
    printf(" %d", oft[fd].inode.blocks[i]);
    i++;
  }
  printf("\n");
}
