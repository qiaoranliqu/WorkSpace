#pragma once

#include "cstdlib"
#include "unordered_map"

template<class Key, class Value>
class Base{
    Base(){}
    ~Base(){}
    virtual int insert(const Key &k,const Value &v)=0;
    virtual int modify(const Key &k,const Value &v)=0;
    virtual int erase(const Key &k)=0;
    virtual int lookup(const Key &k,Value &out)=0;
    virtual int Merge(const unordered_map<Key,Value,Hasher32<Key> >& other)=0;
    virtual unordered_map<Key,Value,Hasher32<Key> > toMap()=0;
};