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
} metadata_t;

/*since size is always a multiple of 8, we can use the last one bit in its binary
 representation to denote whether this block is used or not
 0 : unused
 1 : used
 */

#define USE(ptr) (ptr->size += 1)
#define UNUSE(ptr) ( ptr->size = (ptr->size - (ptr->size%8)) )


/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing 
 */

static metadata_t* freelist = NULL;
static metadata_t* head = NULL;

void* dmalloc(size_t numbytes) {
	if(freelist == NULL) { 			//Initialize through sbrk call first time
		if(!dmalloc_init())
			return NULL;
	}
 
	assert(numbytes > 0);

	/* Your code goes here */
	
    size_t numbytes_aligned = ALIGN(numbytes); //align the requested numbytes
    
    metadata_t* cur_freelist_ptr = freelist; //set the current ptr to the head of the freelist
    
    printf("curr_freelist size is %zu \n", cur_freelist_ptr->size);

    int requiredSpace = (numbytes_aligned + METADATA_T_ALIGNED);
    
    printf("required space is : %d \n", requiredSpace);
    
    while (cur_freelist_ptr->size < requiredSpace) {
        if (cur_freelist_ptr->next == NULL){
            return NULL;
        } else {
            cur_freelist_ptr = cur_freelist_ptr->next;
        }
    }
    
    //SPLIT
    
    
    metadata_t* new_freelist_ptr = (metadata_t*) (((void*)cur_freelist_ptr) + METADATA_T_ALIGNED + numbytes_aligned);
    
    
    if (cur_freelist_ptr->next != NULL && cur_freelist_ptr->prev != NULL){
        cur_freelist_ptr->prev->next = new_freelist_ptr;
        cur_freelist_ptr->next->prev = new_freelist_ptr;
        new_freelist_ptr->prev = cur_freelist_ptr->prev;
        
    }
    else if (cur_freelist_ptr->next != NULL && cur_freelist_ptr->prev == NULL){
        new_freelist_ptr->prev = NULL;
        new_freelist_ptr->next = cur_freelist_ptr->next;
        
        new_freelist_ptr->next->prev = new_freelist_ptr;
        
        freelist = new_freelist_ptr;
    }
    else if (cur_freelist_ptr->next == NULL && cur_freelist_ptr->prev != NULL){
        
        new_freelist_ptr->next = NULL;
        new_freelist_ptr->prev = cur_freelist_ptr->prev;
        new_freelist_ptr->prev->next = new_freelist_ptr;
        
    }
    else if (cur_freelist_ptr->next == NULL && cur_freelist_ptr->prev == NULL){
        freelist = new_freelist_ptr;
    }
    
//    if (cur_freelist_ptr->prev != NULL) {
//        cur_freelist_ptr->prev->next = new_freelist_ptr;
//    }
//    
//    if (cur_freelist_ptr->next != NULL) {
//        cur_freelist_ptr->next->prev = new_freelist_ptr;
//    }
//    
//    if (cur_freelist_ptr == freelist && cur_freelist_ptr->next == NULL) { //this is the case when cur_freelist is pointing to the only metadata in the memory
//        freelist = new_freelist_ptr;
//    }
    
    new_freelist_ptr->size = cur_freelist_ptr->size - numbytes_aligned - METADATA_T_ALIGNED;

    cur_freelist_ptr->size = numbytes_aligned;
    
    USE(cur_freelist_ptr);
    
    cur_freelist_ptr->next = NULL;
    cur_freelist_ptr->prev = NULL;
    
    printf("Malloc done, freelist now at : %p\n", freelist);

	return (((void*)cur_freelist_ptr) + METADATA_T_ALIGNED);
}

void dfree(void* ptr) {

	/* Your free and coalescing code goes here */
    metadata_t* to_free_ptr = (metadata_t*) (((void*)ptr) - METADATA_T_ALIGNED);
    
    UNUSE(to_free_ptr);
    
    metadata_t* freelist_prev = NULL;
    metadata_t* freelist_curr = freelist; //freelist ptr for dfree
    
    //loop through freelist, find the correct position of to_free_ptr
    
    while (1) {
        if (freelist_prev == NULL && freelist_curr > to_free_ptr) {
            // to_free_ptr should be at the very beginning of the freelist
            to_free_ptr->next = freelist_curr;
            to_free_ptr->prev = NULL;
            freelist_curr->prev = to_free_ptr;
            freelist = to_free_ptr;
            
            //could only possibly coalesce with the block behind it
            metadata_t* my_ptr = to_free_ptr;
            metadata_t* back_ptr =(metadata_t*)( ((void*)to_free_ptr) + to_free_ptr->size + METADATA_T_ALIGNED);
            if ( freelist_curr == back_ptr ){
                coalesce_with_back((void*)back_ptr,(void*)my_ptr);
            }
            break;
        }
        
        if (freelist_prev < to_free_ptr && freelist_curr > to_free_ptr){
            // to_free_ptr should be inserted between here
            to_free_ptr->next = freelist_curr;
            freelist_curr->prev = to_free_ptr;
            
            to_free_ptr->prev = freelist_prev;
            freelist_prev->next = to_free_ptr;
            
            //could possibily coalesce with both front block and behind block
            metadata_t* my_ptr = to_free_ptr;
            metadata_t* back_ptr = (metadata_t*)(((void*)to_free_ptr) + METADATA_T_ALIGNED);
            
            if ( freelist_curr == back_ptr ){
                coalesce_with_back((void*)back_ptr, (void*)my_ptr);
            }
            
            if (  (metadata_t*)(((void*)freelist_prev) + freelist_prev->size + METADATA_T_ALIGNED ) == my_ptr ) {
                coalesce_with_front((void*)freelist_prev, (void*)my_ptr);
            }
            break;
        }
        
        if (freelist_curr->next == NULL && freelist_curr < to_free_ptr){
            // to_free_ptr should be the new ending of the freelist
            freelist_curr->next = to_free_ptr;
            to_free_ptr->prev = freelist_curr;
            to_free_ptr->next = NULL;
            
            //could only possibly coalesce with the block in front of it
            metadata_t* my_ptr = to_free_ptr;

            if (   (metadata_t*)(((void*)freelist_curr) + freelist_curr->size + METADATA_T_ALIGNED ) == my_ptr ) {
                coalesce_with_front((void*)freelist_curr, (void*)my_ptr);
            }
            break;
        }
        
        freelist_prev = freelist_curr;
        freelist_curr = freelist_curr->next;
    }
    
    
    //finish dfree and coalesce
    printf("Free done, freelist now at : %p\n", freelist);
    
}


void coalesce_with_back(void* back_ptr, void* my_ptr){
    metadata_t* back = (metadata_t*)back_ptr;
    metadata_t* my = (metadata_t*)my_ptr;
    
    my->size += METADATA_T_ALIGNED + back->size;
    
    my->next = back->next;
    
    if(back->next != NULL){
        back->next->prev = my;
    } else {
        //do nothing
    }
}


void coalesce_with_front(void* front_ptr, void* my_ptr){
    metadata_t* front = (metadata_t*) front_ptr;
    metadata_t* my = (metadata_t*) my_ptr;
    
    front->size += METADATA_T_ALIGNED + my->size;
    
    front->next = my->next;
    
    if(my->next != NULL){
        my->next->prev = front;
    } else {
        //do nothing
    }
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
	freelist = (metadata_t*) sbrk(max_bytes); // returns heap_region, which is initialized to freelist
	/* Q: Why casting is used? i.e., why (void*)-1? */
	if (freelist == (void *)-1)
		return false;
    
    head = freelist;
	freelist->next = NULL;
	freelist->prev = NULL;
	freelist->size = max_bytes-METADATA_T_ALIGNED;
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
