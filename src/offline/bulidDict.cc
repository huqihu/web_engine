#include "../../include/DictProducer.h"
#include "../../include/Config.h"
int main()
{
    Config conf;
    DictProducer dp(conf.getConfig("en_libdir"), conf.getConfig("zh_libdir"));
    dp.buildEnDict();
    dp.buildZhDict();

    dp.showFiles();
    dp.showDictSize();
    dp.storeDict(conf.getConfig("en_dictfile"));
    dp.storeZhDict(conf.getConfig("zh_dictfile"));
    dp.storeIndex(conf.getConfig("en_indexfile"));
    dp.storeZhIndex(conf.getConfig("zh_indexfile"));
}