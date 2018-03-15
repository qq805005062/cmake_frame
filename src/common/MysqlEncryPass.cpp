
#include <memory.h>
#include <openssl/aes.h>
#include <openssl/evp.h>  
#include <sys/stat.h>

#include <common/Base64.h>

#include "MysqlEncryPass.h"
namespace common
{

#define	RDN_AES_KEY_LEN		16

#define SHOW_DEBUG		1

#ifdef SHOW_DEBUG
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)

unsigned char aes_key[RDN_AES_KEY_LEN] = {0};
unsigned char aes_iv[RDN_AES_KEY_LEN] = {0};
std::string key= "rmcsrmcsrmcsrmcs";
std::string iv = "mwnetmwnetmwnetm";

const char* framestr_frist_constchar(const char *str,char c)
{
	const char *p = str;
	if(!str)
		return NULL;
	while(*p)
	{
		if(*p == c)
			return p;
		else
			p++;
	}
	return NULL;
}

int frame_strlen(const char *str)
{
	int len = 0;
	if(!str)
		return len;
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
		return NULL;
	p_len = frame_strlen(p);
	q_len = frame_strlen(q);
	if(q_len > p_len)
		return NULL;
	
	do{
		p_for = framestr_frist_constchar(p,tar_fri);
		if(!p_for)
			return NULL;
		else
			p = p_for;
		p_len = frame_strlen(p);
		if(q_len > p_len)
			return NULL;
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
			return p;
		else
		{
			p++;
			q = tar;
		}
	}while(1);

	return NULL;
}

bool UpdateIniKeyValue(const char *section,const char *key,const char *value,const char *filename)
{
	struct stat st;
	if(filename == NULL)
		return false;

	memset(&st,0,sizeof(st));
	if (0 != stat(filename,&st))
	{
		PERROR("configure ini setMysqlPass file stat get error\n");
		return false;
	}

	FILE* pFile = fopen(filename,"rb");
	if(pFile == NULL)
	{
		PERROR("configure ini setMysqlPass open file error\n");
		return false;
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
    	return false;         
    }

	const char *pSection = framestr_first_conststr(pData,section);
	if(pSection == NULL)
	{
		PERROR("config ini file no found section %s info\n",section);
		fclose(pFile);
    	delete[] pData;
    	return false;
	}

	const char *pkey = framestr_first_conststr(pSection,key);
	if(pkey == NULL)
	{
		PERROR("config ini file no found section %s info\n",key);
		fclose(pFile);
    	delete[] pData;
    	return false;
	}

	const char *pSign = framestr_frist_constchar(pkey,61);///'='
	if(pSign == NULL)
	{
		PERROR("config ini file no found = sign\n");
		fclose(pFile);
    	delete[] pData;
    	return false;
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
    	return false;
	}

	size_t nWri = fwrite(pData,1,wLen,pFile);
	if(nWri != wLen)
	{
		PERROR("write ini file %s error :: %d ,err:: %s\n",filename,errno,strerror(errno));
		fclose(pFile);
    	delete[] pData;
    	return false;
	}

	wLen = frame_strlen(value);
	nWri = fwrite(value,1,wLen,pFile);
	if(nWri != wLen)
	{
		PERROR("write ini file %s error :: %d ,err:: %s\n",filename,errno,strerror(errno));
		fclose(pFile);
    	delete[] pData;
    	return false;
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
	    	return false;
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
		    	return false;
			}
		}
	}

	fclose(pFile);
    delete[] pData;
	return true;
}

inline void aes_cbcEncrypt(unsigned char input[], int len, const unsigned char key[16], unsigned char iv[16], unsigned char output[])
{
	AES_KEY key0;
	AES_set_encrypt_key(key, 128, &key0);

	int padding = (16 - len % 16);
	memset((unsigned char*)(input + len),padding,padding);
	len += padding;
	AES_cbc_encrypt((const unsigned char* )input, output, (unsigned long)len, &key0, iv, AES_ENCRYPT); 
}

//解密
inline int aes_cbcDecrypt(unsigned char input[],int len, const unsigned char key[16], unsigned char iv[16], unsigned char output[ ], int & outlen)
{
	AES_KEY aes_key;
	AES_set_decrypt_key(key, 128, &aes_key);
	AES_cbc_encrypt((const unsigned char *)input, output, (unsigned long)len, &aes_key, iv, AES_DECRYPT);
	int padding = output[len - 1];
	if ( (padding<0) || (padding>16))
	{
		return -1;
	}
	memset(output + len - padding, 0, padding);
	outlen= len-padding;
	return 0;
}

std::string aesDecrypt(std::string pass)
{
	std::string express = "";
	if(pass.empty())
	{
		PERROR("The pass will be decode is empty,can't be decode\n");
		return express;
	}
	std::string result;
	if(common::Base64::Decode(pass,&result))
	{
		PDEBUG("common::Base64::Decode right:: %s\n",result.c_str());
	}else{
		PDEBUG("common::Base64::Encode eroor\n");
		return express;
	}

	if(result.length() % RDN_AES_KEY_LEN != 0)
	{
		PERROR("The pass will be decode len %lu is error ,can't be decode\n",result.length());
		return express;
	}

	
	memcpy(aes_key,key.c_str(),RDN_AES_KEY_LEN);
	memcpy(aes_iv,iv.c_str(),RDN_AES_KEY_LEN);

	int len = result.length(),out_len = 0;
	//明文、密文
	char *p_expre = new char[len], *q_encry = new char[len];
	memset(p_expre,0,len);
	memset(q_encry,0,len);
	memcpy(q_encry,result.c_str(),len);
	aes_cbcDecrypt((unsigned char *)(q_encry),len,static_cast<const unsigned char *>(aes_key),aes_iv,(unsigned char *)(p_expre),out_len);
	express.append(p_expre,out_len);
	delete[] p_expre;
	delete[] q_encry;
	return express;
}

//加密
std::string aesEncryption(std::string pass)
{
	int inLen = 0,outLen = 0;
	inLen = pass.length();
	outLen = (inLen / RDN_AES_KEY_LEN) * RDN_AES_KEY_LEN + RDN_AES_KEY_LEN;

	memcpy(aes_key,key.c_str(),RDN_AES_KEY_LEN);
	memcpy(aes_iv,iv.c_str(),RDN_AES_KEY_LEN);
	//明文、密文
	char *p_expre = new char[inLen], *q_encry = new char[outLen];
	memset(p_expre,0,inLen);
	memset(q_encry,0,outLen);
	memcpy(p_expre,pass.c_str(),inLen);
	aes_cbcEncrypt((unsigned char *)(p_expre),inLen,static_cast<const unsigned char *>(aes_key),aes_iv,(unsigned char *)(q_encry));
	std::string express(q_encry,outLen);
	delete[] p_expre;
	delete[] q_encry;
	std::string result;
	if(common::Base64::Encode(express,&result))
	{
		PDEBUG("common::Base64::Encode right:: %s\n",result.c_str());
	}else{
		PDEBUG("common::Base64::Encode eroor\n");
		result = "";
	}
	return result;
}

}
