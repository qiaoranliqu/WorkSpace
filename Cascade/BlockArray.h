#pragma once

#include "Base.h"
#include "common.h"
#include "../common.h"

#define BLOCK_SIZE 64

template<class Key,class Value>
class BlockArray : public Base<Key,Value>{
    public:
    string FileName[BLOCK_SIZE];
    uint32_t ql,qr;
    explicit BlockArray(string _FileName)
    {
        for (int id = 0; id < BLOCK_SIZE; ++id)
            FileName[id] = _FileName + to_string(id);
        ql = qr = 0;
    }
    int insert(const Key &k,const Value &v)
    {
        return ERROR;
    }
    int modify(const Key &k,const Value &v)
    {
        return ERROR;
    }
    int erase(const Key &k)
    {
        return ERROR;
    }
    int lookup(const Key &k,Value &out)
    {
        for (int blockid = (qr - 1 + BLOCK_SIZE) & (BLOCK_SIZE - 1); ;blockid = (blockid - 1 + BLOCK_SIZE) & (BLOCK_SIZE - 1))
        {
                char tmpBuff[BLOCK_SIZE_PER * (sizeof(Key) + sizeof(Value))];
                int fd = open(FileName[blockid].c_str(), O_RDONLY);
                pread(fd, tmpBuff, BLOCK_SIZE_PER*(sizeof(Key)+sizeof(Value)),0);
                uint32_t len = 0;
                for (int id = 0; id < BLOCK_SIZE_PER; ++id)
                {
                        Key Cur_Key = *((Key*)(tmpBuff + len)); len += sizeof(Key);
                        if (Cur_Key == k) 
                        {
                            out = *((Value*)(tmpBuff + len)); len += sizeof(Value);
                            return OK;
                        }
                }
                if (blockid == ql) return EMPTY;
        }
        return ERROR;
    }
    int Merge(unordered_map<Key,Value,Hasher32<Key> > other)
    {
        int fd = open(FileName[qr].c_str(), O_TRUNC | O_CREAT | O_RDWR, 0777);
        char tmpBuff[BLOCK_SIZE_PER * (sizeof(Key) + sizeof(Value))];
        uint32_t tmplen = 0;
        for (auto it = other.begin(); it != other.end(); ++it)
        {
            Key k = it->first; Value v = it->second;
            memcpy(tmpBuff + tmplen, (char*)(&k), sizeof(Key)); tmplen += sizeof(Key);
            memcpy(tmpBuff + tmplen, (char*)(&v), sizeof(Value)); tmplen += sizeof(Value);
        }
        pwrite(fd, tmpBuff, tmplen, 0);
        close(fd);
        qr = (qr + 1) & (BLOCK_SIZE - 1);
        if (ql == qr) return FULL;
        return OK;
    }
    unordered_map<Key,Value,Hasher32<Key> > toMap()
    {
        unordered_map<Key,Value,Hasher32<Key> > mmap;
        char tmpBuff[BLOCK_SIZE_PER*(sizeof(Key)+sizeof(Value))];
        int fd = open(FileName[ql].c_str(), O_RDONLY);
        pread(fd, tmpBuff, BLOCK_SIZE_PER * (sizeof(Key) + sizeof(Value)), 0);
        uint32_t len = 0;
        for (int id = 0; id < BLOCK_SIZE_PER; ++id)
        {
            Key Cur_Key = *((Key*)(tmpBuff + len)); len += sizeof(Key);
            Value Cur_Val = *((Value*)(tmpBuff + len)); len += sizeof(Value);
            mmap.insert(make_pair(Cur_Key,Cur_Val));
        }        
        ql = (ql + 1) & (BLOCK_SIZE - 1);   
        return mmap;     
    }
}