#include<cstdio>
#include<algorithm>
#include<cstring>

#include "../Ludo/MyDesign.h"

using namespace std;

int key,val,ty;

int main()
{
    MyDesign<int32_t,int32_t> rng;
    freopen("data.in","r",stdin);
    freopen("std.out","w",stdout);
    for (int i=0;i<30000;++i)
    {
        scanf("%d",&ty);
        if (ty==0)
        {
            scanf("%d%d",&key,&val);
            rng.insert(key,val);
        }
        else
        {
            scanf("%d",&key);
            printf("%d\n",rng.lookup(key));
        }
    }
}
