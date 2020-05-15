#include "BuddyAllocator.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <math.h>
using namespace std;

LinkedList::LinkedList(uint size) : headerSize(size), head(NULL) {}


void LinkedList::insert(BlockHeader *b) {
    if(!head) {
        b->next = nullptr;
    }
    else{
        b->next = head;
    }
    head = b;
    b->used = true;
    ++listLength;
}

void LinkedList::remove (BlockHeader* b) {
    BlockHeader* curr = head;
    
    if(curr == b){
        if(curr->next != nullptr) { //if our current blockheader we are looking at is not pointing to nullptr then we know we have a populated list.
            head = curr->next; //we are going to make head point to the next object in our list
            --listLength; //remove item
            b->used = false; //free up the block in the list space
        }
        else {
            head = nullptr;
            listLength = 0;
        }
    }
    
    else {
        for(uint i = 1; i < listLength; i++){  //if b isn't the head of our list we must go look for it using this loop
            if(curr->next == b){ //look through list until we can find b.
                if(curr->next->next != nullptr) {  
                    curr->next = curr->next->next; //remove and fix pointer math.
                    --listLength;
                    b->used = false;
                }
                else{ //else b is located somewhere else in the list or the list will be empty.
                    curr->next = nullptr;
                    --listLength;
                    b->used = false;
                }
            }
            else{
                curr = curr->next;
            }
        }
    }
}

BuddyAllocator::BuddyAllocator (uint _basic_block_size, uint _total_memory_size) {
    BasicBlockSize = next_power_of_2(_basic_block_size);
    TotalMemLength = next_power_of_2(_total_memory_size);
    
    if(BasicBlockSize > TotalMemLength) {
        throw std::logic_error("Your block size must be within the boundaries of the ttoal memory length");
    }
    
    start_address = new char[_total_memory_size]; //we are initializing where the allocator start location with the first block input into the freeList
    
    BlockHeader* start_block = (BlockHeader*) start_address; //the start black will point to the blockheader list start address
    start_block->used = true; //obviously if the start block is being added it HAS to be used
    start_block->size = _total_memory_size; //this is just giving default perameters essentially so that the block has a size that correlates with it being the only thing on the list
    start_block->next = NULL; //yes again another default perameter because it will be the only block on the list when we start, thus it will point to nothing.
    
    populateList(freeList);
    
    freeList[getList(start_block->size)].insert(start_block);
    
    /*
    // printf("here1\n");
    // populateList(freeList);  //Create a linnked list for all block sizes.
    int num = (log2(_total_memory_size) - log2(_basic_block_size))+1;
    //printf("num = %d\n", num);
    freeList.resize(num);
    //printf("here2\n");
    //freeList[getList(start_block->block_size)].insert(start_block); //here we are finally inserting the starting block / first block header to the used list.
    freeList.at(num-1).insert(start_block);
    //printf("here3\n");
    */
}

BuddyAllocator::~BuddyAllocator(){
    delete[] start_address; //delete all of the alloc. blocks that are the size of the total memory size.
}

uint BuddyAllocator::next_power_of_2(uint x) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    ++x;
    return x;
    
}

int BuddyAllocator::getList(uint size) {
    
    size = next_power_of_2(size);
    for (int i = 0; i < freeList.size(); i++) {
        
        if(freeList[i].getSize() == size) {
            return i;
        }
    }
    return -1;
}

void BuddyAllocator::populateList(vector<LinkedList>& fl) {
    
    uint block_size = BasicBlockSize;
    while(block_size <= TotalMemLength) {
        
        freeList.push_back(LinkedList(block_size)); //pushing our given blocks into the list to be used.
        block_size *= 2; //multiply the given block size by two
        
    }
}


char* BuddyAllocator::getbuddy(char* addr) {
    
    BlockHeader* temp = (BlockHeader*) addr;  //find the address of the buddy block
    return ((addr - start_address)^temp->size) + start_address; //return the address of the given buddy block to the user.
}

bool BuddyAllocator::arebuddies(char* block1, char* block2){
    return ((getbuddy(block1) == block2) && (getbuddy(block2) == block1)); //this is just a boolean call to the getbuddy function.
    
}

char* BuddyAllocator::merge(char* block1, char* block2) {
    
    BlockHeader* h1 = NULL;
    BlockHeader* h2 = NULL;
    
    if(block2 > block1) { 
        h1 = (BlockHeader*) block1; //if block2 is larger then that is ok and we will assign respective values! 
        h2 = (BlockHeader*) block2;
    }
    else{
        h1 = (BlockHeader*) block2; //else we need to make the first header always larger than the second one.
        h2 = (BlockHeader*) block1;
    }
    if(!arebuddies(block1,block2)) { //if they aren't buddies then we don't want the blocks.
        return NULL;
    }
    else if(h1->size != h2->size){ //if the sizes are not equal then they cannot be buddies and we do not want that pair as well.
        return NULL;
    }
    else {
        freeList[getList(h1->size)].remove(h1); //remove the two blocks so that we can properly remove them from the free list and allocate them.
        freeList[getList(h1->size)].remove(h2);
        
        h1->size *= 2;
        
        char* block = (char*) h1;
        return block;
    }
}

char* BuddyAllocator::split(char* block) {
    
    BlockHeader* bHeader = (BlockHeader*) block;
    int default_size = bHeader->size;
    bHeader->used = true;
    bHeader->size /= 2; //cut the blocks in half obviously because the split function.
    char* block2 = block + bHeader->size;
    BlockHeader* newBlock = (BlockHeader*)(block2);
    newBlock->used = true;
    newBlock->size = bHeader->size;
    
    
    freeList[getList(default_size)].remove(bHeader); //here we are removing blocks and inserting the updated newblocks so that we can stay consistent with our sizes.
    freeList[getList(newBlock->size)].insert(newBlock);
    freeList[getList(newBlock->size)].insert(bHeader);
    
    return (char*) bHeader;
}

char* BuddyAllocator::alloc(uint _length) {
    
    uint requiredSize = next_power_of_2(_length + sizeof(BlockHeader));
    
    if(requiredSize < BasicBlockSize) { //blocks cannot be smaller than the basic block size
        requiredSize = BasicBlockSize;
    }
    if(requiredSize > TotalMemLength){ //check that there is enough needed space.
        cerr << "The memory request is out of bounds of the total memory length." << endl;
        return 0;
    }
    
    BlockHeader* currentBlock = nullptr;
    bool located = false;
    for(int i = 0; i < freeList.size(); ++i){ 
        //find the smallest available block that can fit 
        if(freeList[i].getSize() >= requiredSize && freeList[i].listLength != 0){
            currentBlock = freeList[i].getHead();
            located = true;
            break;
        }
    }
    if(!located){
        cerr << "There are no blocks available for the memory length being requested." << endl;
        return 0;
    }
    char* addr = (char*) currentBlock;
    while(currentBlock->size / 2 >= requiredSize){
        //split the blocks until we have the needed size.
        addr = split(addr);
        currentBlock = (BlockHeader*) addr;
    }
    //remove the block from the freeList and return it.
    currentBlock->used = false;
    freeList[getList(currentBlock->size)].remove(currentBlock);
    
    addr = addr + sizeof(BlockHeader);
    
    return addr;
    
}

int BuddyAllocator::free (char* _a) {
    //Point to the start of the block rather than at the end.
    char* headerPtr = _a - sizeof(BlockHeader);
    BlockHeader* header = (BlockHeader*) headerPtr;
    //find a buddy for the header.
    char* buddyPtr = getbuddy(headerPtr);
    BlockHeader* buddy = (BlockHeader*) buddyPtr;
    //merge blocks until there are no more free buddies.
    while(buddy->used) {
        headerPtr = merge(headerPtr, buddyPtr);
        buddyPtr = getbuddy(headerPtr);
        header = (BlockHeader*) headerPtr;
        buddy = (BlockHeader*) buddyPtr;
    }
    
    header = (BlockHeader*) headerPtr;
    freeList[getList(header->size)].insert(header);
    header->used = true;
    
    return 0;
}

void BuddyAllocator::debug() {
    cout << endl;
    for (int i = 0; i < freeList.size(); ++i) {
        
        BlockHeader* curr = freeList[i].getHead();
        int size = freeList[i].getSize();
        cout << size << ":" << freeList[i].listLength << endl;
        
    }
}

void BuddyAllocator::printlist (){
  cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  int64_t total_free_memory = 0;
  for (int i=0; i<freeList.size(); i++){
    int blocksize = ((1<<i) * BasicBlockSize); // all blocks at this level are this size
    cout << "[" << i <<"] (" << blocksize << ") : ";  // block size at index should always be 2^i * bbs
    int count = 0;
    BlockHeader* b = freeList[i].getHead();
    // go through the list from head to tail and count
    while (b){
      total_free_memory += blocksize;
      count ++;
      // block size at index should always be 2^i * bbs
      // checking to make sure that the block is not out of place
      if (b->size != blocksize){
        cerr << "ERROR:: Block is in a wrong list" << endl;
        exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;
    cout << "Amount of available used memory: " << total_free_memory << " byes" << endl;  
  }
}
