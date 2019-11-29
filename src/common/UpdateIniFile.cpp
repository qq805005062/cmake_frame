
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

#include <sys/stat.h>

#include "UpdateIniFile.h"

namespace common
{

#define PDEBUG(fmt, args...)        fprintf(stderr, "%s :: %s() %d: DEBUG " fmt, __FILE__, __FUNCTION__, __LINE__, ## args)

#define PERROR(fmt, args...)        fprintf(stderr, "%s :: %s() %d: ERROR " fmt, __FILE__, __FUNCTION__, __LINE__, ## args)

const char* framestr_frist_constchar(const char *str, char c)
{
    const char *p = str;
    if(!str)
    {
        return NULL;
    }
    while(*p)
    {
        if(*p == c)
        {
            return p;
        }
        else
        {
            p++;
        }
    }
    return NULL;
}

int frame_strlen(const char *str)
{
    int len = 0;
    if(!str)
    {
        return len;
    }

    while(*str)
    {
        str++;
        len++;
    }

    return len;
}

const char* framestr_first_conststr(const char *str,const char *tar)
{
    const char *q = tar;
    const char tar_fri = *tar;
    const char *p = str,*p_for = NULL;
    int p_len = 0,q_len = 0,i;

    if(!str)
    {
        return NULL;
    }
    p_len = frame_strlen(p);
    q_len = frame_strlen(q);
    if(q_len > p_len)
    {
        return NULL;
    }

    do{
        p_for = framestr_frist_constchar(p,tar_fri);
        if(!p_for)
        {
            return NULL;
        }
        else
        {
            p = p_for;
        }
        p_len = frame_strlen(p);
        if(q_len > p_len)
        {
            return NULL;
        }
        for(i = 0;i < q_len;i++)
        {
            if(*p_for == *q)
            {
                p_for++;
                q++;
                continue;
            }else{
                break;
            }
        }
        if(i == q_len)
        {
            return p;
        }
        else
        {
            p++;
            q = tar;
        }
    }while(1);

    return NULL;
}


void UpdateIniKeyValue(const char *section,const char *key,const char *value,const char *filename)
{
    struct stat st;
    if(filename == NULL)
    {
        return;
    }

    memset(&st,0,sizeof(st));
    if (0 != stat(filename,&st))
    {
        PERROR("configure ini setMysqlPass file stat get error\n");
        return;
    }

    FILE* pFile = fopen(filename,"rb");
    if(pFile == NULL)
    {
        PERROR("configure ini setMysqlPass open file error\n");
        return;
    }

    size_t nDataLen = st.st_size;
    char* pData = new char[nDataLen+1];
    memset(pData,0,(nDataLen+1));
    pData[nDataLen] = '\0';
    size_t nRead = fread(pData,1,nDataLen,pFile);
    if (nRead != nDataLen)
    {
        PERROR("read from info File %s  failed!errno:%d,err:%s\n",filename,errno,strerror(errno));
        fclose(pFile);
        delete[] pData;
        return;
    }

    const char *pSection = framestr_first_conststr(pData,section);
    if(pSection == NULL)
    {
        PERROR("config ini file no found section %s info\n",section);
        fclose(pFile);
        delete[] pData;
        return;
    }

    const char *pkey = framestr_first_conststr(pSection,key);
    if(pkey == NULL)
    {
        PERROR("config ini file no found section %s info\n",key);
        fclose(pFile);
        delete[] pData;
        return;
    }

    const char *pSign = framestr_frist_constchar(pkey,61);///'='
    if(pSign == NULL)
    {
        PERROR("config ini file no found = sign\n");
        fclose(pFile);
        delete[] pData;
        return;
    }

    pSign++;
    fclose(pFile);
    size_t wLen = pSign - pData;
    pFile = fopen(filename,"wb");
    if(pFile == NULL)
    {
        PERROR("open inifile %s failed!error :: %d ,err:: %s\n",filename,errno,strerror(errno));
        fclose(pFile);
        delete[] pData;
        return;
    }

    size_t nWri = fwrite(pData,1,wLen,pFile);
    if(nWri != wLen)
    {
        PERROR("write ini file %s error :: %d ,err:: %s\n",filename,errno,strerror(errno));
        fclose(pFile);
        delete[] pData;
        return;
    }

    wLen = frame_strlen(value);
    nWri = fwrite(value,1,wLen,pFile);
    if(nWri != wLen)
    {
        PERROR("write ini file %s error :: %d ,err:: %s\n",filename,errno,strerror(errno));
        fclose(pFile);
        delete[] pData;
        return;
    }

    pSign = framestr_frist_constchar(pkey,13);///'\r'
    if(pSign)
    {
        wLen = pSign - pData;
        wLen = nRead - wLen;
        nWri = fwrite(pSign,1,wLen,pFile);
        if(nWri != wLen)
        {
            PERROR("write ini file %s error :: %d ,err:: %s\n",filename,errno,strerror(errno));
            fclose(pFile);
            delete[] pData;
            return;
        }
    }else{
        pSign = framestr_frist_constchar(pkey,10);///'\r'
        if(pSign)
        {
            wLen = pSign - pData;
            wLen = nRead - wLen;
            nWri = fwrite(pSign,1,wLen,pFile);
            if(nWri != wLen)
            {
                PERROR("write ini file %s error :: %d ,err:: %s\n",filename,errno,strerror(errno));
                fclose(pFile);
                delete[] pData;
                return;
            }
        }
    }

    fclose(pFile);
    delete[] pData;
    return;
}



}
