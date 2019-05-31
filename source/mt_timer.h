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

#ifndef _MT_TIMER_H__
#define _MT_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include "uthash.h"

#define DEBUG_ENABLE
/************************** Debug Config Start **************************/
#ifdef DEBUG_ENABLE

  #if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L

    #ifdef EFSM_PRINT
      #define MT_TIMER_PRINT_INL(...) EFSM_PRINT(__VA_ARGS__)
    #else
      #define MT_TIMER_PRINT_INL(...) printf(__VA_ARGS__)
    #endif

  #else

    #ifdef EFSM_PRINT
      #define MT_TIMER_PRINT_INL(format, args...) EFSM_PRINT(format, ##args)
    #else
      #define MT_TIMER_PRINT_INL(format, args...) printf(format, ##args)
    #endif
  #endif

#else

  #if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
    #define MT_TIMER_PRINT_INL(...)
  #else
    #define MT_TIMER_PRINT_INL(format, args...)
  #endif

#endif
/************************** Debug Config End **************************/

#ifndef bool
typedef enum{false, true} bool;
#define bool bool
#endif

typedef void (*timer_release_t)(void *data);
typedef void (*timer_callback_t)(void *data);

typedef struct {
    int timer_fd;
    int timer_cnt;
    void *timer_data;
    UT_hash_handle hh;
    timer_callback_t timer_cb;
    timer_release_t release_cb;
}MT_TIMER_NODE; /* MT mean multiple */

typedef struct {
    int  timer_max;
    int  timer_epoll_fd;
    bool timer_active_flag;
    MT_TIMER_NODE *timer_head;
    pthread_t timer_thread_id;
    pthread_rwlock_t  timer_rwlock;
}MT_TIMER_OBJECT;

#define TIMER_THREAD_CREATE(name) \
        static void *mt_timer_thread_##name(void *arg) \
        { \
            int nfds, i; \
            char buf[128]; \
            struct epoll_event event; \
            MT_TIMER_NODE *timer_node = NULL; \
            struct epoll_event events[mt_timer_##name.timer_max]; \
            /*pthread_detach(pthread_self());*/ \
            event.events = EPOLLIN | EPOLLET; \
            MT_TIMER_PRINT_INL("MT-Timer Info: %s thread is running.\n", #name); \
            while(mt_timer_##name.timer_active_flag) \
            { \
                nfds = epoll_wait(mt_timer_##name.timer_epoll_fd, events, mt_timer_##name.timer_max, -1); \
                if(nfds <= 0) \
                    continue; \
                for(i = 0; i < nfds; i++) \
                { \
                    timer_node = (MT_TIMER_NODE *)events[i].data.ptr; \
                    if(NULL == timer_node) \
                        continue; \
                    while(read(timer_node->timer_fd, buf, sizeof(buf)) > 0); \
                    if(NULL == timer_node->timer_cb) \
                        continue; \
                    if(-1 == timer_node->timer_cnt) \
                        timer_node->timer_cb(timer_node->timer_data); \
                    else \
                    { \
                        if(timer_node->timer_cnt) \
                        { \
                            timer_node->timer_cb(timer_node->timer_data); \
                            timer_node->timer_cnt--; \
                        } \
                        else \
                        { \
                            event.data.ptr = (void *)timer_node; \
                            epoll_ctl(mt_timer_##name.timer_epoll_fd, EPOLL_CTL_DEL, timer_node->timer_fd, &event); \
                            pthread_rwlock_wrlock(&mt_timer_##name.timer_rwlock); \
                            HASH_DEL(mt_timer_##name.timer_head, timer_node); \
                            pthread_rwlock_unlock(&mt_timer_##name.timer_rwlock); \
                            close(timer_node->timer_fd); \
                            if(timer_node->release_cb) \
                                timer_node->release_cb(timer_node->timer_data); \
                            free(timer_node); \
                        } \
                    } \
                } \
            } \
            MT_TIMER_PRINT_INL("MT-Timer Info: %s thread is exit.\n", #name); \
            pthread_exit(NULL); \
        }

extern int timer_init(MT_TIMER_OBJECT *object, void *(*thread_handler)(void *arg), int max_num);
extern void timer_deinit(MT_TIMER_OBJECT *object);
extern int timer_add(MT_TIMER_OBJECT *object, struct itimerspec *itimespec,
                       int repeat, timer_callback_t cb, void *data, timer_release_t rb);
extern int timer_del(MT_TIMER_OBJECT *object, int timerfd);
extern int timer_count(MT_TIMER_OBJECT *object);
extern int timer_clear(MT_TIMER_OBJECT *object);

/************************** API Interface **************************/
#define TIMER_CREATE(name) \
        MT_TIMER_OBJECT mt_timer_##name = {0, -1, false, NULL, 0, PTHREAD_RWLOCK_INITIALIZER}; \
        TIMER_THREAD_CREATE(name)
#define TIMER_DECLEAR(name) \
        extern MT_TIMER_OBJECT mt_timer_##name
#define TIMER_INIT(name, max) \
        timer_init(&mt_timer_##name, mt_timer_thread_##name, max)
#define TIMER_DEINIT(name) \
        timer_deinit(&mt_timer_##name)
#define TIMER_ADD(name, itimespec, repeat, cb, data, rb) \
        timer_add(&mt_timer_##name, itimespec, repeat, cb, data, rb)
#define TIMER_DEL(name, timerfd) \
        timer_del(&mt_timer_##name, timerfd)
#define TIMER_COUNT(name) timer_count(&mt_timer_##name)
#define TIMER_CLEAR(name) timer_clear(&mt_timer_##name)

#ifdef __cplusplus
}
#endif

#endif // _MT_TIMER_H__

