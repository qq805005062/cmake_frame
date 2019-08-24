#include <string.h>
#include <zlib.h>

#include "gzip.h"

#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

namespace common
{

int gzcompress(void *data, size_t ndata, void *zdata, size_t *nzdata)
{
    int ret = -1;
    z_stream c_stream;
    if (!data || !ndata)
    {
        PERROR("NULL");
        return -1;
    }

    c_stream.zalloc = NULL;
    c_stream.zfree  = NULL;
    c_stream.opaque = NULL;
    if (deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        PERROR("deflateInit2");
        return -1;
    }
    c_stream.next_in   = (Bytef *)data;
    c_stream.avail_in  = ndata;
    c_stream.next_out  = (Bytef *)zdata;
    c_stream.avail_out = *nzdata;
    while (c_stream.avail_in != 0 && c_stream.total_out < *nzdata)
    {
        if (deflate(&c_stream, Z_NO_FLUSH) != Z_OK)
        {
            PERROR("deflate");
            goto end;
        }
    }

    if (c_stream.avail_in != 0)
    {
        PERROR("%d", c_stream.avail_in);
        return c_stream.avail_in;
    }

    for (;;)
    {
        ret = deflate(&c_stream, Z_FINISH);
        if (ret == Z_STREAM_END)
        {
            PERROR("deflate: Z_STREAM_END");
            break;
        }
        else if (ret != Z_OK)
        {
            PERROR("deflate: %d", ret);
            break;
        }
    }
end:
    if (deflateEnd(&c_stream) != Z_OK)
    {
        PERROR("deflateEnd: Failure");
        return -1;
    }
    *nzdata = c_stream.total_out;
    return 0;
}

int gzdecompress(void *zdata, size_t nzdata, void *data, size_t *ndata)
{
    int ret = -1;
    z_stream d_stream; /* decompression stream */
    memset(&d_stream, 0,sizeof(d_stream));
    static char dummy_head[2] = {
        0x8 + 0x7 * 0x10,
        (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
    };

    d_stream.zalloc   = NULL;
    d_stream.zfree    = NULL;
    d_stream.opaque   = NULL;
    d_stream.next_in  = (Bytef *)zdata;
    d_stream.avail_in = 0;
    d_stream.next_out = (Bytef *)data;
    if (inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK)
    {
        PERROR("inflateInit2");
        return -1;
    }
    while (d_stream.total_out < *ndata && d_stream.total_in < nzdata)
    {
        d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
        ret = inflate(&d_stream, Z_NO_FLUSH);
        switch (ret)
        {
        case Z_OK:
                continue;
            case Z_STREAM_END:
                    PERROR("inflate: Z_STREAM_END");
                    goto end;
            case Z_DATA_ERROR:
                    d_stream.next_in = (Bytef *)dummy_head;
                    d_stream.avail_in = sizeof(dummy_head);
                    if ((ret = inflate(&d_stream, Z_NO_FLUSH)) != Z_OK)
                    {
                        PERROR("inflate failed");
                        goto end;
                    }
                    break;
            default:
                    PERROR("inflate: %d", ret);
                    goto end;
        }
    }
end:
    if (inflateEnd(&d_stream) != Z_OK)
    {
        PERROR("inflateEnd: Failure");
        return -1;
    }
    *ndata = d_stream.total_out;
    return 0;
}

}

