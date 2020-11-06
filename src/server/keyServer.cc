#include "../../include/tcpServer_v4.h"
#include "../../include/Mylogger.h"
#include "../../include/Config.h"
#include "../../include/MyDict.h"
#include "../../include/MyTask.h"
#include "../../include/Timer.h"
#include "../../include/Cache.h"
int main()
{
    // 加载配置
    Config conf;
    //设置 日志记录器
    Mylogger::getInstance()->setPriority(log4cpp::Priority::DEBUG);
    Mylogger::getInstance()->addFileAppender(conf.getConfig("logfile"));
    // 读取词典 以及 索引
    MyDict::getDictInstance()->initMyDict(conf.getConfig("en_dictfile"), conf.getConfig("en_indexfile"));
    //MyDict::getDictInstance()->saveTestDict(); // 测试 是否正确读取
    // 服务器
    TcpServer server(5555, 10, 20);
    server.setConnectionCallback(onConnection);
    server.setMassageCallback(onMassage);
    server.setCloseCallback(onClose);

    // 创建定时器线程 ,添加 定时更新缓存的任务
    TimerManager tm(30);                                                          // 参数是槽数
    CacheManager::getCacheInstance()->initCache(10, conf.getConfig("cachefile")); // 缓存的数量 ,与 线程数量相对应
    CacheTimerTask cacheTask(5, CacheManager::getCacheInstance());                //全局缓存 跟新时间
    tm.addTask(&cacheTask);
    Thread timerTh(std::bind(&TimerManager::start, &tm));
    timerTh.start();
    server.start();
}
