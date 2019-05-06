/* 
 * MIT License
 *
 * Copyright (c) 2019 极简美 @ konishi5202@163.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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

