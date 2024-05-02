#include <barelib.h>
#include <fs.h>

#define TIMEOUT 0x5
#define status_is(cond) (t__status & (0x1 << cond))
#define test_count(x) sizeof(x) / sizeof(x[0])
#define init_tests(x, c) for (int i=0; i<c; i++) x[i] = "OK"
#define assert(test, ls, err) if (!(test)) ls = err
#define runner(txt, n) do {				   \
    init_tests(n ## _t, test_count(n ## _prompt));         \
    t__runner(txt, test_count(n ## _prompt), n ## _tests); \
  } while(0)
#define feedback(txt, n) t__printer("\n" txt " Tests:", n ## _t, n ## _prompt, test_count(n ## _prompt))

void t__print(const char*);
void t__runner(const char*, uint32, void(*)(void));
void t__printer(const char*, char**, const char**, uint32);

extern byte t__skip_resched;
void* memcpy(void*, const void*, int);

static const char* general_prompt[] = {
				       "  Program Compiles:   ",
				       "  Creates blockstore: ",
				       "  Creates filesystem: ",
				       "  Mounts filesystem:  ",
};
static const char* create_prompt[] = {
				       "  Returns error if directory is full:            ",
				       "  Returns error if filename exists in directory: ",
				       "  Returns error if no blocks remain for file:    ",
				       "  Sets filename to a directory entry:            ",
				       "  Sets inode index to a directory entry:         ",
				       "  Marks inode as used in bitmask:                ",
				       "  Saves changes back to the blockstore:          ",
};
static const char* open_prompt[] = {
				       "  Returns error if file is already open:    ",
				       "  Returns error if the file does not exist: ",
				       "  Adds file to the open file table:         ",
				       "  Reads file's control block into inode:    ",
				       "  Returns the index of the oft entry:       ",
};
static const char* close_prompt[] = {
				       "  Returns error if file is not open:     ",
				       "  Writes inode block back to blockstore: ",
				       "  Removes file from open file table:     ",
};

static char* general_t[test_count(general_prompt)];
static char* create_t[test_count(create_prompt)];
static char* open_t[test_count(open_prompt)];
static char* close_t[test_count(close_prompt)];

static int t__strcmp(char* a, char* b) {
  int i;
  for (i=0; a[i] == b[i] && a[i] != '\0'; i++);
  return a[i] != b[i];
}

static void general_tests(void) {
  fsystem_t tmp_fs;
  assert(bs_stats().blocksz != 0, general_t[1], "FAIL - Invalid ramdisk block size");
  assert(bs_stats().nblocks != 0, general_t[1], "FAIL - Invalid ramdisk block count");
  assert(bs_read(0, 0, &tmp_fs, sizeof(fsystem_t)) != -1, general_t[1], "FAIL - filesystem could not be allocated");
  assert(bs_stats().blocksz != 0 && tmp_fs.device.blocksz == bs_stats().blocksz, general_t[2], "FAIL - Data written to superblock does not match filesystem");
  assert(bs_stats().blocksz != 0 && tmp_fs.device.blocksz == bs_stats().blocksz, general_t[2], "FAIL - Data written to superblock does not match filesystem");
  assert(fsd != NULL, general_t[3], "FAIL - fsd was not allocated");
}

static void create_tests(void) {
  char name[FILENAME_LEN];
  char bitmap[fsd->freemasksz];
  inode_t inode;

  fsd->root_dir.numentries = DIR_SIZE;
  assert(fs_create("test") == -1, create_t[0], "FAIL - Return code was incorrect");

  memcpy(fsd->root_dir.entry[3].name, "example", 7);
  fsd->root_dir.numentries = 4;
  assert(fs_create("example") == -1, create_t[1], "FAIL - Return code was incorrect");

  for (int i=2; i<fsd->device.nblocks; i++)
    fs_setmaskbit(i);

  assert(fs_create("blah") == -1, create_t[2], "FAIL - File written even when no space exists");

  for (int i=2; i<fsd->device.nblocks; i++)
    fs_clearmaskbit(i);
  
  fsd->root_dir.numentries = 0;
  fs_create("other_thing");
  
  memcpy(name, fsd->root_dir.entry[0].name, FILENAME_LEN);
  assert(t__strcmp(name, "other_thing") == 0, create_t[3], "FAIL - Filename does not match requested file name");
  assert(fsd->root_dir.entry[0].inode_block == 2, create_t[4], "FAIL - Inode block number does not match expected value for new file");
  assert(fs_getmaskbit(2), create_t[5], "FAIL - Freemask was not updated to reflect reserved block for inode");

  bs_read(2, 0, &inode, sizeof(inode_t));
  assert(inode.id == 2, create_t[6], "FAIL - inode block does not contain the correct ID value");
  assert(inode.size == 0, create_t[6], "FAIL - inode block size was not 0");

  bs_read(1, 0, bitmap, fsd->freemasksz);
  for (int i=0; i<fsd->freemasksz; i++)
    assert(fsd->freemask[i] == bitmap[i], create_t[6], "FAIL - Freemask in block store does not match filesystem freemask");
  
  fs_create("HELLOWORLDER");

  memcpy(name, fsd->root_dir.entry[1].name, FILENAME_LEN);
  assert(t__strcmp(name, "HELLOWORLDER") == 0, create_t[3], "FAIL - Second filename does not match requested file name");
  assert(fsd->root_dir.entry[1].inode_block == 3, create_t[4], "FAIL - Second inode block number does not match expected block number");
  assert(fs_getmaskbit(3), create_t[5], "FAIL - Freemask was not updated to reflect second requested inode");
  bs_read(1, 0, bitmap, fsd->freemasksz);
  for (int i=0; i<fsd->freemasksz; i++)
    assert(fsd->freemask[i] == bitmap[i], create_t[6], "FAIL - Freemask in block store does not match filesystem freemask after multiple creates");
}

static void open_tests(void) {
  inode_t inode;
  for (int i=2; i<fsd->device.nblocks; i++)
    fs_clearmaskbit(i);
  
  inode.id = 2;
  inode.size = 0;
  fsd->root_dir.numentries = 4;
  memcpy(fsd->root_dir.entry[0].name, "test 001.xz", 12);
  fsd->root_dir.entry[0].inode_block = 2;
  bs_write(2, 0, &inode, sizeof(inode_t));
  fs_setmaskbit(2);
  inode.id = 4;
  inode.size = 0;
  memcpy(fsd->root_dir.entry[2].name, "test 002.cat", 13);
  fsd->root_dir.entry[2].inode_block = 4;
  bs_write(4, 0, &inode, sizeof(inode_t));
  fs_setmaskbit(4);
  bs_write(1, 0, fsd->freemask, fsd->freemasksz);
  
  assert(fs_open("foobar") == -1, open_t[1], "FAIL - Function returned unexpected value when no file exists");
  assert(fs_open("test 001.xz") == 0, open_t[4], "FAIL - Function did not return the correct index of opened file");
  assert(oft[0].state == FSTATE_OPEN, open_t[2], "FAIL - State of oft was not set to OPEN");
  assert(oft[0].direntry == 0, open_t[2], "FAIL - oft entry does not point to correct directory entry");
  assert(oft[0].head == 0, open_t[2], "FAIL - oft head was not set correctly");
  assert(oft[0].inode.id == 2, open_t[3], "FAIL - oft inode record has incorrect id");
  assert(oft[0].inode.size == 0, open_t[3], "FAIL - oft inode record has incorrect size");

  assert(fs_open("test 001.xz") == -1, open_t[0], "FAIL - Did not return expected error");
  assert(fs_open("test 002.cat") == 1, open_t[4], "FAIL - Function could not open multiple files at once");
  assert(oft[1].state == FSTATE_OPEN, open_t[2], "FAIL - Second file's oft entry was not OPEN");
  assert(oft[1].direntry == 2, open_t[2], "FAIL - oft entry does not point to correct directory entry");
  assert(oft[1].head == 0, open_t[2], "FAIL - oft head was not set correctly");
  assert(oft[1].inode.id == 4, open_t[3], "FAIL - Second oft inode record has incorrect id");
  assert(oft[1].inode.size == 0, open_t[3], "FAIL - Second oft inode record has incorrect size");
}

static void close_tests(void) {
  /* TODO */
  /*
    "  Returns error if file is not open:     ",
    "  Writes inode block back to blockstore: ",
    "  Removes file from open file table:     ",
  */
  inode_t inode;
  oft[0].state = FSTATE_OPEN;
  oft[0].direntry = 0;
  oft[0].head = 0;
  oft[0].inode.id = 3;
  oft[0].inode.size = 12;

  assert(fs_close(1) == -1, close_t[0], "FAIL - Returned unexpected result when closing bad fd");
  fs_close(0);
  bs_read(3, 0, &inode, sizeof(inode_t));
  assert(oft[0].state == FSTATE_CLOSED, close_t[2], "FAIL - State was not set to closed");
  assert(inode.id == 3, close_t[1], "FAIL - Data in block does not match expected inode values");
  assert(inode.size == 12, close_t[1], "FAIL - Data in block does not match expected inode values");
}

void t__ms9(uint32 idx) {
  t__skip_resched = 1;
  if (idx == 0) {
    t__print("\n");
    runner("general", general);
  }
  else if (idx == 1)
    runner("create", create);
  else if (idx == 2)
    runner("open", open);
  else if (idx == 3)
    runner("close", close);
  else {
    t__print("\n----------------------------\n");
    feedback("General", general);
    feedback("Create", create);
    feedback("Open", open);
    feedback("Close", close);
    t__print("\n");
  }
}
