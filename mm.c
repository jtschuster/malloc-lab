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
#define WSIZE (SIZE_T_SIZE)
#define DSIZE (SIZE_T_SIZE*2)

#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) (size | alloc)

// Read and Write word at address
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) ((*(size_t *)(p)) = (val))

// Read size and allocated? fields of headevoid set_prev()void set_prev()r
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// Given ptr to payload, get header and footer
#define HDRP(bp) ((char*)bp - SIZE_T_SIZE)
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - 2*SIZE_T_SIZE)

// Given block to payload, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - SIZE_T_SIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - SIZE_T_SIZE*2)))

// Given a ptr to a block payload, gives ptr to next block (or NULL)
#define NEXT(bp) (*((size_t **)bp))
#define PREV(bp) (*(((size_t **)bp) + 1))


#define WSIZE (SIZE_T_SIZE)
#define DSIZE (SIZE_T_SIZE*2)

void * listroot;
void * heap_listp;


/*
 * mm_init - initialize the malloc package.
 */

void breaker() {
    printf("hello ther general kenobi");
}

void set_next(void * blockp, void * nextp) {
    // PUT(NEXT(blockp), (size_t)nextp);
    NEXT(blockp) = (size_t *)nextp;
}

void set_prev(void * blockp, void * prevp) {
    // PUT(PREV(blockp), (size_t)prevp);
    PREV(blockp) = (size_t *)prevp;
}

void add_to_free_list(void* blockp) {
    //printf("adding to list");
    breaker();
    set_next(blockp, (size_t *)NEXT(listroot)); //block.next = list.next
    set_prev(blockp, (size_t *)listroot);       //block.prev = listroot
    set_next(listroot, blockp);                 //listroot.next = block
    if (NEXT(blockp)) {                         // if block.next != NULL
        set_prev(NEXT(blockp), blockp); // block.next.prev = block
    }
    breaker();
}

void pluck_from_free_list(void* blockp) {
    //printf("plucking");
    breaker();
    if (PREV(blockp)) {
        set_next(PREV(blockp), NEXT(blockp));
    }
    if (NEXT(blockp)) {
        set_prev(NEXT(blockp), PREV(blockp));
    }
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */
        //add_to_free_list(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        pluck_from_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        //add_to_free_list(bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        pluck_from_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        //add_to_free_list(bp);
    }

    else { /* Case 4 */
        pluck_from_free_list(NEXT_BLKP(bp));
        pluck_from_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        //add_to_free_list(bp);
    }
    return bp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * SIZE_T_SIZE : words * SIZE_T_SIZE;
    size = size + 2* SIZE_T_SIZE;
    if ((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(SIZE_T_SIZE, 1)); /* New epilogue header */
    breaker();
    /* Coalesce if the previous block was free */
    return coalesce(bp);
 }



int mm_init(void)
{
    printf("in init");
    if ((heap_listp = mem_sbrk(4*SIZE_T_SIZE)) == (void *)-1){
        return -1;
    }
    listroot = heap_listp;
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1*SIZE_T_SIZE), PACK(SIZE_T_SIZE*2, 1)); /* Prologue header */ //keeps coalescing from messing  up
    PUT(heap_listp + (2*SIZE_T_SIZE), PACK(SIZE_T_SIZE*2, 1)); /* Prologue footer */
    PUT(heap_listp + (3*SIZE_T_SIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*SIZE_T_SIZE);

    breaker();
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/SIZE_T_SIZE) == NULL){
        return -1;
    }
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

static void *find_fit(size_t asize)
{
    /* First fit search */
    size_t *blockp = NEXT(listroot);
    printf("Finding fit \n");
    while (blockp){
        printf("%u \n %u \n %u \n",GET_SIZE(HDRP(blockp)), asize, GET_ALLOC(HDRP(blockp)));
        breaker();
        if (GET_SIZE(HDRP(blockp)) > asize) {
            printf("found a block to fit it in!");
            return blockp;
        }
        blockp = NEXT(blockp);
    }
    return NULL; /* No fit */
}


static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    breaker();
    if ((csize - asize) >= 0 /*(4*SIZE_T_SIZE)*/) { //we already set asize to include the header and footer in mm_alloc?
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        pluck_from_free_list(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK((csize-asize), 0));
        PUT(FTRP(bp), PACK((csize-asize), 0));
        add_to_free_list(bp);
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}


void *mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    /*
    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);*/

    if (size > 2*SIZE_T_SIZE) {
        asize = ALIGN(2*SIZE_T_SIZE + size);
    }
    else {
        asize = ALIGN(4 * SIZE_T_SIZE);
    }


    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}




/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    add_to_free_list( coalesce(bp));
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
