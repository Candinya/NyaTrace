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

#endif // TRACING_DEFS_H
