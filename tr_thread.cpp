#include <iostream>
#include <cstdio>

#include "tr_utils.h"
#include "tr_thread.h"

#ifdef SETTING_USE_ICMPDLL

#include <windows.h>

#else

#include <ws2tcpip.h>

#endif

#include "ipdb.h"

#pragma comment(lib, "ws2_32")
using namespace std;

IPDB * ipdb; // 用于读取 IP 对应数据的类操作接口

TRThread::TRThread(QObject *parent) {
    // 初始化 IPDB 实例
    ipdb = new IPDB;
}

TRThread::~TRThread() {
    // 销毁 IPDB 实例
    delete ipdb;
}

void TRThread::run() {

    // 设置状态为非中止
    isStopping = false;

    // WinSock2 相关初始化
    WSADATA wsa; // 一种数据结构。这个结构被用来存储被WSAStartup函数调用后返回的Windows Sockets数据
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { // 进行相应的socket库绑定,MAKEWORD(2,2)表示使用WINSOCK2版本
        cerr << "Failed to start winsocks2 library with error: " << WSAGetLastError() << endl;
        emit setMessage(QString("WinSock2 动态链接库初始化失败，错误代码： %1 。").arg(WSAGetLastError())); // 提示信息
        return; // 结束
    }

    // 获得主机名字符串
    string hostStdString = hostname.toStdString();
    const char * hostCharString = hostStdString.c_str();

    cout << "Target host: " << hostCharString << endl;

    // 将命令行参数转换为IP地址
    u_long ulDestIP = inet_addr(hostCharString); // 初始化目标地址，将一个点分十进制的IP转换成一个长整数型数（u_long类型）

    if (ulDestIP == INADDR_NONE) { // INADDR_NONE 是个宏定义，代表IpAddress 无效的IP地址。
        // 转换不成功时按域名解析
        hostent *pHostent = gethostbyname(hostCharString); // 返回对应于给定主机名的包含主机名字和地址信息的hostent结构的指针
        if (pHostent) {
            ulDestIP = (*(in_addr *) pHostent->h_addr).s_addr;//in_addr 用来表示一个32位的IPv4地址.

            // 更新状态
            cout << "Tracing route to " << hostCharString
                 << " [" << inet_ntoa(*(in_addr *) (&ulDestIP)) << "] "
                 << "with maximun hops " << DEF_MAX_HOP
                 << endl;

            emit setMessage(
                QString("开始追踪路由 %1 [%2] ，最大跃点数为 %3 。")
                   .arg(hostCharString, inet_ntoa(*(in_addr *) (&ulDestIP)))
                   .arg(DEF_MAX_HOP)
            );

        } else { // 主机名解析失败
            cerr << "Failed to resolve host with error: " << WSAGetLastError() << endl;
            emit setMessage(QString("主机名解析失败，错误代码： %1 。").arg(WSAGetLastError()));
            WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
            return; // 结束
        }
    } else {
        // 更新状态
        cout << "Tracing route to " << hostCharString
             << " with maximun hops " << DEF_MAX_HOP
             << endl;
        emit setMessage(
            QString("开始追踪路由 %1 ，最大跃点数为 %2 。")
               .arg(hostCharString)
               .arg(DEF_MAX_HOP)
        );
    }

    cout << "Target IP: " << inet_ntoa(*(in_addr *) (&ulDestIP)) << endl;

#ifdef SETTING_USE_ICMPDLL

    // 载入 icmp.dll 动态链接库
    HMODULE hIcmpDll = LoadLibraryA("icmp.dll");
    if (hIcmpDll == NULL) {
        cerr << "Failed to load icmp.dll" << endl;
        emit setMessage(QString("icmp.dll 动态链接库加载失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        return; // 结束
    }

    // 定义 3 个函数指针
    lpIcmpCreateFile  IcmpCreateFile;
    lpIcmpCloseHandle IcmpCloseHandle;
    lpIcmpSendEcho    IcmpSendEcho;

    // 从ICMP.DLL中获取所需的函数入口地址
    IcmpCreateFile = (lpIcmpCreateFile)GetProcAddress(hIcmpDll,"IcmpCreateFile");
    IcmpCloseHandle = (lpIcmpCloseHandle)GetProcAddress(hIcmpDll,"IcmpCloseHandle");
    IcmpSendEcho = (lpIcmpSendEcho)GetProcAddress(hIcmpDll,"IcmpSendEcho");

    // 打开 ICMP 句柄
    HANDLE hIcmp;
    if ((hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE){
        cerr << "Failed to open ICMP handle" << endl;
        emit setMessage(QString("ICMP 句柄打开失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        return; // 结束
    }

#else

    sockaddr_in destSockAddr;// 用来处理网络通信的地址
    ZeroMemory(&destSockAddr, sizeof(sockaddr_in)); // 用0来填充一块内存区域
    destSockAddr.sin_family = AF_INET; //sin_family： 地址族，协议簇 AF_INET（TCP/IP – IPv4）
    destSockAddr.sin_addr.s_addr = ulDestIP; //s_addr： 32位IPv4地址

    // 使用 ICMP 协议创建 Raw Socket
    SOCKET sockRaw = WSASocket(
        AF_INET, SOCK_RAW,     // 套接字类型：原始套接字
        IPPROTO_ICMP, NULL, 0, // IPPROTO_ICMP表示ICMP报头由程序构造
        WSA_FLAG_OVERLAPPED    // iFlags：套接口属性描述
    );
    if (sockRaw == INVALID_SOCKET) { //无效套接字
        cerr << "Failed to create socket with error: " << WSAGetLastError() << endl;
        emit setMessage(QString("套接字创建失败，错误代码： %1 。").arg(WSAGetLastError()));
        WSACleanup();// 终止Winsock 2 DLL (Ws2_32.dll) 的使用
        return; // 结束
    }

    // 设置套接字属性
    int iTimeout = DEF_ICMP_TIMEOUT; // 设置超时
    if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char *) &iTimeout, sizeof(iTimeout)) == SOCKET_ERROR) {
        // 设置与某个套接字关联的选项。选项可能存在于多层协议中,为了操作套接字层的选项，应该将层的值指定为SOL_SOCKET
        // 将要被设置或者获取选项的套接字,选项所在的协议层,需要访问的选项名(套接字的操作都自动带有超时时间),指向包含新选项值的缓冲,现选项的长度

        cerr << "Failed to set socket timed out with error: " << WSAGetLastError() << endl;
        emit setMessage(QString("套接字超时设置失败，错误代码： %1 。").arg(WSAGetLastError()));
        closesocket(sockRaw);
        WSACleanup();
        return;
    }

#endif

    // 创建ICMP包发送缓冲区和接收缓冲区
#ifdef SETTING_USE_ICMPDLL

    IP_OPTION_INFORMATION IpOption;
    ZeroMemory(&IpOption,sizeof(IP_OPTION_INFORMATION));

    char SendData[DEF_ICMP_DATA_SIZE];
    ZeroMemory(SendData, sizeof(SendData));

    char ReplyBuf[sizeof(ICMP_ECHO_REPLY) + DEF_ICMP_DATA_SIZE];
    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuf;

#else

    char IcmpSendBuf[sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE];
    memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf));
    char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
    memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf));

    // 填充待发送的ICMP包（头和数据部分）
    ICMP_HEADER *pIcmpHeader = (ICMP_HEADER *) IcmpSendBuf;
    pIcmpHeader->type = ICMP_ECHO_REQUEST;
    pIcmpHeader->code = 0;
    pIcmpHeader->id = (USHORT) GetCurrentProcessId();
    memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE); // 数据部分填充

#endif

#ifndef SETTING_USE_ICMPDLL

    // 准备结果存储
    DECODE_RESULT stDecodeResult;

    USHORT usSeqNo = 0;

#endif

    BOOL bReachDestHost = FALSE;

    // 开始探测路由，当达到最大跳数或收到来自目标主机的报文，或收到主进程的中止信号时结束
    for (int iTTL = 1; (iTTL < DEF_MAX_HOP) && !bReachDestHost && !isStopping; iTTL++) {
        cout << "TTL: " << iTTL << endl;

        // 准备用于返回的结果
        QString timeConsumption;
        QString ipAddress;

        // 准备从 City 数据库中查询结果
        QString cityName;
        QString countryName;
        double  latitude  = 0.0;
        double  longitude = 0.0;
        bool    isLocationValid = true;

        // 准备从 ISP 数据库中查询结果
        QString isp;
        QString org;
        uint    asn = 0;
        QString asOrg;

        // 设置 TTL 并填充发送报文
#ifdef SETTING_USE_ICMPDLL

        IpOption.Ttl = iTTL;

#else

        // 设置IP数据报头的ttl字段
        setsockopt(
            sockRaw, IPPROTO_IP, IP_TTL, (char *) &iTTL,
            sizeof(iTTL)
        ); // 为 0 （IPPROTO_IP) 的 raw socket 。用于接收任何的IP数据包。其中的校验和和协议分析由程序自己完成。

        // 填充 ICMP 数据报文的剩余字段
        ((ICMP_HEADER *) IcmpSendBuf)->cksum = 0;
        ((ICMP_HEADER *) IcmpSendBuf)->seq = htons(usSeqNo++); // 将无符号短整型主机字节序转换为网络字节序,将一个数的高低位互换, (如:12 34 --> 34 12)
        ((ICMP_HEADER *) IcmpSendBuf)->cksum = GenerateChecksum((USHORT *) IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE);

        // 记录序列号和当前时间
        stDecodeResult.usSeqNo = ((ICMP_HEADER *) IcmpSendBuf)->seq;
        stDecodeResult.dwRoundTripTime = GetTickCount(); // 返回从操作系统启动到当前所经过的毫秒数，常常用来判断某个方法执行的时间

#endif

        // 发送数据报并等待消息到达
#ifdef SETTING_USE_ICMPDLL

        if (
            IcmpSendEcho(
                hIcmp, (IPAddr)ulDestIP,
                SendData, sizeof (SendData), &IpOption,
                ReplyBuf, sizeof(ReplyBuf),
                DEF_ICMP_TIMEOUT
            ) != 0
        ) {
            // 得到返回

            cout << "Current hop IP: "<< inet_ntoa(*(in_addr*)&pEchoReply->Address) << " "
                 << "Target IP: "     << inet_ntoa(*(in_addr*)&ulDestIP) << " "
                 << "Time: "     << pEchoReply->RoundTripTime << "ms" << endl;

            // 记录当前跳数的耗时
            if (pEchoReply->RoundTripTime) {
                timeConsumption = QString("%1 毫秒").arg(pEchoReply->RoundTripTime);
            } else {
                timeConsumption = QString("小于 1 毫秒");
            }

            // 记录当前跳数的地址
            ipAddress = QString("%1").arg(inet_ntoa(*(in_addr*)&pEchoReply->Address));

            // 判断当前是否完成路由追踪（即目标站点返回的数据包）
            if ((unsigned long)pEchoReply->Address==ulDestIP) {
                cout << "Current IP match target, final target reached!" << endl;
                bReachDestHost = TRUE;
            }

            // 在 City 数据库中查询当前 IP 对应信息
            if (!ipdb->LookUpIPCityInfo(
                inet_ntoa(*(in_addr*)&pEchoReply->Address),
                cityName,
                countryName,
                latitude,
                longitude,
                isLocationValid
            )) {
                // 查询失败，使用填充字符
                cityName    = QString("未知");
                countryName = QString("");
                isLocationValid = false;
            }

            // 在 ISP 数据库中查询当前 IP 对应信息
            if (!ipdb->LookUpIPISPInfo(
                inet_ntoa(*(in_addr*)&pEchoReply->Address),
                isp,
                org,
                asn,
                asOrg
            )) {
                // 查询失败，使用填充字符
                isp  = QString("未知");
                org   = QString("");
                asOrg = QString("未知");
            }

        } else {
            // 记录当前跳的情况：超时未响应
            timeConsumption = QString("请求超时");
            // 其余置空
            ipAddress = QString("");

            cityName = QString("");
            countryName = QString("");

            isLocationValid = false;

            isp  = QString("");
            org   = QString("");
            asOrg = QString("");
        }

#else

        // 发送ICMP的EchoRequest数据报
        if (sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, (sockaddr *) &destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR) {
            // 如果目的主机不可达则直接退出
            if (WSAGetLastError() == WSAEHOSTUNREACH) {
                cerr << "Failed to send package with error: " << WSAGetLastError() << endl;
                emit setMessage(QString("数据报文发送失败，错误代码： %1 。").arg(WSAGetLastError()));
            }
            closesocket(sockRaw);
            WSACleanup();
            return;
        }


        // 接收ICMP的EchoReply数据报
        // 因为收到的可能并非程序所期待的数据报，所以需要循环接收直到收到所要数据或超时
        sockaddr_in from;
        int iFromLen = sizeof(from);
        int iReadDataLen;


        while (true) {
            // 等待数据到达
            cout << "Waiting for data..." << endl;
            iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (sockaddr *) &from, &iFromLen);
            if (iReadDataLen != SOCKET_ERROR) {
                // 有数据包到达
                // 解码得到的数据包，如果解码正确则跳出接收循环发送下一个EchoRequest包
                if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, stDecodeResult)) {

                    cout << "Current hop IP: "<< inet_ntoa(stDecodeResult.dwIPaddr) << " "
                         << "Target IP: "     << inet_ntoa(destSockAddr.sin_addr) << endl;

                    // 判断当前是否完成路由追踪（即目标站点返回的数据包）
                    if (stDecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr) {
                        cout << "Current IP match target, final target reached!" << endl;
                        bReachDestHost = TRUE;
                    }

                    // 计算当前跳数的耗时
                    stDecodeResult.dwRoundTripTime = GetTickCount() - stDecodeResult.dwRoundTripTime;
                    if (stDecodeResult.dwRoundTripTime) {
                        timeConsumption = QString("%1 毫秒").arg(stDecodeResult.dwRoundTripTime);
                    } else {
                        timeConsumption = QString("小于 1 毫秒");
                    }

                    // 记录当前跳数的地址
                    ipAddress = QString("%1").arg(inet_ntoa(stDecodeResult.dwIPaddr));

                    // 在 City 数据库中查询当前 IP 对应信息
                    if (!ipdb->LookUpIPCityInfo(
                        inet_ntoa(stDecodeResult.dwIPaddr),
                        cityName,
                        countryName,
                        latitude,
                        longitude,
                        isLocationValid
                    )) {
                        // 查询失败，使用填充字符
                        cityName    = QString("未知");
                        countryName = QString("");
                        isLocationValid = false;
                    }

                    // 在 ISP 数据库中查询当前 IP 对应信息
                    if (!ipdb->LookUpIPISPInfo(
                        inet_ntoa(stDecodeResult.dwIPaddr),
                        isp,
                        org,
                        asn,
                        asOrg
                    )) {
                        // 查询失败，使用填充字符
                        isp  = QString("未知");
                        org   = QString("");
                        asOrg = QString("未知");
                    }

                    break;
                }
            } else if (WSAGetLastError() == WSAETIMEDOUT) {
                // 记录当前跳的情况：超时未响应
                timeConsumption = QString("请求超时");
                // 其余置空
                ipAddress = QString("");

                cityName = QString("");
                countryName = QString("");

                isLocationValid = false;

                isp  = QString("");
                org   = QString("");
                asOrg = QString("");
                break;
            } else {
                cerr << "Failed to call recvFrom with error: " << WSAGetLastError() << endl;
                emit setMessage(QString("套接字监听调用失败，错误代码： %1 。").arg(WSAGetLastError()));
                closesocket(sockRaw);
                WSACleanup();
                return;
            }
        }

#endif

        // 更新进度条数据
        emit setHop(
            iTTL, timeConsumption, ipAddress,                            // 基础信息
            cityName, countryName, latitude, longitude, isLocationValid, // GeoIP2 City
            isp, org, asn, asOrg                                         // GeoIP2 ISP
        );

    }

    // 追踪完成，更新状态，回收资源
#ifdef SETTING_USE_ICMPDLL
    IcmpCloseHandle(hIcmp);
    FreeLibrary(hIcmpDll);
#else
    closesocket(sockRaw);
#endif
    WSACleanup();

    cout << "Trace Route finish." << endl;
    emit setMessage("路由追踪完成。"); // 提示完成信息

}

#ifndef SETTING_USE_ICMPDLL

// 工具函数
// 产生网际校验和
USHORT TRThread::GenerateChecksum(USHORT *pBuf, int iSize) { // 16
    unsigned long cksum = 0;// 32
    while (iSize > 1) { // 40
        cksum += *pBuf++;
        iSize -= sizeof(USHORT);
    }
    if (iSize) {
        cksum += *(UCHAR *) pBuf; // 8
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (USHORT) (~cksum); // ~ 按位取反
}

// 解码得到的数据报，因为涉及到 UI 操作所以放到 TR_GUI 里
BOOL TRThread::DecodeIcmpResponse(char *pBuf, int iPacketSize, DECODE_RESULT &stDecodeResult) {

    // 检查数据报大小的合法性
    IP_HEADER *pIpHdr = (IP_HEADER *) pBuf;
    int iIpHdrLen = pIpHdr->hdr_len * 4;//单位4个字节
    if (iPacketSize < (int) (iIpHdrLen + sizeof(ICMP_HEADER))) {
        return FALSE;
    }

    // 按照ICMP包类型检查id字段和序列号以确定是否是程序应接收的Icmp包
    ICMP_HEADER *pIcmpHdr = (ICMP_HEADER *) (pBuf + iIpHdrLen);
    USHORT usID, usSquNo;//ICMP头 标识符和序列号
    if (pIcmpHdr->type == ICMP_ECHO_REPLY) {
        usID = pIcmpHdr->id;
        usSquNo = pIcmpHdr->seq;
    } else if (pIcmpHdr->type == ICMP_TIMEOUT) {
        char *pInnerIpHdr = pBuf + iIpHdrLen + sizeof(ICMP_HEADER);  //载荷中的IP头
        int iInnerIPHdrLen = ((IP_HEADER *) pInnerIpHdr)->hdr_len * 4;//载荷中的IP头长
        ICMP_HEADER *pInnerIcmpHdr = (ICMP_HEADER *) (pInnerIpHdr + iInnerIPHdrLen);//载荷中的ICMP头
        usID = pInnerIcmpHdr->id;
        usSquNo = pInnerIcmpHdr->seq;
    } else {
        return FALSE;
    }
    if (usID != (USHORT) GetCurrentProcessId() || usSquNo != stDecodeResult.usSeqNo) {
        return FALSE;
    }

    // 处理正确收到的ICMP数据报
    if (pIcmpHdr->type == ICMP_ECHO_REPLY ||
        pIcmpHdr->type == ICMP_TIMEOUT) {

        // 返回解码结果
        stDecodeResult.dwIPaddr.s_addr = pIpHdr->sourceIP;
        return TRUE;
    }
    return FALSE;
}

#endif
