#include <errno.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/DirUtil.h"
#include "common/Error.h"

namespace common
{

char DirUtil::lastError_[256] = { 0 };

int DirUtil::MakeDir(const std::string& path)
{
	if (path.empty())
	{
		_LOG_LAST_ERROR("param path is nullptr");
		return -1;
	}

	if (access(path.c_str(), F_OK) == 0)
	{
		return 0;
	}

	if (mkdir(path.c_str(), 0755) != 0)
	{
		_LOG_LAST_ERROR("mkdir %s failed(%s)", path.c_str(), strerror(errno));
		return -1;
	}

	return 0;
}

int DirUtil::MakeDirP(const std::string& path)
{
	if (path.empty())
	{
		_LOG_LAST_ERROR("param path is nullptr");
		return -1;
	}
	
	if (path.size() > PATH_MAX)
	{
		_LOG_LAST_ERROR("path length %ld > PATH_MAX(%d)", path.size(), PATH_MAX);
		return -1;
	}

	int offset = 0;
	if (path.at(0) == '/')
	{
		offset = 1;
	}

	size_t len = path.length();
	char tmp[PATH_MAX] = { 0 };
	snprintf(tmp, sizeof tmp, "%s", path.c_str());

	for (size_t i = offset; i < len; i++)
	{
		if (tmp[i] != '/')
		{
			continue;
		}

		tmp[i] = '\0';
		if (MakeDir(tmp) != 0)
		{
			return -1;
		}
		tmp[i] = '/';
	}
	return MakeDir(path);
}

std::string DirUtil::GetExePath()
{
	std::string result;
	char buf[1024];
	ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
	if (n > 0)
	{
		result.assign(buf, n);
		size_t pos = result.find_last_of("/");
		if (pos != std::string::npos)
		{
			result.erase(pos + 1);
		}
	}
	return result;
}

} // end namespace common
