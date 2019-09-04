
#include <stdio.h>

#include "MD5.h"
#include "AES.h"

#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)

int main(int argc,char **argv)
{
    std::string input = "55";
    std::string output;
    PDEBUG("hello world");
    common::stringMd5TohexStr(input, output);
    PDEBUG("output %s::0x%s", input.c_str(), output.c_str());

    std::string aesKey(output.c_str(), 16);

    common::aesEcbEncrypt(input, aesKey, output);
    common::aesEcbDecrypt(output, aesKey, input);
    PDEBUG("input %s", input.c_str());
    return 0;
}

