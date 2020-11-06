#pragma once
#include "myhead.h"
#include "Mylogger.h"
class DirScanner
{
    public:
    vector<string> getFilesOfDir(const string &dirPath)
    {
        DIR *pdir = opendir(dirPath.c_str());
        if (nullptr == pdir)
        {
            LogError("opendir %s", strerror(errno));
            return vector<string>();
        }
        vector<string> files;
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
                files.push_back(dirPath + "/" + tem);
            }
        }
        closedir(pdir);
        return files;
    }
};