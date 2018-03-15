#include <common/AES.h>

namespace common
{

int AES::encode(const std::string& key, const std::string& input, std::string& output)
{
	if (key.length() * 8 > 256)
	{
		return -1;
	}

	if (key.length() % AES_BLOCK_SIZE > 0)
	{
		std::string keyBak = key;
		size_t len = keyBak.length();
		size_t fill = AES_BLOCK_SIZE - len % AES_BLOCK_SIZE;
		len += fill;
		keyBak.append(fill, 0x00);
		if (AES_set_encrypt_key(reinterpret_cast<const unsigned char* >(keyBak.c_str()),
		                        static_cast<int>(len * 8),
		                        &key_) < 0)
		{
			return -2;
		}
	}
	else
	{
		int ret = AES_set_encrypt_key(reinterpret_cast<const unsigned char* >(key.c_str()),
		                        static_cast<int>(key.length() * 8),
		                        &key_);

		if (ret < 0)
		{
			return ret;
		}
	}

	std::string inputBak = input;
	size_t length = inputBak.length();
	size_t padding = 0;
	if (length % AES_BLOCK_SIZE > 0)
	{
		padding = AES_BLOCK_SIZE - length % AES_BLOCK_SIZE;
	}
	else
	{
		padding = AES_BLOCK_SIZE;
	}

	length += padding;
	inputBak.append(padding, padding);

	for (size_t i = 0; i < length / AES_BLOCK_SIZE; i++)
	{
		unsigned char outbuf[AES_BLOCK_SIZE] = { 0 };
		AES_encrypt(reinterpret_cast<const unsigned char* >(inputBak.c_str() + i * AES_BLOCK_SIZE), outbuf, &key_);
		output.append(reinterpret_cast<char* >(outbuf), AES_BLOCK_SIZE);
	}

	return 0;
}

int AES::decode(const std::string& key, const std::string& input, std::string& output)
{
	if (key.length() * 8 > 256)
	{
		return -1;
	}

	if (key.length() % AES_BLOCK_SIZE > 0)
	{
		std::string keyBak = key;
		size_t len = keyBak.length();
		size_t fill = AES_BLOCK_SIZE - len % AES_BLOCK_SIZE;
		len += fill;
		keyBak.append(fill, 0x00);

		if (AES_set_decrypt_key(reinterpret_cast<const unsigned char* >(keyBak.c_str()),
		                        static_cast<int>(len * 8),
		                        &key_) < 0)
		{
			return -2;
		}
	}
	else
	{
		if (AES_set_decrypt_key(reinterpret_cast<const unsigned char* >(key.c_str()),
		                        static_cast<int>(key.length() * 8),
		                        &key_) < 0)
		{
			return -2;
		}
	}

	size_t len = input.length();
	if (len % AES_BLOCK_SIZE > 0)
	{
		return -2;
	}

	for (size_t i = 0; i < len / AES_BLOCK_SIZE; i++)
	{
		unsigned char outbuf[AES_BLOCK_SIZE] = { 0 };
		AES_decrypt(reinterpret_cast<const unsigned char*>(input.c_str() + i * AES_BLOCK_SIZE), outbuf, &key_);
		output.append(reinterpret_cast<char* >(outbuf), AES_BLOCK_SIZE);
	}

	// 去掉最后的padding部分
	size_t outLen = output.length();
	size_t padding = static_cast<size_t>(output[outLen - 1]);
	// output = output.substr(0, outLen - padding);
	output.resize(outLen - padding);

	return 0;
}

int AES::cbcEncrypt(const std::string& key, const std::string& input, const std::string& initIV, std::string& output)
{
	if (key.length() * 8 > 256)
	{
		return -1;
	}

	if (key.length() % AES_BLOCK_SIZE > 0)
	{
		std::string keyBak = key;
		size_t len = keyBak.length();
		size_t fill = AES_BLOCK_SIZE - len % AES_BLOCK_SIZE;
		len += fill;
		keyBak.append(fill, 0x00);

		if (AES_set_decrypt_key(reinterpret_cast<const unsigned char* >(keyBak.c_str()),
		                        static_cast<int>(len * 8),
		                        &key_) < 0)
		{
			return -2;
		}
	}
	else
	{
		if (AES_set_decrypt_key(reinterpret_cast<const unsigned char* >(key.c_str()),
		                        static_cast<int>(key.length() * 8),
		                        &key_) < 0)
		{
			return -2;
		}
	}

	std::string inputBak = input;
	size_t length = inputBak.length();
	size_t padding = 0;
	if (length % AES_BLOCK_SIZE > 0)
	{
		padding = AES_BLOCK_SIZE - length % AES_BLOCK_SIZE;
	}
	else
	{
		padding = AES_BLOCK_SIZE;
	}

	length += padding;
	inputBak.append(padding, padding);

	unsigned char outbuf[length + 1];
	AES_cbc_encrypt(reinterpret_cast<const unsigned char* >(inputBak.c_str()),
	                outbuf, length, &key_,
	                reinterpret_cast<unsigned char* >(const_cast<char* >(initIV.c_str())),
	                AES_ENCRYPT);
	output.append(reinterpret_cast<char* >(outbuf), length);

	return 0;
}

int AES::cbcDecrypt(const std::string& key, const std::string& input, const std::string& initIV, std::string& output)
{
	if (key.length() * 8 > 256)
	{
		return -1;
	}

	if (key.length() % AES_BLOCK_SIZE > 0)
	{
		std::string keyBak = key;
		size_t len = keyBak.length();
		size_t fill = AES_BLOCK_SIZE - len % AES_BLOCK_SIZE;
		len += fill;
		keyBak.append(fill, 0x00);

		if (AES_set_decrypt_key(reinterpret_cast<const unsigned char* >(keyBak.c_str()),
		                        static_cast<int>(len * 8),
		                        &key_) < 0)
		{
			return -2;
		}
	}
	else
	{
		if (AES_set_decrypt_key(reinterpret_cast<const unsigned char* >(key.c_str()),
		                        static_cast<int>(key.length() * 8),
		                        &key_) < 0)
		{
			return -2;
		}
	}

	size_t len = input.length();
	if (len % AES_BLOCK_SIZE > 0)
	{
		return -2;
	}

	unsigned char outbuf[len + 1];
	AES_cbc_encrypt(reinterpret_cast<const unsigned char* >(input.c_str()),
	                outbuf, len, &key_,
	                reinterpret_cast<unsigned char* >(const_cast<char* >(initIV.c_str())),
	                AES_DECRYPT);

	// 去掉最后的padding部分
	size_t padding = static_cast<size_t>(input[len - 1]);
	output.resize(len - padding);

	return 0;
}

} // end namespace common
