#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "timer.h"

void timeout_handle(void *arg)
{
    printf("[%ld]:timeout1\n", time(NULL));
}

void timeout_handler(void *arg)
{
    printf("[%ld]:timeout2\n", time(NULL));
}

int main(void)
{
    int timer1;
    struct itimerspec itimespec;

    timer_init(10);
    itimespec.it_value.tv_sec = 3;
    itimespec.it_value.tv_nsec = 0;
    itimespec.it_interval.tv_sec = 1;
    itimespec.it_interval.tv_nsec = 0;
    
    timer1 = timer_add(&itimespec, 8, timeout_handle, NULL);
    timer_add(&itimespec, 3, timeout_handler, NULL);
    printf("[%ld]:timer_add : %d\n", time(NULL), timer_count());
    
    sleep(4);//getchar();
    timer_del(timer1);
    printf("[%ld]:timer_del : %d\n", time(NULL), timer_count());
    timer_clear();
    printf("[%ld]:timer_clear : %d\n", time(NULL), timer_count());
    getchar();

    timer_deinit();
    
    return 0;
}

