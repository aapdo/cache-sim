#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#define ll long long

struct Access {
    char accessType; // 'l' 또는 's'를 저장 (읽기/쓰기 명령어 타입)
    ll address;      // 메모리 주소
};

// classes defined
class Cache;

// helper functions
bool isValidConfig(ll cs, ll bs, ll sa);
ll hexToDec(char hexVal[]);
int log2(ll x);

void incReads();
void incWrites();
ll getReads();
ll getWrites();


// cache class
class Cache{

    private:
        ll hits, misses;
        ll memAccs;
        ll* cacheBlocks;
        int level;
        std::string policy;

    public:
        void incHits();
        void incMisses();
        void incMemAccs();
        int getLevel();
        std::string getPolicy();
        ll getTag(ll address);
        ll getIndex(ll address);
        ll getMemAccs();
        virtual ll getBlockPosition(ll address);
        virtual void insert(ll address, ll blockToReplace);

        ll getHits();
        ll getMisses();
        float getHitRate();

        virtual ll getBlockToReplace(ll address) = 0;
        virtual void update(ll blockToReplace, int status) = 0;

        virtual ~Cache();

    protected:
        Cache(ll cacheSize, ll blockSize, ll setAssociativity, int level, std::string policy);
        ll cacheSize;
        ll blockSize;
        ll setAssociativity;
        ll numberOfSets;
        int offsetSize;
        int indexSize;
};