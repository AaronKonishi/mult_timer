#ifndef _TIMER_H__
#define _TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/timerfd.h>

typedef void (*timer_callback_t)(void *data);

extern int timer_init(int max_num);
extern void timer_deinit(void);
extern int timer_add(struct itimerspec *itimespec, int repeat, timer_callback_t cb, void *data);
extern int timer_del(int timerfd);
extern int timer_count(void);
extern int timer_clear(void);

#ifdef __cplusplus
}
#endif

#endif // _TIMER_H__

