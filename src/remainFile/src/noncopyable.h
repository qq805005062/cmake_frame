#ifndef __REMAIN_MGR_NONCOPYABLE_H__
#define __REMAIN_MGR_NONCOPYABLE_H__

namespace REMAIN_MGR
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

} // end namespace REMAIN_MGR

#endif // __REMAIN_MGR_NONCOPYABLE_H__

