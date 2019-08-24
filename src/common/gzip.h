#ifndef __COMMON_GZIP_H__
#define __COMMON_GZIP_H__

#include <string>

//#include "gzip.h"

namespace common
{

/*
 * [gzcompress] gzip压缩数据方法
 * @author xiaoxiao 2019-08-23
 * @param data 待压缩数据指针
 * @param ndata 待压缩数据长度
 * @param zdata 压缩之后存放数据的指针，需要调用者自己控制空间，内部不会分配空间，因为是压缩，所以基本上不会比原数据长
 * @param nzdata 分配存放数据空间的长度，即是入参，也是出参
 *
 * @return 0是压缩成功，其他是错误
 */
int gzcompress(void *data, size_t ndata, void *zdata, size_t *nzdata);

/*
 * [gzdecompress] gzip解压数据方法
 * @author xiaoxiao 2019-08-23
 * @param zdata 待解压数据指针
 * @param nzdata 待解压数据长度
 * @param data 解压之后存放数据的指针，需要调用者自己控制空间，内部不会分配空间，其实一般也不知道会放大多大倍，放大10倍应该够了
 * @param ndata 解压之后存放数据空间的长度，即是入参，也是出参
 *
 * @return 0是解压成功，其他是错误
 */
int gzdecompress(void *zdata, size_t nzdata, void *data, size_t *ndata);

}

#endif
