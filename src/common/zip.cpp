#include <zlib.h>

#include "zip.h"

#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

namespace common
{

int zipcompress(void *data, size_t ndata, void *zdata, size_t *nzdata)
{
    return 0;
}

int zipdecompress(void *zdata, size_t nzdata, void *data, size_t *ndata)
{
    return 0;
}

}
