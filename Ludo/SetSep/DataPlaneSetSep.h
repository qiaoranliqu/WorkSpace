#pragma once

#include "../common.h"
#include "setsep.h"
#include "bucket_map_to_group.h"

template<class K,class V,uint8_t VL>
class DataPlaneSetsep{
    static_assert(VL <= 64, "The value is too long, please consider other implementation to save memory");
    public:
    typedef unsigned __int128 uint128_t;
  
    static constexpr uint KeysPerBlock = 1024;
    static constexpr uint BucketsPerBlock = 256;
    static constexpr uint GroupsPerBlock = 64;

    struct Group {
        uint16_t seeds[VL];
        uint8_t bitmaps[VL];
    };
};