/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Jenni and Jackson",
    /* First member's full name */
    "Jackson Schuster",
    /* First member's email address */
    "jacksonschuster2021@u.northwestern.edu",
    /* Second member's full name (leave blank if none) */
    "Jenni Hutson",
    /* Second member's email address (leave blank if none) */
    "jennihutson@u.northwestern.edu"};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
//returns the value of the size of the block 
size_t blockSize(void * headerStart) {
    return( (*((size_t *)headerStart)) & ~0x7 ); 
}

//Takes heap ptr and returns true is 'used' but is set
_Bool used(void * headerStart) { //1 will indicate used
    return ( (*((size_t* )headerStart)) & 0x1);  // cast as ptr to size_t, dereference, then compare the lsb
}

void *mm_malloc(size_t size) // implicit list with size_t_size bytes indicating size at start
{
    int newsize = ALIGN(size + SIZE_T_SIZE); // ensures size is big enough for a header of size_t_size + the size of the data
    //find a free block big enough
    void* check_ptr = mem_heap_lo();
    void* hi_ptr = mem_heap_hi();
    int checkedSize;
    while (check_ptr < hi_ptr) {
        checkedSize = blockSize(check_ptr);
        if (!(used(check_ptr))) { // it's free
            if (newsize < checkedSize) { // its big enough
                *((size_t *)check_ptr) = newsize + 1;
                return (void *)((char *)check_ptr + SIZE_T_SIZE); 
            }
        }
        check_ptr = (void *)((char *)check_ptr + checkedSize); //increment ptr to go the blocksize ahead and check again
    }

    //if we can't find a freed place:
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1) //we've reached the max heap size
        return NULL;
    else
    {
        *(size_t *)p = newsize + 1; // should be a multiple of 8, so adding 1 will set used bit                      
        return (void *)((char *)p + SIZE_T_SIZE); //return a void type ptr
    }
}




/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    (*(size_t *)ptr) = (*(size_t *)ptr) & ~0x7;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
