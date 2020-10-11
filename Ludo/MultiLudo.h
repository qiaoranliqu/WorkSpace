#pragma once

#include "Base.h"
#include "common.h"
#include "SingleLudo.h"

#define MAX_TABLE_SIZE 16

template<class Key,class Value,int MAXBITS=5,bool filter_use=true>
class MultiLudo : public Base{
    public:
    string PreFixName;
    SingleLudo<Key,Value>* Ludo[MAX_TABLE_SIZE];
    int TABLE_NUM=0;
    bool empty_table=true;
    explicit MultiLudo()
    {
        empty_table=true;
    }
    explicit MultiLudo(string &_FileName)
    {
        PreFixName=_FileName;
    }
    int insert(const Key &k,const Value &v)
    {
            return ERROR;
    }
    int delete(const Key &k)
    {
            return ERROR;
    }
    int modify(const Key &k,const Value &v)
    {
            return ERROR;
    }
    int lookup(const Key &k,Value &out)
    {
        for (int i=TABLE_NUM-1;i>=0;--i)
            if (Ludo[i]->lookup(k,out)==true) return OK;
        return EMPTY;
    }
    unordered_map<Key,Value,Hasher32<Key> > toMap()
    {
            unordered_map<Key,Value,Hasher32<Key> > mmap;
            for (int i=0;i<TABLE_NUM;++i)
            {
                unordered_map<Key,Value,Hasher32<Key> >tmpmap;
                auto it=tmpmap.begin();

            }
            TABLE_NUM=0;
            return mmap;
    }
    int Merge(unordered_map<Key,Value,Hasher32<key> >&migrate)
    {
            Ludo[TABLE_NUM]=new SingleLudo(migrate,PreFixName+"_"+to_string(TABLE_NUM));
            TABLE_NUM++;
            if (TABLE_NUM==MAX_TABLE_SIZE) return FULL;
            return OK;
    }
};