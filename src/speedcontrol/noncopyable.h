#ifndef __SPEED_CONTROL_NONCOPYABLE_H__
#define __SPEED_CONTROL_NONCOPYABLE_H__

namespace SPEED_CONTROL
{

class noncopyable
{
protected:
	noncopyable() = default;
	~noncopyable() = default;

private:
	noncopyable(const noncopyable& ) = delete;
	const noncopyable& operator=(const noncopyable& ) = delete;
};

} // end namespace common

#endif // __SPEED_CONTROL_NONCOPYABLE_H__