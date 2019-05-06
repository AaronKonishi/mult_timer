#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mt_timer.h"

void timeout_handle(void *arg)
{
    printf("[%ld]:timeout1\n", time(NULL));
}

void timeout_handler(void *arg)
{
    printf("[%ld]:timeout2\n", time(NULL));
}

TIMER_CREATE(test);

int main(void)
{
    int timer;
    struct itimerspec itimespec;

    TIMER_INIT(test, 10);
    itimespec.it_value.tv_sec = 3;
    itimespec.it_value.tv_nsec = 0;
    itimespec.it_interval.tv_sec = 1;
    itimespec.it_interval.tv_nsec = 0;
    
    timer = TIMER_ADD(test, &itimespec, 8, timeout_handle, NULL);
    TIMER_ADD(test, &itimespec, 3, timeout_handler, NULL);
    printf("[%ld]:timer_add : %d\n", time(NULL), TIMER_COUNT(test));
    
    sleep(4);//getchar();
    TIMER_DEL(test, timer);
    printf("[%ld]:timer_del : %d\n", time(NULL), TIMER_COUNT(test));
    TIMER_CLEAR(test);
    printf("[%ld]:timer_clear : %d\n", time(NULL), TIMER_COUNT(test));
    getchar();

    TIMER_DEINIT(test);
    
    return 0;
}

