#pragma once
#include "myhead.h"
#include "Mylogger.h"
#include "SplitTool.h"

struct Result
{
    Result(int id, set<pair<string, double>> &resSet, double sim)
        : _docId(id), _keyWeight(resSet), _similarity(sim)
    {
    }
    int _docId;
    set<pair<string, double>> _keyWeight;
    double _similarity;
};
namespace std
{
    template <>
    class less<Result>
    {
    public:
        bool operator()(const Result &lhs, const Result &rhs)
        {
            return lhs._similarity < rhs._similarity;
        }
    };
} // namespace std

class MyWeb
{
public:
    MyWeb(const string &ripePageFile, const string &offsetFile, const string &stopWordFile, const string &invertIndexFile)
        : _ripePageFile(ripePageFile), _offsetFile(offsetFile), _stopWordFile(stopWordFile), _invertIndexFile(invertIndexFile), _spliter()
    {
        
    }
    void loadInvertIndex();
    vector<string> searchWebPages(const string &sentence);

private:
    string _ripePageFile;
    string _offsetFile;
    string _stopWordFile;
    string _invertIndexFile;
    unordered_map<int, pair<size_t, size_t>> _offsetLib;       // 网页偏移库 ,偏离量是 size_t,然后保存一个网页的内容
    unordered_map<string, map<int, double>> _invertIndexTable; // 倒排库 从单词索引到 相关的网页以及这个单词再在这个网页中的占比
    unordered_set<string> _stopWordSet;
    SplitTool _spliter;
    //static MyWeb * _pSelf;
};
vector<string> MyWeb::searchWebPages(const string &sentence)
{
    vector<string> keyWordList = _spliter.cut(sentence);

    for (auto iter = keyWordList.begin(); iter != keyWordList.end();)
    {
        if (_stopWordSet.count(*iter) > 0)
        {
            iter = keyWordList.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    cout << "--------------------------------------------------------" << endl;
    cout << "cut result" << endl;
    for (auto elem : keyWordList)
    {
        cout << " *" << elem << "* " ;
    }
    cout<<endl;
    // 计算 权重
    map<string, int> query_dict;
    for (auto &elem : keyWordList)
    {
        if (query_dict.count(elem) > 0)
        {
            query_dict[elem] += 1;
        }
        else
        {
            query_dict.insert(make_pair(elem, 1));
        }
    }
    //
    cout << "word frequency(base): " << endl;
    for (auto elem : query_dict)
    {
        cout << "* " << elem.first << " : " << elem.second << " *";
    }
    cout<<endl;
    set<int> pagesList;
    if (_invertIndexTable.count(query_dict.begin()->first))
    {
        for (auto &elem : _invertIndexTable[query_dict.begin()->first])
        {
            pagesList.insert(elem.first);
        }
    }
    else // 第一个单词就没有找到 则为空
    {
        return vector<string>();
    }
    for (auto one_word : query_dict)
    {
        if (_invertIndexTable.count(one_word.first))
        {
            set<int> tem_set;
            for (auto elem : _invertIndexTable[one_word.first])
            {
                tem_set.insert(elem.first);
            }
            vector<int> tem_page_set;
            set_intersection(pagesList.begin(), pagesList.end(), tem_set.begin(), tem_set.end(), back_inserter(tem_page_set));
            pagesList.clear();
            pagesList.insert(tem_page_set.begin(), tem_page_set.end());
            cout << "set size " << pagesList.size() << endl;
        }
        else
        { // 任何一个 分词 没有找到 就立即返回
            return vector<string>();
        }
    }
    cout << " ****final set size = " << pagesList.size() << endl;
    // for (auto elem : pagesList)
    // {
    //     cout << "page id = " << elem << endl;
    // }
    // 对于 set<int>中的内容,查找所有的单词 以及权重,保存在webPageList
    map<int, set<pair<string, double>>> webPageList;
    for (auto &elem : pagesList)
    {
        webPageList.insert(make_pair(elem, set<pair<string, double>>()));
        for (auto &one_word : query_dict)
        {
            for (auto &result : _invertIndexTable[one_word.first])
            {
                if (result.first == elem)
                {
                    webPageList[elem].insert(make_pair(one_word.first, result.second));
                }
            }
        }
    }
    // 展示一下结果
    for (auto elem : webPageList)
    {
        cout << "pageid = " << elem.first <<" word frequency "<< endl;
        for (auto val : elem.second)
        {
            cout << "* " << val.first << ":" << val.second << " *";
        }
        cout<<endl;
    }
    // 余弦相似度算法排序
    priority_queue<Result> resList;
    for (auto elem : webPageList)
    {
        double x_y = 0, x_x = 0, y_y = 0;
        auto r_iter = elem.second.begin();
        for (auto l_iter = query_dict.begin(); l_iter != query_dict.end(); ++l_iter, ++r_iter)
        {
            x_y += (l_iter->second) * (r_iter->second);
            x_x += (l_iter->second) * (l_iter->second);
            y_y += (r_iter->second) * (r_iter->second);
        }
        double sim = x_y / (sqrt(x_x) * sqrt(y_y));
        resList.push(Result(elem.first, elem.second, sim));
    }
    // 显示 pageid 和 相似度
    vector<string> _webSet;
    ifstream ifs(_ripePageFile);
    if (!ifs.good())
    {
        LogError("failed to open %s", _ripePageFile.c_str());
    }
    int tem_size = resList.size();
    for (int idx = 0; idx != tem_size; idx++)
    {
        cout << "pageid = " << resList.top()._docId << " sim = " << resList.top()._similarity << endl;

        size_t offset = _offsetLib[resList.top()._docId].first;
        size_t len = _offsetLib[resList.top()._docId].second;
        ostringstream oss;
        string line;
        ifs.seekg((streampos)offset);
        int line_cnt = 0;
        int char_cnt = 0;
        while (getline(ifs, line))
        {
            char_cnt += line.size();
            line_cnt++;
            if (char_cnt <= len && line_cnt <= 10) // 获取网页的10行 ,或者达到1000
            {
                oss << line;
                oss << endl;
            }
            else
            {
                break;
            }
        }
        _webSet.push_back(oss.str());
        resList.pop();
    }
    ifs.close();
    for (auto elem : _webSet)
    {
        cout << elem << endl;
    }
    cout<<"_webSet size = "<<_webSet.size()<<endl;
    return _webSet;
}

void MyWeb::loadInvertIndex()
{
    
    // 先读取 offset
    ifstream ifs(_offsetFile);
    if (!ifs.good())
    {
        LogError("failed to open %s", _offsetFile.c_str());
        return;
    }
    string line;
    while (getline(ifs, line))
    {
        istringstream iss(line);
        int docId;
        size_t offset;
        size_t len;
        iss >> docId >> offset >> len;
        _offsetLib.insert(make_pair(docId, make_pair(offset, len)));
    }
    ifs.close();
    // 读取 倒排索引文件
    ifs.open(_invertIndexFile);
    if (!ifs.good())
    {
        LogError("failed to open %s", _invertIndexFile.c_str());
        return;
    }
    while (getline(ifs, line))
    {
        istringstream iss(line);
        string word;
        int docId;
        double weight;
        iss >> word;
        _invertIndexTable.insert(make_pair(word, map<int, double>()));
        while (iss >> docId >> weight)
        {
            _invertIndexTable[word].insert(make_pair(docId, weight));
        }
    }
    ifs.close();
    // 读取停用词
    ifs.open(_stopWordFile);
    if (!ifs.good())
    {
        LogError("failed to open %s", _invertIndexFile.c_str());
        return;
    }
    string word;
    while (ifs >> word)
    {
        _stopWordSet.insert(word);
    }
    ifs.close();
    _stopWordSet.insert(" ");
    _stopWordSet.insert("\n");
    _stopWordSet.insert("   ");
}
