#pragma once

#include <cstdlib>
#include <unistd.h>
#include "../common.h"
#include "Cuckoo/common.h"
#include "Cuckoo/cuckoo.h"

#define MAX_TABLE_NUM 5
#define MAX_BUFF_SIZE 4096
#define DEFAULT_SIZE 16384

template<class Key, class Value>
class MyDesign{
	public:
	uint32_t TABLE_NUM=3;
	uint32_t STATUS;
	uint32_t fd;
	Base* table[MAX_TABLE_NUM];
	const string LogFile="MyDesign.log";
	char* LogBuf[MAX_BUFF_SIZE];
	char* TempBuf[MAX_BUFF_SIZE];
	explicit MyDesign(/*uint32_t _TABLE_NUM,vector<Config> configure*/)
	{
		/*
		    **TODO:Make configuration flexible
			TABLE_NUM=_TABLE_NUM;
			if (_TABLE_NUM>MAX_TABLE_NUM) 
			{
				fprintf(stderr,"TABLE_NUM_IS_TOO_BIG\n");
			}
			for (int i=0;i<_TABLE_NUM;++i)
				table[i]=BuildByConfig(configure[i]);
		*/	
		table[0]=new CuckooMap(DEFAULT_SIZE)<Key,Value>;
		table[1]=new CuckooMap(DEFAULT_SIZE)<Key,Value>;
//		table[1]=new MultiLudo();
		table[2]=new CuckooMap(DEFAULT_SIZE)<Key,Value>;
		fd=open(LogFile,O_RDWR | O_APPEND | O_CREAT);
		fsync(fd);
//		table[2]=new SingleLudo();
	}
	void Insert_Log(string Cur_Type,const Key &Cur_Key,const Value &Cur_Value)
	{
			strncpy(TempBuff,Cur_Type);
	}
	uint32_t insert(const Key& Cur_Key,const Value& Cur_Val)
	{
		Insert_Log("i",Cur_Key,Cur_Val);
		STATUS=table[0].insert(Cur_Key,Cur_Val);
		if (STATUS==OK) return OK;
		int Cur_Table=1;
		while (STATUS==FULL&&Cur_Table<TABLE_NUM)
		{
			STATUS=table[Cur_Table].Merge(table[Cur_Table-1].toMap());//for all layer>0,the table should return full if it can't add any element.
			if (STATUS==OK) break;
			if (STATUS==FULL)
			{
				++Cur_Table;
				continue;
			}
			Counter::count("Cuckoo Error in merge for layer "+to_string(Cur_Table));
			return ERROR;
		}
		if (Cur_Table==TABLE_NUM)
		{
			Counter::count("Cuckoo Error all layers are full");
			return ERROR;
		}
		table[0].insert(Cur_Key,Cur_Val);
		return OK;
	}
	uint32_t erase(Key& Cur_Key)
	{
		Insert_Log("e",Cur_Key,0);
		STATUS=table[0].erase(Cur_Key);
		if (STATUS==OK) return OK;
		int Cur_Table=1;
		while (STATUS==FULL&&Cur_Table<TABLE_NUM)
		{
			STATUS=table[Cur_Table].Merge(table[Cur_Table-1].toMap());//for all layer>0,the table should return full if it can't add any element.
			if (STATUS==OK) break;
			if (STATUS==FULL)
			{
				++Cur_Table;
				continue;
			}
			Counter::count("Cuckoo Error in merge for layer "+to_string(Cur_Table));
			return ERROR;
		}
		if (Cur_Table==TABLE_NUM)
		{
			Counter::count("Cuckoo Error all layers are full");
			return ERROR;
		}
		table[0].erase(Cur_Key);
		return OK;		
	}
	uint32_t modify(const Key& Cur_Key,const Value& Cur_Val)
	{
		Insert_Log("m",Cur_Key,Cur_Val);
		STATUS=table[0].modify(Cur_Key,Cur_Val);
		if (STATUS==OK) return OK;
		int Cur_Table=1;
		while (STATUS==FULL&&Cur_Table<TABLE_NUM)
		{
			STATUS=table[Cur_Table].Merge(table[Cur_Table-1].toMap());//for all layer>0,the table should return full if it can't add any element.
			if (STATUS==OK) break;
			if (STATUS==FULL)
			{
				++Cur_Table;
				continue;
			}
			Counter::count("Cuckoo Error in merge for layer "+to_string(Cur_Table));
			return ERROR;
		}
		if (Cur_Table==TABLE_NUM)
		{
			Counter::count("Cuckoo Error all layers are full");
			return ERROR;
		}
		table[0].modify(Cur_Key,Cur_Val);
		return OK;	
	}
	Value lookup(const Key& Cur_Key)
	{
		Insert_Log("l",Cur_Key,0);
		int i;
		Value Cur_Value;
		for (i=0;i<TABLE_NUM;++i)
			if (table[i].lookup(Cur_Key,Cur_Value)) break;
		if (Cur_Value==-1) 
			Counter::count("Cuckoo Error the key is deleted");
		if (i==TABLE_NUM)
			Counter::count("Cuckoo Error never find the key");
		return Cur_Value;
	}
};
	
