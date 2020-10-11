#pragma once

#include "Base.h"
#include "common.h"
#include "Ludo/MinimalPerfectCuckoo/minimal_perfect_cuckoo.h"
#include "VacuumFilter/vacuum_filter.h"

template<class Key,class Value,int MAXBITS=5,bool filter_use=true>
class SingleLudo : public Base{
    public:
    string FileName;
    cuckoofilter::VacuumFilter<Key,MAXBITS,cuckoofilter::BetterTable>* MyFilter;//Filter Array collector;
    int* Seed;//Seed Array collector;
    bool empty_table=true;
    int MySize;
    explicit SingleLudo()
    {
        empty_table=true;
    }
    explicit SingleLudo(string &_FileName)
    {
        FileName=_FileName;
    }
    explicit SingleLudo(const unordered_map<Key,Value,Hasher32<Key> > &migrate,const string &_FileName)
    {
        FileName=_FileName;
        Clear(migrate);
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
            if (filter_use==true&&MyFilter.Contain(k)) return EMPTY;
            //TODO
    }
    void Clear(const unordered_map<Key,Value,Hasher32<Key> > &migrate)
    {
            auto iter=migrate.begin();
            ControlPlaneMinimalPerfectCuckoo<Key,Value> tmpludo(migrate.size());
            MySize=tmpludo.getBucketSize();
            if(filter_use) MyFilter=new cuckoo::VacuumFilter<Key,MAXBITS,cuckoofilter::BetterTable>(migrate.size());
            while (iter!=migrate.end())
            {
                    tmpludo.insert(iter->first,iter->second);
                    if(filter_use) MyFilter.Add(iter->first);
                    ++iter;
            } 
            empty_table=false;
            tmpludo.to_File(FileName);
    }
    unordered_map<Key,Value,Hasher32<Key> > toMap()
    {
             unordered_map<Key,Value,Hasher32<Key> > mmap;
             fd=open(FileName.c_str(),O_RDWR);
             if (fd==-1) Counter::count("Ludo toMap file open failed");
             int tmpsize;
             if (read(fd,&tmpsize,sizeof(tmpsize))!=tmpsize) Counter::count("SingleLudo storefile error");
             if (tmpsize!=MySize) Counter::count("SingleLudo file size is not satisified");
             Key nowKey; Value nowValue;
             for (int bucket=0;i<MySize;++i)
                for (int slot=0;slot<4;++slot)
                {
                    if (read(fd,&nowKey,sizeof(nowKey))!=sizeof(nowKey)) Counter::count("SingleLudo key error"); 
                    if (read(fd,&nowValue,sizeof(nowValue))!=sizeof(nowValue)) Counter::count("SingleLudo value error");
                    if (nowKey!=0&&mmap.find(nowKey)==mmap.end()) mmap.insert(make_pair(nowKey,nowValue));
                }
            return mmap;
    }
    void send_to_map(unordered_map<Key,Value,Hasher32<Key> > &mmap)
    {
             fd=open(FileName.c_cstr(),O_RDWR);
             if (fd==-1) Counter::count("Ludo toMap file open failed");
             int tmpsize;
             if (read(fd,&tmpsize,sizeof(tmpsize))!=tmpsize) Counter::count("SingleLudo storefile error");
             if (tmpsize!=MySize) Counter::count("SingleLudo file size is not satisified");
             Key nowKey; Value nowValue;
             for (int bucket=0;i<MySize;++i)
                for (int slot=0;slot<4;++slot)
                {
                    if (read(fd,&nowKey,sizeof(nowKey))!=sizeof(nowKey)) Counter::count("SingleLudo key error"); 
                    if (read(fd,&nowValue,sizeof(nowValue))!=sizeof(nowValue)) Counter::count("SingleLudo value error");
                    if (nowKey!=0) mmap.insert(make_pair(nowKey,nowValue));
                }
            return mmap;
    }
    int Merge(unordered_map<Key,Value,Hasher32<key> >&migrate)
    {
            if (!empty_table) send_to_map(migrate);
            Clear(Merge);
            return OK;
    }
};