#ifndef TR_UTILS_H
#define TR_UTILS_H

#include <winsock2.h> // 类型定义

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
const DWORD DEF_ICMP_TIMEOUT = 3000;   // 默认超时时间，单位ms
const int DEF_ICMP_DATA_SIZE = 32;     // 默认ICMP数据部分长度
const int MAX_ICMP_PACKET_SIZE = 1024; // 最大ICMP数据报的大小
const int DEF_MAX_HOP = 30;            // 最大跳站数

// 工具函数
USHORT GenerateChecksum(USHORT* pBuf, int iSize);
BOOL DecodeIcmpResponse(char* pBuf, int iPacketSize, DECODE_RESULT& stDecodeResult);

#endif // TR_UTILS_H
