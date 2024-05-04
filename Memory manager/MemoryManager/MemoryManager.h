#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <list>
#include <sstream>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <iterator>
#include <climits>

using namespace std;

class MemoryBlocks {
public:
    bool isHole;
    size_t size;
    size_t position;
    MemoryBlocks* prev;
    MemoryBlocks* next;
    MemoryBlocks(size_t size,size_t position, bool isHole);
    
};

class MemoryManager {
    private:
        function<int(int, void *)> allocator;
        unsigned wordSize;
        unsigned numOfWords;
        unsigned memoryInTotal;
        uint8_t* blockInitAddress;
        uint8_t* bitmap; 
        MemoryBlocks* head;

    public:
        MemoryManager(unsigned wordSize, function<int(int, void *)> allocator);
        ~MemoryManager();
        void initialize(size_t sizeInWords);
        void shutdown();
        void *allocate(size_t sizeInBytes);
        void free(void *address);
        void setAllocator(function<int(int, void *)> allocator);
        int dumpMemoryMap(char *filename);
        void *getList();
        void *getBitmap();
        unsigned getWordSize();
        void *getMemoryStart();
        unsigned getMemoryLimit();
};

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);
