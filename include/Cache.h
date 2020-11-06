#pragma once
#include "Mylogger.h"
#include "myhead.h"
#include "Timer.h"
#include "bo_threadPool.h"
#define GETCACHE(index) CacheManager::getCacheInstance()->getCache(index)
class LRUCache
{
public:
    LRUCache(int capa = 10)
        : _capa(capa)
    {
    }
    void addElement(const string &key, const string &value);
    string getValue(const string &key);
    void swap(LRUCache &rhs)
    {
        _hashMap.swap(rhs._hashMap);
        _resultList.swap(rhs._resultList);
    }
    void addCache(const LRUCache &rhs)
    {
        for (auto elem : rhs._resultList)
        {
            addElement(elem.first, elem.second);
        }
    }
    //private:
    unordered_map<string, list<pair<string, string>>::iterator> _hashMap;
    list<pair<string, string>> _resultList;
    int _capa;
};

void LRUCache::addElement(const string &key, const string &value)
{
    if (getValue(key) != string())
    {
        return; //如果不为空,则在 getValue 中已经更新了
    }
    else
    {                                    // 如果为空,则插入
        if (_resultList.size() == _capa) // 缓存满了 则删除末尾元素
        {
            _hashMap.erase(_resultList.end()->first);
            _resultList.pop_back();
        }
        _resultList.push_front(make_pair(key, value));
        _hashMap.insert(make_pair(key, _resultList.begin()));
    }
    // _resultList.push_front(make_pair(key, value));
    // _hashMap.insert(make_pair(key, _resultList.begin()));
}
string LRUCache::getValue(const string &key)
{
    auto iter = _hashMap.find(key);
    if (iter == _hashMap.end())
    {
        return string();
    }
    else
    {
        _resultList.splice(_resultList.begin(), _resultList, iter->second);
        // 这里需要注意的是 ,list 中,移动节点的位置,不需要移动节点本身,只需要修改节点的 两个指针的指向 就可以了
        //由于 节点本身没有改变,所以 hashMap 中的key-value 的信息不需要修改
        return _resultList.begin()->second;
    }
}
// threadCache 不进行 文件操作,只有cacheManager 读取 存入 文件 然后初始化 插入缓存
class ThreadCache
{
public:
    ThreadCache(int cacheSize = 10)
        : _mainCache(cacheSize), _spareCache(cacheSize), _pendingUpdateCache(cacheSize)
    {
    }
    void init(LRUCache &cache)
    {
        _mainCache._hashMap = cache._hashMap;
        _mainCache._resultList = cache._resultList;
        _spareCache._hashMap = cache._hashMap;
        _spareCache._resultList = cache._resultList;
    }
    void addElement(const string &key, const string &value);
    string getValue(const string &key);
    list<pair<string, string>> &getPendingList();
    //private:
    LRUCache _mainCache;          //主缓存
    LRUCache _spareCache;         //备用缓存(更新主缓存的时候 ,切换备用缓存使用)
    LRUCache _pendingUpdateCache; // 热点数据
    MutexLock _mainCacheMutex;    // 主缓存锁
    MutexLock _spareCacheMutex;
    MutexLock _pendingCacheMutex;
    // 注意 有尝试加锁的时候, 加锁成功的时候,需要使用 raii ,加锁失败则不用理会
};

list<pair<string, string>> &ThreadCache::getPendingList()
{
    return _pendingUpdateCache._resultList;
}
void ThreadCache::addElement(const string &key, const string &value)
{
    // 由于需要更新,所以 三个 缓存不是 每一个都确保能够在任何时间都能使用, 主缓存 使用lock ,另外 两个使用 tryLock
    if (0 == _spareCacheMutex.tryLock()) // 如果尝试加锁成功,则更新,没加锁成功 则无需理会(会丢失一部分 热数据)
    {
        _spareCache.addElement(key, value);
        _spareCacheMutex.unlock();
    }
    if (0 == _pendingCacheMutex.tryLock())
    {
        _pendingUpdateCache.addElement(key, value);
        _pendingCacheMutex.unlock();
    }
    MutexLockGuard autoUnlock(_mainCacheMutex);
    _mainCache.addElement(key, value);
}
string ThreadCache::getValue(const string &key)
{
    if (0 == _spareCacheMutex.tryLock())
    {
        _spareCache.getValue(key);
        _spareCacheMutex.unlock();
    }
    if (0 == _pendingCacheMutex.tryLock())
    {
        _pendingUpdateCache.getValue(key);
        _pendingCacheMutex.unlock();
    }
    MutexLockGuard autoUnlock(_mainCacheMutex);
    return _mainCache.getValue(key);
}

class CacheManager
{
public:
    static CacheManager *getCacheInstance()
    {
        if (_pInstance == nullptr)
        {
            _pInstance = new CacheManager();
        }
        return _pInstance;
    }
    void initCache(int cacheNums, const string &cacheFilePath, int cacheSize = 10)
    {
        cout << "cacheList.size() = " << _threadCacheList.size() << endl;
        _threadCacheList.reserve(cacheNums);
        int cnt = cacheNums;
        while (cnt-- > 0)
        {
            _threadCacheList.push_back(cacheSize);
        }
        _cacheFilePath = cacheFilePath;
        _cacheSize = cacheSize;
        loadCache(_cacheFilePath);
    }
    void loadCache(const string &rhs);
    void periodicUpdateCache();
    void writeCacheToFile();
    ThreadCache &getCache(int index)
    {
        return _threadCacheList[index];
    }

private:
    CacheManager() {}
    static CacheManager *_pInstance;
    vector<ThreadCache> _threadCacheList;
    LRUCache _globalCache;
    LRUCache _globalUpdatePendingCache;
    string _cacheFilePath;
    int _cacheSize;
};
CacheManager *CacheManager::_pInstance = CacheManager::getCacheInstance();
// 饿汉模式,保证多线程安全
void CacheManager::writeCacheToFile()
{
    // 把 当前文件改个名字 改成 .old ,然后 创建一个 新的缓存文件
    cout << "write cache to file ,sizeof cache is %d " << _globalCache._resultList.size() << endl;
    for (auto elem : _globalCache._resultList)
    {
        cout << elem.first << " " << elem.second << endl;
    }
    //rename()
    //  两个缓存文件 ,
    ifstream ifs(_cacheFilePath);
    if (ifs.good())
    {
        ifs.close();
        ifs.open(_cacheFilePath + ".old");
        if (ifs.good())
        {

            ifs.close();
            remove((_cacheFilePath + ".old").c_str());
            cout << "delete old cache file" << endl;
        }
        int ret = rename(_cacheFilePath.c_str(), (_cacheFilePath + ".old").c_str());
        cout << "backup last cachefile to .old" << endl;
        if (0 != ret)
        {
            LogError("failed to rename cacheFile % s", _cacheFilePath.c_str());
        }
    }
    ifs.close();
    ofstream ofs(_cacheFilePath);

    if (ofs.good())
    {
        cout << "create a new cache file" << endl;
        for (auto iter = _globalCache._resultList.rbegin(); iter != _globalCache._resultList.rend(); ++iter)
        {
            ofs << iter->first << " " << iter->second << endl;
        }
    }
    else
    {
        LogError("failed to create new cache file");
    }
}
void CacheManager::periodicUpdateCache()
{
    // step 1  ;;;;收集 每一个线程 pending缓存
    for (auto &elem : _threadCacheList)
    {
        //cout<<"mutex "<<elem._mainCacheMutex.getMutexPtr()<<endl;
        LRUCache temCache;
        {
            MutexLockGuard autoUnlock(elem._pendingCacheMutex);
            temCache.swap(elem._pendingUpdateCache);
        }
        _globalUpdatePendingCache.addCache(temCache);
    }
    // step 2  将全局 pending 缓存 更新到每一个 线程的 主 备用 缓存中(注意 ,加锁 以及顺序)
    for (auto &elem : _threadCacheList)
    { // 先加锁 备用缓存,更新备用缓存,然后加锁 主 备用 缓存 ,交换,然后再次 更新备用缓存(交换之前是主缓存)
        // 然后 更新 全局 总 缓存,并且将其 写入到文件之中
        {
            MutexLockGuard autoUnlock(elem._spareCacheMutex);
            elem._spareCache.addCache(_globalUpdatePendingCache);
        }
        {
            MutexLockGuard autoUnlock1(elem._spareCacheMutex);
            MutexLockGuard autoUnlock2(elem._mainCacheMutex);
            elem._spareCache.swap(elem._mainCache);
        }
        {
            MutexLockGuard autoUnlock2(elem._spareCacheMutex); // 此处的备用缓存 ,在交换之前是主 缓存 ,两个缓存都要更新
            elem._spareCache.addCache(_globalUpdatePendingCache);
        }
    }
    _globalCache.addCache(_globalUpdatePendingCache);
    writeCacheToFile();
}
void CacheManager::loadCache(const string &cacheFilePath)
{
    cout << "in cacheManager:: init cache" << endl;
    ifstream ifs(cacheFilePath);
    if (!ifs.good())
    {
        LogError("failed to open %s ", cacheFilePath);
        return; // 没有找到缓存就不加载
    }
    string line;
    string key;
    string value;

    while (getline(ifs, line))
    {
        istringstream iss(line);
        iss >> key;
        value = iss.str(); // 将一行的剩下部分全部读取进来作为一个value(value 中间有空格,用>> 会以空格作为分隔符)
        if (key == string())
        {
            continue;
        }
        cout << "loadcache done" << endl;
        _globalCache.addElement(key, value);
        cout << "loadcache done" << endl;
        key.clear();
        value.clear();
    }
    ifs.close();
    // 读取完缓存文件之后,将这些数据写入到 每一个线程的缓存之中去
    for (auto &elem : _threadCacheList)
    {
        elem.init(_globalCache);
    }
}

class CacheTimerTask
    : public TimerTask
{
public:
    CacheTimerTask(int second, CacheManager *pCache)
        : TimerTask(second), _pCache(pCache) {}
    void process() override
    {
        cout << "cacheTimerTask active" << endl;
        _pCache->periodicUpdateCache();
    }
    CacheManager *_pCache;
};
