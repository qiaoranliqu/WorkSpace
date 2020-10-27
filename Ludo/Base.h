#pragma once

#include "cstdlib"
#include <unordered_map>
#include "hash.h"

using namespace std;

template<class Key, class Value>
class Base{
    public:
    Base(){}
    ~Base(){}
    virtual int insert(const Key &k,const Value &v)=0;
    virtual int modify(const Key &k,const Value &v)=0;
    virtual int erase(const Key &k)=0;
    virtual int lookup(const Key &k,Value &out)=0;
    virtual int Merge(unordered_map<Key,Value,Hasher32<Key> > other)=0;
    virtual unordered_map<Key,Value,Hasher32<Key> > toMap()=0;
};