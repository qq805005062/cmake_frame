#pragma once

#include <string>
#include <memory>
#include <assert.h>
#include <stdarg.h>

#include <common/Singleton.h>

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include <log4cxx/logger.h>
#include <log4cxx/logmanager.h>
#include <log4cxx/logstring.h>
#include <log4cxx/propertyconfigurator.h>

typedef std::shared_ptr<std::string> StringPtr;
namespace common
{

class Log4cxxImpl
{
public:
	Log4cxxImpl() = default;
	~Log4cxxImpl() { logger_ = 0; }

	static Log4cxxImpl& Instance() { return common::Singleton<Log4cxxImpl>::instance(); }

	int Init(const char* configFile);
	int UnInit();
	void Stop() { log4cxx::LogManager::shutdown(); } 

	log4cxx::LoggerPtr getDefaultLogger();
	static StringPtr Format(const char* fmt, ...);

private:
	Log4cxxImpl(const Log4cxxImpl&) = delete;
	void operator=(const Log4cxxImpl&) = delete;
private:
	log4cxx::LoggerPtr logger_ = 0;
};

} // end namespace common

#define INFO(fmt, ...) \
{ \
	log4cxx::LoggerPtr logger = common::Log4cxxImpl::Instance().getDefaultLogger(); \
    if (nullptr != logger) { \
        if (logger->isInfoEnabled()) { \
            StringPtr buffer = common::Log4cxxImpl::Format(fmt, ##__VA_ARGS__); \
            LOG4CXX_INFO(logger, *buffer.get()); \
        }\
    } \
}

#define TRACE(fmt, ...) \
{ \
	log4cxx::LoggerPtr logger = common::Log4cxxImpl::Instance().getDefaultLogger(); \
	if (nullptr != logger) { \
        if (logger->isTraceEnabled()) { \
            StringPtr buffer = common::Log4cxxImpl::Format(fmt, ##__VA_ARGS__); \
            LOG4CXX_TRACE(logger, *(buffer.get())); \
        }\
    }\
}

#define DEBUG(fmt, ...) \
{ \
    log4cxx::LoggerPtr logger = common::Log4cxxImpl::Instance().getDefaultLogger(); \
    if (nullptr != logger)  { \
        if (logger->isDebugEnabled()) { \
            StringPtr buffer = common::Log4cxxImpl::Format(fmt, ##__VA_ARGS__); \
            LOG4CXX_DEBUG(logger, *(buffer.get())); \
        } \
    } \
}

#define WARN(fmt, ...) \
{ \
    log4cxx::LoggerPtr logger = common::Log4cxxImpl::Instance().getDefaultLogger(); \
    if (nullptr != logger) { \
        if (logger->isWarnEnabled()) { \
            StringPtr buffer = common::Log4cxxImpl::Format(fmt, ##__VA_ARGS__); \
            LOG4CXX_WARN(logger, *(buffer.get())); \
        } \
    } \
}

#define ERROR(fmt, ...) \
{ \
    log4cxx::LoggerPtr logger = common::Log4cxxImpl::Instance().getDefaultLogger(); \
    if (nullptr != logger) { \
        if (logger->isErrorEnabled()) { \
            StringPtr buffer = common::Log4cxxImpl::Format(fmt, ##__VA_ARGS__); \
            LOG4CXX_ERROR(logger, *buffer.get()); \
        } \
    } \
}

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic warning "-Wold-style-cast"
#endif
