#ifndef __COMMMON_EXCEPTION_H__
#define __COMMMON_EXCEPTION_H__

#include <exception>
#include <string>

namespace common
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

}//end namespace common

#endif //end __COMMMON_EXCEPTION_H__


