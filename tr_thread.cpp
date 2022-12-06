#include <cstdio>
#include <QThreadPool>
#include <QDebug>

#include "tr_thread.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

TRThread::TRThread() {

    qDebug() << "TRThread start constructing...";

    // 初始化 IPDB 实例
    ipdb = new IPDB;

    // 初始化 WinSock2 与 icmp.dll

    // WinSock2 相关初始化
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { // 进行相应的socket库绑定,MAKEWORD(2,2)表示使用WINSOCK2版本
        qCritical() << "Failed to start winsocks2 library with error: " << WSAGetLastError();
        //emit setMessage(QString("WinSock2 动态链接库初始化失败，错误代码： %1 。").arg(WSAGetLastError())); // 提示信息
        exit(-1); // 结束
    }

    // 载入依赖的动态链接库
    hIcmpDll = LoadLibraryA("IPHLPAPI.DLL");
    if (hIcmpDll == NULL) {
        qCritical() << "Failed to load ICMP module";
        //emit setMessage(QString("icmp.dll 动态链接库加载失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1); // 结束
    }

    // 从动态链接库中获取所需的函数入口地址
    IcmpCreateFile   = (lpIcmpCreateFile )GetProcAddress(hIcmpDll,"IcmpCreateFile"  );
    IcmpCloseHandle  = (lpIcmpCloseHandle)GetProcAddress(hIcmpDll,"IcmpCloseHandle" );
    Icmp6CreateFile  = (lpIcmpCreateFile )GetProcAddress(hIcmpDll,"Icmp6CreateFile" );
    Icmp6CloseHandle = (lpIcmpCloseHandle)GetProcAddress(hIcmpDll,"Icmp6CloseHandle");

    // 打开 ICMP 句柄
    if ((hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE) {
        qCritical() << "Failed to open ICMP handle";
        //emit setMessage(QString("ICMP 句柄打开失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1); // 结束
    }
    if ((hIcmp6 = Icmp6CreateFile()) == INVALID_HANDLE_VALUE) {
        qCritical() << "Failed to open ICMP6 handle";
        //emit setMessage(QString("ICMP 句柄打开失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1); // 结束
    }

    // 新建一个线程池
    tracingPool = new QThreadPool;

    // 清空子线程数组
    for (int i = 0; i < DEF_MAX_HOP; i++) {
        workers[i] = NULL;
    }
}

TRThread::~TRThread() {
    // 停止所有子线程
    requestStop();

    // 销毁线程池
    delete tracingPool;

    // 销毁 IPDB 实例
    delete ipdb;

    // 回收资源
    IcmpCloseHandle(hIcmp);
    Icmp6CloseHandle(hIcmp6);

    FreeLibrary(hIcmpDll);

    WSACleanup();

    qDebug() << "TRThread destroied.";
}

void TRThread::run() {

    isStopping = false;

    // 获得主机名字符串
    std::string hostStdStr = hostname.toStdString();
    const char * hostCharStr = hostStdStr.c_str();

    qDebug() << "Target host: " << hostCharStr;

    sockaddr_storage targetHostIPAddress; // 用于存储目标地址

    if (parseIPAddress(hostCharStr, targetHostIPAddress)) {
        // 解析成功，更新状态
        qDebug() << "Tracing route to " << hostCharStr
             << " with maximun hops " << DEF_MAX_HOP;
        emit setMessage(
            QString("开始追踪路由 %1 ，最大跃点数为 %2 。")
               .arg(hostCharStr)
               .arg(DEF_MAX_HOP)
        );
    } else {
        qDebug() << "Target host is not IP address, resolving...";
        // 按照域名解析
        hostent * pHostent = gethostbyname(hostCharStr);
        if (pHostent != NULL) {
            switch (pHostent->h_addrtype) {
            case AF_INET:
                // 是 IPv4
                targetHostIPAddress.ss_family = AF_INET;
                (*(sockaddr_in*)&targetHostIPAddress).sin_addr.s_addr = *(u_long *)pHostent->h_addr_list[0];
                break;
            case AF_INET6:
                // 是 IPv6
                targetHostIPAddress.ss_family = AF_INET6;
                memcpy((*(sockaddr_in6*)&targetHostIPAddress).sin6_addr.s6_addr, pHostent->h_addr_list[0], pHostent->h_length);
                break;
            default:
                // 不是 IPv4 也不是 IPv6
                qWarning() << "Resolved with invalid reason: " << pHostent->h_addrtype;
                emit setMessage(QString("主机名解析结果异常，结果类型为： %1 。").arg(pHostent->h_addrtype));
                WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
                return; // 结束
                // break;
            }

            char printIPAddress[INET6_ADDRSTRLEN]; // INET6_ADDRSTRLEN 大于 INET_ADDRSTRLEN ，所以可以兼容（虽然可能有点浪费）
            switch(targetHostIPAddress.ss_family) {
            case AF_INET:
                // 是 IPv4
                inet_ntop(AF_INET, &(*(sockaddr_in*)&targetHostIPAddress).sin_addr, printIPAddress, INET_ADDRSTRLEN);
                break;
            case AF_INET6:
                // 是 IPv6
                inet_ntop(AF_INET6, &(*(sockaddr_in6*)&targetHostIPAddress).sin6_addr, printIPAddress, INET6_ADDRSTRLEN);
                break;
            default:
                // 是无效的 IP 地址
                const char * errorNotice = "无效的 IP 地址";
                strncpy_s(printIPAddress, errorNotice, strlen(errorNotice));
                qWarning() << "Invalid IP structure";
                break;
            }

            // 更新状态
            qDebug() << "Tracing route to " << hostCharStr
                 << " [" << printIPAddress << "] "
                 << "with maximun hops " << DEF_MAX_HOP;

            emit setMessage(
                QString("开始追踪路由 %1 [%2] ，最大跃点数为 %3 。")
                   .arg(hostCharStr, printIPAddress)
                   .arg(DEF_MAX_HOP)
            );
        } else {
            // 解析失败
            qCritical() << "Failed to resolve host with error: " << WSAGetLastError();
            emit setMessage(QString("主机名解析失败，错误代码： %1 。").arg(WSAGetLastError()));
            WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
            return; // 结束
        }
    }

    // 初始化最大跳数据
    maxHop    = DEF_MAX_HOP;
    oldMaxHop = DEF_MAX_HOP;

    // 置线程池最大线程计数为跳数上限，让所有的线程能一起运行
    tracingPool->setMaxThreadCount(DEF_MAX_HOP);

    // 使用子线程开始追踪路由
    for (int i = 0; (i < DEF_MAX_HOP) && !isStopping; i++) {
        qDebug() << "TTL: " << i + 1;

        workers[i] = new TRTWorker;

        workers[i]->iTTL = i + 1;
        workers[i]->targetHostIPAddress = &targetHostIPAddress;
        workers[i]->hIcmp = hIcmp;
        workers[i]->hIcmp6 = hIcmp6;
        workers[i]->ipdb = ipdb;

        connect(workers[i], &TRTWorker::reportIPAndTimeConsumption, this, [=](const int ttl, const unsigned long timeConsumption, const QString & ipAddress, const bool isValid, const bool isTargetHost) {

            if (ttl > maxHop) {
                // 超过目标主机，丢弃
                return;
            }

            // 检查是否为目标主机
            if (isTargetHost && ttl <= maxHop) {
                // 是目标主机，并且比最大跳还要靠前

                // 设定为新的最大跳
                maxHop = ttl;

                // 终止所有更高跳数的线程（从 maxHop 到 DEF_MAX_HOP 之间的已经在上一轮被清理掉了）
                for (int i = ttl; i < oldMaxHop; i++) {
                    if (workers[i] != NULL) {
                        workers[i]->requestStop();
                    }
                }

                // 更新旧的最大跳数据
                oldMaxHop = maxHop;

                // 发出状态命令，删除表中的多余行（暂时好像不需要？）

            }

            // 基础信息
            QString ipAddressStr;
            QString timeConsumptionStr;

            if (isValid) {
                ipAddressStr = ipAddress;

                // 回收内存
                // delete printIPAddress;

                // 记录当前跳数的耗时
                if (timeConsumption > 0) {
                    timeConsumptionStr = QString("%1 毫秒").arg(timeConsumption);
                } else {
                    timeConsumptionStr = QString("小于 1 毫秒");
                }
            } else {

                // 记录当前跳的情况：超时未响应
                timeConsumptionStr = QString("请求超时");
                ipAddressStr       = QString("重试 %1 次").arg(timeConsumption);

            }

            emit setIPAndTimeConsumption(ttl, timeConsumptionStr, ipAddressStr);
        });

        connect(workers[i], &TRTWorker::reportInformation, this, [=](
            const int ttl,
            const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const bool & isLocationValid,
            const QString & isp, const QString & org, const uint & asn, const QString & asOrg
        ) {

            if (ttl <= maxHop) {
                emit setInformation(ttl, cityName, countryName, latitude, longitude, isLocationValid, isp, org, asn, asOrg);
            }

        });

        connect(workers[i], &TRTWorker::reportHostname, this, [=](const int ttl, const QString & hostname) {

            if (ttl <= maxHop) {
                emit setHostname(ttl, hostname);
            }

        });

        connect(workers[i], &TRTWorker::incProgress, this, [=](const int progress) {
            // 发送进度
            emit incProgress(progress);
        });

        connect(workers[i], &TRTWorker::fin, this, [=](const int ttl) {

            // 子线程运行完成，标记当前 worker 为 NULL
            qDebug() << "Worker " << i << " finished.";
            workers[ttl - 1] = NULL;

        });

        // 将任务分配到线程池并启动
        tracingPool->start(workers[i]);

        // 休息一会，来尽可能创造到达目标主机的首包时间差
        usleep(DEF_INTERVAL_PER_THREAD);

    }

    // 等待所有的线程运行结束
    tracingPool->waitForDone();

    // 追踪完成，更新状态
    qDebug() << "Trace Route finish.";

}

bool TRThread::parseIPAddress(const char * ipStr, sockaddr_storage & targetHostIPAddress) {

    IN_ADDR  addr4; // IPv4 地址的暂存区域
    IN6_ADDR addr6; // IPv6 地址的暂存区域

    if (inet_pton(AF_INET, ipStr, &addr4) != 0) {
        // 输入是 IPv4 地址
        qDebug() << "It's IPv4";
        targetHostIPAddress.ss_family = AF_INET;
        (*(sockaddr_in*)&targetHostIPAddress).sin_addr = addr4;
    } else if (inet_pton(AF_INET6, ipStr, &addr6) != 0) {
        // 输入是 IPv6 地址
        qDebug() << "It's IPv6";
        targetHostIPAddress.ss_family = AF_INET6;
        (*(sockaddr_in6*)&targetHostIPAddress).sin6_addr = addr6;
    } else {
        // 什么都不是
        return false;
    }

    return true;
}

void TRThread::requestStop() {
    // 设置自身需要停止
    isStopping = true;

    // 对每一个子线程发出停止信号
    for (int i = 0; i < DEF_MAX_HOP; i++) {
        if (workers[i] != NULL) {
            workers[i]->requestStop();
        }
    }
}

TRTWorker::TRTWorker() {

    // 设置运行完成后自动销毁
    setAutoDelete(true);
//    setAutoDelete(false);

    currentHopIPAddress = new sockaddr_storage;
    ZeroMemory(currentHopIPAddress, sizeof(sockaddr_storage));

}

TRTWorker::~TRTWorker() {
    // 销毁需要销毁的东西
    delete currentHopIPAddress;
}

void TRTWorker::run() {

    // 标记为非停止
    isStopping = false;

    // 执行第一项任务：获得 IP
    switch (targetHostIPAddress->ss_family) {
    case AF_INET:
        GetIPv4();
        break;
    case AF_INET6:
        GetIPv6();
        break;
    }

    // 回报一份权重，作为进度条增长的数值
    emit incProgress(1);

    // 如果获得的结果有效，并且并非正在停止，再执行第二项任务：获得 IP 信息
    if (isIPValid && !isStopping) {
        GetInfo();
    }

    // 回报一份权重，作为进度条增长的数值
    emit incProgress(1);

    // 如果获得的结果有效，并且并非正在停止，再执行第三项任务：获得对应的主机名
    if (isIPValid && !isStopping) {
        GetHostname(); // 这一步最耗时，所以放到最后
    }

    // 回报一份权重，作为进度条增长的数值
    emit incProgress(1);

    // 全部任务完成
    emit fin(iTTL);

}

void TRTWorker::GetIPv4() {

    // 第一步：追踪
    // ICMP 包发送缓冲区和接收缓冲区
    IP_OPTION_INFORMATION IpOption;
    char SendData[DEF_ICMP_DATA_SIZE];
    char ReplyBuf[sizeof(ICMP_ECHO_REPLY) + DEF_ICMP_DATA_SIZE];

    // ICMP 回复的结果
    PICMP_ECHO_REPLY pEchoReply;

    // 初始化内存区间
    ZeroMemory(&IpOption,sizeof(IP_OPTION_INFORMATION));
    ZeroMemory(SendData, sizeof(SendData));
    pEchoReply = (PICMP_ECHO_REPLY)ReplyBuf;

    // 设置 TTL
    IpOption.Ttl = iTTL;

    // 设置有效值
    isIPValid = false;

    // 设置超时次数
    int timeoutCount = 0;

    while (!isStopping) {

        // 发送数据报并等待消息到达
        if (
            IcmpSendEcho2(
                hIcmp, NULL, NULL, NULL,
                ((sockaddr_in*)targetHostIPAddress)->sin_addr.s_addr, SendData, sizeof (SendData), &IpOption,
                ReplyBuf, sizeof(ReplyBuf),
                DEF_ICMP_TIMEOUT
            ) != 0
        ) {
            // 得到返回
            ((sockaddr_in*)currentHopIPAddress)->sin_addr.s_addr = pEchoReply->Address;
            isIPValid = true;

            // 任务完成，退出线程
            break;
        } else {
            // 这里可以无视条件回报，因为失败的请求一定不会被认为是目标主机
            timeoutCount++;
            emit reportIPAndTimeConsumption(
                iTTL, timeoutCount, 0, false, false
            );
        }
    }


    char printIPAddress[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(*(sockaddr_in*)currentHopIPAddress).sin_addr, printIPAddress, INET_ADDRSTRLEN);

    // 判断是否为末端主机（最后一跳）
    bool isTargetHost = ((sockaddr_in*)currentHopIPAddress)->sin_addr.s_addr == ((sockaddr_in*)targetHostIPAddress)->sin_addr.s_addr;

    // 完成追踪
    if (isIPValid && !isStopping) {
        // 仅在没有被请求停止的时候回报
        emit reportIPAndTimeConsumption(
            iTTL, pEchoReply->RoundTripTime, QString(printIPAddress), true, isTargetHost
        );
    }

}

void TRTWorker::GetIPv6() {

    // 第一步：追踪
    // ICMP 包发送缓冲区和接收缓冲区
    IP_OPTION_INFORMATION IpOption;
    char SendData[DEF_ICMP_DATA_SIZE];
    char ReplyBuf[sizeof(ICMP6_ECHO_REPLY) + DEF_ICMP_DATA_SIZE];

    // ICMP 回复的结果
    PICMPV6_ECHO_REPLY pEchoReply;

    // 初始化内存区间
    ZeroMemory(&IpOption,sizeof(IP_OPTION_INFORMATION));
    ZeroMemory(SendData, sizeof(SendData));
    pEchoReply = (PICMPV6_ECHO_REPLY)ReplyBuf;

    // 设置 TTL
    IpOption.Ttl = iTTL;

    // 设置有效值
    isIPValid = false;

    // 设置超时次数
    int timeoutCount = 0;

    while (!isStopping) {

        // 发送数据报并等待消息到达
        if (
            Icmp6SendEcho2(
                hIcmp6, NULL, NULL, NULL,
                NULL, (sockaddr_in6*)targetHostIPAddress, SendData, sizeof (SendData), &IpOption,
                ReplyBuf, sizeof(ReplyBuf),
                DEF_ICMP_TIMEOUT
            ) != 0
        ) {
            // 得到返回
            memcpy(((sockaddr_in6*)currentHopIPAddress)->sin6_addr.s6_addr, &pEchoReply->Address, INET6_ADDRSTRLEN);
            isIPValid = true;

            // 任务完成，退出线程
            break;
        } else {
            // 这里可以无视条件回报，因为失败的请求一定不会被认为是目标主机
            timeoutCount++;
            emit reportIPAndTimeConsumption(
                iTTL, timeoutCount, NULL, false, false
            );
        }
    }

    char printIPAddress[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &(*(sockaddr_in6*)currentHopIPAddress).sin6_addr, printIPAddress, INET6_ADDRSTRLEN);

    // 判断是否为末端主机（最后一跳）
    bool isTargetHost = memcmp(&((sockaddr_in6*)currentHopIPAddress)->sin6_addr, &((sockaddr_in6*)targetHostIPAddress)->sin6_addr, INET6_ADDRSTRLEN);

    // 完成追踪
    if (isIPValid && !isStopping) {
        // 仅在没有被请求停止的时候回报
        emit reportIPAndTimeConsumption(
            iTTL, pEchoReply->RoundTripTime, QString(printIPAddress), true, isTargetHost
        );
    }

}

void TRTWorker::GetInfo() {

    // 查询 IP 对应的信息

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

    // 在 City 数据库中查询当前 IP 对应信息
    if (!ipdb->LookUpIPCityInfo(
        (sockaddr *)currentHopIPAddress,
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
        (sockaddr *)currentHopIPAddress,
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

    // 更新数据
    emit reportInformation(
        iTTL,
        cityName, countryName, latitude, longitude, isLocationValid, // GeoIP2 City
        isp, org, asn, asOrg                                         // GeoIP2 ISP
    );

}

void TRTWorker::GetHostname() {

    // 这一步是最耗时的操作，它基本上请求必定会超时，所以放到这里来尽可能优化一下体验

    // 参考 https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-getnameinfo
    char hostnameBuf[NI_MAXHOST];

    if (
        getnameinfo(
            (sockaddr *)currentHopIPAddress,
            sizeof (sockaddr),
            hostnameBuf, NI_MAXHOST,
            NULL, 0,
            NI_NAMEREQD
        ) == 0
    ) {
        // 查询到了主机名
        emit reportHostname(iTTL, QString(hostnameBuf));
    }

    // 否则就是打咩

}

void TRTWorker::requestStop() {
    isStopping = true;
}
