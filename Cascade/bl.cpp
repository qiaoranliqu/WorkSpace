#include<cstdio>
#include<algorithm>
#include<cstring>
#include<map>

using namespace std;

int ty,key,val;

map<int,int>mp;

int main()
{
    freopen("data.in","r",stdin);
    freopen("bl.out","w",stdout);
    for (int i=0;i<500000;++i)
    {
        scanf("%d%d",&ty,&key);
        if (ty==0) 
        {
            scanf("%d",&val);
            mp[key]=val;
        }
        else if (ty==1)
        {
            if (mp.find(key)==mp.end()) puts("None key matches!");
            else
            printf("%d\n",mp[key]);
        }
        else if (ty==2)
        {
            if (mp.find(key)==mp.end()) puts("This key is already deleted!");
            else mp.erase(key);
        }
        else if (ty==3)
        {
            scanf("%d",&val);
            mp[key]=val;
        }
    }
}