#include <time.h>
#include <stdio.h>

typedef time_t  (*time_func)(time_t *);

int main(int argc, char *argv[]) {
    time_t tloc;
    int retval = 0;

    time_func func = (time_func)0xffffffffff600000;

    retval = func(&tloc);
    if (retval < 0) {
        perror("time_func");
        return -1;
    }
    printf("%ld\n", tloc);


    return 0;
}
