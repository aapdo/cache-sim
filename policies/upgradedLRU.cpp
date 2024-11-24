#include "UpgradedLRU.h"
#include <iostream>
#include <cstdlib>

UpgradedLRU::UpgradedLRU(ll cacheSize, ll blockSize, ll setAssociativity, int level, size_t sectorSize)
    : Cache(cacheSize, blockSize, setAssociativity, level, "UpgradedLRU"), sectorSize(sectorSize) {
    numSectors = blockSize / sectorSize; // 블록당 섹터 개수 계산
    cache.resize(numberOfSets * setAssociativity, CacheLine(numSectors, sectorSize)); // 섹터화된 캐시 구조 생성
    lastUsed = (ll*)calloc(numberOfSets * setAssociativity * numSectors, sizeof(ll)); // 섹터별 LRU 관리용 배열 초기화
    if (lastUsed == NULL) {
        std::cerr << "Failed to allocate memory for LRU tracking in UpgradedLRU cache\n";
        exit(EXIT_FAILURE);
    }
}

ll UpgradedLRU::getBlockPosition(ll address) {
    ll index = getIndex(address);
    ll tag = getTag(address);

    for (ll block = index * setAssociativity; block < (index + 1) * setAssociativity; block++) {
        for (size_t sector = 0; sector < numSectors; sector++) {
            if (cache[block].sectors[sector].valid && tag == cache[block].sectors[sector].tag) {
                return block; // 캐시 히트 시 해당 블록 반환
            }
        }
    }
    return -1; // 캐시 미스
}

ll UpgradedLRU::getBlockToReplace(ll address) {
    ll index = getIndex(address);
    ll min_block = index * setAssociativity;
    bool isEmpty = true;
    for (ll block = index * setAssociativity; block < (index + 1) * setAssociativity; block++) {
        for (size_t sector = 0; sector < numSectors; sector++) {
            if (!cache[block].sectors[sector].valid) {
                isEmpty = false;
            }
        }
        if(isEmpty) return block;

        for (size_t sector = 0; sector < numSectors; sector++) {
            ll sectorIndex = block * numSectors + sector;
            if (lastUsed[sectorIndex] < lastUsed[min_block * numSectors]) {
                min_block = block;
            }
        }
    }
    return min_block; // 가장 오래된 블록 반환
}

void UpgradedLRU::insert(ll address, ll blockToReplace) {
    ll tag = getTag(address);
    ll sector = (address % blockSize) / sectorSize;

    cache[blockToReplace].sectors[sector].valid = true;
    cache[blockToReplace].sectors[sector].dirty = false; // 새로 삽입되므로 더티 아님
    cache[blockToReplace].sectors[sector].tag = tag; // 태그 저장
    update(blockToReplace, 0); // LRU 업데이트
}

// write allocate 정책을 사용함.
// write 명령어를 수행해야 하는데 miss가 발생하는 경우 allocate 후 write 함. 
// 이 때 write buffer를 사용하기 때문에 바로 메모리에 쓰지 않고 write buffer에 저장.
// read / write일 때 write buffer에 있는 데이터를 사용해야 하는 경우 다시 불러오도록 만들었음.
void UpgradedLRU::insert(Access access, ll blockToReplace) {
    ll tag = getTag(access.address);
    // address % blockSzie = block offset
    // offest / sectorSize = sector offset
    ll sector = (access.address % blockSize) / sectorSize;


    // 기존 블록이 유효하고 더티 상태인 경우 evict
    if (cache[blockToReplace].sectors[sector].valid &&
        cache[blockToReplace].sectors[sector].dirty) {
        
        // 더티 데이터를 Write Buffer에 추가
        int blockAddress = cache[blockToReplace].sectors[sector].address;

        if(isInWriteBuffer(blockAddress)) writeBuffer[blockAddress] = cache[blockToReplace].sectors[sector].tag;
        else writeBuffer.insert({blockAddress, cache[blockToReplace].sectors[sector].tag});
        #ifdef DEBUG
        std::cout << "Evicted dirty sector from block " << blockToReplace 
                      << " and added to Write Buffer.\n";
        #endif
             
        // 기존 블록 evict
        evict(blockToReplace);
    }

    cache[blockToReplace].sectors[sector].valid = true;
    cache[blockToReplace].sectors[sector].address = access.address;
    // 명령어 타입에 따라 처리
    if (access.accessType == 's') { // 쓰기 명령어
        cache[blockToReplace].sectors[sector].dirty = true;
        cache[blockToReplace].sectors[sector].tag = tag;
        // Write Buffer에 있는 경우 
        if(isInWriteBuffer(access.address)) {
            // write buffer에 있는 경우 메모리에 접근하지 않고 값만 변경해줌.
            writeBuffer[access.address] = cache[blockToReplace].sectors[sector].tag;
        }
        // 교체해서 새로 insert할 블럭이 write buffer에 없는 경우 메모리에 접근해야 함.
        else {
            writeBuffer.insert({access.address, cache[blockToReplace].sectors[sector].tag});
            incMemAccs();
        }
    } else { // 읽기 명령어
        cache[blockToReplace].sectors[sector].dirty = false; // 읽기는 더티 아님
        if(isInWriteBuffer(access.address)) {
            // write buffer에 있는 경우 메모리에 접근하지 않고 write buffer에서 읽어옴.
            cache[blockToReplace].sectors[sector].tag = writeBuffer[access.address];
        }
        // 교체해서 새로 insert할 블럭이 write buffer에 없는 경우 메모리에 접근해야 함.
        else {
            cache[blockToReplace].sectors[sector].tag = tag;
            incMemAccs();
        }
    }
}

bool UpgradedLRU::isInWriteBuffer(ll address){
    return (writeBuffer.find(address) != writeBuffer.end());
}

void UpgradedLRU::update(ll block, int status) {
    if(writeBuffer.size() > 19) flushWriteBuffer();
    ll baseIndex = block * numSectors;
    for (size_t sectorIdx = 0; sectorIdx < numSectors; sectorIdx++) {
        if (cache[block].sectors[sectorIdx].valid) {
            lastUsed[baseIndex + sectorIdx] = time; // LRU 갱신
        }
    }
    time++;
}

// write buffer가 100개 이상의 데이터를 가질 때 flush 하는 정책.
void UpgradedLRU::flushWriteBuffer() {
    // write buffer에 저장된 것들을 모두 메모리에 저장함. 이 때 저장된 수에 따라? 주소에 따라? 메모리 접근 횟수를 추가시켜야함.
    incMemAccs(writeBuffer.size());
    writeBuffer.clear();
    #if DEBUG
    std::cout << "Flushed address: " << address << " with data size: " << data.size() << "\n";
    #endif
}

void UpgradedLRU::evict(ll block) {
    for (size_t sector = 0; sector < numSectors; sector++) {
        cache[block].sectors[sector].valid = false; // 섹터 무효화
        cache[block].sectors[sector].dirty = false;
        cache[block].sectors[sector].address = -1;
        cache[block].sectors[sector].tag = -1;
    }
}

UpgradedLRU::~UpgradedLRU() {
    free(lastUsed); // LRU 관리 배열 해제
}
