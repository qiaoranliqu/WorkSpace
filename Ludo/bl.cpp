#include<cstdio>
#include<algorithm>
#include<cstring>
#include<map>

using namespace std;

int ty,key,val;

map<int,int>mp;

int main()
{
    freopen("release/data.in","r",stdin);
    freopen("release/bl.out","w",stdout);
    for (int i=0;i<30000;++i)
    {
        scanf("%d%d",&ty,&key);
        if (ty==0) 
        {
            scanf("%d",&val);
            mp[key]=val;
        }
        else printf("%d\n",mp[key]);
    }
}