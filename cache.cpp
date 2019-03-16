//implementation of cache.h goes here

#include "cache.h"

//implementation of error handling
void printError(std::string errorMessage) {
    std::cout << "ERROR: " << errorMessage << std::endl;
}

//implementation of helper functions

bool isPowerOfTwo(long long x) {
    // First x in the below expression is for the case when x is 0 */
    return x && (!(x&(x-1))); 
}

//x is non_zero
int log2(long long x) {
    int power = 0;
    while(x > 1) {
        x = x>>1;
        power++;
    }
    return power;
}

long long hexadecimalToDecimal(char hexVal[]) {
    int len = strlen(hexVal);

    // Initializing base value to 1, i.e 16^0 
    long long base = 1;

    long long decVal = 0;

    // Extracting characters as digits from last character 
    for (int i=len-1; i>=2; i--) //2 to avoid 0x
    {
        // if character lies in '0'-'9', converting  
        // it to integral 0-9 by subtracting 48 from 
        // ASCII value. 
        if (hexVal[i]>='0' && hexVal[i]<='9')
        {
            decVal += (hexVal[i] - '0')*base;

            // incrementing base by power 
            base = base * 16;
        }

        // if character lies in 'A'-'F' , converting  
        // it to integral 10 - 15 by subtracting 55  
        // from ASCII value 
        else if (hexVal[i] >= 'a' && hexVal[i] <= 'f')
        {
            decVal += (hexVal[i] - 'a' + 10) * base;

            // incrementing base by power 
            base = base*16;
        }
    }

    return decVal;
}

//implementaion of I/O

std::vector<long long> readTrace(char filePath[]) {

    FILE *trace = fopen(filePath, "r");
    return readTrace(trace);
}

std::vector<long long> readTrace(FILE *trace) {
    /******************************************************
    * sample input line -> 0x7f110d39287e: R 0x7ffced08e7f8
    * end of line -> #eof 
    *******************************************************/

    std::vector<long long> addresses; // stores addresses of all data accesses
    char instruction[20], access[5], address[20];

    while(!feof(trace)) {
        fscanf(trace, "%*s %*s %s", address);
        addresses.push_back(hexadecimalToDecimal(address));
    }
    addresses.pop_back();  //last line is read twice

    return addresses;
}

//implementation of CacheLine class

CacheLine::CacheLine() {
    this->valid = false;
    this->tag = 0;
    this->blockSize = 0;
}

CacheLine::CacheLine(int blockSize) {
    this->valid = false;
    this->tag = 0;
    this->blockSize = blockSize;
}

bool CacheLine::isValid() {
    return valid;
}

long long CacheLine::getTag() {
    return tag;
}

void CacheLine::setValid(bool valid) {
    this->valid = valid;
}

void CacheLine::setTag(long long tag) {
    this->tag = tag;
}

void CacheLine::setBlockSize(long long blockSize) {
    this->blockSize = blockSize;
}


//implementation of Cache class

Cache::Cache(int numberOfSets, int blockSize, int setAssociativity) {
    this->numberOfSets = numberOfSets;
    this->setAssociativity = setAssociativity;
    
    //error handling
    bool error = false;
    if(!isPowerOfTwo(numberOfSets)) {
        printError("Cache: Number of sets is not power of 2. (first parameter in Cache constructor)");
        error = true;
    }
    if(!isPowerOfTwo(blockSize)) {
        printError("Cache: Block size is not power of 2. (second parameter in Cache constructor)");
        error = true;
    }
    if(!isPowerOfTwo(setAssociativity)) {
        printError("Cache: Set Associativity is not power of 2. (third parameter in Cache constructor)");
        error = true;
    }
    if(setAssociativity == 0) {
        printError("Cache: Set Associativity cannot be equal to 0. (third parameter in Cache constructor)");
        error = true;
    }
    if(error == true) {
        return;
    }

    numberOfRows = numberOfSets * setAssociativity;
    offsetSize = log2(blockSize);
    indexSize = log2(numberOfSets);
    hits = misses = 0;

    try {
        cacheLines = new CacheLine[numberOfRows];
        nextFreeBlockInSet = new int[numberOfRows];
    } 
    catch (std::bad_alloc xa) { //catch if 
        printError("Cache: Allocation failure");
        return;
    }

    for(int i = 0; i < numberOfRows; i++) {
        cacheLines[i].setBlockSize(blockSize);
        nextFreeBlockInSet[i] = 0;
    }
}

void Cache::incHits() {
    hits++;
}

void Cache::incMisses() {
    misses++;
}

long long Cache::isDataInCache(long long address) {
    long long index = this->getIndexFromAddress(address);
    long long tag = this->getTagFromAddress(address);
    return this->isBlockInCache(index, tag);
}

long long Cache::isBlockInCache(long long index, long long tag) {

    long long i = index*setAssociativity;

    if(cacheLines[i].isValid() == false) { //first time
        return -1;
    }
    
    for( ; i < index*setAssociativity + nextFreeBlockInSet[index]; i++) {
        if(cacheLines[i].getTag() == tag) {
            return i;
        }
    }

    return -1;
}

bool Cache::isSetFull(long long index) {
    return nextFreeBlockInSet[index] == setAssociativity;
}

long long Cache::insertDataToCache(long long address) {
    long long index = this->getIndexFromAddress(address);
    long long tag = this->getTagFromAddress(address);
    return this->insertBlockToCache(index, tag);
}

long long Cache::insertBlockToCache(long long index, long long tag) {
    long long row = index * setAssociativity + nextFreeBlockInSet[index];
    cacheLines[row].setTag(tag);
    cacheLines[row].setValid(true);
    nextFreeBlockInSet[index]++;
    return row;
}


void Cache::evictAndInsertData(long long evictionAddress, long long insertionAddress) {
    long long eIndex = this->getIndexFromAddress(evictionAddress);
    long long eTag = this->getTagFromAddress(evictionAddress);
    long long iIndex = this->getIndexFromAddress(insertionAddress);
    long long iTag = this->getTagFromAddress(insertionAddress);
    evictAndInsertBlock(eIndex, eTag, iIndex, iTag);
}

void Cache::evictAndInsertBlock(long long eIndex, long long eTag, long long iIndex, long long iTag) {
    
    if(eIndex != iIndex) {
        printError("evictAndInsertBlock: Evicting and Incoming block don't belong to the same set!");
        return;
    }
    else if(eTag == iTag) {
        printError("evictAndInsertBlock: Evicting and Inserting block are the same!");
        return;
    }

    long long index = iIndex;
    for(int i = index*setAssociativity; i < index*setAssociativity + nextFreeBlockInSet[index]; i++) {
        if(cacheLines[i].getTag() == eTag) {
            cacheLines[i].setTag(iTag);
        }
    }
}

void Cache::evictAndInsertBlock(long long row, long long insertionAddress) {

    long long eIndex = row / setAssociativity;
    long long iIndex = this->getIndexFromAddress(insertionAddress);
    long long iTag = this->getTagFromAddress(insertionAddress);

    if(eIndex != iIndex) {
        printError("evictAndInsertBlock: Evicting and Incoming block don't belong to the same set!");
        return;
    }

    cacheLines[row].setTag(iTag);
}

double Cache::hitRate() {
    return (double)(hits)/(hits + misses);
}

double Cache::missRate() {
    return (double)(misses)/(hits + misses);
}

long long Cache::getNumberOfHits() {
    return hits;
}

long long Cache::getNumberOfMisses() {
    return misses;
}

long long Cache::getTagFromAddress(long long address) {
    return address>>(offsetSize+indexSize);
}

long long Cache::getIndexFromAddress(long long address) {
    return (((1<<indexSize) - 1) & (address >> offsetSize));
}

void Cache::displayCache() {
    /****************************************
     * Display of one set with two elements
     * 2:              //index
     * |1|2345|        //valid bit and tag
     * |0|    |  
     ***************************************/

    int row = 0;
    for(int set = 0; set < numberOfSets; set++) {
        printf("%d:\n", set);
        for(int posInSet = 0; posInSet < setAssociativity; posInSet++) {
            printf("|%d|%20lld|\n", cacheLines[row].isValid(), cacheLines[row].getTag());
            row++;
        }
    }

    printf("\n");
}

Cache::~Cache() {
    delete[] cacheLines;
    delete[] nextFreeBlockInSet;
}