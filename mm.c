/*
 * mm.c
 * Implements an explicit segmented list system of keeping track of free blocks
 * Stores an array of list roots at the base of the heap and keeps list updated
 * 
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

#define SIZE_T_SIZE (4)
//#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define WSIZE (SIZE_T_SIZE)
#define DSIZE (SIZE_T_SIZE*2)

#define CHUNKSIZE (1<<10)

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

#define NUM_OF_LISTS (16)

#define WSIZE (SIZE_T_SIZE)
#define DSIZE (SIZE_T_SIZE*2)

void * listroot;
void * heap_listp;

// finds the appropriate list to search in given the size of the block 
void * find_list(size_t size);

// Called in heap_checker 
// looks for a block in the free block segmented list structure and returns 1 if it is found, and 0 if it is not
size_t find_in_list(void* bp) {
    void *root = find_list(GET_SIZE(HDRP(bp)));
    void *elem = NEXT(root);
    while (elem) {
	if (bp == elem) {
	    return 1;
	}
	elem = NEXT(elem);	
    }
    return 0;
}

// scans through the whole contents of the heap and checks to make sure all free blocks are stored in on the segmented lists
void heap_checker() {
    void *bp;
    for(bp=heap_listp; GET_SIZE(HDRP(bp)) > 0; bp=NEXT_BLKP(bp)) {
	if (!GET_ALLOC(HDRP(bp))) {
	    size_t found = find_in_list(bp);
	    if (found == 0) {
		printf("Couldn't find free block at address %p in list", bp);
	    }
	}
    }
}

/*
 * mm_init - initialize the malloc package.
 */

// used to set the ptr to the next block in the free block list
void set_next(void * blockp, void * nextp) {
    // PUT(NEXT(blockp), (size_t)nextp);
    NEXT(blockp) = (size_t *)nextp;
}
// used to set the ptr to the previous block in the free block list
void set_prev(void * blockp, void * prevp) {
    // PUT(PREV(blockp), (size_t)prevp);
    PREV(blockp) = (size_t *)prevp;
}

/* List sizes 
 * 0: 0-31
 * 1: 32-63
 * 2: 64-127
 * 3: 128-255
 * 4: 256-511
 * 5: 512-1023
 * 6: 1024-2047
 * 7: 2048-4095
 * 8: 4096-8191
 * 9: 8192-16383
 * 10: 16384-32767
 * 11: 32768-65536
 * ... 
 */


//given the index of the list, returns a ptr to the root of the list
//like indexing an array of roots of lists
void* index_lists(size_t ind) {
    return (void *)(((char *)listroot) + ind*SIZE_T_SIZE);
}



void* find_list(size_t size) {
    size_t lsize = 32;
    size_t i;
    for(i=0; i<NUM_OF_LISTS; i++) {
        if (lsize > size) {
	    return index_lists(i);
	}
        lsize = lsize << 1;
    }   
    return index_lists(i);	
}


// given a free block pointer, returns the block in a segmented list to insert it after
void* find_place_to_insert(void *bp){
    size_t sz = GET_SIZE(HDRP(bp));
    void* root = find_list(sz);
    void *blockp = NEXT(root);
    void *pblockp = root;
    while (blockp){
        if ((GET_SIZE(HDRP(blockp)) >=sz)) {
            return pblockp;
        }
        pblockp = blockp;
        blockp = NEXT(blockp);
    }
    return pblockp;
}


//given a free block pointer, inserts it into the list
void add_to_free_list(void* blockp) {
    void *pblockp = find_place_to_insert(blockp);
    set_next(blockp, NEXT(pblockp)); //block.next = list.next
      
    set_prev(blockp, pblockp);       //block.prev = listroot
    set_next(pblockp, blockp);                 //listroot.next = block
    if (NEXT(blockp)) {                         // if block.next != NULL (not adding to end of the list)
	    set_prev(NEXT(blockp), blockp); // block.next.prev = block
    }
    return; 
}

// given a free block ptr, removes the block from the segmented free list
void pluck_from_free_list(void* blockp) {     
    if (PREV(blockp)) {
        set_next(PREV(blockp), NEXT(blockp));
    }
    if (NEXT(blockp)) {
          
	set_prev(NEXT(blockp), PREV(blockp));
    }
    return;
}

// given a freed block pointer, looks to the adjecent blocks and will try to 
//coalesce them and add them to a segmented list, then returns a ptr to the newly coaslesced block
static void *coalesce(void *bp)
{
    if (bp == NULL) {
        return NULL;    
    }
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc) { /* Case 1 */
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        pluck_from_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        pluck_from_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else { /* Case 4 */
        pluck_from_free_list(NEXT_BLKP(bp));
        pluck_from_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    add_to_free_list(bp);
    return bp;
}

// used when there is not enough room in the heap
// extends the heap by words 4 byte words and tries to coalesce what was 
//  the last block in the heap before it was extended
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * SIZE_T_SIZE : words * SIZE_T_SIZE;
    size = size;
    if ((long)(bp = mem_sbrk(size)) == -1){
        return NULL;
    }
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0)); /* Free block header */
    PUT(FTRP(bp), PACK(size, 0)); /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */
    /* Coalesce if the previous block was free */
    bp = coalesce(bp);
    return bp;
 }

// sets up NULL ptrs that act as list roots for the segmented lists
void init_lists() {
    int i;
    listroot = mem_sbrk(NUM_OF_LISTS*SIZE_T_SIZE);
    for(i=0; i<NUM_OF_LISTS; i++) {
        PUT(((char *)listroot+i*SIZE_T_SIZE), (size_t)NULL);
    }   
}


// initializes the heap with null segmented free lists and a heap of CHUNKSIZE bytes
int mm_init(void)
{
    //initialize list_root_array;
    init_lists();
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
    return 0;
}

// given a block size, scans through the segmented lists and returns a ptr to a block that can 
//  fit a block of asize bytes or returns NULL if it cannot find a block big enough    
static void *find_fit(size_t asize)
{
    /* First fit search */
    size_t *root = (find_list(asize));
    size_t* blockp = NEXT(root);
    while(root !=(void *) ((char*)heap_listp - SIZE_T_SIZE)) {  
        while (blockp){
            if (GET_SIZE(HDRP(blockp)) >= asize) {
                return blockp;
            }
            blockp = NEXT(blockp);
        }
        root = (size_t *)(((char *)root) + SIZE_T_SIZE);
	blockp = NEXT(root);
    }
    return NULL; /* No fit */
}

// inserts a size of asize into blocks given bp
// if asize is small enough that another block could fit in the extra space, make another block
//  rather than having excess padding
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
     
    if ((csize - asize) >= (4*SIZE_T_SIZE)) { 
	PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        pluck_from_free_list(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK((csize-asize), 0));
        PUT(FTRP(bp), PACK((csize-asize), 0));
        add_to_free_list(bp);
    }
    else {
        pluck_from_free_list(bp);
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}
//size_t limiter = 0;

// mm_malloc will allocated a block of data big enough to hold the data and return a ptr to that block
//  if it can't find a block already in the heap, it will extend the heap to make room
void *mm_malloc(size_t size)
{
    size_t asize; /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;
//    limiter++;
//    if (limiter%200 == 0) {
//	heap_checker();
//    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

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
    bp = extend_heap(extendsize/WSIZE);
    if ((bp) == NULL) {
        return NULL;
    }

    place(bp, asize);
    return bp;
}




// mm_free takes a ptr to a block, changes the allocated indicator bit to be 0, and tries to coalesce contiguous free blocks
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    coalesce(bp);
}


// replace - similar idea to place, but only checks to see if there is enough room to make a new free block
//  if there is room to make a new free block, it changes the size indicators of the allocated block and adds 
//  the newly freed block to a free list
static void replace(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (4*SIZE_T_SIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
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

// copies size bytes starting at old to new
void mm_copy(void * old, void* new, size_t size) {
    size_t i;
    for (i = 0; i < size; i++) {
	(*((char *)new + i)) = (*((char *)old + i));
    }
}


//returns NULL if we resized the block and fit it in
//returns a pointer if we need to reallocate and copy
void* reall_coalesce(void *bp, size_t newsize)
{
    if (bp == NULL) {
        return NULL;    
    }
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    if (!next_alloc) { 
        size_t size = GET_SIZE(HDRP(bp));
	if (size + GET_SIZE(HDRP(NEXT_BLKP(bp))) >= newsize)
	{
            size = size + GET_SIZE(HDRP(NEXT_BLKP(bp)));
	    pluck_from_free_list(NEXT_BLKP(bp));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
	    replace(bp, size);
            return NULL;
        }
    }
    return bp;
}

//reallocates the size of an already allocated block
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t oldSize;
    size_t asize;
    oldSize = GET_SIZE(HDRP(oldptr));
    if (size > 2*SIZE_T_SIZE) {
        asize = ALIGN(2*SIZE_T_SIZE + size);
    }
    else {
        asize = ALIGN(4 * SIZE_T_SIZE);
    }

    if (asize <= oldSize){ // enter if we are making the block smaller
        replace(ptr, asize);
	return ptr;
    }
    //coalesce 
    if (!reall_coalesce(ptr, asize)) { // enter if we could just expand into the next block
	    return ptr;
    } 
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    mm_copy(ptr, newptr, oldSize);
    mm_free(ptr);
    return newptr;
}
