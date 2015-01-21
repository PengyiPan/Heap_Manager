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

#define FOOTER_T_ALIGNED (ALIGN(sizeof(footer_t)))

/*since size is always a multiple of 8, we can use the last one bit in its binary
 representation to denote whether this block is used or not
 0 : unused
 1 : used
 */
#define TO_USED(ptr) (ptr->size += 1)
#define TO_UNUSED(ptr) ( ptr->size = (ptr->size - (ptr->size%8)) )

/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing 
 */

static metadata_t* freelist = NULL;
static metadata_t* head = NULL; //this pointer will always point to the beginning of the sbrk call, where no footer is in front of it
static metadata_t* tail = NULL; // this points to the

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

	metadata_t* cur_freelist = freelist; //set the current ptr to the head of the freelist

    //printf("curr_freelist size is %zu \n", cur_freelist->size);

    

	//printf("Metadata size is %zu. Footer size is %zu\n", METADATA_T_ALIGNED, FOOTER_T_ALIGNED);
	
	int requiredSpace = (numbytes_aligned + FOOTER_T_ALIGNED);

    //****************if is tail, check numbytes / size
    
    int tightestSize = MAX_HEAP_SIZE;
    metadata_t* tightestBlock = NULL;

    //Go through the loop and find the tightest block.
	while (cur_freelist != NULL) {
		int sizeOfThisBlock = cur_freelist->size;

		if(sizeOfThisBlock >= requiredSpace){
			if(sizeOfThisBlock < tightestSize){
				tightestBlock = cur_freelist;
				tightestSize = sizeOfThisBlock;
			}
		}
		cur_freelist = cur_freelist->next;
		//printf("curr_block_size %lu < requiredSpace %d \n", cur_freelist->size, requiredSpace);
	}

	if(tightestBlock == NULL){ // Could not find any free block bigger than required space
		return NULL;
	}


	cur_freelist = tightestBlock;
	int minimumSizeForSplit = (numbytes_aligned + FOOTER_T_ALIGNED + METADATA_T_ALIGNED);

	if(tightestSize <= minimumSizeForSplit){ //Cannot be split
		footer_t* new_footer = (footer_t*) (((void*)cur_freelist) + METADATA_T_ALIGNED + cur_freelist->size - FOOTER_T_ALIGNED);
		new_footer->size = cur_freelist->size - FOOTER_T_ALIGNED;
		TO_USED(new_footer);
		TO_USED(cur_freelist); //update the cur_freelist boolean
		//remove the allocated block from the freelist

		if (cur_freelist->prev != NULL && cur_freelist->next != NULL) { //cur_freelist in the middle
			cur_freelist->prev->next = cur_freelist->next;
			cur_freelist->next->prev = cur_freelist->prev;
		}
		else if (cur_freelist->prev == NULL && cur_freelist->next != NULL) { //cur_freelist at the start and not the only one
			freelist = cur_freelist->next;
			cur_freelist->next->prev = freelist;
		}

		else if (cur_freelist->prev != NULL && cur_freelist->next == NULL) { //cur_freelist in the end and not the only one
	        cur_freelist->prev->next = NULL;
		}
		
		else{ // cur_freelist only block
			freelist = NULL;
		}

		cur_freelist->next = NULL;
		cur_freelist->prev = NULL;

	    //~ptr arith bug~ return (cur_freelist + METADATA_T_ALIGNED);
	    return (metadata_t*) ((void*)cur_freelist + METADATA_T_ALIGNED);


	}
	else{ // Should be split
		// SPLIT step 1: Create footer for the block we're allocating
		//~ptr arith bug~footer_t* new_footer = (footer_t*) (cur_freelist + METADATA_T_ALIGNED + numbytes_aligned);
	    footer_t* new_footer = (footer_t*) (((void*)cur_freelist) + METADATA_T_ALIGNED + numbytes_aligned);
	    
		new_footer->size = numbytes_aligned;
	    
		//new_footer->is_free = false;
	    
		// SPLIT step 2: Create metadata for the remaining free block
	    //~ptr arith bug~ metadata_t* new_freelist = (metadata_t*) (new_footer + FOOTER_T_ALIGNED);
	    metadata_t* new_freelist = (metadata_t*) (((void*)new_footer) + FOOTER_T_ALIGNED);

	    
		// SPLIT step 3: Remove the allocated block from the free list.
		//Edge case: the first or last block in freelist is split.
	    
	    new_freelist->prev = cur_freelist->prev;
	    new_freelist->next = cur_freelist->next;

		if (cur_freelist->prev != NULL) {
			cur_freelist->prev->next = new_freelist;
		}
		
		if (cur_freelist->next != NULL) {
			cur_freelist->next->prev = new_freelist;
		}

		if (cur_freelist->prev == NULL && cur_freelist->next == NULL) { //this is the case when cur_freelist is pointing to the only metadata in the memory
	        freelist = new_freelist;
		}
	    
	    // only one of the two is null?
	    

		
		//SPLIT step 4: fill in the metadata for the remaining free block
		new_freelist->size = (cur_freelist->size) - numbytes_aligned - FOOTER_T_ALIGNED - METADATA_T_ALIGNED;
		//new_freelist->is_free = true;

		cur_freelist->size = numbytes_aligned; //update the cur_freelist size 
		TO_USED(cur_freelist);//update the cur_freelist boolean

	    cur_freelist->next = NULL;
		cur_freelist->prev = NULL;

	    //~ptr arith bug~ return (cur_freelist + METADATA_T_ALIGNED);
	    
	    return (metadata_t*) ((void*)cur_freelist + METADATA_T_ALIGNED);
	}
    
    
} 

void dfree(void* ptr) {

	/* Your free and coalescing code goes here */
    
	//~ptr arith bug~ metadata_t* to_free_ptr = ptr - METADATA_T_ALIGNED;
    metadata_t* to_free_ptr = (metadata_t*) (((void*)ptr) - METADATA_T_ALIGNED);
    printf("Dfree called. to_free_ptr: %p\n", to_free_ptr);

	/** Start coalescing **/
    
    TO_UNUSED(to_free_ptr);

    //************* add testing case for tail
    
	//check the block behind it
    //~ptr arith bug~ metadata_t* next_block = ptr + (to_free_ptr->size) + FOOTER_T_ALIGNED; //POSSIBLE PROBLEM: cannot find next block metadata
    metadata_t* next_block =  (metadata_t*) (((void*) ptr) + (to_free_ptr->size) + FOOTER_T_ALIGNED);
    printf("to_free_ptr size: %lu\n", to_free_ptr->size);
    printf("next_block : %p\n", next_block);


    if (next_block->size%8 == 0) {

		to_free_ptr->prev = next_block->prev;
		to_free_ptr->next = next_block->next;

		if (next_block->prev != NULL) {
			next_block->prev->next = to_free_ptr;
		}
		
		if (next_block->next != NULL) {
			next_block->next->prev = to_free_ptr;
		}
        
        //increase the size of to_free_ptr
		to_free_ptr->size += FOOTER_T_ALIGNED + METADATA_T_ALIGNED + (next_block->size); //POSSIBLE PROBLEM: give back the footer next block or not?

 		printf("new to free ptr seize: %lu\n", to_free_ptr->size);


		// if (to_free_ptr->prev == NULL && to_free_ptr->next == NULL) { 
		// 	printf("Something wrong with next.\n");
		// 	return;
		// }			

	}

    //check the block in front of it
	if (to_free_ptr != head) { //since head does not have a foot infront of it, cannot check

		//ptrarithbug footer_t* prev_footer = (footer_t*) to_free_ptr - FOOTER_T_ALIGNED;
        footer_t* prev_footer = (footer_t*) (((void*)to_free_ptr) - FOOTER_T_ALIGNED);
        
        if (prev_footer->size%8 == 0) { //prev block is free
            
            //ptr airth bug metadata_t* prev_block = (metadata_t*) prev_footer - prev_footer->size - METADATA_T_ALIGNED; // new line
            metadata_t* prev_block = (metadata_t*) (((void*)prev_footer) - prev_footer->size - METADATA_T_ALIGNED); // new line

            if (prev_block->size%8 == 0){
                
                prev_block->size += FOOTER_T_ALIGNED + METADATA_T_ALIGNED + (to_free_ptr->size) ; //increase the size
                
                
                if (to_free_ptr->prev != NULL && to_free_ptr->next != NULL) { // when the to_free_ptr successfully coalesced the block behind
                    prev_block->next = to_free_ptr->next;
                    prev_block->next->prev = prev_block;
                }
                
                if (to_free_ptr->prev != NULL && to_free_ptr->next == NULL) {
                    prev_block->next = to_free_ptr->next; //==NULL
                }
                
                if (to_free_ptr->prev == NULL && to_free_ptr->next != NULL) {
                    prev_block->next = to_free_ptr->next;
                    prev_block->next->prev = prev_block;
                }
                
                
                to_free_ptr = prev_block;
            }
        }
        
		
	}

	/** Done with coalescing **/

	/** Start to free **/
	// We free the list by putting the (coalesced) block in front of the the freelist.
    

	//to_free_ptr->is_free = true;
	//ptr arith bug footer_t* footer_to_change = (footer_t*) (to_free_ptr + METADATA_T_ALIGNED + (to_free_ptr->size));
	footer_t* footer_to_change = (footer_t*) (((void*)to_free_ptr) + METADATA_T_ALIGNED + (to_free_ptr->size));
	printf("to_free_ptr pos: %p\n", to_free_ptr);
	printf("to free prt size after coalescing: %lu\n", to_free_ptr->size);
	printf("footer to change: %p\n", footer_to_change);
	printf("tail: %p\n", tail);

    footer_to_change->size = to_free_ptr->size;

	
    TO_UNUSED(footer_to_change);

	if (to_free_ptr->prev != NULL) {

		to_free_ptr->prev->next = to_free_ptr->next;

		if (to_free_ptr->next != NULL) {
			to_free_ptr->next->prev = to_free_ptr->prev;
		}
	}
 
	//put it at the front
	to_free_ptr->next = freelist; 
	to_free_ptr->prev = NULL;

	freelist->prev = to_free_ptr;

	freelist = to_free_ptr;

	/** Done with free **/


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


	freelist = (metadata_t*) sbrk(max_bytes); //create a footer at the beginning of the list to satisfy the general testing case in coalescing
  	
  	if (freelist == (void *)-1)
		return false;
  	
  	head = freelist;
    tail = freelist+ MAX_HEAP_SIZE;

	freelist->next = NULL;
	freelist->prev = NULL;
	freelist->size = max_bytes - METADATA_T_ALIGNED - FOOTER_T_ALIGNED;
    
	//freelist->is_free = true;

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