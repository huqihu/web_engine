#pragma once
#include "myhead.h"
#include "Mylogger.h"
#include "SplitTool.h"
#include "DirScanner.h"
// 这个类 是用来处理离线文件的,是独立的main 函数完成的
class DictProducer
{
public:
    DictProducer(const string &, const string &zhDir);
    void buildEnDict();
    void buildZhDict();
    void storeDict(const string &rhs);
    void showDictSize();
    void showFiles();
    void storeIndex(const string &rhs);
    void dealWord(string &tem);
    void insertIntoDict(const string &str);
    bool isMemberOfStr(const string &str, const char &ch);
    void insertIntoZhDict(const string &str);
    void insertIntoZhIndex(const string &);
    void storeZhDict(const string &rhs);
    void storeZhIndex(const string &rhs);

private:
    string _dir;
    vector<string> _files;
    map<string, int> _dict;
    map<char, set<int>> _index;

    string _zhDir;
    vector<string> _zhFiles;
    map<string, int> _zhDict;
    map<string, set<int>> _zhIndex;
    SplitTool _splitEr;
};
void DictProducer::buildZhDict()
{
    //先统计词频,并且将每一个存在的汉字全部存入 _zhIndex 中的 <汉字,空集合>
    for (auto elem : _zhFiles)
    {
        ifstream ifs(elem);
        if (!ifs.good())
        {
            LogError("failed to open %s", elem.c_str());
            continue;
        }
        string line;
        vector<string> wordVec;
        while (getline(ifs, line))
        {
            wordVec = _splitEr.cut(line);
            for (auto elem : wordVec)
            {
                if (elem.size()%3 == 0) //分词得到的空格 是一个字节,需要处理掉
                {
                    insertIntoZhDict(elem);
                    insertIntoZhIndex(elem);
                }
            }
        }
        ifs.close();
    }
    // 建立汉字索引
    
}
void DictProducer::insertIntoZhIndex(const string &str)
{ // 建立索引的下标,仅仅是 下标!!!! 不插入任何数据
    string tem;
    for (int idx = 0; idx != str.size(); idx += 3)
    {
       // cout << "size = " << str.size() << endl;
       // cout << str << endl;
        tem = str.substr(idx, 3);
        if (0 == _zhIndex.count(tem))
        {
            _zhIndex.insert(make_pair(tem, set<int>()));
        }
    }
}
void DictProducer::insertIntoZhDict(const string &str)
{ // 仅仅增加 词频
    if (_zhDict.count(str))
    {
        _zhDict[str] += 1;
        // 注意 ,map 使用下标访问的时候,即使这个下标不存在,当使用下标访问的时候,就会直接创建对应的下标和 value的键值对
    }
    else
    {
        _zhDict.insert({str, 1});
    }
}
void DictProducer::storeZhIndex(const string &rhs)
{
    ofstream ofs(rhs);
    if (!ofs.good())
    {
        LogError("failed to open %s ", rhs.c_str());
        return;
    }
    for (auto elem : _zhIndex)
    {
        ofs << elem.first << " " << endl;
    }
    ofs.close();
}

void DictProducer::storeZhDict(const string &rhs)
{ //将 _dict 中的内容 存入一个文件中去
    ofstream ofs(rhs);
    if (!ofs.good())
    {
        LogError("open %s failed", rhs.c_str());
        return;
    }
    string one_word;
    for (auto elem : _zhDict)
    {
        one_word = elem.first + " " + to_string(elem.second) + "\n";
        ofs << one_word;
    }
    ofs.close();
}
DictProducer::DictProducer(const string &rhs, const string &zhDir)
    : _dir(rhs),
      _zhDir(zhDir)
{ // 读取目录下面的文件,然后将这些文件放到 _files 里面去
    DIR *pdir = opendir(rhs.c_str());
    if (nullptr == pdir)
    {
        perror("opendir");
        LogError("opendir %s", strerror(errno));
        exit(1);
    }
    struct dirent *pDirent; //这个叫做文件项
    while (pDirent = readdir(pdir))
    {
        if (strcmp(".", pDirent->d_name) == 0 || strcmp("..", pDirent->d_name) == 0)
        {
            continue;
        }
        else
        {
            string tem(pDirent->d_name);
            _files.push_back(rhs + "/" + tem);
        }
    }
    closedir(pdir);

    DirScanner ds;
    _zhFiles = ds.getFilesOfDir(_zhDir);
}
void DictProducer::buildEnDict()
{ // 读取 _files 中每一个文件的内容,统计里面的内容 并且建立 词典 存入 _dict
    for (auto file : _files)
    {
        ifstream ifs(file);
        if (!ifs.good())
        {
            LogError("file %s not exist", file.c_str());
            continue;
        }
        string line;
        string tem;
        while (getline(ifs, line))
        {
            istringstream iss(line);
            while (iss >> tem)
            {
                dealWord(tem);
                if (tem.size())
                {
                    insertIntoDict(tem);
                }
            }
        }
        ifs.close();
    }

    //对字母建立索引
    for (char ch = 'a'; ch <= 'z'; ch++)
    {
        _index.insert(make_pair(ch, set<int>()));
    }
    int line = 0;           // word_frequency 中的 单词对应的行数
    for (auto elem : _dict) // 遍历 _dict中的每一个元素,然后根据其中的字母,将这个单词加入到对应的字母索引下面去
    {
        string word = elem.first;
        for (char ch = 'a'; ch <= 'z'; ch++)
        {
            if (isMemberOfStr(word, ch))
            {
                auto iter = _index.find(ch);
                iter->second.insert(line);
            }
        }
        line++;
    }
}
bool DictProducer::isMemberOfStr(const string &str, const char &ch)
{
    for (int index = 0; index != str.size(); ++index)
    {
        if (ch == str[index])
        {
            return true;
        }
    }
    return false;
}
void DictProducer::insertIntoDict(const string &str)
{ // 仅仅增加 词频
    if (_dict.count(str))
    {
        _dict[str] += 1;
    }
    else
    {
        _dict.insert({str, 1});
    }
}
void DictProducer::dealWord(string &tem)
{
    for (int i = 0; i != tem.size(); i++)
    {
        if (isalpha(tem[i]))
        {
            if (isupper(tem[i]))
            {
                tem[i] -= 32;
            }
        }
        else // 只要获取的单词中,有一个不是 纯英文的,则返回
        {
            tem.clear();
            return; //此处要及时返回,不然i已经大于0了,清空之后的tem大小为0,会无限循环下去
        }
    }
}
void DictProducer::storeDict(const string &rhs)
{ //将 _dict 中的内容 存入一个文件中去
    ofstream ofs(rhs);
    if (!ofs.good())
    {
        LogError("open %s failed", rhs.c_str());
        return;
    }
    string one_word;
    for (auto elem : _dict)
    {
        one_word = elem.first + " " + to_string(elem.second) + "\n";
        ofs << one_word;
    }
    ofs.close();
}
void DictProducer::storeIndex(const string &rhs)
{ //存储 索引文件
    ofstream ofs(rhs.c_str());
    if (!ofs.good())
    {
        LogError("open %s failed", rhs.c_str());
        return;
    }
    for (auto elem : _index)
    {
        ofs << elem.first << " ";
        for (auto val : elem.second)
        {
            ofs << val << " ";
        }
        ofs << endl;
    }
    ofs.close();
}
void DictProducer::showFiles()
{
    cout << "en files" << endl;
    for (auto elem : _files)
    {
        cout << elem << endl;
    }
    cout << "zh files " << endl;
    for (auto elem : _zhFiles)
    {
        cout << elem << endl;
    }
}
void DictProducer::showDictSize()
{
    cout << "en dict size = " << _dict.size() << endl;
    cout << "en index size = " << _index.size() << endl;
    cout << "zh dict size = " << _zhDict.size() << endl;
    cout << "zh index size = " << _zhIndex.size() << endl;
}