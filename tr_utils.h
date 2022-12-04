#ifndef TR_UTILS_H
#define TR_UTILS_H

#include <winsock2.h> // 类型定义
#include <IPHlpApi.h>

// 声明3个函数类型的指针
typedef HANDLE (WINAPI *lpIcmpCreateFile)(VOID);
typedef BOOL (WINAPI *lpIcmpCloseHandle)(HANDLE  IcmpHandle);
typedef DWORD (WINAPI *lpIcmpSendEcho)(
        HANDLE                   IcmpHandle,
        IPAddr                   DestinationAddress,
        LPVOID                   RequestData,
        WORD                     RequestSize,
        PIP_OPTION_INFORMATION   RequestOptions,
        LPVOID                   ReplyBuffer,
        DWORD                    ReplySize,
        DWORD                    Timeout
);

// 常量定义
const int DEF_ICMP_DATA_SIZE = 32;   // 默认ICMP数据部分长度
const u_short DEF_PORT_TO_GET_HOSTNAME = 36872;

// 一些通用的设置
const int DEF_MAX_HOP = 30;          // 最大跳站数
const DWORD DEF_ICMP_TIMEOUT = 3000; // 默认超时时间，单位ms
const int DEF_MAX_TRY = 3;           // 最大尝试次数

#endif // TR_UTILS_H
