#pragma once

#include "stdlib.h"
#include "unordered_map"
#include "vector"

#include "Base.h"

#define Default_Size 76423

template<class Key,class Value,uint32_t Size=Default_Size,uint32_t Bucket_Num=2,
        uint32_t Slot_Num=4>
class FullCuckoo : public Base{

}