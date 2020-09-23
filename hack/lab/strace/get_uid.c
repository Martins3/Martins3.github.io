#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

int main(void)
{
    printf("uid is %d\n", getuid());
    return 0;
}
