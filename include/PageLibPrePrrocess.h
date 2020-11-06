#pragma once
#include "myhead.h"
#include "Mylogger.h"
#include "SplitTool.h"
#include "DirScanner.h"
#include "SplitTool.h"
#include "tinyxml2.h"

#include <cmath>
using namespace tinyxml2;
class WebPage
{
    friend bool operator==(const WebPage &lhs, const WebPage &rhs);

public:
    // WebPage(int docId, const string &docTitle,
    //         const string &docUrl, const string &docSummary,
    //         const string &docContent, vector<string> topWords, map<string, int> wordsMap)
    //     : _docId(docId), _docTitle(docTitle), _docUrl(docUrl), _docSummary(docSummary), _docContent(docContent), _topWords(topWords), _wordsMap(wordsMap)
    // {
    // }
    WebPage() {}
    int getId()
    {
        return _docId;
    }
    void changeId(int newId)
    {
        _docId = newId;
    }
    int _docId;
    string _docTitle;
    string _docUrl;
    string _docSummary;
    string _docContent;
    vector<pair<string, int>> _topWords;
    map<string, int> _wordsMap;
    int _topKeysNumber = 10;
};
bool operator==(const WebPage &lhs, const WebPage &rhs)
{
    vector<string> sameWordList;
    int cnt = 0;
    for (auto l_elem : lhs._topWords)
    {
        for (auto r_elem : rhs._topWords)
        {
            if (l_elem.first == r_elem.first)
            {
                cnt++;
                sameWordList.push_back(l_elem.first);
            }
        }
    }

    if (2.5 * cnt / lhs._topKeysNumber >= 1) // 超过 40% 的相同 热词 ,就认为是同一个 网页
    {
        cout << "----------------------------------------------------" << endl;
        for (auto elem : lhs._topWords)
        {
            cout << elem << " ";
        }
        cout << endl;
        for (auto elem : rhs._topWords)
        {
            cout << elem << " ";
        }
        cout << endl;
        cout << "same top words count:" << cnt << endl;
        for (auto elem : sameWordList)
        {
            cout << elem << " ";
        }
        return true;
    }
    else
    {
        return false;
    }
}
class PageLibPreProcess
{
public:
    PageLibPreProcess(const string &weblibDir, const string &ripePageFile, const string &offsetFile, const string &stopWordFile, const string &invertIndexFile)
        : _weblibDir(weblibDir), _ripePageFile(ripePageFile), _offsetFile(offsetFile), _stopWordFile(stopWordFile), _spliter(), _dirScanner(), _invertIndexFile(invertIndexFile)
    {
        //读取文件集合
        _files = _dirScanner.getFilesOfDir(_weblibDir);
        _pageNums = 0;
    }
    void showWebPages();
    void doProcess();
    void loadStopWord();
    void loadWeblib();
    void createWebPage(const string &file);
    void getTopWords();
    void insertIntoDict(vector<string> &, WebPage &);
    void removeDupWebpage();
    void writeWebPageToFile();
    void saveOffset();
    void buildInvertIndex();
    void saveInvertIndex();

private:
    string _weblibDir;
    string _ripePageFile;
    string _offsetFile;
    string _stopWordFile;
    string _invertIndexFile;
    vector<string> _files;
    list<WebPage> _webPageList;
    SplitTool _spliter;
    DirScanner _dirScanner;
    unordered_map<int, pair<size_t, size_t>> _offsetLib;       // 网页偏移库 ,偏离量是 size_t,然后保存一个网页的内容
    unordered_map<string, map<int, double>> _invertIndexTable; // 倒排库 从单词索引到 相关的网页以及这个单词再在这个网页中的占比
    unordered_set<string> _stopWordSet;
    int _pageNums;
};
void PageLibPreProcess::saveInvertIndex()
{
    ofstream ofs(_invertIndexFile);
    if (!ofs.good())
    {
        LogError("failed to open %s", _invertIndexFile.c_str());
        return;
    }
    for (auto &elem : _invertIndexTable)
    {
        ofs << elem.first << " ";
        for (auto val : elem.second)
        {
            ofs << val.first << " " << val.second << " ";
        }
        ofs << endl;
    }
    ofs.close();
}
void PageLibPreProcess::buildInvertIndex()
{
    // 计算df
    cout << "before calcute df" << endl;
    map<string, double> idf;
    for (auto &webPage : _webPageList)
    {
        for (auto &elem : webPage._wordsMap)
        {
            if (idf.count(elem.first))
            {
                idf[elem.first] += 1;
            }
            else
            {
                idf.insert(make_pair(elem.first, 1));
            }
        }
    }
    //计算 idf
    cout << "before calcute idf file_nums =" << _pageNums << endl;
    for (auto &elem : idf)
    {
        elem.second = log(_pageNums / (elem.second + 1)) / log(2);
    }
    //计算 权重 ,然后插入到倒排索引表之中
    cout << "before calculate weight" << endl;
    for (auto &webPage : _webPageList)
    {

        map<string, double> keyWeight;
        double idf_value;
        int tf_value;
        double weight;
        // 计算最初始的 weight
        for (auto &elem : webPage._wordsMap)
        {
            assert(idf.count(elem.first) != 0);
            idf_value = idf[elem.first];
            weight = idf_value * elem.second;
            keyWeight.insert(make_pair(elem.first, weight));
        }
        // 计算 所有权重的 平方和 之后对其开方
        double sum_of_square = 0;
        for (auto &elem : keyWeight)
        {
            sum_of_square += (elem.second) * (elem.second);
        }
        double sqrt_value = sqrt(sum_of_square);
        // 重新计算权重 ,并且插入到排序索引中去

        for (auto &elem : keyWeight)
        {
            double tem_weight = elem.second / sum_of_square;
            if (_invertIndexTable.count(elem.first)) // 如果不是第一次出现,则直接插入
            {
                _invertIndexTable[elem.first].insert(make_pair(webPage._docId, tem_weight));
            }
            else // 第一次出现
            {
                _invertIndexTable.insert(make_pair(elem.first, map<int, double>()));
                _invertIndexTable[elem.first].insert(make_pair(webPage._docId, tem_weight));
            }
        }
    }
}
void PageLibPreProcess::saveOffset()
{
    ofstream ofs(_offsetFile);
    for (auto elem : _offsetLib)
    {
        ofs << elem.first << " " << elem.second.first << " " << elem.second.second << endl;
    }
    ofs.close();
}
void PageLibPreProcess::writeWebPageToFile()
{
    ofstream ofs(_ripePageFile);
    if (!ofs.good())
    {
        LogError("failed to open %s", _ripePageFile.c_str());
        return;
    }
    size_t begin_offset;
    size_t end_offset;
    size_t pageSize;
    for (auto &webPage : _webPageList)
    {
        begin_offset = ofs.tellp();
        ofs << webPage._docId << endl;
        ofs << webPage._docUrl << endl;
        ofs << webPage._docTitle << endl;
        ofs << webPage._docSummary << endl;
        ofs << webPage._docContent << endl;
        end_offset = ofs.tellp();
        pageSize = end_offset - begin_offset;
        _offsetLib.insert(make_pair(webPage._docId, make_pair(begin_offset, pageSize))); // 保存偏移 和 网页大小
    }
    ofs.close();
}
void PageLibPreProcess::removeDupWebpage()
{
    // list 成员函数 unique 可以出去连续重复的,需要提前sort ,webPage 没办法sort ,需要自己写去重的代码
    //注意迭代器失效问题
    int preSize = _webPageList.size();

    for (auto l_iter = _webPageList.begin(); l_iter != _webPageList.end(); ++l_iter)
    {
        auto tem_iter = l_iter;
        for (auto r_iter = ++tem_iter; r_iter != _webPageList.end();)
        {
            if ((*l_iter) == (*r_iter))
            {
                r_iter = _webPageList.erase(r_iter);
            }
            else
            {
                r_iter++;
            }
        }
    }
    cout << endl
         << "----------------------------------------------------" << endl;
    cout << "webPage nums before de_dup: " << preSize << " after de_dup: " << _webPageList.size() << endl;
    // 对网页进行 编号 ,然后将这些网页存入到 磁盘中 ,并且记录下来这些网页 的偏移量
    int docId = 0;
    for (auto &webPage : _webPageList)
    {
        webPage._docId = docId;
        docId++;
    }
    _pageNums = docId;
}
void PageLibPreProcess::loadStopWord()
{
    ifstream ifs(_stopWordFile);
    if (!ifs.good())
    {
        LogError("failed to open %s ", _stopWordFile.c_str());
        return;
    }
    string word;
    while (ifs >> word)
    {
        _stopWordSet.insert(word);
    }
    ifs.close();
    //需要 注意的 是 加载 停用词的时候,用 ifs 来读取的时候,不会将 空格 换行 制表符 读进来,所以需要 在 停用词集合里面insert 一下
    _stopWordSet.insert(" ");
    _stopWordSet.insert("\n");
    _stopWordSet.insert("   ");
}
void PageLibPreProcess::getTopWords()
{
    for (auto &webPage : _webPageList)
    {
        // 将标题 摘要 全文内容 都统计到词典中
        vector<string> wordList;
        wordList = _spliter.cut(webPage._docTitle);
        insertIntoDict(wordList, webPage);
        wordList = _spliter.cut(webPage._docSummary);
        insertIntoDict(wordList, webPage);
        wordList = _spliter.cut(webPage._docContent);
        insertIntoDict(wordList, webPage);
        // 开始 抽取 词频最高的几个词
        multimap<int, string, std::greater<int>> wordFreqList;
        for (auto elem : webPage._wordsMap)
        {
            wordFreqList.insert(make_pair(elem.second, elem.first));
        }
        // 将词频最高的几个词,插入到 topWords
        auto iter = wordFreqList.begin();
        for (int idx = 0; idx != webPage._topKeysNumber; ++idx, ++iter)
        {
            webPage._topWords.push_back(make_pair(iter->second, iter->first));
        }
    }
}
void PageLibPreProcess::insertIntoDict(vector<string> &vec, WebPage &webPage)
{
    for (auto elem : vec)
    {
        if (_stopWordSet.count(elem) == 0) //如果不在在停用词集合中,才处理 停用词则 直接无视
        {
            if (webPage._wordsMap.count(elem) == 0) // 没有 则插入单词
            {
                webPage._wordsMap.insert(make_pair(elem, 1));
            }
            else // 有则 加一
            {
                webPage._wordsMap[elem] += 1;
            }
        }
    }
}
void PageLibPreProcess::showWebPages()
{
    for (auto &elem : _webPageList)
    {
        cout << "id " << endl
             << elem._docId << endl;
        cout << "url " << endl
             << elem._docUrl << endl;
        cout << "title " << endl
             << elem._docTitle << endl;
        cout << "summary " << endl
             << elem._docSummary << endl;
        cout << "content " << endl
             << elem._docContent << endl;
        cout << "topWords" << endl;
        for (auto &val : elem._topWords)
        {
            cout << " " << val.first << " " << val.second << endl;
        }
    }
}
void PageLibPreProcess::doProcess()
{
    loadStopWord();
    loadWeblib();
    getTopWords();
    removeDupWebpage();
    //writeWebPageToFile();
    //saveOffset();
    buildInvertIndex();
    //saveInvertIndex();
}
void PageLibPreProcess::loadWeblib()
{
    for (auto file : _files)
    {
        createWebPage(file);
    }
}
void PageLibPreProcess::createWebPage(const string &file_name)
{
    static int file_cnt = 0;
    file_cnt++;
    cout << "file cnt " << file_cnt << endl;
    cout << "file " << file_name << endl;
    char *pFile = realpath(file_name.c_str(), nullptr);
    XMLDocument doc;
    if (doc.LoadFile(file_name.c_str()))
    {
        doc.PrintError();
        return;
    }
    XMLElement *root = doc.RootElement();
    XMLElement *channel = root->FirstChildElement("channel");
    XMLElement *item;
    if (channel)
    {
        item = channel->FirstChildElement("item");
    }
    else
    {
        item = nullptr;
    }

    string tem;
    while (item)
    { //每次都读取下一个item,为空指针说明读取完成
        //每一个item下面有一个title link description content ,下面不需要遍历
        // 每次插入之前, 先将webListPage 中增加一个元素 ,然后插入的时候 ,插入到尾部
        //printf("item = %p", item);
        _webPageList.push_back(WebPage());
        WebPage &rss_elem = _webPageList.back();
        rss_elem._docUrl = pFile; // url的地方使用的是 文件的名字

        XMLElement *title = item->FirstChildElement("title");
        if (title)
        {
            printf("%p \n",title);
            rss_elem._docTitle = title->GetText();
            cout<<"title = "<<rss_elem._docTitle<<endl;
        }

        XMLElement *description = item->FirstChildElement("description");
        if (description)
        {
            tem = description->GetText();
            tem = regex_replace(tem, regex("<[^>]*>"), ""); //正则表达式替换掉没用的信息
            rss_elem._docSummary = tem;
        }

        XMLElement *content = item->FirstChildElement("content:encoded");
        if (content)
        {
            tem = content->GetText();
            tem = regex_replace(tem, regex("<[^>]*>"), "");
            rss_elem._docContent = tem;
        }

        item = item->NextSiblingElement("item"); //查找下一个item
    }
    //这个文件读取完成之后结束
}