#pragma once

#include "cstdlib"
#include "unordered_map"

template<class Key, class Value>
class Base{
    Base(){}
    ~Base(){}
    virtual int insert(Key& Cur_Key,Value& Cur_Value)=0;
    virtual int modify(Key& Cur_Key,Value& Cur_Value)=0;
    virtual int erase(Key& Cur_Key)=0;
    virtual int Merge(unordered_map<Key,pair<Value,bool>,Hasher32<Key> >& other)=0;
    virtual unordered_map<Key,pair<Value,bool>,Hasher32<Key> > toMap()=0;
};