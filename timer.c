#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/epoll.h>

#include "uthash.h"
#include "timer.h"

#ifndef bool
typedef enum{false, true} bool;
#define bool bool
#endif
    
typedef struct {
    int timer_fd;
    int timer_cnt;
    void *timer_data;
    UT_hash_handle hh;
    timer_callback_t timer_cb;
}MT_TIMER_NODE;

typedef struct {
    int  timer_max;
    int  timer_epoll_fd;
    bool timer_active_flag;
    MT_TIMER_NODE *timer_head;
    pthread_t timer_thread_id;
    pthread_rwlock_t  timer_rwlock;
}MT_TIMER_OBJECT;

MT_TIMER_OBJECT timer_object_name = {0, -1, false, NULL, 0, PTHREAD_RWLOCK_INITIALIZER};

static void *mt_timer_thread_name(void *arg)
{
    int nfds, i;
    char buf[128];
    MT_TIMER_NODE *timer_node = NULL;
    struct epoll_event events[timer_object_name.timer_max];

    /*pthread_detach(pthread_self());*/
    printf("MT-Timer Info: timer thread is running.\n");
    while(timer_object_name.timer_active_flag)
    {
        nfds = epoll_wait(timer_object_name.timer_epoll_fd, events, timer_object_name.timer_max, -1);
        if(nfds <= 0)
            continue;
        for(i = 0; i < nfds; i++)
        {
            timer_node = (MT_TIMER_NODE *)events[i].data.ptr;
            if(NULL == timer_node)
                continue;
            while(read(timer_node->timer_fd, buf, sizeof(buf)) > 0);
            if(NULL == timer_node->timer_cb)
                continue;

            if(-1 == timer_node->timer_cnt)
                timer_node->timer_cb(timer_node->timer_data);
            else
            {
                if(timer_node->timer_cnt)
                {
                    timer_node->timer_cb(timer_node->timer_data);
                    timer_node->timer_cnt--;
                }
            }
        }
    }
    printf("MT-Timer Info: timer thread is exit.\n");
    pthread_exit(NULL);
}

int timer_init(int max_num)
{
    timer_object_name.timer_max = max_num;
    timer_object_name.timer_active_flag = true;
    timer_object_name.timer_epoll_fd = epoll_create(max_num);
    if(timer_object_name.timer_epoll_fd < 0)
        return -1;

    if(pthread_create(&timer_object_name.timer_thread_id, NULL, mt_timer_thread_name, NULL) != 0)
        return -1;

    return 0;
}

void timer_deinit(void)
{
    struct itimerspec itimespec;
    
    timer_object_name.timer_active_flag = false;

    itimespec.it_value.tv_sec = 0;
    itimespec.it_value.tv_nsec = 50;
    itimespec.it_interval.tv_sec = 0;
    itimespec.it_interval.tv_nsec = 0;

    timer_add(&itimespec, 0, NULL, NULL);
    
    pthread_join(timer_object_name.timer_thread_id, NULL);
}

int timer_add(struct itimerspec *itimespec, int repeat, timer_callback_t cb, void *data)
{
    struct epoll_event event;
    MT_TIMER_NODE *handler = NULL;

    handler = (MT_TIMER_NODE *)malloc(sizeof(MT_TIMER_NODE));
    if(NULL == handler)
        return -1;

    handler->timer_cb = cb;
    handler->timer_cnt = repeat;
    handler->timer_data = data;
    handler->timer_fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC|TFD_NONBLOCK);
    if(handler->timer_fd < 0)
    {
        free(handler);
        return -2;
    }
    if(timerfd_settime(handler->timer_fd, 0, itimespec, NULL) == -1)
    {
        free(handler);
        return -3;
    }

    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = handler;
    if(epoll_ctl(timer_object_name.timer_epoll_fd, EPOLL_CTL_ADD, handler->timer_fd, &event) < 0)
    {
        free(handler);
        return -4;
    }

    pthread_rwlock_wrlock(&timer_object_name.timer_rwlock);
    HASH_ADD_INT(timer_object_name.timer_head, timer_fd, handler);
    pthread_rwlock_unlock(&timer_object_name.timer_rwlock);
    
    return handler->timer_fd;
}

int timer_del(int timerfd)
{
    struct epoll_event event;
    MT_TIMER_NODE *handler = NULL;

    HASH_FIND_INT(timer_object_name.timer_head, &timerfd, handler);
    if(NULL == handler)
        return 0;
    pthread_rwlock_wrlock(&timer_object_name.timer_rwlock);
    HASH_DEL(timer_object_name.timer_head, handler);
    pthread_rwlock_unlock(&timer_object_name.timer_rwlock);
    event.data.ptr = (void *)handler;
    event.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(timer_object_name.timer_epoll_fd, EPOLL_CTL_DEL, handler->timer_fd, &event) < 0)
        return -1;
    close(handler->timer_fd);
    free(handler);
    
    return 0;
}

int timer_count(void)
{
    int count = 0;
    pthread_rwlock_rdlock(&timer_object_name.timer_rwlock);
    count = (int) HASH_COUNT(timer_object_name.timer_head);
    pthread_rwlock_unlock(&timer_object_name.timer_rwlock);
    return count;
}

int timer_clear(void)
{
    struct epoll_event event;
    MT_TIMER_NODE *handler = NULL;

    event.events = EPOLLIN | EPOLLET;
    for(handler = timer_object_name.timer_head; handler != NULL; handler = handler->hh.next)
    {
        pthread_rwlock_wrlock(&timer_object_name.timer_rwlock);
        HASH_DEL(timer_object_name.timer_head, handler);
        pthread_rwlock_unlock(&timer_object_name.timer_rwlock);
        event.data.ptr = (void *)handler;
        if(epoll_ctl(timer_object_name.timer_epoll_fd, EPOLL_CTL_DEL, handler->timer_fd, &event) < 0)
            return -1;
        close(handler->timer_fd);
        free(handler);
    }
    return 0;
}

