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
            if (cache[block].sectors[sector].valid && tag == cache[block].sectors[sector].data[0]) {
                return block; // 캐시 히트 시 해당 블록 반환
            }
        }
    }
    return -1; // 캐시 미스
}

ll UpgradedLRU::getBlockToReplace(ll address) {
    ll index = getIndex(address);
    ll min_block = index * setAssociativity;
    for (ll block = index * setAssociativity; block < (index + 1) * setAssociativity; block++) {
        if (!cache[block].sectors[0].valid) {
            return block; // 비어 있는 블록 반환
        }
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
    cache[blockToReplace].sectors[sector].data[0] = tag; // 태그 저장
    update(blockToReplace, 0); // LRU 업데이트
}

void UpgradedLRU::update(ll block, int status) {
    ll baseIndex = block * numSectors;
    for (size_t sector = 0; sector < numSectors; sector++) {
        if (cache[block].sectors[sector].valid) {
            lastUsed[baseIndex + sector] = time; // LRU 갱신
        }
    }
    time++;
}

void UpgradedLRU::flushWriteBuffer() {
    while (!writeBuffer.empty()) {
        auto [address, data] = writeBuffer.front();
        writeBuffer.pop();
        // 메모리에 데이터를 기록하는 시뮬레이션
        std::cout << "Flushed address: " << address << " with data size: " << data.size() << "\n";
    }
}

void UpgradedLRU::evict(ll block) {
    for (size_t sector = 0; sector < numSectors; sector++) {
        if (cache[block].sectors[sector].dirty) {
            // 더티 섹터는 메모리에 쓰기 (시뮬레이션)
            std::cout << "Evicting dirty sector from block " << block << "\n";
        }
        cache[block].sectors[sector].valid = false; // 섹터 무효화
        cache[block].sectors[sector].dirty = false;
    }
}

UpgradedLRU::~UpgradedLRU() {
    free(lastUsed); // LRU 관리 배열 해제
}
