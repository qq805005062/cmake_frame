#ifndef PROCESS_STAT_EX_H
#define PROCESS_STAT_EX_H

#include <stdint.h>

// ��ȡ��ǰ����cpuʹ����(%)������-1ʧ�ܣ����ڵ���0�ɹ����״ε��÷���0���൱�ڳ�ʼ��
int get_cpu_usage();

// ��ȡ��ǰ�����ڴ�������ڴ�ʹ����������-1ʧ�ܣ�0�ɹ� ���ص�λbyte
int get_memory_usage(uint64_t* mem, uint64_t* vmem);

// ��ȡ��ǰ�����ܹ�����д��IO�ֽ���������-1ʧ�ܣ�0�ɹ�
int get_io_bytes(uint64_t* read_bytes, uint64_t* write_bytes);

//ȡ���̿�����,����-1ʧ�ܣ����ڵ���0�ɹ�,���صĵ�λΪk
int get_disk_free_space(const char* driver);


#endif/*PROCESS_STAT_H*/

