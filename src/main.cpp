#include <iostream>
#include <stdlib.h>
#include <chrono>
#include <fstream>
#include <vector>
#include <unistd.h>
#include "ioUtils.hpp" //contains I/O functions
#include "cache.h" //contains all auxillary functions
#include "../policies/plru.h"
#include "../policies/lru.h"
#include "../policies/srrip.h"
#include "../policies/nru.h"
#include "../policies/lfu.h"
#include "../policies/fifo.h"
#include "../policies/UpgradedLRU.h"
// #include "../policies/policy.h"

using namespace std;
using namespace std::chrono;

#define ll long long

Cache* createCacheInstance(string& policy, ll cs, ll bs, ll sa, int level){
    
    // check validity here and exit if invalid
    if(policy == "plru"){
        Cache* cache = new PLRU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "lru"){
        Cache* cache = new LRU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "srrip"){
        Cache* cache = new SRRIP(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "nru"){
        Cache* cache = new NRU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "lfu"){
        Cache* cache = new LFU(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "fifo"){
        Cache* cache = new FIFO(cs, bs, sa, level);
        return cache;
    }
    else if(policy == "upgradedLRU"){
        Cache* cache = new UpgradedLRU(cs, bs, sa, level, 16);
        return cache;
    }
    // else if(policy == "<policy>"){
    //     Cache* cache = new <POLICY>(cs, bs, sa, level);
    //     return cache;
    // }
}

int main(int argc, char *argv[]){

    // 입력 파일로부터 캐시 설정을 읽기 위한 ifstream 객체 생성
    ifstream params;
    params.open(argv[1]); // 첫 번째 인자로 받은 파일 경로를 열기
    string word;

    // 캐시 레벨(levels)의 개수를 읽어옴
    params >> word;
    int levels = stoi(word.c_str()); // 문자열을 정수로 변환

    // 캐시 레벨별로 Cache 객체 포인터를 저장할 벡터 생성
    vector<Cache*> cache(levels);

    // 각 캐시 레벨의 설정을 반복적으로 읽음
    int iterator = 0;
    string policy; // 교체 정책 이름 (예: "LRU", "LFU")
    while (iterator < levels) {
        params >> policy;

        // 캐시 크기(cs), 블록 크기(bs), 연관도(sa)를 읽어옴
        ll cs, bs, sa;
        params >> word; cs = stoll(word.c_str()); // 캐시 크기
        params >> word; bs = stoll(word.c_str()); // 블록 크기
        params >> word; sa = stoll(word.c_str()); // 세트 연관도

        // createCacheInstance를 호출해 캐시 객체를 생성하고, 해당 레벨에 추가
        cache[iterator++] = createCacheInstance(policy, cs, bs, sa, iterator);
    }

    // INTERACTIVE 모드에서는 curses 라이브러리를 이용한 출력 설정
    #if INTERACTIVE
    initscr(); // curses 초기화
    raw(); // 키보드 입력을 원시 모드로 설정
    noecho(); // 키보드 입력이 에코되지 않도록 설정
    printTraceInfoOutline(); // 디버깅 출력 틀 생성
    for (int levelItr = 0; levelItr < levels; levelItr++) {
        printCacheStatusOutline(cache[levelItr]); // 각 캐시의 상태를 출력
    }
    #endif

    // 실행 시간 측정을 위한 시작 시간 기록
    auto start = high_resolution_clock::now();

    // 메모리 접근을 시뮬레이션
    // Write-Through Policy
    while (true) {
        // 다음 메모리 주소를 읽어옴 (주소가 -1이면 EOF)
        Access access = getNextAddress();
        char accesType = access.accessType;
        ll address = access.address;
        // eof
        if (accesType == -1) break;

        // 모든 캐시 레벨을 순회하며 데이터 찾기 시도
        for (int levelItr = 0; levelItr < levels; levelItr++) {
            // 해당 주소가 현재 캐시에 있는지 확인
            ll block = cache[levelItr]->getBlockPosition(address);

            if (block == -1) { // 캐시 미스 발생
                cache[levelItr]->incMisses(); // 미스 카운트 증가
                ll blockToReplace = cache[levelItr]->getBlockToReplace(address); // 교체할 블록 선택
                cache[levelItr]->insert(address, blockToReplace); // 새로운 블록 삽입
                
                // UpgradedLRU인지 확인 후 Access 타입으로 insert 호출
                if (policy == "upgradedLRU") {
                    static_cast<UpgradedLRU*>(cache[levelItr])->insert(access, blockToReplace);
                } else {
                    cache[levelItr]->insert(address, blockToReplace);
                }

                cache[levelItr]->update(blockToReplace, 0); // 교체 정책 업데이트 (0 = 미스)

                #if INTERACTIVE
                printTraceInfo(); // 현재 접근 정보 출력
                printCacheStatus(cache[levelItr]); // 현재 캐시 상태 출력
                #endif
            } else { // 캐시 히트 발생
                cache[levelItr]->incHits(); // 히트 카운트 증가
                cache[levelItr]->update(block, 1); // 교체 정책 업데이트 (1 = 히트)

                #if INTERACTIVE
                printTraceInfo(); // 현재 접근 정보 출력
                printCacheStatus(cache[levelItr]); // 현재 캐시 상태 출력
                #endif
                break; // 캐시 히트가 발생하면 더 이상 다른 레벨을 검사하지 않음
            }
        }
    }

    // 실행 시간 측정을 위한 종료 시간 기록
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<std::chrono::milliseconds>(stop - start);

    #if INTERACTIVE
    usleep(2000000); // 2초 대기
    endwin(); // curses 세션 종료
    #endif

    // 최종 결과 출력
    printTraceInfo2(); // 최종 접근 통계 정보 출력
    for (int levelItr = 0; levelItr < levels; levelItr++) {
        printCacheStatus2(cache[levelItr], duration); // 각 캐시 레벨의 상태와 실행 시간 출력
        delete cache[levelItr]; // 동적으로 생성한 캐시 객체 삭제
    }

    return 0; // 프로그램 종료
}

