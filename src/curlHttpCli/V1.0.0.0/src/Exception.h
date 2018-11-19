#ifndef __XIAO_EXCEPTION_H__
#define __XIAO_EXCEPTION_H__

#include <exception>
#include <string>

namespace CURL_HTTP_CLI
{

class Exception : public std::exception
{
public:
	explicit Exception(const char* what);
	explicit Exception(const std::string& what);
	virtual ~Exception() throw();
	virtual const char* what() const throw();
	const char* stackTrace() const throw();

private:
	void fillStackTrace();

private:
	std::string message_;
	std::string stack_;
};

}

#endif

