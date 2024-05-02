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

int t__with_timeout(int32, void*, ...);

uint32 fs_read(uint32, char*, uint32);
uint32 fs_write(uint32, char*, uint32);

extern byte t__skip_resched;
extern int32 t__status;

static const char* general_prompt[] = {
				       "  Program Compiles:   ",
};
static const char* read_prompt[] = {
				    "  Read file from start <partial-block>:       ",
				    "  Read file from start <complete-block>:      ",
				    "  Read file from start <multi-block>:         ",
				    "  Read file from start <multi-block-partial>: ",
				    "  Read block offset <partial>:                ",
				    "  Read block offset <rest-of-block>:          ",
				    "  Read block offset <multi-block>:            ",
				    "  Read block offset <multi-block-partial>:    ",
				    "  Read later block <partial>:                 ",
				    "  Read later block <full block>:              ",
				    "  Read later block <multi-block>:             ",
				    "  Read more than filesize:                    ",
};
static const char* write_prompt[] = {
				    "  Write start of file <partial>:        ",
				    "  Write start of file <complete-block>: ",
				    "  Write start of file <multi-block>:    ",
				    "  Overwrite existing file:              ",
				    "  Append to end of file <partial>:      ",
				    "  Append to end of file <multi-block>:  ",
};

static char* general_t[test_count(general_prompt)];
static char* read_t[test_count(read_prompt)];
static char* write_t[test_count(write_prompt)];

static void file_setup(void) {
  char block[1024];
  inode_t inode;

  for (int i=2; i<512; i++)
    fs_clearmaskbit(i);
  
  inode.id = 5;
  inode.size = 2300;
  for (int i=0; i<INODE_BLOCKS; i++)
    inode.blocks[i] = 600;
  inode.blocks[0] = 8;
  inode.blocks[1] = 12;
  inode.blocks[2] = 4;
  inode.blocks[3] = 13;
  inode.blocks[4] = 14;
  bs_write(5, 0, (char*)&inode, sizeof(inode_t));
  fs_setmaskbit(5);
  oft[0].state = FSTATE_OPEN;
  oft[0].inode = inode;
  oft[0].head = 0;
  
  inode.id = 4;
  inode.size = 0;
  for (int i=0; i<INODE_BLOCKS; i++)
    inode.blocks[i] = 600;
  bs_write(4, 0, (char*)&inode, sizeof(inode_t));
  fs_setmaskbit(4);
  oft[1].state = FSTATE_OPEN;
  oft[1].inode = inode;
  oft[1].head = 0;
  
  for (int i=0; i<1024; i++)
    block[i] = 0;
  for (int i=0; i<512; i++)
    block[i] = i + 8;
  bs_write(8, 0, block, 512);
  fs_setmaskbit(8);
  for (int i=0; i<512; i++)
    block[i] = i + 12;
  bs_write(12, 0, block, 512);
  fs_setmaskbit(12);
  for (int i=0; i<512; i++)
    block[i] = i + 4;
  bs_write(4, 0, block, 512);
  fs_setmaskbit(4);
  for (int i=0; i<512; i++)
    block[i] = i + 13;
  bs_write(13, 0, block, 512);
  fs_setmaskbit(13);
  for (int i=0; i<252; i++)
    block[i] = i + 14;
  bs_write(14, 0, block, 252);
  fs_setmaskbit(14);
}

static void general_tests(void) {}

static void read_tests(void) {
  char block[1024];
  uint32 rlen;
  file_setup();
  rlen = fs_read(0, block, 100);
  for (int i=0; i<100; i++)
    assert(block[i] == (char)(i + 8), read_t[0], "FAIL - Read does not match written value");
  for (int i=100; i<1024; i++)
    assert(block[i] == 0, read_t[0], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 100, read_t[0], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 100, read_t[0], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 0;

  rlen = fs_read(0, block, 512);
  for (int i=0; i<512; i++)
    assert(block[i] == (char)(i + 8), read_t[1], "FAIL - Read does not match written value");
  for (int i=512; i<1024; i++)
    assert(block[i] == 0, read_t[1], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 512, read_t[1], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 512, read_t[1], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 0;

  rlen = fs_read(0, block, 1024);
  for (int i=0; i<512; i++)
    assert(block[i] == (char)(i + 8), read_t[2], "FAIL - Read does not match written value");
  for (int i=512; i<1024; i++)
    assert(block[i] == (char)(i + 12), read_t[2], "FAIL - Read does not match written value");
  assert(oft[0].head == 1024, read_t[2], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 1024, read_t[2], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 0;

  rlen = fs_read(0, block, 700);
  for (int i=0; i<512; i++)
    assert(block[i] == (char)(i + 8), read_t[3], "FAIL - Read does not match written value");
  for (int i=512; i<700; i++)
    assert(block[i] == (char)(i + 12), read_t[3], "FAIL - Read does not match written value");
  for (int i=700; i<1024; i++)
    assert(block[i] == 0, read_t[3], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 700, read_t[3], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 700, read_t[3], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 40;

  rlen = fs_read(0, block, 200);
  for (int i=0; i<200; i++)
    assert(block[i] == (char)(i + 48), read_t[4], "FAIL - Read does not match written value");
  for (int i=200; i<1024; i++)
    assert(block[i] == 0, read_t[4], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 240, read_t[4], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 200, read_t[4], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 40;

  rlen = fs_read(0, block, 472);
  for (int i=0; i<472; i++)
    assert(block[i] == (char)(i + 48), read_t[5], "FAIL - Read does not match written value");
  for (int i=472; i<1024; i++)
    assert(block[i] == 0, read_t[5], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 512, read_t[5], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 472, read_t[5], "FAIL - Returned length did not match read request length");
  
  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 40;

  rlen = fs_read(0, block, 984);
  for (int i=0; i<472; i++)
    assert(block[i] == (char)(i + 48), read_t[6], "FAIL - Read does not match written value");
  for (int i=472; i<984; i++)
    assert(block[i] == (char)(i - 472 + 12), read_t[6], "FAIL - Read does not match written value");
  for (int i=984; i<1024; i++)
    assert(block[i] == 0, read_t[6], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 1024, read_t[6], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 984, read_t[6], "FAIL - Returned length did not match read request length");
  
  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 40;

  rlen = fs_read(0, block, 900);
  for (int i=0; i<472; i++)
    assert(block[i] == (char)(i + 48), read_t[7], "FAIL - Read does not match written value");
  for (int i=472; i<900; i++)
    assert(block[i] == (char)(i - 472 + 12), read_t[7], "FAIL - Read does not match written value");
  for (int i=900; i<1024; i++)
    assert(block[i] == 0, read_t[7], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 940, read_t[7], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 900, read_t[7], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 1024;

  rlen = fs_read(0, block, 200);
  for (int i=0; i<200; i++)
    assert(block[i] == (char)(i + 4), read_t[8], "FAIL - Read does not match written value");
  for (int i=200; i<1024; i++)
    assert(block[i] == 0, read_t[8], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 1224, read_t[8], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 200, read_t[8], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 1024;

  rlen = fs_read(0, block, 512);
  for (int i=0; i<512; i++)
    assert(block[i] == (char)(i + 4), read_t[9], "FAIL - Read does not match written value");
  for (int i=512; i<1024; i++)
    assert(block[i] == 0, read_t[9], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 1536, read_t[9], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 512, read_t[9], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 1024;

  rlen = fs_read(0, block, 1024);
  for (int i=0; i<512; i++)
    assert(block[i] == (char)(i + 4), read_t[10], "FAIL - Read does not match written value");
  for (int i=512; i<1024; i++)
    assert(block[i] == (char)(i + 13), read_t[10], "FAIL - Buffer was written to past length value");
  assert(oft[0].head == 2048, read_t[10], "FAIL - oft head pointer was not adjusted to the new location");
  assert(rlen == 1024, read_t[10], "FAIL - Returned length did not match read request length");

  for (int i=0; i<1024; i++)
    block[i] = 0;
  oft[0].head = 2048;

  rlen = fs_read(0, block, 512);
  for (int i=0; i<252; i++)
    assert(block[i] == (char)(i + 14), read_t[11], "FAIL - Data does not match the end of the file");
  assert(oft[0].head == 2300, read_t[11], "FAIL - oft head pointer was not at the end of file");
  assert(rlen > 251, read_t[11], "FAIL - Did not read to the end of the file");
  assert(rlen < 253, read_t[11], "FAIL - Read past the end of the file");
}

static void write_tests(void) {
  char block[1024], cmp[1024];
  uint32 newblock;
  inode_t inode;
  for (int i=0; i<512; i++)
    block[i] = i;
  
  file_setup();
  fs_write(1, block, 100);
  newblock = oft[1].inode.blocks[0];
  assert(oft[1].inode.size == 100, write_t[0], "FAIL - Size of the file does not match write size");
  assert(newblock != 600, write_t[0], "FAIL - Block number not written to inode");
  if (newblock != 600) {
    bs_read(newblock, 0, cmp, 512);
    for (int i=0; i<100; i++)
      assert(cmp[i] == block[i], write_t[0], "FAIL - Data inside block does not match file write");
  }
  assert(oft[1].head == 100, write_t[0], "FAIL - oft head was not changed to reflect the write");
  bs_read(oft[1].inode.id, 0, (char*)&inode, sizeof(inode_t));
  assert(inode.blocks[0] == oft[1].inode.blocks[0], write_t[0], "FAIL - inode was not written back to block store");

  file_setup();
  fs_write(1, block, 512);
  newblock = oft[1].inode.blocks[0];
  assert(oft[1].inode.size == 512, write_t[1], "FAIL - Size of the file does not match write size");
  assert(newblock != 600, write_t[1], "FAIL - Block index not written to inode");
  assert(oft[1].inode.blocks[1] == 600, write_t[1], "FAIL - Aquired too many blocks for write");
  if (newblock != 600) {
    bs_read(newblock, 0, cmp, 512);
    for (int i=0; i<512; i++)
      assert(cmp[i] == block[i], write_t[1], "FAIL - Data inside block does not match file write");
  }
  assert(oft[1].head == 512, write_t[1], "FAIL - oft head was not changed to reflect the write");
  bs_read(oft[1].inode.id, 0, (char*)&inode, sizeof(inode_t));
  assert(inode.blocks[0] == oft[1].inode.blocks[0], write_t[1], "FAIL - inode was not written back to block store");

  file_setup();
  fs_write(1, block, 800);
  newblock = oft[1].inode.blocks[0];
  assert(oft[1].inode.size == 800, write_t[2], "FAIL - Size of the file does not match write size");
  assert(newblock != 600, write_t[2], "FAIL - Block number not written to inode");
  assert(oft[1].inode.blocks[1] != 600, write_t[2], "FAIL - Block index not written to inode");
  if (newblock != 600) {
    bs_read(newblock, 0, cmp, 512);
    for (int i=0; i<512; i++)
      assert(cmp[i] == block[i], write_t[2], "FAIL - Data inside block does not match file write");
    bs_read(oft[1].inode.blocks[1], 0, cmp, 512);
    for (int i=512; i<800; i++)
      assert(cmp[i-512] == block[i], write_t[2], "FAIL Data inside block 2 does not match file write");
  }
  assert(oft[1].head == 800, write_t[2], "FAIL - oft head was not changed to reflect the write");
  bs_read(oft[1].inode.id, 0, (char*)&inode, sizeof(inode_t));
  assert(inode.blocks[0] == oft[1].inode.blocks[0], write_t[2], "FAIL - inode was not written back to block store");
  assert(inode.blocks[1] == oft[1].inode.blocks[1], write_t[2], "FAIL - inode was not written back to block store");

  file_setup();
  fs_write(0, block, 300);
  newblock = oft[0].inode.blocks[0];
  assert(oft[0].inode.size == 2300, write_t[3], "FAIL - Size of the file does not match original size");
  assert(newblock != 600, write_t[3], "FAIL - Block index not written to inode");
  if (newblock != 600) {
    bs_read(newblock, 0, cmp, 512);
    for (int i=0; i<300; i++)
      assert(cmp[i] == block[i], write_t[3], "FAIL - Data inside block does not match file write");
    for (int i=300; i<512; i++)
      assert(cmp[i] == (char)(i + 8), write_t[3], "FAIL - Data wrote past requeste write length");
  }
  assert(oft[0].head == 300, write_t[3], "FAIL - oft head was not changed to reflect the write");
  bs_read(oft[0].inode.id, 0, (char*)&inode, sizeof(inode_t));
  assert(inode.blocks[0] == oft[0].inode.blocks[0], write_t[3], "FAIL - inode was not written back to block store");

  file_setup();
  oft[0].head = 2300;
  fs_write(0, block, 100);
  newblock = oft[0].inode.blocks[4];
  assert(oft[0].inode.size == 2400, write_t[4], "FAIL - Size of the file does not match original size");
  assert(newblock != 600, write_t[4], "FAIL - Block index not written to inode");
  assert(oft[0].inode.blocks[5] == 600, write_t[4], "FAIL - Aquired too many blocks for write");
  if (newblock != 600) {
    bs_read(newblock, 0, cmp, 512);
    for (int i=252; i<352; i++)
      assert(cmp[i] == block[i-252], write_t[4], "FAIL - Data inside block does not match file write");
  }
  assert(oft[0].head == 2400, write_t[4], "FAIL - oft head was not changed to reflect the write");
  bs_read(oft[0].inode.id, 0, (char*)&inode, sizeof(inode_t));
  assert(inode.blocks[4] == oft[0].inode.blocks[4], write_t[4], "FAIL - inode was not written back to block store");

  file_setup();
  oft[0].head = 2300;
  fs_write(0, block, 512);
  newblock = oft[0].inode.blocks[4];
  assert(oft[0].inode.size == 2812, write_t[5], "FAIL - Size of the file does not match original size");
  assert(newblock != 600, write_t[5], "FAIL - Block index not written to inode");
  assert(oft[0].inode.blocks[5] != 600, write_t[5], "FAIL - Block 2 index not written to inode");
  if (newblock != 600) {
    bs_read(newblock, 252, cmp, 260);
    for (int i=0; i<260; i++)
      assert(cmp[i] == block[i], write_t[5], "FAIL - Data inside block does not match file write");
    bs_read(oft[0].inode.blocks[5], 0, cmp, 512);
    for (int i=0; i<252; i++)
      assert(cmp[i] == block[i+260], write_t[5], "FAIL - Data inside second block does not match file write");
  }
  assert(oft[0].head == 2812, write_t[5], "FAIL - oft head was not changed to reflect the write");
  bs_read(oft[0].inode.id, 0, (char*)&inode, sizeof(inode_t));
  assert(inode.blocks[5] == oft[0].inode.blocks[5], write_t[5], "FAIL - inode was not written back to block store");
}

void t__ms10(uint32 idx) {
  t__skip_resched = 1;
  if (idx == 0) {
    t__print("\n");
    runner("general", general);
  }
  else if (idx == 1)
    runner("read", read);
  else if (idx == 2)
    runner("write", write);
  else {
    t__print("\n----------------------------\n");
    feedback("General", general);
    feedback("Read", read);
    feedback("Write", write);
    t__print("\n");
  }
}
