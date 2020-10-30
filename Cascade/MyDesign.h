#pragma once

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../common.h"
#include "common.h"
#include "SimpleLink.h"
#include "BlockArray.h"

#define MAX_TABLE_NUM 5
#define MAX_BUFF_SIZE 4096
#define DEFAULT_SIZE 16384
#define logFileSize 2147483647

template<class Key, class Value>
class MyDesign{
	public:
	uint32_t TABLE_NUM=3;
	uint32_t STATUS;
	uint32_t fd;
	Base<Key,Value>* table[MAX_TABLE_NUM];
	const string LogFile="MyDesign.log";
	char LogBuff[MAX_BUFF_SIZE];
	char TempBuff[MAX_BUFF_SIZE];
	explicit MyDesign()
	{
		table[0]=new SimpleLink<Key,Value>(DEFAULT_SIZE,LogFile);
		table[1]=new BlockArray<Key,Value>("SecondLayer");
		table[2]=new ComplexLink<Key,Value>("ThirdLayer");
		fd=open(LogFile.c_str(),O_RDWR | O_CREAT | O_TRUNC,0777);
//		fsync(fd);
		close(fd);
//		table[2]=new SingleLudo();
	}
	uint32_t insert(const Key& Cur_Key,const Value& Cur_Val)
	{
		STATUS=table[0]->insert(Cur_Key,Cur_Val);
		if (STATUS==OK) return OK;
		int Cur_Table=1;
		while (STATUS==FULL&&Cur_Table<TABLE_NUM)
		{
			STATUS=table[Cur_Table]->Merge(table[Cur_Table-1]->toMap());//for all layer>0,the table should return full if it can't add any element.
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
		table[0]->insert(Cur_Key,Cur_Val);
		return OK;
	}
	uint32_t erase(Key& Cur_Key)
	{
		STATUS=table[0]->erase(Cur_Key);
		if (STATUS==OK) return OK;
		int Cur_Table=1;
		while (STATUS==FULL&&Cur_Table<TABLE_NUM)
		{
			STATUS=table[Cur_Table]->Merge(table[Cur_Table-1]->toMap());//for all layer>0,the table should return full if it can't add any element.
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
		table[0]->erase(Cur_Key);
		return OK;		
	}
	uint32_t modify(const Key& Cur_Key,const Value& Cur_Val)
	{
		STATUS=table[0]->modify(Cur_Key,Cur_Val);
		if (STATUS==OK) return OK;
		int Cur_Table=1;
		while (STATUS==FULL&&Cur_Table<TABLE_NUM)
		{
			STATUS=table[Cur_Table]->Merge(table[Cur_Table-1]->toMap());//for all layer>0,the table should return full if it can't add any element.
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
		table[0]->modify(Cur_Key,Cur_Val);
		return OK;	
	}
	Value lookup(const Key& Cur_Key)
	{
//		fprintf(stderr,"lookuplog\n");
		int i;
		Value Cur_Value;
		for (i=0;i<TABLE_NUM;++i)
		{
//			fprintf(stderr,"TABLE[%d] lookup\n",i);
			if (table[i]->lookup(Cur_Key,Cur_Value)==OK) break;
		}
		if (Cur_Value==-1) 
			Counter::count("Cuckoo Error the key is deleted"),puts("None key matches!");
		else 
		if (i==TABLE_NUM)
			Counter::count("Cuckoo Error never find the key"),puts("None key matches!");
//		if (Cur_Value==0) fprintf(stderr,"%d %d\n",Cur_Key,i);
		return Cur_Value;
	}
};
	
