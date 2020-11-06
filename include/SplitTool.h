#pragma once
#include "myhead.h"
#include "Mylogger.h"

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