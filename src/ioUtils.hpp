#include "cache.h"
#include <stdio.h>
#if INTERACTIVE
#include <ncurses.h>
#endif

#define ll long long

#define instructionOffset 30

#define levelOffset 0
#define hitRateOffset 55
#define accessesOffset 0
#define hitsOffset 30
#define missesOffset 55

ll getNextAddress() {
    char accessType; // 'l' 또는 's' 저장
    char address[20]; // 메모리 주소 저장

    // 입력 읽기
    int result = scanf(" %c %s", &accessType, address);
    if (result == EOF) {
        // 파일 끝에 도달한 경우
        return -1;
    }
    if (result != 2) {
        // 입력 형식이 잘못된 경우
        throw std::invalid_argument("Invalid input format in trace file.");
    }

    // 명령어 타입 처리
    if (accessType == 'l') {
        incReads(); // 읽기 명령 증가
    } else if (accessType == 's') {
        incWrites(); // 쓰기 명령 증가
    } else {
        throw std::invalid_argument("Invalid access type: " + std::string(1, accessType));
    }

    // 주소 변환 및 반환
    return hexToDec(address);
}

#ifdef INTERACTIVE
void printTraceInfoOutline(){
    move(0, 0);
    printw("Total Number Of Instructions:"); refresh();
    move(1, 0);
    printw("Number of Reads             :"); refresh();
    move(2, 0);
    printw("Number of Writes            :"); refresh();
}

void printTraceInfo(){
    move(0, instructionOffset);
    printw("%lld", getReads() + getWrites()); refresh();
    move(1, instructionOffset);
    printw("%lld", getReads()); refresh();
    move(2, instructionOffset);
    printw("%lld", getWrites()); refresh();
}
#endif
void printTraceInfo2(){
    printf("\n");
    printf("Total Number Of Instructions: %lld\n", getReads() + getWrites());
    printf("Number of Reads             : %lld\n", getReads());
    printf("Number of Writes            : %lld\n", getWrites());   
    printf("\n");
}

#ifdef INTERACTIVE
void printCacheStatusOutline(Cache* cache){
    int verticalOffset = 5 + 3 * (cache->getLevel()-1);
    move(verticalOffset, levelOffset);
    attron(A_BOLD);
    printw("L%d: %s", cache->getLevel(), cache->getPolicy().c_str()); refresh();
    attroff(A_BOLD);
    move(verticalOffset, hitRateOffset);
    printw("Hit Rate: "); refresh();
    move(verticalOffset + 1, accessesOffset);
    printw("Accesses: "); refresh();
    move(verticalOffset + 1, hitsOffset);
    printw("Hits: "); refresh();
    move(verticalOffset + 1, missesOffset);
    printw("Misses: "); refresh();
}

void printCacheStatus(Cache* cache){
    int verticalOffset = 5 + 3 * (cache->getLevel()-1);
    move(verticalOffset, hitRateOffset+10);
    printw("%f", cache->getHitRate()); refresh();
    move(verticalOffset+1, accessesOffset+10);
    printw("%lld", cache->getHits()+cache->getMisses()); refresh();
    move(verticalOffset+1, hitsOffset+6);
    printw("%lld", cache->getHits()); refresh();
    move(verticalOffset+1, missesOffset+8);
    printw("%lld", cache->getMisses()); refresh();
}
#endif
void printCacheStatus2(Cache* cache){
    printf("L%d: %s\t\t\t\t\t\tHit Rate: %f\n",  
        cache->getLevel(), cache->getPolicy().c_str(), cache->getHitRate());
    printf("Accesses: %lld\t\tHits: %lld\t\tMisses: %lld\n",
        cache->getHits()+cache->getMisses(), cache->getHits(), cache->getMisses());
    printf("\n");
}