#pragma once

#define Hash

#include "Base.h"
#include "hash.h"
#include "common.h"
#include "../common.h"

#define MAX_HASH_LINK_SIZE 1024 
#define LogBufferSize 4096


template<class Key,class Value>
class SimpleLink : public Base<Key,Value>{
    public:
    Hasher32<Key> h;
    struct Mapping{
        Key k;
        uint32_t LogPos,NextLink;
    };
    uint32_t Head[MAX_HASH_LINK_SIZE];
    int32_t Empty_Link;
    string FileName;
    vector<Mapping> Element;
    const uint32_t avgdown = BLOCK_SIZE_PER / MAX_HASH_LINK_SIZE;
    char LogBuff[LogBufferSize];
    char tmpBuff[LogBufferSize];
    bool *clockbit;
    uint32_t LogBufferOffset=0,logFileOffset=0;
    uint32_t append_log(int appendlen)
    {
        memcpy( LogBuff+LogBufferOffset, tmpBuff, appendlen);
        LogBufferOffset += appendlen;
        if (LogBufferOffset == LogBufferSize) 
        {
            int fd = open(FileName.c_str(),O_RDWR);
            pwrite(fd,LogBuff,LogBufferSize,logFileOffset);
            LogBufferOffset = 0;
            logFileOffset += LogBufferSize;
            close(fd);
        }
        return logFileOffset + LogBufferOffset - appendlen;
    }
    uint32_t log_insert(const Key &k,const Value &v)
    {
        memcpy(tmpBuff,(char*)(&k),sizeof(Key));
        memcpy(tmpBuff+sizeof(Key),(char*)(&v),sizeof(Value));
        return append_log(sizeof(Key)+sizeof(Value));
    }

    Value ReadSlot(uint32_t offset,int length,int fd = -1)
    {
        if (offset >= logFileOffset)
            return *(Value&)(LogBuff + (offset - logFileOffset));
        else 
        {
            Value tmp;
            if (fd == -1)
            {
                fd = open(FileName.c_str(),O_RDONLY);
                pread(fd, tmp, length, offset);
                close(fd);
            }
            else
                pread(fd, tmp, length, offset);
            return tmp;
        }
    }
    explicit SimpleLink(uint32_t MaxSize,string _FileName)
    {
        FileName = _FileName;
        Element.resize(0);
        Element.resize(MaxSize);
        clockbit = new bool[MaxSize];
        for (uint32_t id = 0; id < MaxSize; ++id)
            Element[id].NextLink = ( id + 1 == MaxSize ) ? id + 1 : -1,
            clockbit[id] = false;
        for (uint32_t id = 0; id < MAX_HASH_LINK_SIZE; ++id) 
            Head[id] = -1;
        Empty_Link = 0;
        h.setSeed(rand());
        int fd = open(FileName.c_str(),O_RDWR | O_TRUNC | O_CREAT,0777);
        close(fd);
    }

    int insert(const Key &k,const Value &v)
    {
        uint32_t log_offset = log_insert(k,v);
        uint32_t Cur_ID = h(k) & (MAX_HASH_LINK_SIZE - 1);
        Mapping &tmp_bucket = Head[Cur_ID];
        for (uint32_t id = Head[Cur_ID]; id != -1; id = tmp_bucket.NextLink)
        {
            tmp_bucket = Element[id];
            if (tmp_bucket.k == k) 
            {
                tmp_bucket.LogPos = log_offset;
                clockbit[id] = true;
                return OK;
            }
        }
        if (Empty_Link == -1) return FULL;
        uint32_t bucketid = Empty_Link;
        tmp_bucket = Element[Empty_Link];
        Empty_Link = tmp_bucket.NextLink;
        tmp_bucket.k = k;
        tmp_bucket.LogPos = log_offset;
        tmp_bucket.NextLink = Head[Cur_ID];
        Head[Cur_ID] = bucketid;
        clock[bucketid] = false;
        
        return OK;
    }
    int modify(const Key &k,const Value &v)
    {
        return insert(k,v);
    }
    virtual int erase(const Key &k)
    {
        return insert(k,-1);
    }
    virtual int lookup(const Key &k,Value &out)
    {
        uint32_t Cur_ID = h(k) & (MAX_HASH_LINK_SIZE - 1);
        Mapping &tmp_bucket = Head[Cur_ID];
        for (uint32_t id = Head[Cur_ID]; id != -1; id = tmp_bucket.NextLink)
        {
            tmp_bucket = Element[id];
            if (tmp_bucket.k == k) 
            {
                clockbit[id] = true;
                out = ReadSlot(tmp_bucket.LogPos+sizeof(Key),sizeof(Value));
                return OK;
            }
        }
        return true;
    }
    int Merge(unordered_map<Key,Value,Hasher32<Key> > other)
    {
        return ERROR;
    }
    uint32_t LinkClear(uint32_t Cur_ID, uint32_t limit, unordered_map<Key,Value,Hasher32<Key> > &mmap,int fd)
    {
        Mapping &tmp_bucket = Head[Cur_ID];
        uint32_t lastid = -1,NextLink = -1;
        for (uint32_t id = Head[Cur_ID]; id != -1 && limit; id = NextLink)
        {
            tmp_bucket = Element[id];
            NextLink = tmp_bucket.NextLink;
            if (clockbit[id] == false) 
            {
                --limit;
                mmap.insert(make_pair(tmp_bucket.k,ReadSlot(tmp_bucket.LogPos+sizeof(Key),sizeof(Value),fd)));
                if (lastid != -1)
                Element[lastid].NextLink = tmp_bucket.NextLink;
                else Head[Cur_ID] = tmp_bucket.NextLink;
                tmp_bucket.NextLink = Empty_Link;
                Empty_Link = id;
            }
            else clockbit[id] = false,lastid = id;
        }
        for (uint32_t id = Head[Cur_ID]; id != -1 && limit; id = Head[Cur_ID])
        {
            tmp_bucket = Element[id];
            --limit;
            mmap.insert(make_pair(tmp_bucket.k,ReadSlot(tmp_bucket.LogPos+sizeof(Key),sizeof(Value))));
            Head[Cur_ID] = tmp_bucket.NextLink;
            tmp_bucket.NextLink = Empty_Link;
            Empty_Link = id;
        }
        return limit;
    }
    unordered_map<Key,Value,Hasher32<Key> > toMap()
    {
        uint32_t res = 0;
        unordered_map<Key,Value,Hasher32<Key> > mmap;
        int fd = open(FileName.c_str(),O_RDONLY);
        for (int id = 0; id < MAX_HASH_LINK_SIZE; ++id) res += LinkClear(id,avgdown,mmap,fd);
        for (int id = 0; res > 0; ++id) res -= LinkClear(id & (MAX_HASH_LINK_SIZE - 1),1,mmap,fd);
        close(fd);
        return mmap;
    }
};