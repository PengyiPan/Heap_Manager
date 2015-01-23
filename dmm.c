#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

/*

Group members: Pengyi Pan (pp83), Yubo Tian (yt65), and Chun Sun Baak (cb276)
    
Files submitted: dmm.c;

Source for reference: Randal E. Bryant and David R. O’Hallaron. Dynamic Memory Management, Chapter 9 from Com- puter Systems: A Programmer’s Perspective, 2/E (CS:APP2e). 
http://www.cs.duke.edu/courses/ compsci310/current/internal/dynamicmem.pdf.
We did not consult Alexander or Professor for this project.

Approximate time spent on the project: twenty hours per member.


This READ_ME is organized as follows:

(1) Summary;
(2) descriptions of and considerations made for metadata and the footer;
(3) descriptions of and considerations made for dmalloc;
(4) descriptions of and considerations made for dfree and coalesce; 
(5) results for heap size of 4MB; and
(6) reflection.

(1) Summary:

Our code uses an unsorted, doubly-linked list of free blocks for dynamic memory management. 
Our metadata contain information on the size of the free block, pointer to the previous free block, pointer to the next free block, 
and an indicator value that tells us whether that block is currently in use. 
The size of our metadata is the same as that of the default one, but we are able to squeeze in an additional 
piece of information — whether the the block in question is currently in use — by taking advantage of the “free” first bit of the size. 
More information on the metadata appears in the subsequent section.
Additionally, we employ a footer for each block that is allocated. Doing so allowed us to perform a O(1) operation for dfree and coalesce, 
because we would be able to calculate the physical addresses of the block immediately before the one in question.
Hence, we would be able to perform coalesce in an O(1) manner, since we need not go through the sorted list in determining where the freed block belongs.
Since the list need not be sorted anymore, for dfree, we are simply able to add the freed block to the start of the free list, an O(1) operation.
The tradeoffs considered in making this footer are described in section (2).

Finally, our dmalloc employs a first-fit algorithm but is still able to achieve a success rate over 80%. 
Specifically, dmalloc loops through the list of free blocks until it encounters the first block whose
size is bigger than (requested space + size of the medatadata and footer), aligned on a longword boundary.
After we split the free block into two, we allocate the first block to the user and remove it from the free list; 
the second block, on the other hand, is kept in the free list. 

Overall, the code exhibits a 81.56% success rate for test_stress2. 



(2) Descriptions of and considerations made for metadata and footer:

The components of our metadata for each block are very similar to the original provided:
each metadatum contains information on the size of the block, pointer to the previous free block,
and pointer to the next free block. However, we improve on the default metadata by also inserting a flag that indicates
whether a block is free without having to increase the size of the overhead. We do so by taking advantage of the fact that
the size of each block is aligned on a longword boundary. This feature implies that the first three bits of the size on the right will always be zero;
thus, the first bit does not convey any information on the size of the block. We exploit this feature by
changing the first bit of the size to “1” if the block is currently being used and “0” otherwise. 
By doing so, we are able to tell whether a block is free merely by looking at its metadata without having to increase the size of the overhead.

We have included a footer for each block that is allocated that includes the size of the block and on whether the block is free.
Doing so allowed us to perform a dfree and coalesce in O(1), because we would be able to know whether the block right before the block in question 
is currently free or not simply by performing an arithmetic operation to get to its footer. Because we are able to directly calculate the pointer to the
adjacent blocks, we have no need to sort the list of free blocks. Thus, we simply attach the newly freed block to the start of the free list,
effectively also making dfree O(1).

The tradeoff of including footter would be that doing so adds a burden to the overhead, increasing the size of it to increase by 
8 bytes, or an 25~50% increase of the original overhead. In response, we tried to shrink the size of the metadata and the footer as much as posible.
For instance, as previously mentioned, we got rid of a separate boolean value "is_free," which would take up an additional 8 bytes, and integrated the
information to that conveying the size of the block. In the end, we acknowleedged that including a footer would be the only way through which we can leave the
list of free blocks unsorted and still coalesce the blocks, so we included the footer.

Beside the characteristics included in the final version, we have also considered many options in deciding what should be included in the metadata. 
For instance, we considered removing the previous pointer and keeping only the next pointer for the metadata.
Doing so would allow us to reduce the overhead of the metadata. However, we eventually ruled against that idea because
it impeded the code from quickly removing the newly allocated block from the free list.


(3) Descriptions of and considerations made for dmalloc:

Our dmalloc employs a first-fit algorithm. Specifically, dmalloc loops through the list of free blocks until it encounters the first block whose size is
bigger than the sum of the requested space and the overhead , aligned on a longword boundary. After we split the free block into two, 
we allocate the first block to the user and remove it from the free list; the second block, on the other hand, is kept in the free list. 

Currently, our dmalloc splits a free block into two if its size is bigger than that requested by the user. 
Alternatively, we have considered splitting the block only if the size of the free block is /significantly/ bigger than the requested size.
The advantage of such practice would be that it would prevent a free block whose size is fairly close to the requested size from splitting into two 
and leaving a free block whose size is too small to be frequently usable. The disadvantage of it, on the other hand, would be that in the case that
the user only needs such small size, not splitting would prevent the user from accessing that memory. In the end, 
it was not clear that the advantage would outweigh the disadvantage. Moreover, setting the cutoff from which a block will be split into two blocks
seemed arbitrary without any further empirical information. Thus, we opted not to employ this alternative.

Another option we considered was that of implementing the best-fit algorithm. Again, however, we came to the conclusion that it is unclear whether
such implementation would enhance the efficiency of our code. Specifically, two opposing arguments could be made for such strategy. 
An argument in favor of the best-fit strategy would suggest that by doing so, we would be able to keep some big free blocks as they are until the user
requests a correspondingly big size of memory, i.e. we would be able to avoid some fragmentation. An argument opposing to it, however, 
would contend that by choosing the free block whose size is closest to the block, after it is split into two, the size of the remaining free block
would be too small to be useful. In the end, because the  effect of the best-fit strategy is ambiguous, we opted not to employ this alternative.
Moreover, we were still able to achieve an 80% success rate with the first-fit strategy.


(4) Descriptions of and considerations made for dfree and coalesce:

When the user attempts to free the memory, we first see if it is applicable to coalesce the block by checking whether the blocks adjacent to the block being freed
is currently free. We can do so in constant time: for the block behind it, we can simply calculate the pointer of its metadata and see if it says that that block is free.
Similarly, for the block ahead of it, we can simply calculate the pointer of its footer and see if it says that that block is free.
We have created a separate function for coalescing for the sake of readability of the code. 
When the coalesced block is obtained, we simply attach this block to the head of the free list. Doing so only takes O(1) operation.
Finally, we deal with the edge cases — when the block being freed should be at the very beginning or the end of the free list — separately.

We considered sorting the likned list each time we allocate or free the block. We came to the conclusion that doing so would be superfluous
since we need not have the list sorted if we use the footer. As mentioned previously, using the footer, we were able to shrink the running time to
O(1); the cost was that the size of our overhead increased. We were sitl able to achieve a success rate of over 80% on test_stress2, however, and
we acknowledged that such increase in overhead was a necessary cost for decreasing the running time to O(1).


(5) Results for heap size of 4MB:

test_basic: “Basic testcases passed!”
test_coalesce: “Coalescing test cases passed!”
test_stress1: “Stress testcases1 passed!”
test_stress2: “Loop count: 50000, malloc successful: 40781, malloc failed: 9219, execution time: 0.015495 seconds Stress testcases2 passed!“ (The success percentage, when calculated, is 81.562%.)

In the end, with an unsorted, doubly-linked freee list and a first-fit algorithm, we were able to achieve a success rate higher than 80%. 

NOTE: our code properly works when the max heap size is 4MB.


(6) Reflection:

Through this project, we were above all able to learn about C in depth. This project also taught us the importance of “drawing” the program so as to make its operations more transparent and easily understandable. A group project of this magnitude was the first for some members in the group. To them, this project reminded them of the importance of communicating and synthesizing ideas in a group project. Finally, debugging the code was a considerably difficult task to do. This task reminded us one again on the importance of repeatedly writing and testing only small chunks of code. Finally, through this project, we were able to get familiar with Git and realized how useful it is.



Best,

Pengyi Pan (pp83), Yubo Tian (yt65), and Chun Sun Baak (cb276)

*/



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
     Size is the size of the block the footer belongs to.
     */
    // footer->size contains only numbytes, excluding the header's size
    size_t size;
    
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

static metadata_t* freelist = NULL;
static metadata_t* head = NULL; //this pointer will always point to the beginning of the sbrk call, where no footer is in front of it
static metadata_t* tail = NULL; // this points to the tail of the whole memory block

void* dmalloc(size_t numbytes) {
    
    //Initialize freelist through sbrk call first time
    
    if(freelist == NULL) {
        if(!dmalloc_init()) {
            return NULL;  //if freelist is successfully initiated, won't return NULL
        }
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