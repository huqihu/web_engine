#pragma once
#include "Mylogger.h"
#include "bo_threadPool.h"
#include "myhead.h"

class TimerTask
{
public:
    TimerTask(int second)
        : _seconds(second) {}

    virtual ~TimerTask(){};
    void decreaseCount()
    {
        --_count;
    }
    void setCount(int count)
    {
        _count = count;
    }
    int getTimeVal()
    {
        return _seconds;
    }
    int getCount()
    {
        return _count;
    }
    virtual void process() = 0; //要执行的任务
private:
    int _seconds;
    int _count;
};

class TimerManager
{
public:
    TimerManager(int slotSize)
        : _timerFd(createTimerFd()), _isRunning(false), _slotSize(slotSize), _curSlot(0)
    {
        _timerWheelList.reserve(_slotSize);
        for (int i = 0; i != _slotSize; i++)
        {
            _timerWheelList.push_back(list<TimerTask *>());
        }
    }
    int createTimerFd();
    void start();
    void stop();
    void addTask(TimerTask *pTask);
    void delTask(TimerTask *pTask);
    void handleTask();
    void setTime(int value, int interVal);
    void readTimerFd();

private:
    int _timerFd;
    bool _isRunning;
    vector<list<TimerTask *>> _timerWheelList;
    int _curSlot;
    int _slotSize;
};

int TimerManager::createTimerFd()
{
    int fd = timerfd_create(CLOCK_REALTIME, 0);
    if (-1 == fd)
    {
        perror("timerfd_create()");
    }
    return fd;
}
void TimerManager::start()
{
    pollfd pfd; //poll 这个接口是直接将一个 pollfd结构体数组 传进去,其与select无太多区别
    pfd.fd = _timerFd;
    pfd.events = POLLIN;
    _isRunning = true;
    setTime(1, 1);
    while (_isRunning)
    {
        int ret = poll(&pfd, 1, 1000);
        //注意 这里的poll 有超时时间 因为 系统应该每个一段时间检查一下_isRunnin标志位来看看timerWhel是不是处于运转状态
        if (-1 == ret && EINTR == errno)
        {
            continue;
        }
        else if (-1 == ret)
        {
            perror("poll");
            return;
        }
        else if (0 == ret)
        {
            cout << "poll timeout" << endl;
        }
        else
        {
            readTimerFd(); //别忘了这个,清空timerfd,不清空的话,水平模式下,poll 会被一直触发 定时其
            handleTask();
        }
    }
    cout << "timerManager shutdown" << endl;
}
void TimerManager::handleTask()
{
    //先 ++ curSlot ,然后遍历当前曹 如果里面的 count =0,就将其取出 然后在后面执行任务,执行完毕之后 在加进去
    //对于不是0的,则是count 减一
    _curSlot = (_curSlot + 1) % _slotSize;
    list<TimerTask *> temList;
    //注意迭代器失效问题 当删除一个元素之后,需要重新设置迭代器的位置
    //list 中 删除一个节点之后 这个节点就被delete了,原有的迭代器是指向这个节点的,访问这个节点很危险
    for (auto task = _timerWheelList[_curSlot].begin(); task != _timerWheelList[_curSlot].end();)
    {
        //这里 无论有没有删除这个节点,都必须 在下一次访问下一个节点
        if ((*task)->getCount() <= 0)
        { //先加入到temList 然后移除
            temList.push_back((*task));
            addTask((*task));
            task = _timerWheelList[_curSlot].erase(task);
            //erase之后,返回的是下一个节点(指的是被删除节点的下一个节点)的迭代器
        }
        else
        {
            (*task)->decreaseCount();
            task++;
        }
    }
    for (auto task : temList)
    {
        task->process();
    }
}
void TimerManager::readTimerFd()
{
    uint64_t tem;
    int ret = read(_timerFd, &tem, sizeof(tem));
    if (ret < sizeof(tem))
    {
        perror("readTimerFd()");
    }
}
void TimerManager::setTime(int value, int interVal)
{
    struct itimerspec timeValue;
    timeValue.it_value.tv_sec = value;
    timeValue.it_value.tv_nsec = 0;
    timeValue.it_interval.tv_sec = interVal;
    timeValue.it_interval.tv_nsec = 0;
    timerfd_settime(_timerFd, 0, &timeValue, nullptr); //第三个设置时间 如果是0,则停止定时器 ,第四个是以前设置的时间,不需要就写nullptr
}
void TimerManager::addTask(TimerTask *pTask)
{
    if (pTask)
    {
        //根据时间和曹数 ,获取圈数和 槽位置 然后设置这个任务
        if (pTask->getTimeVal() < 1)
        {
            cout << "wrong task time" << endl;
            return;
        }
        int curSlot = _curSlot;
        int slot = (pTask->getTimeVal() + curSlot) % _slotSize;
        int count = pTask->getTimeVal() / _slotSize;
        _timerWheelList[slot].push_back(pTask);
        if (slot == curSlot) //如果是 插入的任务时间是曹数的整数倍,那么 此时会插入到当前曹 的任务 ,执行间隔会多一圈
        {
            pTask->setCount(count - 1);
        }
        else
        {
            pTask->setCount(count);
        }
    }
}
void TimerManager::delTask(TimerTask *pTask)
{
    if (pTask)
    {
        for (auto &slot : _timerWheelList) //注意 这里由于 要修改 slot的值,所以需要使用引用,如果不使用引用的话,没办法修改(不使用引用就是 拷贝的一个局部变量)
        {
            for (auto task : slot)
            {
                if (task == pTask)
                {
                    slot.remove(task);
                    return;
                }
            }
        }
    }
}

void TimerManager::stop()
{
    setTime(0, 0); //当设置时间为 0的时候,timer_Fd会停止
    if (_isRunning)
    {
        _isRunning = false;
    }
}
