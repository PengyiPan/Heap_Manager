
Group members: Pengyi Pan (pp83), Yubo Tian (p83), and Chun Sun Baak (cb276)
	
Files submitted: dmm.c;

Source for reference: Randal E. Bryant and David R. O’Hallaron. Dynamic Memory Management, Chapter 9 from Com- puter Systems: A Programmer’s Perspective, 2/E (CS:APP2e). 
http://www.cs.duke.edu/courses/ compsci310/current/internal/dynamicmem.pdf.
Note: We did not consult either Alexander or Professor for this project.

Approximate time spent on the project: twenty hours per member.


This READ_ME is organized as follows:

(1) Summary;
(2) descriptions of and considerations made for metadata;
(3) descriptions of and considerations made for dmalloc;
(4) descriptions of and considerations made for dfree and coalesce; 
(5) results for heap size of 4MB; and
(6) reflection.

(1) Summary:

Our code uses a sorted, doubly-linked list of free blocks for dynamic memory management. Our metadata contain information on the size of the free block, pointer to the previous free block, pointer to the next free block, and an indicator value that tells us whether that block is currently in use. The size of our metadata is the same as that of the default one, but we are able to squeeze in an additional piece of information — whether the the block in question is currently in use — by taking advantage of the “free” first bit of the size. The pointers that point to the previous and next block allow us to sort the list easily. More information on the metadata appears in the subsequent section.

Our dmalloc employs a first-fit algorithm but is still able to achieve a success rate over 80%. Specifically, dmalloc loops through the list of free blocks until it encounters the first block whose size is bigger than (requested space + size of the medatadata that will be used after splitting), aligned on a longword boundary. After we split the free block into two, we allocate the first block to the user and remove it from the free list; the second block, on the other hand, is kept in the free list. 

When the user frees the memory, the code goes through the (sorted) list of free nodes until the pointer to the block is in between two nodes. We check whether we are able to coalesce the blocks by seeing if the blocks right in front and behind it in physical space are contained in the free list. If so, we coalesce the blocks, first with the node behind it and then with the one in front of it. We have created a separate function for coalescing for the sake of readability of the code. Having our list of free blocks sorted entails that the process of freeing and coalescing a block take O(#free blocks). We have considered various ways with which we could do so but in the end opted not to do so for reasons elaborated in subsequent sections. Overall, the code exhibits a 82.872% success rate for test_stress2. 


(2) Descriptions of and considerations made for metadata:

The components of our metadata for each block are very similar to the original provided: each metadatum contains information on the size of the block, pointer to the previous free block, and pointer to the next free block. However, we improve on the default metadata by also inserting a flag that indicates whether a block is free without having to increase the size of the overhead. We do so by taking advantage of the fact that  the size of each block is aligned on a longword boundary. This feature implies that the first bit of the size will always be zero; thus, the first bit does not convey any information on the size of the block. We exploit this feature by changing the first bit of the size to “1” if the block is currently being used and “0” otherwise. By doing so, we are able to tell whether a block is free merely by looking at its metadata without having to increase the size of the overhead. Finally, the pointers that point to the previous and next block allow us to sort the list easily.

We have considered including a footer for each block that is allocated that includes the size of the block and on whether the block is free. Doing so would allow us to perform a timely dfree, because we would be able to know whether the block right before the block in question is currently free or not simply by performing an arithmetic operation to get to its footer. However, the tradeoff would be that doing so added a heavy burden to the overhead, increasing the size of it to increase by 25~50%. In the end, we worked on this version for over fifteen hours; however, we were not able to clean up the various bugs in the code of that version. Thus, we decided to stick with this version without footers.

Thus, we decided to leave it out for efficiency of dmalloc.

Beside the characteristics included in the final version, we have also considered many options in deciding what should be included in the metadata. For instance, we considered removing the previous pointer and keeping only the next pointer for the metadata. Doing so would allow us to reduce the overhead of the metadata. However, we eventually ruled against that idea because it impeded the code from quickly removing the allocated block from the free list.


(3) Descriptions of and considerations made for dmalloc:

Our dmalloc employs a first-fit algorithm. Specifically, dmalloc loops through the list of free blocks until it encounters the first block whose size is bigger than (requested space + size of the medatadata that will be used after splitting), aligned on a longword boundary. After we split the free block into two, we allocate the first block to the user and remove it from the free list; the second block, on the other hand, is kept in the free list. 

Currently, our dmalloc splits a free block into two if its size is bigger than that requested by the user. Alternatively, we have considered splitting the block only if the size of the free block is significantly bigger than the requested size. The advantage of such practice would be that it would prevent a free block whose size is fairly close to the requested size from splitting into two and leaving a free block whose size is too small to be usable. The disadvantage of it would be that in the case that the user only needs such small size, not splitting would prevent the user from accessing that memory. In the end, it was not clear that the advantage would outweigh the disadvantage. Moreover, setting the cutoff from which a block will be split into two blocks seemed arbitrary without any empirical evidence. Thus, we opted not to employ this alternative.

Another option we considered was that of implementing the best-fit algorithm. Again, however, we came to the conclusion that it is unclear whether such implementation would enhance the efficiency of our code. Specifically, two opposing arguments could be made for such strategy. An argument in favor of the best-fit strategy would suggest that by doing so, we would be able to keep some big free blocks as they are until the user requests a correspondingly big size of memory. An argument opposing to it, however, would contend that by choosing the free block whose size is closest to the block, after it is split into two, the size of the remaining free block would be too small to be useful. In the end, because the  effect of the best-fit strategy is ambiguous, opted not to employ this alternative.


(4) Descriptions of and considerations made for dfree and coalesce:

When the user frees the memory, the code goes through the (sorted) list of free nodes until the pointer to the block is in between two nodes. We check whether we are able to coalesce the blocks by seeing if the blocks right in front and behind it in physical space are contained in the free list. We do so by checking the address of the pointers for the blocks in the free list, in the middle of which stands the block currently being freed. If those two indeed correspond to the actual adjacent blocks in the physical space, we coalesce the blocks, first with the node behind it and then with the one in front of it. We have created a separate function for coalescing for the sake of readability of the code. Finally, we deal with the edge cases — when the block being freed should be at the very beginning or the end of the free list — separately.

Having our list of free blocks sorted entails that the process of freeing and coalescing a block take O(#free blocks). If we implement a footer, we would be able to decrease it to O(1). This would be because we would be able to know whether the block right before the block in question is currently free or not simply by performing an arithmetic operation to get to its footer. In that case, the free list would not have to be sorted — we would simply be attach the block being freed to the start of the free list. In the end, however, for the reasons specified in (2), we were unable to employ this version.


(5) Results for heap size of 4MB:

test_basic: “Basic testcases passed!”
test_coalesce: “Coalescing test cases passed!”
test_stress1: “Stress testcases1 passed!”
test_stress2: “Loop count: 50000, malloc successful: 41436, malloc failed: 8564, execution time: 0.02272 seconds. Stress testcases2 passed!“ (The success percentage, when calculated, is 82.872%.)

In the end, with a sorted, doubly-linked freee list and a first-fit algorithm, we were able to achieve a success rate higher than 80%. 

NOTE: our code properly works when the max heap size is 4MB.


(6) Reflection:

Through this project, we were above all able to learn about C in depth. This project also taught us the importance of “drawing” the program so as to make its operations more transparent and easily understandable. A group project of this magnitude was the first for some members in the group. To them, this project reminded them of the importance of communicating and synthesizing ideas in a group project. Finally, debugging the code was a considerably difficult task to do. This task reminded us one again on the importance of repeatedly writing and testing only small chunks of code. Finally, through this project, we were able to get familiar with Git and realized how useful it is.



Best,

Pengyi Pan (pp83), Yubo Tian (p83), and Chun Sun Baak (cb276)

