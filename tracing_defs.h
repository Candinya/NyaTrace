#ifndef TRACING_DEFS_H
#define TRACING_DEFS_H

#include <winsock2.h> // 类型定义
#include <IPHlpApi.h>
#include <IcmpAPI.h>

// 声明 2 个函数类型的指针
typedef HANDLE (WINAPI *lpIcmpCreateFile )(VOID);
typedef BOOL   (WINAPI *lpIcmpCloseHandle)(HANDLE IcmpHandle);

// 常量定义
const int DEF_ICMP_DATA_SIZE = 32;   // 默认ICMP数据部分长度

// 一些通用的设置
const int DEF_MAX_HOP = 30;          // 最大跳站数
const DWORD DEF_ICMP_TIMEOUT = 3000; // 默认超时时间，单位ms
const unsigned long DEF_INTERVAL_PER_THREAD = 100; // 每一条线程启动距离上一条的等待时间

#endif // TRACING_DEFS_H
