#pragma once

#include "cstdlib"
#include "unordered_map"

template<class Key, class Value>
class Base{
    Base(){}
    ~Base(){}
    virtual uint32_t insert(Key& Cur_Key,Value& Cur_Value)=0;
    virtual uint32_t modify(Key& Cur_Key,Value& Cur_Value)=0;
    virtual uint32_t erase(Key& Cur_Key)=0;
    virtual uint32_t Merge(unordered_map<Key,pair<Value,bool>,Hasher32<Key> >& other)=0;
    virtual unordered_map<Key,pair<Value,bool>,Hasher32<Key> > toMap()=0;
};