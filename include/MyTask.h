#include "tcpServer_v4.h"
#include "Mylogger.h"
#include "Config.h"
#include "MyDict.h"
#include "MyResult.h"
#include "protocol.h"
#include "Cache.h"
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
    void statistic(int index);
    int distance(const string &str1, const string &str2);

private:
    string _msg;
    TcpConnectionPtr _conn;
    priority_queue<MyResult, vector<MyResult>, std::greater<MyResult>> _resultQue;
};

void MyTask::process()
{
    LogDebug("threadName: %s", current_thread::name);
    // 先 查找缓存,如果缓存中有,则直接获取,如果没有,则搜索,并且加入缓存
    string cacheResult = GETCACHE(atoi(current_thread::name)).getValue(_msg);
    if (cacheResult != string())
    {
        _conn->addResultToPendingList(cacheResult);
        LogDebug("get result of %s from cache", _msg.c_str());
        return;
    }
    string queryWord = _msg;
    cout << "queryWord*" << _msg << "*" << endl;
    //对搜索词 去重,注意 : 这个只是为了 减少 添加索引时候 的无效操作
    sort(queryWord.begin(), queryWord.end());
    auto end_iter = unique(queryWord.begin(), queryWord.end());
    queryWord.erase(end_iter, queryWord.end());
    // 开始 添加索引
    set<int> tem_set;
    for (auto ch : queryWord)
    {
        tem_set.insert(GETINDEX[ch].begin(), GETINDEX[ch].end());
    }
    //索引 添加 完毕,开始 搜索 单词
    for (auto elem : tem_set)
    {
        statistic(elem); //这个函数 会将搜索之后的 单词 放到 优先级队列之中
    }
    // 将搜索结果拼接成一个字符串(这么做是为了方便)
    ostringstream oss;
    for (int index = 0; index != 10 && index < _resultQue.size(); ++index)
    {
        oss << _resultQue.top()._word << "|" << _resultQue.top()._dist << "|" << _resultQue.top()._freq << " ";
        _resultQue.pop();
    }
    if (0 == _resultQue.size()) // 如果没有搜索 ,也要发送一些东西过去
    {
        oss << "no result" << endl; 
    }
    // 将结果加入到 pendingList, 也可以生成一个
    LogDebug("get result of %s from index", _msg.c_str());
    string result = oss.str();
    // 将结果添加 到缓存之中
    GETCACHE(atoi(current_thread::name)).addElement(_msg, result);
    _conn->addResultToPendingList(result);
}

void MyTask::statistic(int index)
{
    string word = GETDICT[index].first;
    int freq = GETDICT[index].second;
    int dist = distance(_msg, word);
    _resultQue.push(MyResult(word, freq, dist));
}

// 动态规划,计算 两个字符串之间的距离
int MyTask::distance(const string &str1, const string &str2)
{
    if (str1 == str2)
    {
        return 0;
    }
    int max1 = str1.size();
    int max2 = str2.size();
    int **ptr = new int *[max1 + 1];
    for (int i = 0; i < max1 + 1; i++)
    {
        ptr[i] = new int[max2 + 1];
    }
    for (int i = 0; i < max1 + 1; i++)
    {
        ptr[i][0] = i;
    }
    for (int i = 0; i < max2 + 1; i++)
    {
        ptr[0][i] = i;
    }
    for (int i = 1; i < max1 + 1; i++)
    {
        for (int j = 1; j < max2 + 1; j++)
        {
            int d;
            int temp = min(ptr[i - 1][j] + 1, ptr[i][j - 1] + 1);
            if (str1[i - 1] == str2[j - 1])
            {
                d = 0;
            }
            else
            {
                d = 1;
            }
            ptr[i][j] = min(temp, ptr[i - 1][j - 1] + d);
        }
    }

    int dis = ptr[max1][max2];
    for (int i = 0; i < max1 + 1; i++)
    {
        delete[] ptr[i];
        ptr[i] = NULL;
    }
    delete[] ptr;
    ptr = NULL;
    return dis;
}

//
//
//
//
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
    if (REQUEST_KEY_WORDS == id)
    {
        MyTask task(queryWord, rhs);
        rhs->addtaskToThreadPool(bind(&MyTask::process, task));
        LogDebug("id: %d massage: **%s** , fd= %d,connection : %s", id, queryWord.c_str(), rhs->fd(), rhs->toString().c_str());
    }
}
void onClose(const TcpConnectionPtr &rhs)
{
    LogDebug("lost connection fd= %d,connection : %s", rhs->fd(), rhs->toString().c_str());
}