#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

/* You can improve the below metadata structure using the concepts from Bryant
 * and OHallaron book (chapter 9).
 */

typedef struct metadata {
    /* size_t is the return type of the sizeof operator. Since the size of
     * an object depends on the architecture and its implementation, size_t
     * is used to represent the maximum size of any object in the particular
     * implementation.
     * size contains the size of the data object or the amount of free
     * bytes
     */
    size_t size;
    struct metadata* next;
    struct metadata* prev; //What's the use of prev pointer?
    /*(yt) dont we need a doubly linked list in order to coalescing the consecutive free blocks after dfree call?
     *(yt) we used prev to walk through the freelist?
     */
    //bool is_free;
} metadata_t;

typedef struct footer {
    /*
     We use footer to reduce the runtime of coalescing and free to O(1)
     Size is the size of the block the footer belongs to , the in_use is
     a boolean showing whether the block is free or not.
     */
    // footer->size contains only numbytes, excluding the header's size
    size_t size;
    //bool is_free; //*********************TODO get rid of the is_free
    
} footer_t;

void coalesce(metadata_t* ptr);

#define FOOTER_T_ALIGNED (ALIGN(sizeof(footer_t)))

/*since size is always a multiple of 8, we can use the last one bit in its binary
 representation to denote whether this block is used or not
 0 : unused
 1 : used
 */
#define TO_USED(ptr) (ptr->size = (ptr->size | 0x1)) 
#define TO_UNUSED(ptr) ( ptr->size = (ptr->size & (~0x7)) )

/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing
 */

static metadata_t* freelist = NULL;
static metadata_t* head = NULL; //this pointer will always point to the beginning of the sbrk call, where no footer is in front of it
static metadata_t* tail = NULL; // this points to the tail of the whole memory block

void* dmalloc(size_t numbytes) {
    
    //Initialize freelist through sbrk call first time
    
    if(freelist == NULL) {
        if(!dmalloc_init()) {
            return NULL;  //if freelist is successfully initiated, won't return NULL
        }
        //printf("init freelist size is %zu \n", freelist->size);
    }
    
    //after the first time, freelist will not be null, code goes here:
    
    assert(numbytes > 0);
    
    /* Your code goes here */
    
    size_t numbytes_aligned = ALIGN(numbytes); //align the requested numbytes
    
    metadata_t* cur_freelist = freelist; //set the current ptr to the head of the freelist and start iteration
        
    
    size_t requiredSpace = (numbytes_aligned + FOOTER_T_ALIGNED + METADATA_T_ALIGNED);
    

    while ((cur_freelist->size) < requiredSpace ) { //move the cur_freelist ptr to the start of the block with enough size 

        if ((cur_freelist->next) == NULL){
            return NULL; //not enough space in freelist
        }else {
            cur_freelist = cur_freelist->next;
            
        }
    }
    
    
    // SPLIT step 1: Create footer for the block we're allocating

    footer_t* new_footer = (footer_t*) (((void*)cur_freelist) + METADATA_T_ALIGNED + numbytes_aligned);
    
    new_footer->size = numbytes_aligned;
    
    TO_USED(new_footer);
    
    // SPLIT step 2: Create metadata for the remaining free block

    metadata_t* new_freelist = (metadata_t*) (((void*)new_footer) + FOOTER_T_ALIGNED);
    
     
    // SPLIT step 3: Remove the allocated block from the free list.
    //Edge case: the first block in freelist is split.
    
    new_freelist->prev = cur_freelist->prev;
    new_freelist->next = cur_freelist->next;
    
    if (cur_freelist->prev != NULL) {
        cur_freelist->prev->next = new_freelist;
    }
    
    if (cur_freelist->next != NULL) {
        cur_freelist->next->prev = new_freelist;
    }
    
    if (cur_freelist == freelist) { //this is the case when cur_freelist is pointing to the only metadata in the memory

        new_freelist->next = cur_freelist->next;
        new_freelist->prev = cur_freelist->prev;
        
        freelist = new_freelist;
        
    }
       
    //SPLIT step 4: fill in the metadata and the footer for the remaining free block
    new_freelist->size = (cur_freelist->size) - numbytes_aligned - FOOTER_T_ALIGNED - METADATA_T_ALIGNED;

    footer_t* footer_to_change = (footer_t*) (((void*) new_freelist) + METADATA_T_ALIGNED + (new_freelist->size));
    
    footer_to_change->size = new_freelist->size;
    
    
    cur_freelist->size = numbytes_aligned; //update the cur_freelist size
    TO_USED(cur_freelist); //update the cur_freelist boolean
    
    cur_freelist->next = NULL;
    cur_freelist->prev = NULL;
    
    
    return (void*) ((void*)cur_freelist + METADATA_T_ALIGNED);
    
}

/*
    In order to keep the dfree() under constant time, we move the to-free block to
    the beginning of the free list. This operation doesn't involve looping 
    through the freelist so it's O(1).
*/

void dfree(void* ptr) {
    
    //start moving to front
    
    metadata_t* to_free_ptr = (metadata_t*) (((void*)ptr) - METADATA_T_ALIGNED);
        
    if (to_free_ptr != freelist) {
        
        to_free_ptr->next = freelist;
        to_free_ptr->prev = NULL;
        
        freelist->prev = to_free_ptr;
        
        freelist = to_free_ptr;
        
    }
    
    
    TO_UNUSED(to_free_ptr);
    
    footer_t* footer_to_change = (footer_t*) (((void*) to_free_ptr) + METADATA_T_ALIGNED + (to_free_ptr->size));
    
    TO_UNUSED(footer_to_change);
    
    
    coalesce(freelist);
}

/*
    The coalesce function is also under constant time since it only check the
    block behind and in front of it. 
*/

void coalesce(metadata_t* ptr) {
    
    //check the block behind it, this take constant time.
    
    metadata_t* next_block =  (metadata_t*) (((void*) ptr) + METADATA_T_ALIGNED + (ptr->size) + FOOTER_T_ALIGNED);
    
    if (next_block->size%8 == 0) {
        
        if (next_block->prev != NULL) {
            
            next_block->prev->next = next_block->next;
        }
        
        if (next_block->next != NULL) {
            
            next_block->next->prev = next_block->prev;
        }
        
        //increase the size of to_free_ptr
        ptr->size += FOOTER_T_ALIGNED + METADATA_T_ALIGNED + (next_block->size);
        
        footer_t* footer_to_change = (footer_t*) (((void*) ptr) + METADATA_T_ALIGNED + (ptr->size));
        
        footer_to_change->size = ptr->size;
        
    }
    
    //check the block in front of it, this take constant time.
    
    if (ptr != head) {
        
        
        footer_t* prev_footer = (footer_t*) (((void*)ptr) - FOOTER_T_ALIGNED);
        
        if (prev_footer->size%8 == 0) { //prev block is free
            
            metadata_t* prev_block = (metadata_t*) (((void*)prev_footer) - prev_footer->size - METADATA_T_ALIGNED); // new line
            
            if (prev_block->prev != NULL) {
                
                prev_block->prev->next = prev_block->next;                
                
            }
            
            if (prev_block->next != NULL) {
                                
                prev_block->next->prev = prev_block->prev;
            }
            
            
            prev_block->size += FOOTER_T_ALIGNED + METADATA_T_ALIGNED + (ptr->size) ; //increase the size
            
            prev_block->prev = NULL;
            
            if (freelist->next != prev_block) {

                prev_block->next = freelist->next;
                
                if (freelist->next != NULL) {

                    freelist->next->prev = prev_block;
                }
            }
                     
            freelist = prev_block;
            
            footer_t* footer_to_change = (footer_t*) (((void*) freelist) + METADATA_T_ALIGNED + (freelist->size));
            
            footer_to_change->size = freelist->size;
            
        }
    }
    
    footer_t* footer_to_change = (footer_t*) (((void*) freelist) + METADATA_T_ALIGNED + (freelist->size));
    
    footer_to_change->size = freelist->size;
    
    TO_UNUSED(freelist);
    TO_UNUSED(footer_to_change);
    
    
}


bool dmalloc_init() {
    
    /* Two choices:
     * 1. Append prologue and epilogue blocks to the start and the end of the freelist
     * 2. Initialize freelist pointers to NULL
     *
     * Note: We provide the code for 2. Using 1 will help you to tackle the
     * corner cases succinctly.
     */
    
    size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
    
    
    freelist = (metadata_t*) sbrk(max_bytes); 
    
    if (freelist == (void *)-1)
        return false;
    
    head = freelist;
    tail = (((void *)freelist) + MAX_HEAP_SIZE);
    
    freelist->next = NULL;
    freelist->prev = NULL;
    freelist->size = max_bytes - METADATA_T_ALIGNED - FOOTER_T_ALIGNED;
    
    footer_t* footer_init = (footer_t*) (((void*) freelist) + METADATA_T_ALIGNED + freelist->size);
    
    footer_init->size = freelist->size;
    
    
    return true;
}

/*Only for debugging purposes; can be turned off through -NDEBUG flag*/
void print_freelist() {
    metadata_t *freelist_head = freelist;
    while(freelist_head != NULL) {
        DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p\t",freelist_head->size,freelist_head,freelist_head->prev,freelist_head->next);
        freelist_head = freelist_head->next;
    }
    DEBUG("\n");
    
}