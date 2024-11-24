#include "cache.h"

#define ll long long

ll readCounter=0, writeCounter=0;

// helper functions

bool isValidConfig(ll  cs, ll bs, ll sa){ // needs more analysis
    if((float)cs/(bs*sa) - (int)(cs/(bs*sa)) < 0.0001){ // ignoring precision errors
        return true;
    }
    else {
        return false;
    }
}

ll hexToDec(char hexVal[]) {
    int len = strlen(hexVal);

    // Initializing base value to 1, i.e 16^0 
    ll base = 1;
    ll decVal = 0;

    for (int i=len-1; i>=2; i--){ //2 to avoid 0x
        if (hexVal[i]>='0' && hexVal[i]<='9'){
            decVal += (hexVal[i] - '0')*base;
        }
        else if (hexVal[i] >= 'a' && hexVal[i] <= 'f'){
            decVal += (hexVal[i] - 'a' + 10) * base;
        }
        base *= 16;
    }
    return decVal;
}

int log2(ll x) {
    int power = 0;
    while(x > 1) {
        x = x>>1;
        power++;
    }
    return power;
}

void incReads(){
    readCounter++;
}

void incWrites(){
    writeCounter++;
}

ll getReads(){
    return readCounter;
}

ll getWrites(){
    return writeCounter;
}

std::string Cache::getPolicy(){
    return policy;
}

// Cache class

Cache::Cache(ll cacheSize, ll blockSize, ll setAssociativity, int level, std::string policy){
    
    // 주어진 캐시 구성(cacheSize, blockSize, setAssociativity)이 유효한지 확인
    if(!isValidConfig(cacheSize, blockSize, setAssociativity)){
        printf("Invalid Cache configuration\n");
    }

    // 입력된 캐시 속성들을 객체 변수에 저장
    this->cacheSize = cacheSize;
    this->blockSize = blockSize;
    this->setAssociativity = setAssociativity;
    this->level = level;
    this->policy = policy;
    this->memAccs = 0;

    // 캐시 블록 메모리를 동적 할당 (총 캐시 크기 / 블록 크기만큼 공간 할당)
    cacheBlocks = (ll*)malloc(cacheSize/blockSize * sizeof(ll));
    if(cacheBlocks == NULL){ // 메모리 할당 실패 처리
        printf("Failed to allocate memory for L%d cache\n", this->level);
        exit(0);
    }

    // 세트의 개수를 계산: 캐시 크기 / (블록 크기 * 연관도)
    numberOfSets = cacheSize/(blockSize*setAssociativity);

    // 블록 크기를 기반으로 오프셋 크기 계산 (log2(blockSize))
    offsetSize = log2(blockSize);

    // 세트 개수를 기반으로 인덱스 크기 계산 (log2(numberOfSets))
    indexSize = log2(numberOfSets);
}

void Cache::incHits(){
    hits++;
}

void Cache::incMisses(){
    misses++;
}

void Cache::incMemAccs(){
    memAccs++;
}

void Cache::incMemAccs(ll num){
    memAccs += num;
}

int Cache::getLevel(){
    return level;
}

ll Cache::getTag(ll address){
    return address>>(indexSize + offsetSize);
}

ll Cache::getIndex(ll address){
    return (address>>offsetSize) & ((1<<indexSize)-1);
}

ll Cache::getMemAccs(){
    return memAccs;
}

ll Cache::getBlockPosition(ll address){
    // 주어진 주소로부터 인덱스 값을 계산
    ll index = getIndex(address);

    // 주어진 주소로부터 태그 값을 계산
    ll tag = getTag(address);

    // 지정된 세트 내에서 태그가 일치하는 블록을 찾음
    ll iterator;
    for(iterator=index*setAssociativity; iterator<(index+1)*setAssociativity; iterator++){
        if(tag == cacheBlocks[iterator]){ // 태그가 일치하면 해당 위치 반환
            return iterator;
        }
    }
    // 세트 내에서 태그가 일치하는 블록을 찾지 못하면 -1 반환 (캐시 미스)
    return -1;
}


void Cache::insert(ll address, ll blockToReplace){
    #ifdef DEBUG
    if(getIndex(address) != blockToReplace/setAssociativity){
        printf("ERROR: Invalid insertion: Address %x placed in block %lld", address, blockToReplace);
    }
    #endif
    incMemAccs();
    cacheBlocks[blockToReplace] = getTag(address);
}

ll Cache::getHits(){
    return hits;
}

ll Cache::getMisses(){
    return misses;
}

float Cache::getHitRate(){
    if(hits+misses == 0){
        return 0;
    }
    return (float)(hits)/(hits+misses);
}

Cache::~Cache(){
    free(cacheBlocks);
}