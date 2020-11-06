#pragma once
#include "../include/myhead.h"

#include <log4cpp/Category.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/Priority.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/FileAppender.hh>

class Mylogger
{
public:
	static Mylogger *getInstance();

	static void destroy();

	template <class... Args>
	void warn(const char *msg, Args... args)
	{
		_mycat.warn(msg, args...);
		// 这里需要注意的是，error/debug 等一共八个函数 都是支持多参数的,所以这里直接写就可以了
	}
	template <class... Args>
	void error(const char *msg, Args... args)
	{
		_mycat.error(msg, args...);
	}
	template <class... Args>
	void info(const char *msg, Args... args)
	{
		_mycat.info(msg, args...);
	}

	template <class... Args>
	void debug(const char *msg, Args... args)
	{
		_mycat.debug(msg, args...);
	}

	void setPriority(int val);
	void addFileAppender(string filePath);

private:
	Mylogger();
	~Mylogger();

private:
	log4cpp::Category &_mycat;
	static Mylogger *_pInstance;
};

#define prefix(msg) string("[")                           \
						.append(__FILE__)                 \
						.append(":")                      \
						.append(__FUNCTION__)             \
						.append(":")                      \
						.append(std::to_string(__LINE__)) \
						.append("] ")                     \
						.append(msg)                      \
						.c_str()

#define LogError(msg, ...) Mylogger::getInstance()->error(prefix(msg), ##__VA_ARGS__)
#define LogWarn(msg, ...) Mylogger::getInstance()->warn(prefix(msg), ##__VA_ARGS__)
#define LogInfo(msg, ...) Mylogger::getInstance()->info(prefix(msg), ##__VA_ARGS__)
#define LogDebug(msg, ...) Mylogger::getInstance()->debug(prefix(msg), ##__VA_ARGS__)

Mylogger *Mylogger::_pInstance = getInstance();

Mylogger *Mylogger::getInstance()
{
	if (nullptr == _pInstance)
	{
		_pInstance = new Mylogger();
	}
	return _pInstance;
}

void Mylogger::destroy()
{
	if (_pInstance)
	{
		delete _pInstance;
	}
}

Mylogger::Mylogger()
	: _mycat(log4cpp::Category::getRoot().getInstance("test"))
{
	using namespace log4cpp;
	PatternLayout *ptn1 = new PatternLayout();
	ptn1->setConversionPattern("%m%n");//屏幕上打印信息,减少内容,否则不好查看

	OstreamAppender *ostreamAppender = new OstreamAppender("OstreamAppender", &cout);
	ostreamAppender->setLayout(ptn1);

	_mycat.addAppender(ostreamAppender);

	_mycat.setPriority(log4cpp::Priority::DEBUG);
	cout << "Mylogger()" << endl;
}
void Mylogger::addFileAppender(string filePath)
{
	using namespace log4cpp;
	PatternLayout *ptn2 = new PatternLayout();
	ptn2->setConversionPattern("%d %c [%p] %m%n");
	FileAppender *fileAppender = new FileAppender("FileAppender", filePath);
	fileAppender->setLayout(ptn2);
	_mycat.addAppender(fileAppender);
}

Mylogger::~Mylogger()
{
	cout << "~Mylogger()" << endl;
	log4cpp::Category::shutdown();
}

void Mylogger::setPriority(int val)
{
	_mycat.setPriority(val);
}
