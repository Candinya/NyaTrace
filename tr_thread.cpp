#include <iostream>
#include <cstdio>
#include <QThreadPool>

#include "tr_thread.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

using namespace std;

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

    // 载入依赖的动态链接库
    hIcmpDll = LoadLibraryA("IPHLPAPI.DLL");
    if (hIcmpDll == NULL) {
        cerr << "Failed to load ICMP module" << endl;
        //emit setMessage(QString("icmp.dll 动态链接库加载失败"));
        WSACleanup(); // 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
        exit(-1); // 结束
    }

    // 从动态链接库中获取所需的函数入口地址
    IcmpCreateFile  = (lpIcmpCreateFile )GetProcAddress(hIcmpDll,"IcmpCreateFile" );
    IcmpCloseHandle = (lpIcmpCloseHandle)GetProcAddress(hIcmpDll,"IcmpCloseHandle");

    // 打开 ICMP 句柄
    if ((hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE) {
        cerr << "Failed to open ICMP handle" << endl;
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
    FreeLibrary(hIcmpDll);
    WSACleanup();

    cout << "TRThread destroied." << endl;
}

void TRThread::run() {

    isStopping = false;

    // 获得主机名字符串
    string hostStdString = hostname.toStdString();
    const char * hostCharString = hostStdString.c_str();

    cout << "Target host: " << hostCharString << endl;

    // 将命令行参数转换为IP地址
    u_long ulDestIP = inet_addr(hostCharString); // 初始化目标地址，将一个点分十进制的IP转换成一个长整数型数（u_long类型）

    if (ulDestIP == INADDR_NONE) { // INADDR_NONE 是个宏定义，代表 IpAddress 是无效的 IP 地址。
        // 转换不成功时按域名解析
        hostent *pHostent = gethostbyname(hostCharString); // 返回对应于给定主机名的包含主机名字和地址信息的 hostent 结构的指针
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

    // 初始化最大跳数据
    maxHop    = DEF_MAX_HOP;
    oldMaxHop = DEF_MAX_HOP;

    // 置线程池最大线程计数为跳数上限，让所有的线程能一起运行
    tracingPool->setMaxThreadCount(DEF_MAX_HOP);

    // 使用子线程开始追踪路由
    for (int i = 0; (i < DEF_MAX_HOP) && !isStopping; i++) {
        cout << "TTL: " << i + 1 << endl;

        workers[i] = new TRTWorker;

        workers[i]->iTTL = i + 1;
        workers[i]->ulDestIP = ulDestIP;
        workers[i]->hIcmp = hIcmp;
        workers[i]->ipdb = ipdb;

        connect(workers[i], &TRTWorker::reportIPAndTimeConsumption, this, [=](const int ttl, const unsigned long timeConsumption, const unsigned long ipAddress, const bool isValid) {

            if (ttl > maxHop) {
                // 超过目标主机，丢弃
                return;
            }

            // 检查是否为目标主机
            if (ipAddress == ulDestIP && ttl <= maxHop) {
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
                auto targetIp = inet_ntoa(*(in_addr*)&ipAddress);

                // 记录当前跳数的地址
                ipAddressStr = QString("%1").arg(targetIp);

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
            cout << "Worker " << i << " finished." << endl;
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
    cout << "Trace Route finish." << endl;

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

TRTWorker::TRTWorker(QObject *parent) {

    // 设置运行完成后自动销毁
    setAutoDelete(true);

}

TRTWorker::~TRTWorker() {
    // 销毁需要销毁的东西
}

void TRTWorker::run() {

    // 标记为非停止
    isStopping = false;

    // 执行第一项任务：获得 IP
    GetIP();

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

void TRTWorker::GetIP() {

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
                (IPAddr)ulDestIP, SendData, sizeof (SendData), &IpOption,
                ReplyBuf, sizeof(ReplyBuf),
                DEF_ICMP_TIMEOUT
            ) != 0
        ) {
            // 得到返回
            ipAddress = pEchoReply->Address;
            isIPValid = true;

            // 任务完成，退出线程
            break;
        } else {
            // 这里可以无视条件回报，因为失败的请求一定不会被认为是目标主机
            timeoutCount++;
            emit reportIPAndTimeConsumption(
                iTTL, timeoutCount, 0, false
            );
        }
    }

    // 完成追踪
    if (isIPValid && !isStopping) {
        // 仅在没有被请求停止的时候回报
        emit reportIPAndTimeConsumption(
            iTTL, pEchoReply->RoundTripTime, pEchoReply->Address, true
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

    // 初始化 IP 数据
    auto targetIp = inet_ntoa(*(in_addr*)&ipAddress);

    // 在 City 数据库中查询当前 IP 对应信息
    if (!ipdb->LookUpIPCityInfo(
        targetIp,
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
        targetIp,
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
    sockaddr_in saGNI;
    char hostnameBuf[NI_MAXHOST];

    saGNI.sin_family = AF_INET;
    saGNI.sin_addr.s_addr = ipAddress;
    saGNI.sin_port = htons(DEF_PORT_TO_GET_HOSTNAME);

    if (
        getnameinfo(
            (struct sockaddr *)&saGNI,
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
