#ifndef TRACING_DEFS_H
#define TRACING_DEFS_H

#include <QtGlobal>

#ifdef Q_OS_WIN

#include <winsock2.h> // 类型定义
#include <IPHlpApi.h>
#include <IcmpAPI.h>

// 声明 2 个函数类型的指针
typedef HANDLE (WINAPI *lpIcmpCreateFile )(VOID);
typedef BOOL   (WINAPI *lpIcmpCloseHandle)(HANDLE IcmpHandle);

#endif
#ifdef Q_OS_UNIX

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>

#include <arpa/inet.h>

#include <netdb.h>

#endif

// 常量定义
const int DEF_ICMP_DATA_SIZE = 32;   // 默认ICMP数据部分长度

#endif // TRACING_DEFS_H
