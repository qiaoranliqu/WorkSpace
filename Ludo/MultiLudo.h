#pragma once

#include "Base.h"
#include "common.h"
#include "SingleLudo.h"

#define MAX_TABLE_SIZE 16

template<class Key,class Value,int MAXBITS=5,bool filter_use=true>
class MultiLudo : public Base<Key,Value>{
    public:
    string PreFixName;
    SingleLudo<Key,Value,MAXBITS,filter_use> *Ludo[MAX_TABLE_SIZE];
    int TABLE_NUM=0;
    bool empty_table=true;
    explicit MultiLudo()
    {
        empty_table=true;
    }
    explicit MultiLudo(const string _FileName)
    {
        PreFixName=_FileName;
    }
    int insert(const Key &k,const Value &v)
    {
            return ERROR;
    }
    int erase(const Key &k)
    {
            return ERROR;
    }
    int modify(const Key &k,const Value &v)
    {
            return ERROR;
    }
    int lookup(const Key &k,Value &out)
    {
//        fprintf(stderr,"Start lookup\n");
//        fprintf(stderr,"%d %d\n",TABLE_NUM,Ludo[0]);
        for (int i=TABLE_NUM-1;i>=0;--i)
            if (Ludo[i]->lookup(k,out)==OK)   
            {
                return OK;
            }
        fprintf(stderr,"tskknmsl\n");
        return EMPTY;
    }
    unordered_map<Key,Value,Hasher32<Key> > toMap()
    {
            unordered_map<Key,Value,Hasher32<Key> > mmap;
            for (int i=TABLE_NUM-1;i>=0;--i)
            {
                unordered_map<Key,Value,Hasher32<Key> >tmpmap=Ludo[i]->toMap();
                auto it=tmpmap.begin();
                while (it!=tmpmap.end())
                {
                        if (mmap.find(it->first)==mmap.end()) mmap[it->first]=it->second;
                        ++it;
                }
            }
            TABLE_NUM=0;
            return mmap;
    }
    int Merge(unordered_map<Key,Value,Hasher32<Key> > migrate)
    {
            fprintf(stderr,"Start Merge!\n");
            Ludo[TABLE_NUM]=new SingleLudo(migrate,PreFixName+"_"+to_string(TABLE_NUM));
            TABLE_NUM++;
            if (TABLE_NUM==MAX_TABLE_SIZE) return FULL;
            fprintf(stderr,"Merge Finished!\n");
            return OK;
    }
};
