#include<cstdio>
#include<algorithm>
#include<cstring>
#include<ctime>
#include<map>

using namespace std;

map<int,int>mp;

int key,value,A[10000005];

int main()
{
    freopen("data.in","w",stdout);
    srand((int)time(0));
    for (int i=0;i<500000;++i)
    {
        int ty=(i<10?0:rand()&1);
        if(ty==0)
        {
            int key=rand()%100000+1;
            while(mp.find(key)!=mp.end()) key=rand()%1000000+1;
            value=rand()%100000+1;
            printf("0 %d %d\n",key,value);
            mp[key]=1; A[++A[0]]=key;
        }
        else
        {
            printf("1 %d\n",A[rand()%A[0]+1]);
        }
    }
}