# timer（MT译为Multiple或Multi）

#### 介绍

一个Linux下的超级简洁的定时器：*利用epoll机制和timerfd新特性实现的多重、多用、多个定时器实现*。只需要使用TIMER_CREATE()接口创建一个定时器实体，即**可向其添加成千上万个定时任务，定时任务可达到纳秒级别的精度，且可在同一时间点添加不同的定时任务！**。

#### 软件接口

整个定时器包含如下几类接口。

1. **创建和声明定时器实例**：使用定时器的第一步就是使用TIMER_CREATE()创建一个定时器实例，在其它文件使用到该定时器时，使用TIMER_DECLEAR()进行声明：
```
TIMER_CREATE(name);
TIMER_DECLEAR(name);
```

2. **初始化和反初始化定时器**：在正式使用定时器之前，首先使用TIMER_INIT()初始化前面创建的定时器实例，name是实例名称，max是创建定时器要检测的定时任务数量；当不再使用定时器时，可使用TIMER_DEINIT()反初始化定时器（退出定时器，并释放所有资源）：
```
TIMER_INIT(name, max);
TIMER_DEINIT(name);
```

3. **添加和删除定时任务**：
```
TIMER_ADD(name, itimespec, repeat, cb, data);
TIMER_DEL(name, timerfd);
```

TIMER_ADD()用于向定时器实例name中添加一个定时器，其参数描述如下：
- ittimespec是定时器的定时时间和循环事件，其结构体类型如下：
```
struct timespec {
    time_t tv_sec;  // seconds
    long   tv_nsec; // nanoseconds
};
struct itimerspec {
    struct timespec it_value;
    struct timespec it_interval;
};
```

其中it_value即是超时时间，若想定义周期定时器，则设置it_interval成员；若不想定义周期定时器，则需设置it_interval成员都为0。
- repeat是周期定时器的重复次数，若设置为-1，代表永远重复；
- cb为定时任务超时回调函数，其类型为：void (*timer_callback_t)(void *data)；
- data为定时任务回调函数的参数，为void *类型，用户可指定为自己定义的结构体；

TIMER_ADD()添加定时任务成功返回新定时任务的文件描述符，失败返回 < 0。返回的文件描述符，可用于在TIMER_DEL()中删除定时器。

4. **查询和清空定时器**
```
TIMER_COUNT(name);
TIMER_CLEAR(name);
```

TIMER_COUNT(name)用于查询定时器实例name中现存的定时任务个数；TIMER_CLEAR(name)用于清空定时器实例name中的所有定时任务。

#### 使用实例

下面是一个非常简单的使用示例：共创建了两个定时器，每个第一次超时都是3S，后面每隔1S超时一次；但第一个定时器频次为8，第二个定时器频次为3；当所有定时器都超时后，输入回车即可退出：
```
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
```

#### 参与贡献

1. Fork 本仓库
2. 新建 Feat_xxx 分支
3. 提交代码
4. 新建 Pull Request


#### 码云特技

1. 使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2. 码云官方博客 [blog.gitee.com](https://blog.gitee.com)
3. 你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解码云上的优秀开源项目
4. [GVP](https://gitee.com/gvp) 全称是码云最有价值开源项目，是码云综合评定出的优秀开源项目
5. 码云官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6. 码云封面人物是一档用来展示码云会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)