#pragma once
#include "../../include/tcpServer_v4.h"
#include "../../include/Mylogger.h"
#include "../../include/Config.h"
#include "../../include/PageLibPrePrrocess.h"
#include <json/json.h>

//using namespace Json;
//连接 断开的时候打印信息, 连接 有消息的时候 回复一个消息
class MyTask
{
public:
    MyTask(const string &msg, const TcpConnectionPtr &tcpPtr)
        : _msg(msg), _conn(tcpPtr)
    {
    }
    void process();
private:
    string _msg;
    TcpConnectionPtr _conn;
};

void MyTask::process()
{
    LogDebug("threadName: %s", current_thread::name);
    string queryWord = _msg;
    _conn->addResultToPendingList();
}

// 连接 消息 关闭连接时候的 情况
void onConnection(const TcpConnectionPtr &rhs)
{
    LogDebug("new connection fd= %d,connection : %s", rhs->fd(), rhs->toString().c_str());
    rhs->send("hello");
    // 刚刚连接上,可以在里面做一些 密码验证,等等任务
}
void onMassage(const TcpConnectionPtr &rhs)
{
    int len;
    rhs->readn((char *)&len, (int)sizeof(len));
    if (len > 4096)
    {
        LogError("recvBuf overflow");
        return ;
    }
    char recvBuf[65535];
    rhs->readn(recvBuf, len);
    Json::Value val;
    Json::Reader reader;

    int id;
    string queryWord;
    if (reader.parse(recvBuf, val))
    {
        id = val["ID"].asInt();
        queryWord = val["queryWord"].asString();
    }
    else
    {
        LogError("failed to recv json");
        return;
    }
    MyTask task(queryWord, rhs);
    rhs->addtaskToThreadPool(bind(&MyTask::process, task));
    LogDebug("id: %d massage: **%s** , fd= %d,connection : %s", id, queryWord.c_str(), rhs->fd(), rhs->toString().c_str());
}
void onClose(const TcpConnectionPtr &rhs)
{
    LogDebug("lost connection fd= %d,connection : %s", rhs->fd(), rhs->toString().c_str());
}