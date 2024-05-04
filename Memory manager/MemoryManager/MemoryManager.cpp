#include "MemoryManager.h"

using namespace std;

MemoryBlocks::MemoryBlocks(size_t size,size_t position, bool isHole) {
    this->isHole = isHole;
    this->size = size;
    this->position = position;
    this->next = nullptr;
    this->prev = nullptr;
};

MemoryManager::MemoryManager(unsigned wordSize, function<int(int, void *)> allocator) {
    this->allocator = allocator;
    this->wordSize = wordSize;
    this->numOfWords = 0;
    this->memoryInTotal =0;
    this->blockInitAddress = nullptr;
    this->bitmap = nullptr;
    this->head = nullptr;
}

MemoryManager::~MemoryManager() {
    if(blockInitAddress) {
        this->shutdown();
    }
}

void MemoryManager::initialize(size_t sizeInWords) {
    if(sizeInWords <= 65536) {
        if(head !=  nullptr) {
            this->shutdown();
        }
        memoryInTotal = sizeInWords * wordSize;
        blockInitAddress = new uint8_t[memoryInTotal];
        head = new MemoryBlocks(sizeInWords,0, false);
    }
}

void MemoryManager::shutdown() {
    MemoryBlocks* current = head;
    for (int i=0;i<memoryInTotal;i++) {
        if(current==nullptr)
            break;
        MemoryBlocks* next1 = current->next;
        delete current;
        current = next1;
    }
    delete[] blockInitAddress;
    blockInitAddress = nullptr;
    head = nullptr;
    numOfWords = 0;
    memoryInTotal = 0;
}


void* MemoryManager::allocate(size_t sizeInBytes) {
    void* list = getList();
    if(list == nullptr) {
        return nullptr;
    }
    int sizeAllocate = ceil((float) sizeInBytes / (float) wordSize);
    int allocateIndex = allocator(sizeAllocate, list);
    delete[] (size_t*)list;

    if(allocateIndex == -1) {
        return nullptr;
    }
    MemoryBlocks* current = head;
    for(auto i=0;i<memoryInTotal;i++) {
        if (current==nullptr)
            return nullptr;
        if(current->position == allocateIndex) {
            if(current->size > sizeAllocate) {
                size_t remainingSize = current->size - sizeAllocate;
                MemoryBlocks* newBlock = new MemoryBlocks(remainingSize, current->position+sizeAllocate,false);
                newBlock->next = current->next;
                current->next = newBlock;
                current->size = sizeAllocate;     
                newBlock->prev = current;  
            }
            current->isHole = true;
            uint8_t *result = blockInitAddress+allocateIndex * wordSize;
            return (void*)result;

        }
        current = current->next;
    }
    return nullptr;
}

void MemoryManager::free(void *address) {
    if(address == nullptr) {
        return;
    }
    int globalIndex = (static_cast<uint8_t*>(address) - blockInitAddress)/wordSize;
    MemoryBlocks* current = head;
    MemoryBlocks* previous = nullptr;
    while(current!=nullptr) {
        if(current->position== globalIndex) {
            current->isHole = false;
            if(current->next != nullptr && current->next->isHole == false) {
                MemoryBlocks* theNext = current->next;
                current->size = current->size + current->next->size;
                current->next = current->next->next;
                delete theNext;
            }
            if(previous != nullptr && previous->isHole == false) {
                previous->size = current->size + previous->size;
                previous->next = current->next;
                delete current;
            }
            return;
        }
        previous = current;
        current = current->next;
        
    }
}

void MemoryManager::setAllocator(function<int(int, void *)> allocator) {
    this -> allocator = allocator;
}

int MemoryManager::dumpMemoryMap(char *filename) {
    MemoryBlocks* current = head;
    int file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0777);
    if(file == -1) {
        return -1;
    }
    ostringstream fileStream;
    for(int i=0;i<memoryInTotal;i++) {
        if(current == nullptr)
            break;
        if(!current -> isHole) {
            fileStream << "[" << current->position << ", " << current->size << "]";
            if(current->next != nullptr){
                fileStream << " - ";
            }
        }
        current = current->next;
       
    }
    string stringWrite = fileStream.str();
    if(write(file, stringWrite.c_str(), stringWrite.length()) == -1) {
        close(file);
        return -1;
    }
    else {
        close(file);
            return 0;
    }
}

void* MemoryManager::getList() {
    if(head==nullptr) {
        return nullptr;
    }
    vector<MemoryBlocks*> holeVector;
    MemoryBlocks* current = head;
    for(int i=0;i<memoryInTotal;i++) {
        if(current== nullptr)
            break;
        if(!current -> isHole) {
            holeVector.push_back(current);                
        }
        current = current->next;
    }
    vector<size_t> result;
    result.push_back(holeVector.size());
    for(int i = 0; i < holeVector.size(); i++) {
        result.push_back(holeVector[i]->position);
        result.push_back(holeVector[i]->size);
    }
    uint16_t* returnArray = new uint16_t[result.size()];
    for(auto i =0; i<result.size();i++) {
        returnArray[i] = result[i];
    }
    
    return returnArray;
}

void* MemoryManager::getBitmap() {
    vector<uint8_t> bitMap;
    MemoryBlocks* current = head;
    for(auto i=0;i<memoryInTotal;i++) {
        if(current == nullptr)
            break;
        for(auto j =0; j<current->size;j++) {
            if(current->isHole) {
                bitMap.push_back(1);
            }
            else {
                bitMap.push_back(0);
            }
            
        }
        current = current->next;
    }
    int padding = 8 - bitMap.size()%8;
    if (padding != 8) {
        for(auto i=0;i<padding;i++) {
            bitMap.push_back(0);
        }
    }
    vector<uint8_t> byteMap;
    for(size_t i = 0; i < bitMap.size(); i += 8) {
        uint8_t byte = 0;
        for(size_t j = 0; j < 8; j++) {
            if(bitMap[i+j] == 1) {
                byte |= 1 << j;
            }
        }
        byteMap.push_back(byte);
    }
    uint16_t byteNum = byteMap.size();
    uint8_t high = static_cast<uint8_t>(byteNum >> 8);
    uint8_t low = static_cast<uint8_t>(byteNum & 0xFF);
    byteMap.insert(byteMap.begin(),high);
    byteMap.insert(byteMap.begin(),low);
    uint8_t* returnBitMap = new uint8_t[byteMap.size()];
    current = nullptr;
    delete current;
    for(auto i=0;i<byteMap.size();i++) {
        returnBitMap[i] = byteMap[i];
    }

    return returnBitMap;
}

unsigned MemoryManager::getWordSize() {
    return wordSize;
}

void* MemoryManager::getMemoryStart() {
    return blockInitAddress;
}

unsigned MemoryManager::getMemoryLimit() {
    return memoryInTotal;
}

int bestFit(int sizeInWords, void *list) {
    int remainingSize = INT_MAX;
    int offset = -1;
    uint16_t* temp = static_cast<uint16_t*>(list);
    uint16_t listSize = temp[0];
    for(int i = 2; i < listSize * 2 + 1; i += 2) {
        if(temp[i] >= sizeInWords) {
            int currentRemain = temp[i] - sizeInWords;
            if(currentRemain < remainingSize) {
                remainingSize = temp[i] - sizeInWords;
                offset = temp[i-1];
            }
            if(remainingSize == 0) {
                break;
            }
        }
    }
    return offset;
}

int worstFit(int sizeInWords, void *list) {
    int offset = -1;
    uint16_t* temp = static_cast<uint16_t*>(list);
    uint16_t listSize = temp[0];
    int sizeRemain = 0;
    bool track = true;
    for(int i = 2; i < listSize * 2 + 1; i += 2) {
        if(temp[i] >= sizeInWords) {
            if(track) {
                offset = temp[i-1];
                track = false;
            }
            int currentRemain = temp[i] - sizeInWords;
            if(currentRemain > sizeRemain) {
                sizeRemain = temp[i] - sizeInWords;
                offset = temp[i-1];
            }
        }
    }
    return offset;
}
