#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mt_timer.h"

int timer_init(MT_TIMER_OBJECT *object, void *(*thread_handler)(void *arg), int max_num)
{
    object->timer_max = max_num;
    object->timer_active_flag = true;
    object->timer_epoll_fd = epoll_create(max_num);
    if(object->timer_epoll_fd < 0)
    {
        MT_TIMER_PRINT_INL("MT-Timer error: epoll_create failed %s.\n", strerror(errno));
        return -1;
    }
    
    if(pthread_create(&object->timer_thread_id, NULL, thread_handler, NULL) != 0)
    {
        MT_TIMER_PRINT_INL("MT-Timer error: pthread_create failed %s.\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

void timer_deinit(MT_TIMER_OBJECT *object)
{
    struct itimerspec itimespec;

    object->timer_active_flag = false;

    itimespec.it_value.tv_sec = 0;
    itimespec.it_value.tv_nsec = 50;
    itimespec.it_interval.tv_sec = 0;
    itimespec.it_interval.tv_nsec = 0;

    timer_add(object, &itimespec, 0, NULL, NULL);
    pthread_join(object->timer_thread_id, NULL);
    timer_clear(object);
}

int timer_add(MT_TIMER_OBJECT *object, struct itimerspec *itimespec,
                int repeat, timer_callback_t cb, void *data)
{
    struct epoll_event event;
    MT_TIMER_NODE *handler = NULL;

    handler = (MT_TIMER_NODE *)malloc(sizeof(MT_TIMER_NODE));
    if(NULL == handler)
    {
        MT_TIMER_PRINT_INL("MT-Timer error: malloc failed %s.\n", strerror(errno));
        return -1;
    }
    
    handler->timer_cb = cb;
    handler->timer_cnt = repeat;
    handler->timer_data = data;
    handler->timer_fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC|TFD_NONBLOCK);
    if(handler->timer_fd < 0)
    {
        MT_TIMER_PRINT_INL("MT-Timer error: timerfd_create failed %s.\n", strerror(errno));
        free(handler);
        return -2;
    }
    if(timerfd_settime(handler->timer_fd, 0, itimespec, NULL) == -1)
    {
        MT_TIMER_PRINT_INL("MT-Timer error: timerfd_settime failed %s.\n", strerror(errno));
        close(handler->timer_fd);
        free(handler);
        return -3;
    }

    event.events = EPOLLIN | EPOLLET;
    event.data.ptr = handler;
    if(epoll_ctl(object->timer_epoll_fd, EPOLL_CTL_ADD, handler->timer_fd, &event) < 0)
    {
        MT_TIMER_PRINT_INL("MT-Timer error: epoll_ctl ADD failed %s.\n", strerror(errno));
        close(handler->timer_fd);
        free(handler);
        return -4;
    }

    pthread_rwlock_wrlock(&object->timer_rwlock);
    HASH_ADD_INT(object->timer_head, timer_fd, handler);
    pthread_rwlock_unlock(&object->timer_rwlock);
    
    return handler->timer_fd;
}

int timer_del(MT_TIMER_OBJECT *object, int timerfd)
{
    struct epoll_event event;
    MT_TIMER_NODE *handler = NULL;

    HASH_FIND_INT(object->timer_head, &timerfd, handler);
    if(NULL == handler)
        return 0;
    
    event.data.ptr = (void *)handler;
    event.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(object->timer_epoll_fd, EPOLL_CTL_DEL, handler->timer_fd, &event) < 0)
    {
        MT_TIMER_PRINT_INL("MT-Timer error: epoll_ctl DEL failed %s.\n", strerror(errno));
        return -1;
    }
    
    pthread_rwlock_wrlock(&object->timer_rwlock);
    HASH_DEL(object->timer_head, handler);
    pthread_rwlock_unlock(&object->timer_rwlock);
    close(handler->timer_fd);
    free(handler);
    
    return 0;
}

int timer_count(MT_TIMER_OBJECT *object)
{
    return HASH_COUNT(object->timer_head);
}

int timer_clear(MT_TIMER_OBJECT *object)
{
    struct epoll_event event;
    MT_TIMER_NODE *handler = NULL;

    event.events = EPOLLIN | EPOLLET;
    for(handler = object->timer_head; handler != NULL; handler = handler->hh.next)
    {
        event.data.ptr = (void *)handler;
        if(epoll_ctl(object->timer_epoll_fd, EPOLL_CTL_DEL, handler->timer_fd, &event) < 0)
        {
            MT_TIMER_PRINT_INL("MT-Timer error: epoll_ctl CLEAR failed %s.\n", strerror(errno));
            return -1;
        }
        pthread_rwlock_wrlock(&object->timer_rwlock);
        HASH_DEL(object->timer_head, handler);
        pthread_rwlock_unlock(&object->timer_rwlock);
        close(handler->timer_fd);
        free(handler);
    }
    return 0;
}

