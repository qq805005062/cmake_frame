#ifndef __ZOOKEEPERKAFKA_EXCEPTION_H__
#define __ZOOKEEPERKAFKA_EXCEPTION_H__

#include <exception>
#include <string>

namespace ZOOKEEPERKAFKA
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

} // end namespace ZOOKEEPERKAFKA

#endif // __ZOOKEEPERKAFKA_EXCEPTION_H__