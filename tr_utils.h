#ifndef TR_UTILS_H
#define TR_UTILS_H

#include <winsock2.h> // 类型定义

// 使用 icmp.dll 构造请求，以避免返回包被防火墙拦截
#define SETTING_USE_ICMPDLL

#ifdef SETTING_USE_ICMPDLL

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

#else

// 使用直接构造的方式

#pragma pack(1)

// IP数据报头
typedef struct {
    unsigned char hdr_len :4;      // 4位首部长度
    unsigned char version :4;      // 4位版本
    unsigned char tos;             // 8位服务类型
    unsigned short total_len;      // 16位总长度（字节数）
    unsigned short identifier;     // 16位标识
    unsigned short frag_and_flags; // 3位标志 13位片偏移
    unsigned char ttl;             // 8位生存时间
    unsigned char protocol;        // 8位协议 (TCP, UDP etc)
    unsigned short checksum;       // 16位首部校验和
    unsigned long sourceIP;        // 32位原地址
    unsigned long destIP;          // 32位目的IP地址
} IP_HEADER;

// ICMP数据报头
typedef struct {
    BYTE type;     // 8位类型
    BYTE code;     // 8位代码
    USHORT cksum;  // 16位校验和
    USHORT id;     // 16位标识符
    USHORT seq;    // 16位序列号
} ICMP_HEADER;

// 解码结果
typedef struct {
    USHORT usSeqNo;        // 包序列号
    DWORD dwRoundTripTime; // 往返时间
    in_addr dwIPaddr;      // 对端IP地址
} DECODE_RESULT;

#pragma pack()

// ICMP类型字段
const BYTE ICMP_ECHO_REPLY  = 0;       // 回显应答
const BYTE ICMP_ECHO_REQUEST = 8;      // 请求回显
const BYTE ICMP_TIMEOUT   = 11;        // 传输超时
const int MAX_ICMP_PACKET_SIZE = 1024; // 最大ICMP数据报的大小

#endif

// 共享的常量
const int DEF_ICMP_DATA_SIZE = 32;     // 默认ICMP数据部分长度

// 一些通用的设置
const int DEF_MAX_HOP = 30;            // 最大跳站数
const DWORD DEF_ICMP_TIMEOUT = 3000;   // 默认超时时间，单位ms

#endif // TR_UTILS_H
