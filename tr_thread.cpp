#include <iostream>
#include <cstdio>

#include "tr_thread.h"

#include "ipdb.h"

#pragma comment(lib, "ws2_32")
using namespace std;

IPDB * ipdb; // 用于读取 IP 对应数据的类操作接口

TRThread::TRThread(QObject *parent) {

    cout << "TRThread start constructing..." << endl;

    // 初始化 IPDB 实例
    ipdb = new IPDB;

    // 初始化 WinSock2 与 icmp.dll

    // WinSock2 相关初始化
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { // 进行相应的socket库绑定,MAKEWORD(2,2)表示使用WINSOCK2版本
        cerr << "Failed to start winsocks2 library with error: " << WSAGetLastError() << endl;
        //emit setMessage(QString("WinSock2 动态链接库初始化失败，错误代码： %1 。").arg(WSAGetLastError())); // 提示信息
        exit(-1); // 结束
    }

    // 载入 icmp.dll 动态链接库
    hIcmpDll = LoadLibraryA("icmp.dll");
    if (hIcmpDll == NULL) {
        cerr << "Failed to load icmp.dll" << endl;
        //emit setMessage(QString("icmp.dll 动态链接库加载失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1); // 结束
    }

    // 从ICMP.DLL中获取所需的函数入口地址
    IcmpCreateFile  = (lpIcmpCreateFile )GetProcAddress(hIcmpDll,"IcmpCreateFile" );
    IcmpCloseHandle = (lpIcmpCloseHandle)GetProcAddress(hIcmpDll,"IcmpCloseHandle");
    IcmpSendEcho    = (lpIcmpSendEcho   )GetProcAddress(hIcmpDll,"IcmpSendEcho"   );

    // 打开 ICMP 句柄
    if ((hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE){
        cerr << "Failed to open ICMP handle" << endl;
        //emit setMessage(QString("ICMP 句柄打开失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1); // 结束
    }
}

TRThread::~TRThread() {
    // 销毁 IPDB 实例
    delete ipdb;

    // 回收资源
    IcmpCloseHandle(hIcmp);
    FreeLibrary(hIcmpDll);
    WSACleanup();

    cout << "TRThread destroied." << endl;
}

void TRThread::run() {

    // 设置状态为非中止
    isStopping = false;

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

    // 创建ICMP包发送缓冲区和接收缓冲区

    IP_OPTION_INFORMATION IpOption;
    ZeroMemory(&IpOption,sizeof(IP_OPTION_INFORMATION));

    char SendData[DEF_ICMP_DATA_SIZE];
    ZeroMemory(SendData, sizeof(SendData));

    char ReplyBuf[sizeof(ICMP_ECHO_REPLY) + DEF_ICMP_DATA_SIZE];
    PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuf;

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

        // 设置 TTL
        IpOption.Ttl = iTTL;

        // 发送数据报并等待消息到达
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

        // 更新进度条数据
        emit setHop(
            iTTL, timeConsumption, ipAddress,                            // 基础信息
            cityName, countryName, latitude, longitude, isLocationValid, // GeoIP2 City
            isp, org, asn, asOrg                                         // GeoIP2 ISP
        );

    }

    // 追踪完成，更新状态

    cout << "Trace Route finish." << endl;
    emit setMessage("路由追踪完成。"); // 提示完成信息

}
