#pragma once

#include "cstdlib"
#include "vector"
#include "../common.h"
#include "Cuckoo/cuckoo.h"

#define MAX_TABLE_NUM 5

template<class Key, class Value, uint8_t VL = sizeof(Value) * 8, uint8_t DL = 0>
class MyDesign{
	uint32_t TABLE_NUM=3;
	uint32_t STATUS;
	Base* table[MAX_TABLE_NUM];
	MultiLudo(/*uint32_t _TABLE_NUM,vector<Config> configure*/)
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
		table[0]=new cuckoo()<Key,Value>;
		table[1]=new MultiLudo();
		table[2]=new SingleLudo();
	}
	~MultiLudo()
	{
		for (int i=0;i<TABLE_NUM;++i) delete table[i];
	}
	uint32_t insert(Key& Cur_Key,Value& Cur_Val)
	{
		int My_ID=0;
		while (My_ID<TABLE_NUM&&(STATUS=table[My_ID].insert(Cur_Key,Cur_Val))!=OK)
		{
			switch STATUS{
				case FULL:
					++My_ID;
					if (My_ID==TABLE_NUM) break;
					table[My_ID+1].Merge(table[My_ID].toMap());
					break;
				case EXISTS:
					Output(1,"K-V already exists in table "+toString(My_ID));
					break;
				default:
					Output(1,"Unknown Errors");
				}
		}
		return STATUS;
	}
	uint32_t erase(Key& Cur_Key)
	{
		int My_ID=0;
		while (My_ID<TABLE_NUM&&(STATUS=table[My_ID].erase(Cur_Key))!=OK)
		{
			switch STATUS{
				case FULL:
					++My_ID;
					if (My_ID==TABLE_NUM) break;
					table[My_ID+1].Merge(table[My_ID].toMap());
					break;
				case EXISTS:
					Output(1,"K-V already exists in table "+toString(My_ID));
					break;
				default:
					Output(1,"Unknown Errors");
				}
		}
		return STATUS;
	}
	uint32_t modify(Key& Cur_Key,Value& Cur_Val)
	{
		int My_ID=0;
		while (My_ID<TABLE_NUM&&(STATUS=table[My_ID].modify(Cur_Key,Cur_Val))!=OK)
		{
			switch STATUS{
				case FULL:
					++My_ID;
					if (My_ID==TABLE_NUM) break;
					table[My_ID+1].Merge(table[My_ID].toMap());
					break;
				case EXISTS:
					Output(1,"K-V already exists in table "+toString(My_ID));
					break;
				default:
					Output(1,"Unknown Errors");
				}
		}
		return STATUS;		
	}
};
	
