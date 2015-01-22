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
	struct metadata* prev;
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

	
	int requiredSpace = (numbytes_aligned + FOOTER_T_ALIGNED + METADATA_T_ALIGNED);
    
    //****************if is tail, check numbytes / size
    
    
	while ((cur_freelist->size) < requiredSpace ) { //move the cur_freelist ptr to the start of the block with enough size ///check end
		//printf("curr_block_size %lu < requiredSpace %d \n", cur_freelist->size, requiredSpace);
		if ((cur_freelist->next) == NULL){
			//printf("Not enough space in while loop\n");
			return NULL; //not enough space in freelist
		}else {
			cur_freelist = cur_freelist->next;
		}
	}

    
	// SPLIT step 1: Create footer for the block we're allocating
	//~ptr arith bug~footer_t* new_footer = (footer_t*) (cur_freelist + METADATA_T_ALIGNED + numbytes_aligned);
    footer_t* new_footer = (footer_t*) (((void*)cur_freelist) + METADATA_T_ALIGNED + numbytes_aligned);
    
    
	new_footer->size = numbytes_aligned;
    
    TO_USED(new_footer);

	//new_footer->is_free = false;
    
	// SPLIT step 2: Create metadata for the remaining free block
    //~ptr arith bug~ metadata_t* new_freelist = (metadata_t*) (new_footer + FOOTER_T_ALIGNED);
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
	//printf("New freelist print test: %p\n", new_freelist);

	if (cur_freelist == freelist && cur_freelist->next == NULL) { //this is the case when cur_freelist is pointing to the only metadata in the memory
        //printf("Freelist shift to the right\n");
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
	printf("Malloc done, freelist now at : %p\n", freelist);
    
    

    return (void*) ((void*)cur_freelist + METADATA_T_ALIGNED);
    
} 

void dfree(void* ptr) {

	/* Your free and coalescing code goes here */
    
	//~ptr arith bug~ metadata_t* to_free_ptr = ptr - METADATA_T_ALIGNED;
    metadata_t* to_free_ptr = (metadata_t*) (((void*)ptr) - METADATA_T_ALIGNED);

    //printf("Dfree called. to_free_ptr: %p\n", to_free_ptr);

	/** Start coalescing **/
    
    TO_UNUSED(to_free_ptr);

    //************* add testing case for tail
    
	//check the block behind it
    //~ptr arith bug~ metadata_t* next_block = ptr + (to_free_ptr->size) + FOOTER_T_ALIGNED; //POSSIBLE PROBLEM: cannot find next block metadata
    metadata_t* next_block =  (metadata_t*) (((void*) ptr) + (to_free_ptr->size) + FOOTER_T_ALIGNED);
    //printf("to_free_ptr size: %lu\n", to_free_ptr->size);
    //printf("next_block : %p\n", next_block);


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

 		//printf("new to free ptr seize: %lu\n", to_free_ptr->size);

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

            prev_block->size += FOOTER_T_ALIGNED + METADATA_T_ALIGNED + (to_free_ptr->size) ; //increase the size


            if (to_free_ptr->prev != NULL && to_free_ptr->next == NULL) { // to_free_ptr is tail in freelist
                
                if (prev_block == freelist){ //prev_block is head 

                	prev_block->next = NULL; 
                	prev_block->prev = NULL;

                } else if (prev_block->prev != NULL && prev_block->next != NULL) { //prev_block is in the middle

                	to_free_ptr->prev->next = NULL; //cut the tail
                }

            }
          
            
            else if (to_free_ptr->prev != NULL && to_free_ptr->next != NULL) { // to_free_ptr in the middle of freelist
                
                if (prev_block == freelist){ //prev_block is head

                	to_free_ptr->prev->next = to_free_ptr->next; //bridge 
                	to_free_ptr->next->prev = to_free_ptr->prev;

                } else if (prev_block->prev != NULL && prev_block->next != NULL) { //prev_block in the middle

                	to_free_ptr->prev->next = to_free_ptr->next; //bridge 
                	to_free_ptr->next->prev = to_free_ptr->prev;


                } else if (prev_block->prev != NULL && prev_block->next == NULL){ //prev_block is tail

                 	to_free_ptr->prev->next = to_free_ptr->next; //bridge 
                	to_free_ptr->next->prev = to_free_ptr->prev;               	
                    
                }
                
            }
            
           
            else if (to_free_ptr->prev == NULL && to_free_ptr->next != NULL) { // to_free_ptr is head in freelist
                //prev_block->next = to_free_ptr->next;
                //prev_block->next->prev = prev_block;
                if (prev_block->prev != NULL && prev_block->next != NULL) { //prev_block is in the middle

                	to_free_ptr->next->prev = NULL; // cut the head
                	freelist = to_free_ptr->next; // shift the head

                } else if (prev_block->prev != NULL && prev_block->next == NULL) { //prev_block is tail

                	to_free_ptr->next->prev = NULL; // cut the head
                	freelist = to_free_ptr->next; // shift the head

                }
                
                
            }
            
            
            else if (to_free_ptr->prev == NULL && to_free_ptr->next == NULL){ // NULL, aka: did not coalesce with the 'next block'
                
                if (prev_block == freelist){ //prev_block is head

                	//do nothing

                } else if (prev_block->prev != NULL && prev_block->next != NULL) { //prev_block is in the middle

                    //do nothing

                } else if (prev_block->prev != NULL && prev_block->next == NULL){ //prev_block is tail

                    //do nothing
                }
                
            }
                
                
            to_free_ptr = prev_block;
            
            }
    }
        
		
	/** Done with coalescing **/

	/** Start to free **/
	// We free the list by putting the (coalesced) block in front of the the freelist.
    
	
	TO_UNUSED(to_free_ptr);
	

	footer_t* footer_to_change = (footer_t*) (((void*) to_free_ptr) + METADATA_T_ALIGNED + (to_free_ptr->size));
	// printf("to_free_ptr pos: %p\n", to_free_ptr);
	// printf("to free prt size after coalescing: %lu\n", to_free_ptr->size);
	// printf("footer to change: %p\n", footer_to_change);
	// printf("tail: %p\n", tail);

    footer_to_change->size = to_free_ptr->size;

	
    TO_UNUSED(footer_to_change);

	if (to_free_ptr->prev != NULL) { //if in the middle or tail, create a bridge

		to_free_ptr->prev->next = to_free_ptr->next;

		if (to_free_ptr->next != NULL) {
			to_free_ptr->next->prev = to_free_ptr->prev;
		}
	}

	//start putting to front
 
 	if (to_free_ptr == freelist) {
 		
 		//do nothing

 	} else {

		//put it at the front
		to_free_ptr->next = freelist; 
		to_free_ptr->prev = NULL;

		freelist->prev = to_free_ptr;

		freelist = to_free_ptr;
 	}
	

	printf("Free done, freelist now at : %p\n", freelist);

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
    tail = (((void *)freelist) + MAX_HEAP_SIZE);

    //printf("head in init: %p\n", head);
    //printf("tail in init: %p\n", tail);

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