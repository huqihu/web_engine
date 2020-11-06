#pragma once
#include "myhead.h"
#include "Mylogger.h"

#define GETDICT MyDict::getDictInstance()->getDict()
#define GETINDEX MyDict::getDictInstance()->getIndexTable()
class MyDict
{
public:
    static MyDict *getDictInstance();
    ~MyDict() {}
    void initMyDict(const string &dictEnPath, const string &indexEnPath);
    void saveTestDict();
    vector<pair<string, int>> &getDict();
    map<char, set<int>> &getIndexTable();

private:
    MyDict()
    {
    }
    static MyDict *_pInstance;
    vector<pair<string, int>> _dict;
    map<char, set<int>> _indexTable;
};

MyDict *MyDict::_pInstance = MyDict::getDictInstance(); // 饿汉模式 ,保证多线程安全
MyDict *MyDict::getDictInstance()
{
    if (_pInstance == nullptr)
    {
        _pInstance = new MyDict();
    }
    return _pInstance;
}
void MyDict::initMyDict(const string &dictEnPath, const string &indexEnPath)
{
    ifstream ifs(dictEnPath);
    if (!ifs.good())
    {
        LogError("open %s failed", dictEnPath.c_str());
        return;
    }
    string line;
    string word;
    string tem;
    int frequency;
    while (getline(ifs, line))
    {
        istringstream iss(line);
        iss >> word >> tem;
        frequency = atoi(tem.c_str());
        _dict.push_back(make_pair(word, frequency));
    }
    ifs.close();
    ifs.open(indexEnPath);
    if (!ifs.good())
    {
        LogError("open %s failed", indexEnPath.c_str());
        return;
    }

    char ch;
    int index;
    
    while (getline(ifs, line))
    {
        set<int> tem_set;
        istringstream iss(line);
        iss >> ch;
        int tem;
        while (iss >> tem)
        {
            tem_set.insert(tem);
        }
        _indexTable.insert(make_pair(ch, tem_set));
    }
}
vector<pair<string, int>> &MyDict::getDict()
{
    return _dict;
}
map<char, set<int>> &MyDict::getIndexTable()
{
    return _indexTable;
}

void MyDict::saveTestDict()
{
    ofstream ofs("_dict_test.tem");
    for (auto elem : _dict)
    {
        ofs << elem.first << " " << elem.second << endl;
    }
    ofs.close();
    ofs.open("_index_test.txt");
    for (auto elem : _indexTable)
    {
        ofs << elem.first << " ";
        for (auto val : elem.second)
        {
            ofs << val << " ";
        }
        ofs << endl;
    }
}
