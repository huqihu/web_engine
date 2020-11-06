#include "myhead.h"
#include "Mylogger.h"
class MyResult
{
public:
    MyResult(const string &str, int freq, int dist)
        : _dist(dist), _freq(freq), _word(str)
    {
    }
    int _dist;
    int _freq;
    string _word;
};

namespace std
{
    template <>
    class greater<MyResult>
    {
    public:
        // 当满足 左边 less than 右边的时候 ,返回 true
        // 依次 比较最小编辑距离 词频 词典顺序
        bool operator()(const MyResult &lhs, const MyResult &rhs)
        {
            if (lhs._dist > rhs._dist)
            {
                return true;
            }
            else if (lhs._dist == rhs._dist)
            {
                if (lhs._freq < rhs._freq)
                {
                    return true;
                }
                else if (lhs._freq == rhs._freq)
                {
                    if (lhs._word > rhs._word)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
    };
} // namespace std