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

#define NUM_OF_LISTS (13)

#define LIST_ARR_NEXT(p) ((void *)((size_t *)p + 2))

#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) (size | alloc)

// Read and Write word at address
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) ((*(size_t *)(p)) = (val))

// Read size and allocated? fields of header
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// Given ptr to payload, get header and footer
#define HDRP(bp) ((char*)bp - SIZE_T_SIZE)
#define FTRP(bp) ((char *)bp + GET_SIZE(HDRP(bp)) - 2*SIZE_T_SIZE)

// Given block to payload, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - SIZE_T_SIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - SIZE_T_SIZE*2)))

// takes a pointer to a block and gives a ptr to the next block
#define NEXT(p) ((void *)(*(size_t *)(p)))
#define PREV(p) ((void *)(*(((size_t *)p + 1))))

// #define NEXT(p) ((void *)(*(size_t *)(p)))
// #define PREV(p) ((void *)(*((size_t *)((size_t *)p + 1))))
// takes ptr to a block and changes the ptr stored in the block
#define SET_NEXT(p, newp) ((size_t *)NEXT(p) = (size_t *)(newp))
#define SET_PREV(p, newp) ((size_t *)PREV(p) = (size_t *)(newp))


#define WSIZE (SIZE_T_SIZE)
#define DSIZE (SIZE_T_SIZE*2)

void * heap_listp;
void * list_array_ptr;

void set_next(void* p, void* newp) {
    (*(size_t *)(p)) = ((size_t *)(newp));
}
void set_prev(void* p, void* newp) {
    if (p) {
        (*(((size_t *)p + 1))) = ((size_t *)(newp));
    }
}




/*
 * mm_init - initialize the malloc package.
 */



void remove_from_its_list(void* blockp) {
    size_t size = GET_SIZE(HDRP(blockp));
    void * listroot = list_array_ptr;
    for (int i = 0; i< NUM_OF_LISTS - 1; i++){
        if ((GET_SIZE(HDRP(listroot)) <= size) && (GET_SIZE(HDRP(LIST_ARR_NEXT(listroot)) > size  ))) {
            break;
        }
        listroot = LIST_ARR_NEXT(listroot);
    }
    void *elemp = listroot;
    while (elemp != blockp && elemp) {
        elemp = NEXT(elemp);
    }
    if (elemp) {
        if (PREV(elemp)){
        set_next(PREV(elemp), NEXT(elemp));
        
        }
        set_prev(NEXT(elemp), PREV(elemp));
    }
    
}

// make sure bigger than current but smaller than next
void add_free_block_to_list(void *ptr){
    size_t size = GET_SIZE(HDRP(ptr));
    void * list = list_array_ptr;
    for (int i = 0; i< NUM_OF_LISTS - 1; i++){
        if ((GET_SIZE(HDRP(list)) <= size) && (GET_SIZE(HDRP(LIST_ARR_NEXT(list))) > size  )) {
            break;
        }
        list = LIST_ARR_NEXT(list);
    }
    set_next(ptr, NEXT(list));
    set_prev(NEXT(ptr), (size_t *)ptr); //will eff up if next of list is null
    set_next(list, (size_t *)ptr);
}


static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1 */  //both are already allocated
        add_free_block_to_list(bp);
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */ // only prev is allocated, next is free
        //remove bp from the appropriate list
        remove_from_its_list(bp);
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
        add_free_block_to_list(bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */ //just next is allocated, prev is free
        remove_from_its_list(bp);
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_free_block_to_list(bp);
    }

    else { /* Case 4 */ //both are free
        remove_from_its_list(bp);
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_free_block_to_list(bp);
    }
    return bp;
}

//sets header and footer
void insert_sizes(void *ptr, size_t size, int alloc) {
    PUT((void *)HDRP(ptr), PACK(size, alloc));
    PUT((void *)FTRP(ptr), PACK(size, alloc));
    set_next(ptr, NULL);
    set_prev(ptr, NULL);
}

static void *extend_heap(size_t words)
{
    void *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * SIZE_T_SIZE : words * SIZE_T_SIZE;
    if ((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }
    /* Initialize free block header/footer and the epilogue header */
    insert_sizes((void *)bp, size, 0); /* Free block header */ /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
 }


void init_array_o_lists() {
    size_t size = 16;
    void * base = mem_sbrk((SIZE_T_SIZE+sizeof(void*)) * NUM_OF_LISTS );
    base = (void*)((size_t *)base + 1);
    list_array_ptr = base;
    for (int i = 0; i< NUM_OF_LISTS; i++) {
        size = size << 1;
        PUT(HDRP(base), PACK(size, 1));
        PUT(base, 0);
        base = LIST_ARR_NEXT(base);
    }
}


int mm_init(void)
{
    init_array_o_lists();
    
    
    if ((heap_listp = mem_sbrk(4*SIZE_T_SIZE)) == (void *)-1){
        return -1;
    }
    PUT(heap_listp, 0); /* Alignment padding */
    PUT(heap_listp + (1*SIZE_T_SIZE), PACK(SIZE_T_SIZE*2, 1)); /* Prologue header */ //keeps coalescing from messing  up
    PUT(heap_listp + (2*SIZE_T_SIZE), PACK(SIZE_T_SIZE*2, 1)); /* Prologue footer */
    PUT(heap_listp + (3*SIZE_T_SIZE), PACK(0, 1)); /* Epilogue header */
    heap_listp += (2*SIZE_T_SIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/SIZE_T_SIZE) == NULL){
        return -1;
    }

    add_free_block_to_list(heap_listp);

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
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL; /* No fit */
}

static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp)); //size of full empty block

    if ((csize - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1)); // take what we need from the empty block
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK((csize-asize), 0)); // reset sizes of rest of the empty block
        PUT(FTRP(bp), PACK((csize-asize), 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

//returns pointer to root of list of right size
void* find_block_that_fits(size_t size){
    void * list = list_array_ptr;
    for (int i = 0; i < NUM_OF_LISTS - 1; i++){
        if ((GET_SIZE(HDRP(list)) > size) && (NEXT(list) != NULL)) {
            return list;
        }
        list = LIST_ARR_NEXT(list);
    }
    return NULL;
}

void break_block(void * ptr, size_t size) {
    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t rest_size = old_size - size;
    if (rest_size > 4*SIZE_T_SIZE) {
        insert_sizes(ptr, size, 1);
        void * rest_ptr = NEXT_BLKP(ptr);
        insert_sizes(rest_ptr, rest_size, 0);
        add_free_block_to_list(rest_ptr);
    } 
    else{
        insert_sizes(ptr, size, 1);
    }
}

void *mm_malloc(size_t size) // implicit list with size_t_size bytes indicating size at start
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

    /* Ignore spurious requests */
    if (size == 0) {
        return NULL;
    }
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE){
        asize = 2*DSIZE;
    }
    else {
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }
    /* Search the free list for a fit */
    if ((bp = find_block_that_fits(asize)) != NULL) {
        break_block(bp, asize); //break block
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    break_block(bp, asize);
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
    coalesce(bp);
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
