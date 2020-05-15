#ifndef _BuddyAllocator_h_                   // include file only once
#define _BuddyAllocator_h_
#include <iostream>
#include <cmath>
#include <vector>
using namespace std;
typedef unsigned int uint;

/* declare types as you need */

class BlockHeader{
	
public:
	// think about what else should be included as member variables
	uint size;  // size of the block
	BlockHeader* next = NULL; // pointer to the next block
	bool used; //boolean to check if the block is free.
};

class LinkedList{
	  // this is a special linked list that is made out of BlockHeader s. 
private:
	BlockHeader* head = NULL; // you need a head of the list
	uint headerSize;
	
public:
	/*
	LinkedList() {
		head = nullptr;
		headerSize = 0;
	}
	*/

	uint listLength = 0;
	
	void insert (BlockHeader* b);	// adds a block to the list
	
	void remove (BlockHeader* b);  // removes a block from the list
	
	uint getSize() { return headerSize; }
	
	BlockHeader* getHead() {return head;}
	
	LinkedList(uint size);
};


class BuddyAllocator{
private:
	/* declare more member variables as necessary */
	vector<LinkedList> freeList; 
	uint BasicBlockSize;
	uint TotalMemLength;
	char* start_address; //pointer to the beginning of the total allocated block

private:
	/* private function you are required to implement
	 this will allow you and us to do unit test */
	
	char* getbuddy (char* addr); 
	// given a block address, this function returns the address of its buddy 
	
	bool arebuddies (char* block1, char* block2);
	// checks whether the two blocks are buddies are not
	// note that two adjacent blocks are not buddies when they are different sizes

	char* merge (char* block1, char* block2);
	// this function merges the two blocks returns the beginning address of the merged block
	// note that either block1 can be to the left of block2, or the other way around

	char* split (char* block);
	// splits the given block by putting a new header halfway through the block
	// also, the original header needs to be corrected


public:

	BuddyAllocator(uint _basic_block_size, uint _total_memory_length); 
	/* This initializes the memory allocator and makes a portion of 
	   ’_total_memory_length’ bytes available. The allocator uses a ’_basic_block_size’ as 
	   its minimal unit of allocation. The function returns the amount of 
	   memory made available to the allocator. 
	*/ 

	~BuddyAllocator(); 
	/* Destructor that returns any allocated memory back to the operating system. 
	   There should not be any memory leakage (i.e., memory staying allocated).
	*/ 

	char* alloc(uint _length); 
	/* Allocate _length number of bytes of free memory and returns the 
		address of the allocated portion. Returns 0 when out of memory. */ 
		
	int free(char* _a);
	/* Frees memory that has been allocated by the alloc function.  Should return 
	0 when there is no memory left */
	
	int getList(uint size);
	/* Return the index of the linked list in freeList.*/
	
	void populateList(vector<LinkedList>& fl);
	/* Populates the freeList with a linked list for all block sizes.  Also takes
	into consideration the total memory length*/
		
	uint next_power_of_2(uint x);
	/* returns the index of the linked list in the freeList of a given size. */
	   
	void printlist();
	
	
	void debug();
	/* Mainly used for debugging purposes and running short test cases */
	/* This function prints how many free blocks of each size belong to the allocator
	at that point. It also prints the total amount of free memory available just by summing
	up all these blocks.
	Aassuming basic block size = 128 bytes):

	[0] (128): 5
	[1] (256): 0
	[2] (512): 3
	[3] (1024): 0
	....
	....
	 which means that at this point, the allocator has 5 128 byte blocks, 3 512 byte blocks and so on.*/
};

#endif 