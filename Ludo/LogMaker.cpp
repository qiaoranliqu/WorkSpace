#include<cstdio>
#include<algorithm>
#include<cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int i,j,m,n,p,k,b,c;

char a;

int main()
{
    int fd=open("release/MyDesign.log",O_RDONLY);
    if (fd==-1) puts("nmb");
    freopen("afterlog","w",stdout);
    pread(fd,&n,4,0);
    for (int i=0;i<10000;++i,puts(""))
        {
              pread(fd,&a,1,9*i);
              pread(fd,&b,4,9*i+1);
              pread(fd,&c,4,9*i+5);
              printf("%c %d %d\n",a,b,c);
        }
}