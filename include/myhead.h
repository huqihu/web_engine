#pragma once
//#define _GNU_SOURCE
//#define _XOPEN_SOURCE
#include <crypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <setjmp.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <sys/shm.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <mysql/mysql.h>
#include <sys/timerfd.h>
#include <poll.h>
#define ARGS_CHECK(argc, val)       \
    {                               \
        if (argc != val)            \
        {                           \
            printf("error args\n"); \
            return -1;              \
        }                           \
    }
#define ARGC_CHECK(argc, val)       \
    {                               \
        if (argc != val)            \
        {                           \
            printf("error args\n"); \
            return -1;              \
        }                           \
    }
#define ERROR_CHECK(ret, retVal, funcName) \
    {                                      \
        if (ret == retVal)                 \
        {                                  \
            perror(funcName);              \
            return -1;                     \
        }                                  \
    }
#define THREAD_ERRORCHECK(ret, funcName)                   \
    do                                                     \
    {                                                      \
        if (0 != ret)                                      \
        {                                                  \
            printf("%s : %s \n", funcName, strerror(ret)); \
        }                                                  \
    } while (0)
#define THREAD_ERROR_CHECK(ret, funcName)                  \
    do                                                     \
    {                                                      \
        if (0 != ret)                                      \
        {                                                  \
            printf("%s : %s \n", funcName, strerror(ret)); \
        }                                                  \
    } while (0)
#define CHILD_THREAD_ERROR_CHECK(ret, funcName)            \
    do                                                     \
    {                                                      \
        if (0 != ret)                                      \
        {                                                  \
            printf("%s : %s \n", funcName, strerror(ret)); \
            return (void *)-1;                             \
        }                                                  \
    } while (0)

//下面是c++的
#include <iostream>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>

#include <regex>

#include <map>
#include <vector>
#include <set>

#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <list>
#include <stack>
#include <queue>

#include <cmath>
#include <memory> //包含智能指针,空间配置器等等

#include <algorithm>  //包括泛型算法 指的是依赖于迭代器,操作迭代器实现的算法
#include <numeric>    //包括泛化的算数算法 主要是一些重用的运算
#include <functional> //包含bind
#include <errno.h>
#include <sys/eventfd.h>
using namespace std;
