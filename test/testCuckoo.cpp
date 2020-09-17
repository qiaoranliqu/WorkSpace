#include<cstdio>
#include<algorithm>
#include<cstring>

#include "../Ludo/MyDesign.h"

using namespace std;

int main()
{
    MyDesign<int32_t,int32_t> rng;
    for (int i=0;i<5;++i) rng.insert(i,i);
    for (int i=0;i<5;++i) rng.lookup(i);
    for (int i=0;i<5;++i) rng.modify(i,i+1);
    for (int i=0;i<5;++i) rng.erase(i);
    for (int i=0;i<5;++i) rng.lookup(i);
}
