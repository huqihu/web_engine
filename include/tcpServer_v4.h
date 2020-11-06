#pragma once
#include "bo_threadPool.h"
#include "Mylogger.h"
//维护一个fd  生成 ,关闭 获取 关闭读端 需要负责文件描述符套接字的关闭
class Socket
{
public:
    Socket()
    {
        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == _fd)
        {
            perror("socket");
        }
    }
    Socket(int fd) : _fd(fd) {}
    ~Socket()
    {
        close(_fd);
    }
    int fd()
    {
        return _fd;
    }
    void shutDownWrite()
    {
        ::shutdown(_fd, SHUT_WR);
    }

private:
    int _fd;
};

//保存一个 fd  可以读写 读写需要指定结束条件,以及一些读写条件的集合
class SocketIO
{
public:
    SocketIO(int fd)
        : _fd(fd) {}
    int writen(const char *buf, int len) //write需要考虑的情况有 返回值为-1 的情况
    {
        int left = len;
        const char *p = buf;
        int ret;
        while (left > 0)
        {
            ret = ::write(_fd, p, left); //注意一下 C语言中的函数 可以认为他们都是在匿名空间之中的
            if (-1 == ret && errno == EINTR)
            { //如果是中断引起的错误,则继续
                continue;
            }
            else if (-1 == ret)
            {
                perror("write");
                return len - left;
            } //注意 write 的时候,不会有返回值为0 这种情况,如果链接断开,第一次返回值是 -1 ,第二次会直接发生sigpipe
            else
            {
                left -= ret;
                p += ret; //注意需要将p这个指针进行偏移,
            }
        }
        return len - left; //到了这里 left = 0,返回的是全部的数据
    }
    int readn(char *buf, int len)
    {
        int left = len;
        char *p = buf;
        int ret;
        while (left > 0)
        {
            ret = ::read(_fd, p, left);      //这里的 大小left保证了每次读写不会超过范围
            if (-1 == ret && errno == EINTR) //如果是中断造成的
            {
                continue;
            }
            else if (-1 == ret) //非中断原因造成的错误
            {
                perror("read");
                return len - left;
            }
            else if (0 == ret) //对方断开了连接,read的时候会直接返回0
            {
                return len - left;
            }
            else //正常读取
            {
                left -= ret;
                p += ret;
            }
        }
    }
    int recvPeek(char *buf, int len) //最多读取指定长度的数据到缓冲区中去,但是不会将这些数据取出来
    {
        int ret = recv(_fd, buf, len, MSG_PEEK); //第四个参数可以设定一些属性,比如 msg_waitall等等
        while (-1 == ret && errno == EINTR)
        {
            ret = recv(_fd, buf, len, MSG_PEEK);
        }
        return ret;
    }
    int readLine(char *buf, int maxLen)
    { //遇到换行 或者 到达最大长度 就结束 返回获取的数据长度
        //如果没有找到  则需要在最后一个数据处加一个 结束符(无论找没找到 都要在后面加一结束符)
        int left = maxLen - 1; //最后结束符
        char *p = buf;         //p需要不停改变
        int total = 0;         //当前已经读取到的数目
        int ret;
        while (left > 0)
        {
            ret = recvPeek(p, left);
            for (int idx = 0; idx != ret; idx++)
            {
                if ('\n' == p[idx])
                {
                    int len = idx + 1;
                    readn(p, len); //前面使用的msg_peek不会将缓冲区的数据取出,所以需要主动使用readn将数据取出
                    left -= len;
                    total = +len;
                    p += len;
                    *(p - 1) = '\0';
                    return total - 1;
                }
            }
            left -= ret;
            p += ret;
            total += ret;
            readn(p, ret); //这里需要这么做的原因是 msg_peek只是读取了数据,但是没有将数据取走,所以需要使用readn将数据取走
        }
        *(p - 1) = '\0'; //添加一个结束符号  在这个位置上 添加,是因为 不想要结束符
        return total - 1;
    }

private:
    int _fd;
};

//inetaddr 储存一个网络地址 构造的时候 初始化 ,在后续使用的时候 提供ip/port/sockaddr三种访问方式
//需要注意的是网络字节序和 主机字节序的转换问题  网络字节序是大端模式 主机字节序都有可能
class InetAddress
{
public:
    InetAddress(const string &ip, const unsigned short port)
    {
        ::bzero(&_addr, sizeof(_addr));                //清空
        _addr.sin_family = AF_INET;                    //ip v4协议
        _addr.sin_addr.s_addr = inet_addr(ip.c_str()); //inet_addr的作用是将c风格字符串转换成网络字节序的ip地址
        _addr.sin_port = htons(port);                  //hotns  ===== host to net short
    }
    InetAddress(const unsigned short port) //端口号 0- 65535 使用一个unsigned short绝对够了
    {
        ::bzero(&_addr, sizeof(_addr));
        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(port);
        _addr.sin_addr.s_addr = INADDR_ANY; //INADDR_ANY这个宏代表的是本机 地址 0.0.0.0 代表的即使本机的地址
    }
    InetAddress(const struct sockaddr_in &addr) : _addr(addr) {}
    string ip()
    {
        return string(inet_ntoa(_addr.sin_addr)); //inet_ntoa的作用是 将一个in_addr格式的数据转换成c风格字符串
    }
    unsigned short port()
    {
        return ntohs(_addr.sin_port);
    }
    sockaddr_in *getInetAddrPtr() //这里弄一个返回 sockaddr_in指针的函数的目的是 后面的bind函数需要使用到
    {
        return &_addr;
    }

private:
    struct sockaddr_in _addr;
};

//accepter 其中维护的数据是一个 sock 以及一个 inetaddr结构体
// listen是在sock中自己生成的,inetaddr是要绑定的ip和port 是由 使用者传入的数据
// socket(设置 reuseaddr 服务器time_wait状态也可以正常工作 reuseport 同一物理主机 多个服务器程序运转 ) bind listen accept
class Accepter
{
public:
    Accepter(const unsigned short port)
        : _addr(port),
          _listenSock()
    {
    }
    Accepter(const string &ip, const unsigned short port)
        : _addr(ip, port),
          _listenSock()
    {
    }
    int fd()
    {
        return _listenSock.fd();
    }
    void ready() //直接使用这个来做初始化
    {
        setReuseAddr(true);
        setReusePort(true);
        //bind之前的socket这个生成套接字的步骤已经在Socket的构造函数中完成了
        bind();
        listen();
    }
    int accept()
    {
        int peerfd = ::accept(_listenSock.fd(), nullptr, nullptr); //peer意思是 对方的
        if (-1 == peerfd)
        {
            perror("accept");
        }
        return peerfd;
    }
    void setReuseAddr(bool on)
    {
        int set_value = on;
        int ret = setsockopt(_listenSock.fd(), SOL_SOCKET, SO_REUSEADDR, &set_value, sizeof(set_value));
        //setsockopt这个函数可以对某一个socket_fd进行设置
        //ret = 0 就是成功了,ret =-1就是失败了
        if (-1 == ret)
        {
            perror("setsockopt");
        }
    }
    void setReusePort(bool on)
    {
        int set_value = on;
        int ret = setsockopt(_listenSock.fd(), SOL_SOCKET, SO_REUSEPORT, &set_value, sizeof(set_value));
        if (-1 == ret)
        {
            perror("setsockopt");
        }
    }
    void bind()
    { //使用这个函数的时候需要注意 由于socket中有一个函数叫做bind ,因此在调用socket中的bind的时候
        //需要加上作用域限定符 ::  表示这个bind是匿名空间之中的函数,(那么一定不是 acceptr::bind 就会去调用socket中的bind)
        //命名空间 之间同样存在相互覆盖的情况,如果最内层的命名空间之中就有,那么就直接使用,调用其他命名空间中的数据 需要显示说明
        int ret = ::bind(_listenSock.fd(), (struct sockaddr *)_addr.getInetAddrPtr(), sizeof(_addr));
        if (-1 == ret)
        {
            perror("bind");
        }
    }
    void listen()
    {
        int ret = ::listen(_listenSock.fd(), 128);
        if (-1 == ret)
        {
            perror("listen");
        }
    }

private:
    InetAddress _addr;  //存放绑定的地址
    Socket _listenSock; //这个是监听套接字对象
};

//tcpconnection 一个tcp链接  五元组/ fd /连接的标志位/对外提供的一些操作
//构造函数 接受一个fd
//对外提供  recv send toString(合成一个链接情况的字符串,可以用来打印)  shutdown
//获取 自己以及对方的 ip和端口可以 使用getsockname getpeername 函数来完成

class TcpConnection;
using TcpConnectionPtr = shared_ptr<TcpConnection>;
using TcpConnectionCallback = function<void(const TcpConnectionPtr &tcpConn)>;

class EventLoop;

class TcpConnection //管理一个tcp链接的
    : public Noncopyable,
      public enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(int fd, EventLoop &eventLoop) //这里使用一个fd 来初始化 ,在一个程序中 fd是标示一个链接 最直接的方法
        : _socket(fd),
          _socketIO(fd),
          _localAddr(getLocalAddr(fd)),
          _peerAddr(getPeerAddr(fd)),
          _isShutDown(false),
          _eventLoop(eventLoop)
    {
    }
    ~TcpConnection()
    {
        shutDown();
    }
    int readn(char *buf, int len)
    {
        _socketIO.readn(buf, len);
    }
    int writen(const char *buf, int len)
    {
        _socketIO.writen(buf, len);
    }
    string receive()
    {
        char tem[65535] = {0};
        _socketIO.readLine(tem, 65535);
        return string(tem);
    }
    void send(const string &str)
    {
        _socketIO.writen(str.c_str(), str.size());
    }
    InetAddress getLocalAddr(int fd)
    {
        struct sockaddr_in tem;
        bzero(&tem, sizeof(tem));
        unsigned int len = sizeof(struct sockaddr);               // sockaddr 和sockaddr_in的大小是一样的
        int ret = getsockname(fd, (struct sockaddr *)&tem, &len); //获取本机的addr
        if (-1 == ret)
        {
            perror("getsockname");
        }
        return InetAddress(tem);
    }
    InetAddress getPeerAddr(int fd)
    {
        struct sockaddr_in tem;
        bzero(&tem, sizeof(tem));
        unsigned int len = sizeof(struct sockaddr);
        int ret = getpeername(_socket.fd(), (struct sockaddr *)&tem, &len);
        if (-1 == ret)
        {
            perror("getpeername");
        }
        return InetAddress(tem);
    }
    string toString()
    {
        ostringstream oss;
        oss << "local " << _localAddr.ip() << " " << _localAddr.port() << "   peer " << _peerAddr.ip() << " " << _peerAddr.port() << " ";
        return oss.str();
    }
    void shutDown()
    {
        if (!_isShutDown)
        {
            _isShutDown = true;
            _socket.shutDownWrite();
        }
    }
    bool isConnectionClosed()
    {
        char tem[2] = {0};
        int ret;
        do
        {
            ret = ::recv(_socket.fd(), tem, sizeof(tem), MSG_PEEK);
        } while (ret == -1 && errno == EINTR);
        return 0 == ret;
    }
    //创建对象之后 需要主动调用这几个函数 来设置 三个回调函数
    void setConnectionCallback(TcpConnectionCallback &rhs)
    {
        _onConnection = rhs;
    }
    void setMassageCallback(TcpConnectionCallback &rhs)
    {
        _onMassage = rhs;
    }
    void setCloseCallback(TcpConnectionCallback &rhs)
    {
        _onClose = rhs;
    }
    //在各自合适的时机 调用这个几个函数  被 reactor模型调用的
    void handleConnection()
    {
        if (_onConnection)
        {
            _onConnection(shared_from_this()); //调用这个函数的时候,需要传入一个 tcp对象的智能指针
        }
    }
    void handleMassage()
    {
        if (_onMassage)
        {
            _onMassage(shared_from_this());
        }
    }
    void handleClose()
    {
        if (_onClose)
        {
            _onClose(shared_from_this());
        }
    }
    int fd()
    {
        return _socket.fd();
    }
    //为什么  这个类中,需要使用addTask addResult 这两个函数?????
    // 因为添加,任务到线程池以及 添加结果到pendingList 都是通过传入函数对象的tcp 对象这两个函数来完成的
    //这里,没有直接在 设置的函数对象 中调用 eventLoop中的函数,是为了 eventLoop这个类的封装性
    // 所以 间接得通过 tcp 连接的对象来完成的
    void addtaskToThreadPool(function<void()> &&rhs);
    void addResultToPendingList(const string &rhs);

private:
    Socket _socket;
    SocketIO _socketIO;
    InetAddress _localAddr;
    InetAddress _peerAddr;
    bool _isShutDown;
    //三个回调函数 连接/消息/关闭
    TcpConnectionCallback _onConnection;
    TcpConnectionCallback _onMassage;
    TcpConnectionCallback _onClose;

    //其中需要一个 addToPending 函数以及一个 eventLoop & 数据成员
    EventLoop &_eventLoop;
};

class EventLoop //管理很多个tcp连接的,主要是使用了epoll_wait  所以需要accepter tcpconn,存储容器,epfd 以及函数对象 各种函数等等
                ///用来完成以前 用C语言写过的 网络多路复用IO的 各个功能
    : public Noncopyable
{
public:
    //接收一个accepter 创建 epfd 设置标志位 设置epoll结构体数组大小 将listen_fd加入到epoll中去
    EventLoop(Accepter &accepter, ThreadPool &threadPool)
        : _accepter(accepter), _isLooping(false), _eveList(128), _epfd(createEpollFd()), _eventFd(createEventFd()), _threadPool(threadPool)
    {
        addEpollFd(_accepter.fd());
        addEpollFd(_eventFd);
        //eventfd 使用来接受 线程池 的消息的,注意每次收到eventfd 就绪的时候 ,都需要将 其中的消息撤回来
        //注意下 所有文件描述符都要通过 一个epoll_wait来管理
    }
    void addTaskToThreadPool(function<void()> &&rhs)
    {
        _threadPool.addTask(move(rhs));
    }
    int createEventFd()
    {
        int fd = eventfd(0, 0); //第一个参数是 初始值,第二个参数是 标志位
        if (-1 == fd)
        {
            perror("eventfd");
        }
        return fd;
    }
    void setConnectionCallback(TcpConnectionCallback &&rhs)
    {
        _onConnection = rhs;
    }
    void setMassageCallback(TcpConnectionCallback &&rhs)
    {
        _onMassage = rhs;
    }
    void setCloseCallback(TcpConnectionCallback &&rhs)
    {
        _onClose = rhs;
    }
    void loop()
    {
        _isLooping = true;
        while (true == _isLooping)
        {
            singleEpollWait();
        }
    }
    void unloop()
    {
        _isLooping = false;
    }
    void singleEpollWait()
    {
        int readyNum = -1;
        do
        {
            readyNum = epoll_wait(_epfd, &*_eveList.begin(), _eveList.capacity(), -1);
        } while (-1 == readyNum && errno == EINTR); // 注意 epoll 被信号打断之后 就返回 -1
        if (-1 == readyNum)
        {
            perror("epoll_wait");
        }
        else if (0 == readyNum)
        {
            cout << "epoll_wait time out" << endl;
        }
        else
        { //先检测一下有没有到最大容量,如果到了 则 扩容
            if (readyNum == _eveList.capacity())
            {
                _eveList.resize(2 * readyNum); //resize 函数 会直接构建 这么多个 调用无参构造函数的对象
            }
            for (int idx = 0; idx != readyNum; ++idx)
            {
                LogDebug("epoll_wait active, cur_fd = %d", _eveList[idx].data.fd);
                if (_eveList[idx].data.fd == _eventFd && _eveList[idx].events == EPOLLIN)
                { //如果是 线程任务处理好 了
                    LogDebug("eventfd active,fd = %d", _eveList[idx].data.fd);
                    doReadEventFd();
                    doPendingFuncs();
                }
                else if (_eveList[idx].data.fd == _accepter.fd() && EPOLLIN == _eveList[idx].events)
                { //如果 新建连接 accpet 然后 创建tcp对象 (根据这个tcp链接的身份确认 给这个tcp的三个函数) 设置其三个回调函数
                    handleNewConnection();
                }
                else if (EPOLLIN == _eveList[idx].events) // 不是新建连接,也不是 eventfd ok了,那么就是有新的消息了
                {
                    LogDebug("new massage ,fd = %d", _eveList[idx].data.fd);
                    handleMassage(_eveList[idx].data.fd);
                }
            }
        }
    }
    //对于 eventfd 的读写,只要是 返回值 小于 sizeof(long) = 8 的,则说明是失败了
    void doWriteEventFd()
    {
        unsigned long tem = 1;
        int ret = ::write(_eventFd, &tem, sizeof(tem));
        if (sizeof(tem) != ret)
        {
            perror("doWriteEventFd");
        }
    }
    void addToPendingList(function<void()> &&rhs)
    {
        {
            MutexLockGuard autoUnlock(_mutex);
            _pendingFunc.push_back(move(rhs));
        }
        doWriteEventFd();
    }
    void doReadEventFd()
    { //读取eventfd ,使其清空
        unsigned long tem = 0;
        int ret = ::read(_eventFd, &tem, sizeof(tem));
        if (sizeof(tem) != ret) //如果发生错误,那么从eventFd中read到的数据长度会小于8
        {
            perror("doReadEventFd");
        }
    }
    void doPendingFuncs()
    {
        vector<function<void()>> tem;
        {
            MutexLockGuard autoUnlock(_mutex);
            tem.swap(_pendingFunc);       //这里采用swap ,其时间复杂度是 O(1) 效率很高
        }                                 //这是一个局部作用域 这个作用域结束之后,这个锁会被释放
        for (function<void()> elem : tem) // 将vector 中的任务取出来,直接调用然后执行就可以了,其本身就是一个函数对象
        {
            elem();
        }
    }
    void handleMassage(int fd)
    {
        LogDebug("fd = %d", fd);
        // // debug
        // cout << "disPlay _connList" << endl;
        // for (auto elem : _connList)
        // {
        //     cout << elem.second->toString() << endl;
        // }
        map<int, TcpConnectionPtr>::iterator iter = _connList.find(fd);
        if (iter == _connList.end())
        {
            perror("_connList.find()");
        }
        if (iter->second->isConnectionClosed() == true)
        {
            // 断开连接的时候先执行一下  tcp 中注册的函数,然后在 从 epollfd 以及 连接队列中,将这个tcp 连接删除
            iter->second->handleClose();
            delEpollFd(fd);
            _connList.erase(iter);
            //为什么 这里的 erase 不需要使用锁?? 应为 对于连接表的处理,都是有一个reactor线程来完成的,不需要考虑多线程
            // 另外,由于这里是 智能指针,所以 在erase 智能指针对象之后,前面new 出来的 tcp连接对象会被自动释放掉
        }
        else
        {
            iter->second->handleMassage();
        }
    }

    void delEpollFd(int fd)
    {
        int ret = epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, nullptr);
        if (ret == -1)
        {
            perror("epoll_ctl");
        }
    }
    void handleNewConnection()
    {
        //新链接 加入 epoll , new 一个tcp对象 设置 其几个回调函数
        //然后将其shared_ptr插入到数组中去
        //顺便调用一下 连接的时候的处理函数
        int tem_fd = _accepter.accept();
        if (-1 == tem_fd)
        {
            perror("_accepter.accept()");
            return;
        }
        LogDebug("new fd= %d", tem_fd);
        addEpollFd(tem_fd);
        TcpConnectionPtr tem_ptr(new TcpConnection(tem_fd, *this));
        tem_ptr->setConnectionCallback(_onConnection);
        tem_ptr->setMassageCallback(_onMassage);
        tem_ptr->setCloseCallback(_onClose);

        _connList.insert(pair<int, TcpConnectionPtr>(tem_fd, tem_ptr));
        //connlist完全是由 reactor 线程来管理的,所以不需要 考虑多线程的问题

        tem_ptr->handleConnection();
    }
    int createEpollFd()
    {
        int fd = epoll_create1(0);
        if (-1 == fd)
        {
            perror("epol_create");
        }
        return fd;
    }
    void addEpollFd(int fd)
    {
        struct epoll_event event;
        bzero(&event, sizeof(event));
        event.data.fd = fd;
        event.events = EPOLLIN;
        int ret = epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &event);
        if (-1 == ret)
        {
            perror("epoll_ctl");
        }
    }
    void addtaskToThreadPool(function<void()> &&rhs)
    {
        _threadPool.addTask(rhs);
    }

private:
    Accepter &_accepter;
    int _epfd;
    vector<struct epoll_event> _eveList;
    map<int, TcpConnectionPtr> _connList;
    bool _isLooping;
    //再次注意:不同的TcpConn对象可能会有 不同的功能,所以这里可以提供多种 函数对象,用来给不同类型的TCP用户
    TcpConnectionCallback _onConnection;
    TcpConnectionCallback _onMassage;
    TcpConnectionCallback _onClose;

    int _eventFd;
    vector<function<void()>> _pendingFunc;
    MutexLock _mutex; //这个锁 是用来保护 _pendingFunc的多线程读写安全的
    ThreadPool &_threadPool;
};

void TcpConnection::addtaskToThreadPool(function<void()> &&rhs)
{
    _eventLoop.addTaskToThreadPool(move(rhs));
}
void TcpConnection::addResultToPendingList(const string &rhs)
{
    _eventLoop.addToPendingList(bind(&TcpConnection::send, this, rhs));
}

//封装 eventLoop 和accepter
class TcpServer
{
public:
    TcpServer(unsigned short port, int threadNum, int queCapa)
        : _accepter(port),
          _threadPool(threadNum, queCapa),
          _eveLoop(_accepter, _threadPool)
    {
        _accepter.ready();
        _threadPool.start();
    }
    void start()
    {
        _eveLoop.loop();
    }
    void setConnectionCallback(TcpConnectionCallback &&rhs)
    {
        _eveLoop.setConnectionCallback(move(rhs));
    }
    void setMassageCallback(TcpConnectionCallback &&rhs)
    {
        _eveLoop.setMassageCallback(move(rhs));
    }
    void setCloseCallback(TcpConnectionCallback &&rhs)
    {
        _eveLoop.setCloseCallback(move(rhs));
    }
    void stop()
    {
        _eveLoop.unloop();
    }

private:
    Accepter _accepter;
    ThreadPool _threadPool;
    EventLoop _eveLoop;
};
