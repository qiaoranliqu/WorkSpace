#include<cstdio>
#include<algorithm>
#include<cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int i,j,m,n,p,k;

int Key[100005][4],Value[100005][4];

int main()
{
    int fd=open("release/SecondLayer_0",O_RDONLY);
    freopen("aftertable","w",stdout);
    pread(fd,&n,4,0);
    for (int i=0;i<n;++i,puts(""))
        for (int j=0;j<4;++j)
        {
              pread(fd,&Key[i][j],4,4+(32*i+8*j));
              pread(fd,&Value[i][j],4,8+(32*i+8*j));
              printf("%d,%d ",Key[i][j],Value[i][j]);
        }
}