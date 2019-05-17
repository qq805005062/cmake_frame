#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <memory>
#include <vector>

#include <time.h>
#include <sys/time.h>

#include <curl/curl.h>

#define PDEBUG(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: DEBUG " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)
#define PERROR(fmt, args...)                    fprintf(stderr, "%s :: %s() %d: ERROR " fmt " \n", __FILE__, __FUNCTION__, __LINE__, ## args)


void* httpReqTest(void* arg)
{
    CURL* m_pCurl = curl_easy_init();
    if (NULL == m_pCurl)
    {
        return NULL;
    }
    do{
        //curl_easy_setopt(m_pCurl, CURLOPT_TIMEOUT, 20L);
        curl_easy_setopt(m_pCurl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(m_pCurl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(m_pCurl, CURLOPT_URL, "http://192.169.0.61:44300");
        curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
        
        PDEBUG("curl_easy_perform");
        int ret = curl_easy_perform(m_pCurl);
        PDEBUG("curl_easy_perform ret %d", ret);
        sleep(1);
    }while(1);
    curl_easy_cleanup(m_pCurl);
    return NULL;
}

int main(int argc, char* argv[])
{
    pthread_t pthreadId_;

    for(int i = 0; i < 5; i++)
    {
        if(pthread_create(&pthreadId_, NULL, &httpReqTest, NULL))
        {
            PERROR("httpReqTest init error \n");
            return -1;
        }
    }

    while(1)
    {
        sleep(60);
    }
    return 0;
}

