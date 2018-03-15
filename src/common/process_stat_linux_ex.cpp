#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "process_stat_ex.h"

static uint64_t s_nPidCpu = 0;//前次调用时当前进程cpu量
static uint64_t s_nSumCpu = 0;//前次调用时主机cpu量
int SubStr(char **szSvr,char *szSubOut,char *szCutFlag)
{
    if (NULL == *szSvr || NULL == szSubOut || NULL == szCutFlag)
    {
        return -1;
    }
    int nFlagLen = strlen(szCutFlag);
    int nSub = 0;
    char *pTemp = *szSvr;
    char *pTempFlag = szCutFlag;
    bool bFlag = false;
    while (**szSvr != '\0')
    {
        while (1)
        {
            if(!memcmp(*szSvr,pTempFlag,nFlagLen))
            {
                if (!bFlag)
                {
                    memcpy(szSubOut,pTemp,nSub);
                    szSubOut[nSub] = '\0';
                    bFlag = true;
                }
                *szSvr += nFlagLen;
                nSub += nFlagLen;
            }
            else
            {
                break;
            }
        }
        if (bFlag)
        {
            return nSub;
        }
        else
        {
            ++nSub;
            ++(*szSvr);
        }
    }
    if (**szSvr == '\0')
    {
        strcpy(szSubOut,pTemp);
        *szSvr = NULL;
    }
    return nSub;
}

void TrimLeft(char *pchScr)
{
    if (pchScr != NULL)
    {
        int nLen = strlen(pchScr);
        int nAdd = 0;
        while (1)
        {
            if (pchScr[nAdd] == 32)
            {
                ++nAdd;
                continue;
            }
            else
            {
                char *pTemp = new char[nLen - nAdd + 1];
                if (pTemp != NULL)
                {
                    memset(pTemp,0,nLen - nAdd + 1);
                    memcpy(pTemp,pchScr + nAdd,nLen - nAdd);
                    memset(pchScr,0,nLen);
                    strcpy(pchScr,pTemp);
                    delete [] pTemp;
                    pTemp = NULL;
                }
                break;
            }
        }
    }
}

int read_file(char *szFileName,char *szOutBuf,int nBufLen)
{
    int nRes = 0;
    int fd = open(szFileName,O_RDONLY);
    if(fd != -1)
    {
        nRes = read(fd,szOutBuf,nBufLen);
        close(fd);
    }
    return nRes;
}

//modified by zwf 20160107 begin
/*
// 获取当前进程cpu使用率(%)，返回-1失败，大于等于0成功，首次调用返回0，相当于初始化
int get_cpu_usage()
{
    int nRes = 0;
    uint64_t nSumCpu = 0,nPidCpu = 0;
    char szCpuTemp[20] = {0};
    char szFileName[256+1] = {0};
    char szBuf[1024 * 2] = {0};
    sprintf(szFileName, "/proc/stat");
    int nRead = read_file(szFileName,szBuf,sizeof(szBuf));
    if(nRead != -1)
    {
        char *p = szBuf;
        for(int nAdd = 0;nAdd < 9;++nAdd)
        {
            memset(szCpuTemp,0,sizeof(szCpuTemp));
            int nRet = SubStr(&p,szCpuTemp," ");
            if (nRet > 0)
            {
                nSumCpu += atoll(szCpuTemp);
            }
            else
            {
                return -1;
            }
        }
    }
    memset(szFileName, 0, sizeof(szFileName));
    sprintf(szFileName,"/proc/%d/stat",getpid());
    memset(szBuf,0,sizeof(szBuf));
    int nReadPid = read_file(szFileName,szBuf,sizeof(szBuf));
    if(nReadPid != -1)
    {
        char *p = szBuf;
        for(int nAdd = 0;nAdd < 15;++nAdd)
        {
            memset(szCpuTemp,0,sizeof(szCpuTemp));
            int nRet = SubStr(&p,szCpuTemp," ");
            if (nRet > 0)
            {
                if(nAdd >= 13)
                {
                    nPidCpu += atoll(szCpuTemp);
                }
            }
            else
            {
                return -1;
            }
        }
    }
    if((nSumCpu - s_nSumCpu) > 0)
    {
        nRes = ((nPidCpu - s_nPidCpu) * 100 / (nSumCpu - s_nSumCpu));
    }
    s_nSumCpu = nSumCpu;
    s_nPidCpu = nPidCpu;
    return nRes ;
}

// 获取当前进程内存和虚拟内存使用量，返回-1失败，0成功
int get_memory_usage(uint64_t* mem, uint64_t* vmem)
{
    int nRes = -1;
    char szTemp[20] = {0};
    char szFileName[256+1] = {0};
    char szBuf[1024 * 2] = {0};
    sprintf(szFileName,"/proc/%d/status",getpid());
    int nReadPid = read_file(szFileName,szBuf,sizeof(szBuf));
    if(nReadPid != -1)
    {
        char *p = strstr(szBuf,"VmSize:");
        if (p != NULL)
        {
            p = p + strlen("VmSize:");
            TrimLeft(p);
            memcpy(szTemp,p,sizeof(szTemp) - 1);
            *vmem = atoll(szTemp);
        }

        p = strstr(szBuf,"VmRSS:");
        if (p != NULL)
        {
            p = p + strlen("VmRSS:");
            TrimLeft(p);
            memset(szTemp,0,sizeof(szTemp));
            memcpy(szTemp,p,sizeof(szTemp) - 1);
            *mem  = atoll(szTemp);
        }
        nRes = 0;
    }
    *mem = *mem * 1024;
    *vmem =*vmem * 1024;
    return nRes ;
}
*/

// 获取当前进程cpu使用率(%)，返回-1失败，大于等于0成功，首次调用返回0，相当于初始化
int get_cpu_usage()
{
    int nRes = 0;
    uint64_t nSumCpu = 0,nPidCpu = 0;
	
    char szBuf[1024 * 2] = {0};	
 	char szSysCmd[256] = {0};  //shell命令 	
 	FILE *fp = NULL;
	sprintf(szSysCmd,"sed -n '1p' /proc/stat|gawk '{print $2+$3+$4+$5+$6+$7+$8}' "); //  /proc/stat文件的第2~8域
	if (NULL == (fp=popen(szSysCmd,"r")))
		return -1;

	if (fgets(szBuf,sizeof(szBuf),fp))
	{
		nSumCpu = (uint64_t)(atoll(szBuf));		
	}
	else
	{
		pclose(fp);
		return -1;
	}

	pclose(fp);	

	
	sprintf(szSysCmd,"gawk '{print $14+$15}' /proc/%d/stat",getpid());
	if (NULL == (fp=popen(szSysCmd,"r")))
		return -1;
	
	memset(szBuf,0,sizeof(szBuf));	
	if (fgets(szBuf,sizeof(szBuf),fp))
	{
		nPidCpu = (uint64_t)(atoll(szBuf)); 	
	}
	else
	{
		pclose(fp);
		return -1;
	}	

	pclose(fp);	
    if((nSumCpu - s_nSumCpu) > 0)
    {
        nRes = ((nPidCpu - s_nPidCpu) * 100 / (nSumCpu - s_nSumCpu));
    }
	
    s_nSumCpu = nSumCpu;
    s_nPidCpu = nPidCpu;
	
	//fprintf(stderr,"BBBBBBBBBBBBBBB SumCpu=[%llu],g_SumCpu=[%llu],nPidCpu=[%llu],g_PidCpu=[%llu]\n",nSumCpu,s_nSumCpu,nPidCpu,s_nPidCpu);
    return nRes ;
}


// 获取当前进程内存和虚拟内存使用量，返回-1失败，0成功
int get_memory_usage(uint64_t* mem, uint64_t* vmem)
{	
	char szBuf[1024 * 2] = {0};	
 	char szSysCmd[256] = {0};  //shell命令 	
 	FILE *fp = NULL;
	sprintf(szSysCmd,"awk '{if($0 ~ /VmSize:|VmRSS:/) print $2}' /proc/%d/status ",getpid());
	if (NULL == (fp=popen(szSysCmd,"r")))
		return -1;

	int i = 0;
	while (fgets(szBuf,sizeof(szBuf),fp))
	{
		if (0 == i)
			*vmem = (uint64_t)(atoll(szBuf));	
		else if (1 == i)
			*mem  = (uint64_t)(atoll(szBuf));	
		i++;
	}	

	fclose(fp);	
	*mem = (*mem) * 1024;
    *vmem = (*vmem) * 1024;
   
	
	//fprintf(stderr,"mem=[%llu],vmem=[%llu]\n",*mem,*vmem);
    return 0 ;    
	
}

//modified by zwf 20160107 end


// 获取当前进程总共读和写的IO字节数，返回-1失败，0成功
int get_io_bytes(uint64_t* read_bytes, uint64_t* write_bytes)
{
    return 0;
}


//取磁盘空余量,返回-1失败，大于等于0成功,返回的单位为M
//modified by zwf begin
/*
int get_disk_free_space(const char* driver)
{
    int nRes = -1;
    if (driver != NULL)
    {
        struct statfs sfs;
        if(0 == statfs(driver, &sfs))
        {
            nRes = (sfs.f_bfree * 4) / 1024;
        }
    }
    return nRes;
}
*/

//第一种:
//[root@34 deliver_32exe_in64os]# df /root/deliver_32exe_in64os
//文件系统	         1K-块      已用      可用 已用% 挂载点
///dev/mapper/VolGroup-lv_root
//                      18134344   6653048  10560108  39% /

//第二种:
//[root@localhost 32bits]# df /root/ZWF/32bits
//文件系统	         1K-块      已用      可用 已用% 挂载点
///dev/sda2        19150120   4666712  13510608  26% /


int get_disk_free_space(const char* driver)
{
	unsigned long long nDiskFree = 0;
	 
	char szSysCmd[128] = {0};
	char szBuf[256] = {0};

	FILE *fp = NULL;
	//由于存在分两行显示的问题，所以修改shell语句
	//sprintf(szSysCmd,"df %s|awk '{if(NR>1) print $4}'",driver);	
	sprintf(szSysCmd,"df %s|sed -n '$p'|awk '{if(NF<6)print $3;else print $4}'",driver);
	if (NULL == (fp=popen(szSysCmd,"r")))
			return 0;
	
	if (fgets(szBuf,sizeof(szBuf),fp))
	{
		nDiskFree = (unsigned long long)(atoll(szBuf));	
	}
	else
	{
		pclose(fp);
		return 0;
	}

	pclose(fp); 
	return nDiskFree/1024;
}

//modified by zwf end


























