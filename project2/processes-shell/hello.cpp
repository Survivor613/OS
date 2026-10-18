#include <cstdio>
#include <unistd.h>
int main()
{
    printf("hello");   // 没有 \n
    sleep(3);          // 不读输入，也不结束，就等 3 秒
    printf("\n");
}