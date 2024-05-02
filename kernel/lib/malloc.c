#include <barelib.h>
#include <malloc.h>
#include <thread.h>

extern uint32* mem_start;
extern uint32* mem_end;
static alloc_t* freelist;

/*  Sets the 'freelist' to 'mem_start' and creates  *
 *  a free allocation at that location for the      *
 *  entire heap.                                    */
//--------- This function is complete --------------//
void heap_init(void) {
  freelist = (alloc_t*)mem_start;
  freelist->size = get_stack(NTHREADS) - mem_start - sizeof(alloc_t);
  freelist->state = M_FREE;
  freelist->next = NULL;
}

/*  Locates a free block large enough to contain a new    *
 *  allocation of size 'size'.  Once located, remove the  *
 *  block from the freelist and ensure the freelist       *
 *  contains the remaining free space on the heap.        *
 *  Returns a pointer to the newly created allocation     */
void* malloc(uint64 size) {
  alloc_t* curr = freelist;
  alloc_t* prev = NULL;
  //search for empty block
  while (curr != NULL) {
    //if block is free and appropriate size
    if (curr->state == M_FREE && curr->size >= size) {
      //if size is same as requested size, set as used
      if (curr->size == size) {
        curr->state = M_USED;
      } else {
        //reallocate block to appropriate size if it is too large
        alloc_t* buff = (alloc_t*)((char*)curr + sizeof(alloc_t) + size);
        buff->size = curr->size - size - sizeof(alloc_t);
        buff->state = M_FREE;
        buff->next = curr->next;
        curr->size = size;
        curr->state = M_USED;
        curr->next = buff;
      }
      //if current block is not the first block in freelist,
      //set previous block's next to point to currents next block
      if (prev != NULL) {
        prev->next = curr->next;
      } else {
        //if current is the first block in freelist, set freelist to start
        //at the next block after current
        freelist = curr->next;
      }
      //return address of current +1
      return (void*)(curr+1); 
    }
    prev = curr;
    curr = curr->next;
  }

  return NULL; 
}

/*  Free the allocation at location 'addr'.  If the newly *
 *  freed allocation is adjacent to another free          *
 *  allocation, coallesce the adjacent free blocks into   *
 *  one larger free block.                                */
void free(void* addr) {
    if (addr == NULL)
        return;

    alloc_t* freeblock = (alloc_t*)addr - 1;
    alloc_t* curr = freelist;
    alloc_t* prev = NULL;

    while (curr != NULL && curr < freeblock) {
        prev = curr;
        curr = curr->next;
    }

    //add freed block to list
    if (prev != NULL) {
        prev->next = freeblock;
    } else {
        freelist = freeblock;
    }

    //coalesce up
    if (freeblock->next != NULL && (char*)freeblock + sizeof(alloc_t) + freeblock->size == (char*)freeblock->next) {
        freeblock->size += sizeof(alloc_t) + freeblock->next->size;
        freeblock->next = freeblock->next->next;
    }

    //coalesce down
    if (prev != NULL && (char*)prev + sizeof(alloc_t) + prev->size == (char*)freeblock) {
        prev->size += sizeof(alloc_t) + freeblock->size;
        prev->next = freeblock->next;
        freeblock = prev;
    }

    // mark as free
    freeblock->state = M_FREE;

    // check that addresses increase monotonically
    if (prev != NULL && prev > freeblock) {
        if (prev == freelist) {
            freelist = freeblock;
        } else {
            alloc_t* temp = freelist;
            while (temp->next != freeblock) {
                temp = temp->next;
            }
            temp->next = prev;
        }
        prev->next = curr;
    } else {
        freeblock->next = curr;
    }

    //check for downward coalescing
    if (freeblock->next != NULL && ((char*)freeblock + sizeof(alloc_t) + freeblock->size) == (char*)freeblock->next) {
        freeblock->size += sizeof(alloc_t) + freeblock->next->size;
        freeblock->next = freeblock->next->next;
    }

    //check for upward coalescing
    if (prev != NULL && ((char*)prev + sizeof(alloc_t) + prev->size) == (char*)freeblock) {
        prev->size += sizeof(alloc_t) + freeblock->size;
        prev->next = freeblock->next;
    }
}
