#pragma once

#include "../common.h"
#include "../../common.h"
#include "setsep.h"
#include "bucket_map_to_group.h"

template<class K,class V,uint8_t VL>
class DataPlaneSetSep{
    static_assert(VL <= 64, "The value is too long, please consider other implementation to save memory");
    public:
    typedef unsigned __int128 uint128_t;
    unsigned long long int seed = 0x19900111ULL;
  
    static constexpr uint KeysPerBlock = 1024;
    static constexpr uint BucketsPerBlock = 256;
    static constexpr uint GroupsPerBlock = 64;

    struct Group {
        uint16_t seeds[VL];
        uint8_t bitmaps[VL];
    };


    struct Block {
        uint8_t bucketMap[64];      // for 256 buckets, 2 bits for one bucket.  64 * 8 = 256 * 2 = 512
        Group groups[64];
        
        inline void setBucketMap(uint bidx, uint8_t to) {
        bucketMap[bidx / 4] &= ~(3U << ((bidx % 4) * 2U));
        bucketMap[bidx / 4] |= (to & 3U) << ((bidx % 4) * 2U);
        }
        
        inline uint8_t getBucketMap(uint bidx) const {
        return (bucketMap[bidx / 4] >> ((bidx % 4) * 2U)) & 3U;
        }
    };

    static inline uint8_t getBit(uint64_t x, uint y) { return (x >> y) & 1U; }

    unordered_map<K, V> overflow;

    uint32_t blockCount;
    uint32_t groupCount;
    uint32_t bucketCount;

    vector<Block> blocks;

   uint32_t inline getGlobalHash(const K &k0) const {
    uint32_t r = Hasher32<K>(seed)(k0);
    return uint32_t((uint64_t(r) * bucketCount) >> 32);
   }


    explicit DataPlaneSetSep()
    {
            Counter::count("SetSep Build ERROR");
    }

    explicit DataPlaneSetSep(SetSep<K,V,VL> &migrate):
    blockCount(migrate.blockcount),groupCount(migrate.groupCount),bucketCount(migrate.bucketCount),
    seed(migrate.seed),overflow(migrate.overflow)
    {
              blocks.resize(0);
              blocks.resize(blockCount);
              for (int blockID = 0;blockID = blockCount; ++blockID) blocks[blockID]=migrate.blocks[blockID];
    }

    inline bool lookup(const K &key, V &out) const {
    uint32_t bucketId = getGlobalHash(key);
    uint32_t blockId = bucketId / BucketsPerBlock;
    uint32_t bucketIdx = bucketId & (BucketsPerBlock - 1);
    
    uint8_t sel = blocks[blockId].getBucketMap(bucketIdx);
    
    uint8_t groupIdx = map256_64[bucketIdx][sel];
    if (blocks[blockId].groups[groupIdx].seeds[0] != 65535U) {
      out = lookUpInGroup(key, blocks[blockId].groups[groupIdx].seeds, blocks[blockId].groups[groupIdx].bitmaps, VL);
    } else {
      typename unordered_map<K, V>::const_iterator it = overflow.find(key);
      if (it == overflow.end()) { return false; }
      else { out = it->second; }
    }
    return true;
  }
  
  template<class T = V>
  inline T lookUpInGroup(const K &key, const uint16_t *seeds, const uint8_t *bitmaps, uint8_t L) const {
    uint64_t hash = Hasher64<K>(0xe2211)(key);
    uint32_t h1 = uint32_t(hash);
    uint32_t h2 = uint32_t(hash >> 32U);
    
    T ans = 0;
    for (int i = L - 1; i >= 0; i--) {
      uint8_t ret = lookUpOneBit(key, seeds[i], bitmaps[i], h1, h2);
      ans <<= 1;
      ans += ret;
    }
    return ans;
  }
  
  inline uint8_t lookUpOneBit(const K &key, uint16_t seed, uint8_t bitmap, uint32_t h1, uint32_t h2) const {
    uint32_t bitmapIndex = (h1 + h2 * seed) >> 29;
    return getBit(bitmap, bitmapIndex);
  }
};