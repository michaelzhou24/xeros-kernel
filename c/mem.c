/* mem.c : memory manager
 */

#include <xeroskernel.h>
#include <i386.h>
/* Your code goes here */

extern  long freemem;

extern struct memHeader* freeList = NULL;
int memHeaderSize = sizeof(struct memHeader);

extern void kmeminit( void ) {
    long alignedStart = (freemem/16 + (freemem%16?1:0)) * 16;
    freeList = (struct memHeader*) alignedStart;
    freeList->size = HOLESTART - memHeaderSize - (int) alignedStart;
    freeList->prev = NULL;
    freeList->next = (struct memHeader*) HOLEEND;
    freeList->sanityCheck = NULL;

    //2nd NODE
    struct memHeader *afterHole = (struct memHeader*) HOLEEND;
    afterHole->size = 0x400000 - HOLEEND - memHeaderSize;
    afterHole->prev = freeList;
    afterHole->next = NULL;
    afterHole->sanityCheck = NULL;
}

extern void *kmalloc(size_t size ) {
    struct memHeader *allocated;

    if(size <= 0) {
        return NULL;
    }
    //     1. Compute amount of memory to set aside for this request:
    // int amnt = (size)/16 + ((size%16)?1:0);
    // amnt = amnt*16 + sizeof(struct memHeader);
    
    int amnt = ((size/16) + (size%16?1:0)) * 16;

    allocated = freeList;
    while (allocated != NULL && allocated->size < amnt) {
        allocated = allocated->next;
    } 
    
    // no free memory available
    if (!allocated) {
        return NULL;
    }

    // allocated = freenode that has space for us

    // add new allocated to freelist to replace our cur allocated

    struct memHeader* next = NULL;
    int nextSize = allocated->size - amnt - memHeaderSize;

    if (nextSize > 0) {
        next = (struct memHeader*)(allocated->dataStart + amnt);
        if (allocated->next!= NULL) {
            allocated->next->prev = next;
        }
        if (allocated->prev!= NULL) {
            allocated->prev->next = next;
        } else {
            freeList = next;
        }
        next->prev = allocated->prev;
        next->next = allocated->next;
        next->size = nextSize;
    } else {
        if (allocated->prev!=NULL) {
            allocated->prev->next = next;
        } else {
            freeList = next;
        }
        next = allocated->next;
    }

    allocated->size = amnt;
    allocated->next = NULL;
    allocated->prev = NULL;
    allocated->sanityCheck = (char*) allocated->dataStart;
    // kprintf("\n header->sanityCheck: %d, header->dataStart: %d, ptr: %d \n", allocated->sanityCheck, allocated->dataStart, allocated);
    // kprintf("\nAllocated %d bytes of memory.\n", allocated->size);
    return allocated->dataStart;
}

extern int kfree(void* ptr) {
    // kprintf("\nptr = %d, memheadersize = %d\n", ptr, memHeaderSize);
    struct memHeader *freeNode = 0, *header = (struct memHeader*) (ptr - memHeaderSize);
    // kprintf("\n Test123: %d \n", header->dataStart);
    // kprintf("test xxxx \n");

    // kprintf("\n header->sanityCheck: %d, header->dataStart: %d, ptr: %d \n", header->sanityCheck, header->dataStart, ptr);
    // kprintf("test 456 \n");
    if(header->sanityCheck != (char*)header->dataStart) {
        return 0;
    } else {
        // kprintf("\n Sanity check met. \n");
        header->next = NULL;
        header->prev = NULL; 
        header->sanityCheck = NULL;
        if (freeList == NULL) {
            freeList = header;
        }  else if(freeList >= header) {
            freeNode = freeList;
            while (freeNode->next != NULL && header > freeNode->next) {
                freeNode = freeNode->next;
            }
            if (freeNode->next != NULL) {
                header->next = freeNode->next;
                freeNode->next->prev = header;
            }
            freeNode->next = header;
            header->prev = freeNode;
        }
        else {

            freeList->prev = header;
            header->next = freeList;
            freeList = header;
        } 
    } 
    // Combine free nodes to left
    if (header->prev!=NULL && (header->prev->dataStart + header->prev->size) == (unsigned char*) header ) {
      header->prev->size += header->size + memHeaderSize;
      header->prev->next = header->next;
      if (header->next) {
        header->next->prev = header->prev;
      }
      header = header->prev;
    }
    // Combine free nodes to the right
    if (header->next!=NULL && (header->dataStart + header->size) == (unsigned char*) (header->next)) {
      header->size += header->next->size + memHeaderSize;
      header->next = header->next->next;
      if (header->next && header->next->next) {
          struct memHeader *tmp = header->next->next;
          tmp->prev=header;
        // header->next->next->prev = header;
      }
    }

    // Debug
    // freeNode = freeList;
    // while (freeNode) {
    //     kprintf("\n Free address %d with size %d \n ", freeNode->dataStart, freeNode->size);
    //     freeNode = freeNode->next;
    // }

    return 1;
}