#ifndef PROCESS_STAT_EX_H
#define PROCESS_STAT_EX_H

#include <stdint.h>

// 获取当前进程cpu使用率(%)，返回-1失败，大于等于0成功，首次调用返回0，相当于初始化
int get_cpu_usage();

// 获取当前进程内存和虚拟内存使用量，返回-1失败，0成功 返回单位byte
int get_memory_usage(uint64_t* mem, uint64_t* vmem);

// 获取当前进程总共读和写的IO字节数，返回-1失败，0成功
int get_io_bytes(uint64_t* read_bytes, uint64_t* write_bytes);

//取磁盘空余量,返回-1失败，大于等于0成功,返回的单位为k
int get_disk_free_space(const char* driver);


#endif/*PROCESS_STAT_H*/

