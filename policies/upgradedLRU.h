#ifndef UPGRADED_LRU_H
#define UPGRADED_LRU_H

#include "../src/cache.h"
#include <unordered_map>
#include <vector>
class UpgradedLRU : public Cache {
public:
    UpgradedLRU(ll cacheSize, ll blockSize, ll setAssociativity, int level, size_t sectorSize);

    ll getBlockToReplace(ll address) override; // 교체할 블록 결정
    void update(ll block, int status) override; // LRU 상태 업데이트
    ll getBlockPosition(ll address) override; // 블록 위치 확인 (캐시 히트 검사)
    void insert(ll address, ll blockToReplace) override; // 블록 삽입
    void insert(Access access, ll blockToReplace); // 블록 삽입
    void flushWriteBuffer(); // Write Buffer 플러시
    bool isInWriteBuffer(ll address);
    ~UpgradedLRU();

private:
    struct Sector {
        bool valid;  // 섹터 유효성
        bool dirty;  // 섹터 더티 상태
        std::vector<char> data; // 섹터 데이터

        Sector(size_t sector_size) : valid(false), dirty(false), data(sector_size, 0) {}
    };

    struct CacheLine {
        std::vector<Sector> sectors; // 섹터 배열
        CacheLine(size_t num_sectors, size_t sector_size)
            : sectors(num_sectors, Sector(sector_size)) {}
    };

    size_t sectorSize;     // 섹터 크기
    size_t numSectors;     // 블록당 섹터 개수
    std::vector<CacheLine> cache; // 섹터화된 캐시 구조
    std::unordered_map<ll, std::vector<char>> writeBuffer; // Write Buffer
    ll* lastUsed;          // 섹터별 LRU를 위한 최근 사용 시간
    ll time = 0;           // 글로벌 타이머

    void evict(ll block); // 캐시 교체
};

#endif // UPGRADED_LRU_H
