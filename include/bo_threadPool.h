#pragma once
#include "myhead.h"
class Noncopyable //对象语义的基类,不允许值语义
{
public:
    Noncopyable() {} //需要注意的是 其派生类在创建对象的时候 需要调用noncopyable的构造函数
    //默认的是调用的无参的构造函数,将下面这个设置成delete之后,系统不会自动提供无参的构造函数,所以需要自己写出来
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete;
};

//锁
class MutexLock
    //: public Noncopyable
{
public:
    MutexLock()
    {
        if (pthread_mutex_init(&_mutex, nullptr))
        {
            perror("pthread_mutex_init");
        }
    }
    void lock()
    {
        if (pthread_mutex_lock(&_mutex))
        {
            perror("pthread_mutex_lock");
        }
    }
    int tryLock()
    {
        return pthread_mutex_trylock(&_mutex);
    }
    void unlock()
    {
        int ret;
        ret = pthread_mutex_unlock(&_mutex);
        if (ret)
        {
            cout << strerror(ret) << endl;
        }
    }
    ~MutexLock()
    {
        if (pthread_mutex_destroy(&_mutex))
        {
            perror("pthread_mutex_destroy");
        }
    }
    pthread_mutex_t *getMutexPtr()
    {
        return &_mutex;
    }

private:
    pthread_mutex_t _mutex;
};

// 局部结束时自动打开锁 raii 生命周期 析构函数

class MutexLockGuard
    : public Noncopyable
{
public:
    MutexLockGuard(MutexLock &mutex)
        : _mutex(mutex)
    {
        _mutex.lock();
    }
    ~MutexLockGuard()
    {
        _mutex.unlock();
    }

private:
    MutexLock &_mutex;
};

//条件变量 里面保存着一个MutexLock的引用
// notify   notifyall wait
class Condition
    : public Noncopyable
{
public:
    Condition(MutexLock &mutex)
        : _mutex(mutex)
    {
        if (pthread_cond_init(&_cond, nullptr))
        {
            perror("pthread_cond_init");
        }
    }
    ~Condition()
    {
        if (pthread_cond_destroy(&_cond))
        {
            perror("pthread_cond_destroy");
        }
    }
    void notify()
    {
        if (pthread_cond_signal(&_cond))
        {
            perror("pthread_cond_signal");
        }
    }
    void notifyall()
    {
        if (pthread_cond_broadcast(&_cond))
        {
            perror("pthread_cond_broadcast");
        }
    }
    void wait()
    {
        if (pthread_cond_wait(&_cond, _mutex.getMutexPtr()))
        {
            perror("pthread_cond_wait");
        }
    }

private:
    pthread_cond_t _cond;
    MutexLock &_mutex;
};

using Task = function<void()>;
//taskQueue 保存任务 任务队列 队列容量 两个cond 一个锁
//一个pop  一个push empty full
class TaskQueue
{
public:
    TaskQueue(size_t queCapa)
        : _queCapa(queCapa),
          _que(),
          _mutex(),
          _notEmpty(_mutex),
          _notFull(_mutex),
          _isRunning(true)
    {
    }
    bool empty()
    {
        return _que.size() == 0;
    }
    bool full()
    {
        return _que.size() == _queCapa;
    }

    //当判断 任务队列停止的时候,则增删任务会失败
    void push(Task task)
    {                                      //添加任务  这是生产者执行的线程函数   即使在不同线程中 这个类的成员函数任然可以访问这个类的其他成员
        MutexLockGuard autoUnlock(_mutex); //局部结束的时候 guard的析构函数被调用,此时这个锁会被自动解开
        while (full())                     //虚假唤醒 bug  一个资源缺口 唤醒两个生产者 产生奇怪的事情
        {
            _notFull.wait(); //如果是满的,则等待在notFull上,等待有消费者发送notFull的信号过来
            if (_isRunning == false)
            {
                return;
            }
        }
        _que.push(task); //能到这儿,说明任务没有满
        _notEmpty.notify();
    }
    Task pop() //有消费者线程来访问
    {
        MutexLockGuard autoUnlock(_mutex);
        while (empty()) //虚假唤醒 只有一个资源 同时唤醒两个消费者 如果使用if 那么第一次判断的时候 两个
        //消费者都以为有资源,然后各自去取资源,一个取到了,则资源没有了,此时另外一个线程将会出现未知问题
        //当被唤醒的时候,再次判断一下empty()  假设两个线程使用两个核,同时判断不为空,然后 同时去取资源??
        {
            _notEmpty.wait();
            if (_isRunning == false)
            {
                return nullptr;
            }
        }
        Task task = _que.front();
        _que.pop();
        _notFull.notify();
        return task;
    }
    void wakeAll()
    {
        _isRunning = false; //注意一下 ,这里得是 先设置成false,然后在notify 全部数据
        _notFull.notifyall();
        _notEmpty.notifyall();
    }

private:
    bool _isRunning; //这个标志位是用来退出的,最后需要在 pop 取任务的时候 判断一下 任务队列是不是关闭了
    size_t _queCapa;
    queue<Task> _que; //这个是唯一需要保护的临界资源,使用一把锁保护一个资源
    MutexLock _mutex;
    Condition _notEmpty;
    Condition _notFull;
};

//线程 /对象语义 noncopyable /析构虚函数 /pthreadFunc static 中调用run/run(pthread中是纯虚函数)中执行线程任务
//start() 开启线程 run() 线程任务  pthreadFunc() 调用run()  join()等待创建的子线程退出
//thid 记录线程id  isRunnin记录线程状态
namespace current_thread
{ //这个放在全局位置,那么在线程中,任何一个位置都可以直接使用,比较方便
    __thread const char *name;
}

class ThreadData //名字,回调函数 以及一个 运行在线程中的函数
{
public:
    ThreadData(const string &name, function<void()> cb)
        : _name(name), _cb(cb)
    {
    }
    void runInThread()
    {
        current_thread::name = (_name == string()) ? "no name" : _name.c_str();
        if (_cb)
        {
            _cb();
        }
    }
    string _name;
    function<void()> _cb;
};
class Thread
    : public Noncopyable
{
public:
    using ThreadCallBack = function<void()>;
    Thread(ThreadCallBack &&cb, const string &name = string())
        : _thid(0), _isRunning(false), _callback(cb), _name(name)
    {
    }
    virtual ~Thread() //需要将其设置为虚函数
    {
        if (_isRunning)
        {
            pthread_detach(_thid); //设置pthread_detach之后 子线程运行完成之后 自动回收所有资源 不用其他线程join等待
        }
    }

    void start()
    {
        ThreadData *pData = new ThreadData(_name, _callback); //这里new的一个对象 将线程名字和回调函数传进去了
        if (pthread_create(&_thid, nullptr, threadFunc, pData))
        {
            perror("pthread_create");
        }
        _isRunning = true;
    }
    static void *threadFunc(void *args) //这个是static的函数,其本身不能存储数据,所以需要
    {
        ThreadData *p = static_cast<ThreadData *>(args);
        if (p)
        {
            p->runInThread();
        }
    }
    void join()
    {
        if (_isRunning)
        {
            pthread_join(_thid, nullptr);
        }
        _isRunning = false;
        _thid = 0;
    }

private:
    pthread_t _thid;
    bool _isRunning;
    ThreadCallBack _callback;
    string _name;
};

//线程池
//任务队列 线程组 添加任务 执行任务  开启 关闭 整个线程池的开关 注意一下 开关的时候需要判断一下线程池当前状态
//调用析构函数的时候 如果没退出 去stop中等待所有任务 被拿走 join所有线程
class ThreadPool
    : Noncopyable
{
public:
    ThreadPool(size_t threadNum, size_t queCapa)
        : _threadNum(threadNum),
          _queCapa(queCapa),
          _taskQue(_queCapa),
          _isExit(false)
    {
        _threads.reserve(_threadNum); //对于vector,先预留空间,防止多次拷贝带来的性能损耗
    }
    void start() //先new一堆 WorkerThread,放到vector中,然后逐个开启线程
    {
        for (size_t i = _threadNum; i != 0; i--)
        {
            unique_ptr<Thread> tem(new Thread(bind(&ThreadPool ::threadFunc, this), to_string(i)));
            _threads.push_back(move(tem)); // unique_ptr 没有拷贝构造函数,但是有移动构造函数
        }
        for (auto &elem : _threads) //这里必须加上引用符号,unique不能被赋值(赋值运算函数delete了) 但是可以被引用
        {
            elem->start(); //开启线程
        }
    }
    void stop()
    {
        if (!_isExit)
        {
            //等待任务队列清空
            while (!_taskQue.empty())
            {
                sleep(1);
            }
            _isExit = true;
            _taskQue.wakeAll(); // 让所有阻塞在线程池的函数上的 线程都唤醒然后退出
            for (size_t i = 0; i != _threadNum; ++i)
            {
                _threads[i]->join(); //等待线程被join掉
            }
        }
    }
    void threadFunc() //这个是执行任务的线程来处理的
    {
        while (!_isExit)
        {
            Task task = _taskQue.pop();
            if (task) //如果收到nullptr,则立即退出
            {
                task(); //去执行任务中的函数
            }
            else
            {
                cout << "thread " << pthread_self() << " got a quit signal" << endl;
                return;
            }
        }
    }
    ~ThreadPool()
    {
        if (!_isExit) //如果还没退出,就调用 stop()函数来处理,这里会阻塞 等待 stop ,然后会返回
        {
            stop();
        }
        cout << "~ThreadPool" << endl;
    }
    void addTask(Task task)
    {
        _taskQue.push(task);
    }

private:
    //线程池 里面存储的,就是一个 任务队列(暂时存放任务) 线程数组 用于存放线程对象
    size_t _queCapa;
    TaskQueue _taskQue;
    size_t _threadNum;
    vector<unique_ptr<Thread>> _threads;
    bool _isExit;
};