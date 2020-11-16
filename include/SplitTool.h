#pragma once
#include "myhead.h"
#include "Mylogger.h"
// 注意 这里的include 路径是 相对于 这个文件的路径,即使这个文件被 不在一个路劲的.c文件包含 
//也不会出错( .c 文件在编译的时候 ,会使用相对路径叠加 起来 访问到 这个 .h 文件包含的头文件)
// 但是 这里的 词典的文件路径 ,是在运行的时候访问的 ,并不是 包含在这个 头文件中的 , 是相对于运行的程序的路径
#include "../cppjieba/include/cppjieba/Jieba.hpp"

const char *const DICT_PATH = "../../cppjieba/dict/jieba.dict.utf8";
const char *const HMM_PATH = "../../cppjieba/dict/hmm_model.utf8";
const char *const USER_DICT_PATH = "../../cppjieba/dict/user.dict.utf8";
const char *const IDF_PATH = "../../cppjieba/dict/idf.utf8";
const char *const STOP_WORD_PATH = "../../cppjieba/dict/stop_words.utf8";

class SplitTool
{
public:
    SplitTool()
        : jieba(DICT_PATH, // 这 干嘛的？？
                HMM_PATH,
                USER_DICT_PATH,
                IDF_PATH,
                STOP_WORD_PATH)
    {
    }
    vector<string> cut(const string &sentence)
    {
        vector<string> words;
        jieba.Cut(sentence, words, true); // 这里的true ?? false??
        return words;
    }
    cppjieba::Jieba jieba;
};