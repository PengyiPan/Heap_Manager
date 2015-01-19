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
	bool is_free;
} metadata_t;

typedef struct footer {
    /* 
 	 We use footer to reduce the runtime of coalescing and free to O(1)
 	 Size is the size of the block the footer belongs to, the in_use is
 	 a boolean showing whether the block is free or not.
	*/
	size_t size;
	bool is_free;

} footer_t;

#define FOOTER_T_ALIGNED (ALIGN(sizeof(footer_t)))

/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing 
 */

static metadata_t* freelist = NULL;
static metadata_t* head = NULL; //this pointer will always point to the beginning of the sbrk call, where no footer is in front of it

void* dmalloc(size_t numbytes) {
	if(freelist == NULL) { 			//Initialize through sbrk call first time
		if(!dmalloc_init()) {
			return NULL;  //if freelist is successfully initiated, wont return NULL
		}
	}
    
    //after the first time, freelist will not be null, code goes here:

	assert(numbytes > 0);

	/* Your code goes here */

	metadata_t* cur_freelist = freelist; //set the current ptr to the head of the freelist

	size_t numbytes_aligned = ALIGN(numbytes); //align the requested numbytes

	while ((cur_freelist->size) < (numbytes_aligned + FOOTER_T_ALIGNED + METADATA_T_ALIGNED)) { //move the cur_freelist ptr to the start of the block with enough size
		if ((cur_freelist->next) == NULL){
			return NULL;
		}else {			
			cur_freelist = cur_freelist->next;
		}
	}

	footer_t* new_footer = (footer_t*) (cur_freelist + METADATA_T_ALIGNED + numbytes_aligned); //create the footer first

	new_footer->size = numbytes_aligned;
	new_footer->is_free = false;

	metadata_t* new_freelist = (metadata_t*) (new_footer + FOOTER_T_ALIGNED);

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
	
	new_freelist->size = (cur_freelist->size) - numbytes_aligned - FOOTER_T_ALIGNED;
	new_freelist->is_free = true;

	cur_freelist->size = numbytes_aligned; //update the cur_freelist size 
	cur_freelist->is_free = false;
	cur_freelist->next = NULL;
	cur_freelist->prev = NULL;
   
    return (cur_freelist + METADATA_T_ALIGNED);
              
} 

void dfree(void* ptr) {

	/* Your free and coalescing code goes here */
	metadata_t* to_free_ptr = ptr - METADATA_T_ALIGNED;

	/** Start coalescing **/


	//check the block behind it
	metadata_t* next_block = ptr + (to_free_ptr->size) + FOOTER_T_ALIGNED; //POSSIBLE PROBLEM: cannot find next block metadata

	if (next_block->is_free == true) {

		to_free_ptr->prev = next_block->prev;
		to_free_ptr->next = next_block->next;

		if (next_block->prev != NULL) {
			next_block->prev->next = to_free_ptr;
		}
		
		if (next_block->next != NULL) {
			next_block->next->prev = to_free_ptr;
		}

		to_free_ptr->size += FOOTER_T_ALIGNED + METADATA_T_ALIGNED + (next_block->size); //increase the size //POSSIBLE PROBLEM: give back the footer next block or not?

		if (to_free_ptr->prev == NULL && to_free_ptr->next == NULL) { 
			printf("Something wrong with next.\n");
			return;
		}			
	}

	if (to_free_ptr != head) { //since head does not have a foot infront of it, cannot check

		//check the block in front of it
		footer_t* prev_footer = (footer_t*) to_free_ptr - FOOTER_T_ALIGNED; 

		if (prev_footer->is_free == true) {

			metadata_t* prev_block = (metadata_t*) prev_footer - (prev_footer->size) - METADATA_T_ALIGNED;
			
			prev_block->size += FOOTER_T_ALIGNED + METADATA_T_ALIGNED + (to_free_ptr->size); //increase the size

			if (to_free_ptr->prev != NULL && to_free_ptr->next != NULL) { // when the to_free_ptr successfully coalesced the block behind
				prev_block->next = to_free_ptr->next;
				prev_block->next->prev = prev_block;
			}

			to_free_ptr = prev_block;
		}
	}

	/** Done with coalescing **/

	/** Start to free **/
	// We free the list by putting the (coalesced) block in front of the the freelist.

	footer_t* footer_to_change = (footer_t*) (to_free_ptr + METADATA_T_ALIGNED + (to_free_ptr->size));
	footer_to_change->size = to_free_ptr->size;
	footer_to_change->is_free = true;

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
	if (freelist == (void *)-1)
		return false;

	freelist = (metadata_t*) sbrk(max_bytes); //create a footer at the beginning of the list to satisfy the general testing case in coalescing
  	head = freelist;

	freelist->next = NULL;
	freelist->prev = NULL;
	freelist->size = max_bytes - METADATA_T_ALIGNED;
	freelist->is_free = true;

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