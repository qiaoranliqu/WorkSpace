#pragma once

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "MultiLudo.h"
#include "../common.h"
#include "common.h"
#include "Cuckoo/cuckoo.h"
#include "SingleLudo.h"

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
		table[0]=new CuckooMap<Key,Value>(DEFAULT_SIZE);
		table[1]=new MultiLudo<Key,Value>("SecondLayer");
		table[2]=new SingleLudo<Key,Value>("ThirdLayer");
		fd=open(LogFile.c_str(),O_RDWR | O_APPEND | O_CREAT,0777);
//		fsync(fd);
		close(fd);
//		table[2]=new SingleLudo();
	}
	int LogBufferOffset=0,logFileOffset=0;
	void appendLog(char* k,int klen)
	{
//		fprintf(stderr,"%d %c\n",klen,k[0]);
		int kOffset=0,fd=open(LogFile.c_str(),O_RDWR | O_APPEND);
//		fprintf(stderr,"%d\n",fd);
//		exit(0);
		while (LogBufferOffset+(klen-kOffset)>=4096)
		{
			int toWrite=4096-LogBufferOffset;
			memcpy(LogBuff+LogBufferOffset,k+kOffset,toWrite);
			LogBufferOffset=0;
			kOffset+=toWrite;
			fprintf(stderr,"%d\n",logFileOffset);
			pwrite(fd,LogBuff,4096,logFileOffset);
			logFileOffset+=4096;
		}
		if (logFileOffset > logFileSize)
		{
			 Counter::count("Desgin fail logFile is too large");
			 return;
		}
		int toWrite=klen-kOffset;
		memcpy(LogBuff+LogBufferOffset,k+kOffset,toWrite);
		LogBufferOffset+=toWrite;
		close(fd);
	}
	void Insert_Log(string Cur_Type,const Key &Cur_Key,const Value &Cur_Value)
	{
		int tmplength=0;
		strncpy(TempBuff,Cur_Type.c_str(),Cur_Type.length()); tmplength+=Cur_Type.length();
		strncpy(TempBuff+tmplength,(char*)&Cur_Key,sizeof(Key)*8); tmplength+=sizeof(Key)*8;
		strncpy(TempBuff+tmplength,(char*)&Cur_Value,sizeof(Value)*8); tmplength+=sizeof(Value)*8;
		appendLog(TempBuff,tmplength);
	}
	uint32_t insert(const Key& Cur_Key,const Value& Cur_Val)
	{
		Insert_Log("i",Cur_Key,Cur_Val);
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
		Insert_Log("e",Cur_Key,0);
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
		Insert_Log("m",Cur_Key,Cur_Val);
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
		Insert_Log("l",Cur_Key,0);
		int i;
		Value Cur_Value;
		for (i=0;i<TABLE_NUM;++i)
			if (table[i]->lookup(Cur_Key,Cur_Value)) break;
		if (Cur_Value==-1) 
			Counter::count("Cuckoo Error the key is deleted");
		if (i==TABLE_NUM)
			Counter::count("Cuckoo Error never find the key");
		return Cur_Value;
	}
};
	
