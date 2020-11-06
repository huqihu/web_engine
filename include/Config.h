#pragma once
#include "myhead.h"
#include "Mylogger.h"
// 配置文件类 ,主要用于保存 配置文件的路径并且从中获取信息
class Config
{
public:
    Config(string filePath = "../../conf/config.conf") // 注意 ,相对工作路径是 相对于当前工作路径来说的
        : _confFilePath(filePath)
    {
        LogDebug("filePath = %s", filePath.c_str());
        readConfigFile();
    }
    void readConfigFile();
    map<string, string> &getConfigMap();
    string getConfig(const string &);

private:
    string _confFilePath;
    map<string, string> _configMap;
};
void Config::readConfigFile()
{
    ifstream ifs(_confFilePath);
    if (!ifs.good())
    {
        perror("ifs");
        LogError("serious error,config file not found ");
    }
    string line;
    string my_key;
    string my_value;
    while (getline(ifs, line))
    {
        istringstream iss(line);
        iss >> my_key >> my_value;
        if (my_key.size() != 0 && my_value.size() != 0) //当在一行读入的两个 string 都不为空的时候 才可以加入配置
        {
            LogDebug("conf k-v : %s %s ", my_key.c_str(), my_value.c_str());
            _configMap.insert(make_pair(my_key, my_value));
        }
        my_key.clear();
        my_value.clear();
    }
}

map<string, string> &Config::getConfigMap()
{
    return _configMap;
}

string Config::getConfig(const string &rhs)
{
    map<string, string>::iterator iter = _configMap.find(rhs);
    if (iter == _configMap.end())
    {
        LogError("file %s not exist ", rhs.c_str());
        return string();
    }
    return iter->second;
}
