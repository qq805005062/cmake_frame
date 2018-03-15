#include "Log4cxxImpl.h"

using namespace common;

int Log4cxxImpl::Init(const char* configFile)
{
	if (configFile == nullptr)
	{
		return -1;
	}

	log4cxx::PropertyConfigurator::configureAndWatch(configFile, 60 * 1000);
	logger_ = log4cxx::Logger::getLogger("msgLogger");
	if (!logger_)
	{
		logger_ = log4cxx::Logger::getRootLogger();
	}

	return 0;
}

int Log4cxxImpl::UnInit()
{
	logger_ = 0;
	return 0;
}

log4cxx::LoggerPtr Log4cxxImpl::getDefaultLogger()
{
	return logger_;
}

StringPtr Log4cxxImpl::Format(const char* fmt, ...)
{
	va_list marker;
    va_start(marker, fmt);
    StringPtr result(new std::string());
    size_t nLength = vsnprintf(NULL, 0, fmt, marker);
    // va_end(marker);

    va_start(marker, fmt);
    result->resize(nLength + 1);
    size_t nSize = vsnprintf(const_cast<char* >(result->data()), nLength + 1, fmt, marker);
    va_end(marker);
    result->resize(nSize);
    return result;
}