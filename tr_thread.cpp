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

    // 新建一个线程锁
    maxHopMutexLock = new QMutex;
}

TRThread::~TRThread() {
    // 停止所有子线程
    requestStop();

    // 销毁线程池
    delete tracingPool;

    // 销毁线程锁
    delete maxHopMutexLock;

    // 销毁 IPDB 实例
    delete ipdb;

    // 回收资源
    IcmpCloseHandle(hIcmp);
    FreeLibrary(hIcmpDll);
    WSACleanup();

    cout << "TRThread destroied." << endl;
}

void TRThread::run() {

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

    // 初始化最大跳数据
    maxHop = DEF_MAX_HOP;

    // 使用子线程开始追踪路由
    for (int i = 0; i < DEF_MAX_HOP; i++) {
        cout << "TTL: " << i + 1 << endl;

        workers[i] = new TRTWorker;

        workers[i]->iTTL = i + 1;
        workers[i]->maxTry = DEF_MAX_TRY;
        workers[i]->ulDestIP = ulDestIP;
        workers[i]->hIcmp = hIcmp;

        connect(workers[i], &TRTWorker::reportHop, this, [=](
            const int ttl, const unsigned long timeConsumption, const unsigned long ipAddress,
            const bool isValid
        ) {
            cout << "Worker " << i << " with TTL " << ttl << " returned data."
                 << " TTL: " << ttl << " Time Consumption: " << timeConsumption << " IP Address: " << ipAddress << endl;

            // 检查是否为目标主机
            if (ipAddress == ulDestIP) {
                // 是目标主机，检测此时是否为最大跳
                if (ttl > maxHop) {
                    // 丢弃
                    return;
                } else {

                    // 输出日志
                    cout << "New max hop found: " << ttl
                         << " , terminating other workers..." << endl;
                    // 这里需要一个并发锁，防止读写错乱
                    // 提示：我不确定这样是否为严格顺序且线程安全的，
                    // 但就实验结果来看似乎都还比较稳定，没有出现额外的记录？
                    maxHopMutexLock->lock(); // 上锁，防止意外操作
                    if (ttl <= maxHop) {

                        // 终止所有更高跳数的线程（从 maxHop 到 DEF_MAX_HOP 之间的已经在上一轮被清理掉了）
                        for (int i = ttl; i < maxHop; i++) {
                            if (workers[i] != NULL) {
                                workers[i]->requestStop();
                            }
                        }

                        // 设定为新的最大跳
                        maxHop = ttl;

                        // 发出状态命令，删除表中的多余行（暂时好像不需要？）
                    }
                    maxHopMutexLock->unlock(); // 解锁
                }

            }

            // 基础信息
            QString ipAddressStr;
            QString timeConsumptionStr;

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

            if (isValid) {

                // 记录当前跳数的地址
                ipAddressStr = QString("%1").arg(inet_ntoa(*(in_addr*)&ipAddress));

                // 记录当前跳数的耗时
                if (timeConsumption > 0) {
                    timeConsumptionStr = QString("%1 毫秒").arg(timeConsumption);
                } else {
                    timeConsumptionStr = QString("小于 1 毫秒");
                }

                // 在 City 数据库中查询当前 IP 对应信息
                if (!ipdb->LookUpIPCityInfo(
                    inet_ntoa(*(in_addr*)&ipAddress),
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
                    inet_ntoa(*(in_addr*)&ipAddress),
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
                timeConsumptionStr = QString("请求超时");
                // 其余置空
                ipAddressStr = QString("");

                cityName = QString("");
                countryName = QString("");

                isLocationValid = false;

                isp  = QString("");
                org   = QString("");
                asOrg = QString("");

                // 进一包
                emit incProgress(1);
            }

            // 更新数据
            emit setHop(
                ttl, timeConsumptionStr, ipAddressStr,                       // 基础信息
                cityName, countryName, latitude, longitude, isLocationValid, // GeoIP2 City
                isp, org, asn, asOrg                                         // GeoIP2 ISP
            );
        });

        connect(workers[i], &TRTWorker::fin, this, [=](const int remainPacks) {
            // 子线程运行完成，标记当前 worker 为 NULL
            cout << "Worker " << i << " finished." << endl;
            workers[i] = NULL;

            // 发送剩余包数
            emit incProgress(remainPacks);
        });

        // 将任务分配到线程池并启动
        tracingPool->start(workers[i]);

    }

    // 等待所有的线程运行结束
    tracingPool->waitForDone();

    // 追踪完成，更新状态
    cout << "Trace Route finish." << endl;
    emit setMessage("路由追踪完成。"); // 提示完成信息

}

void TRThread::requestStop() {
    // 对每一个子线程发出停止信号
    for (int i = 0; i < DEF_MAX_HOP; i++) {
        if (workers[i] != NULL) {
            workers[i]->requestStop();
        }
    }
}

TRTWorker::TRTWorker(QObject *parent) {
    // 初始化工作进程

    ZeroMemory(&IpOption,sizeof(IP_OPTION_INFORMATION));
    ZeroMemory(SendData, sizeof(SendData));
    pEchoReply = (PICMP_ECHO_REPLY)ReplyBuf;

    // 设置运行完成后自动销毁
    setAutoDelete(true);

}

TRTWorker::~TRTWorker() {
    // 销毁需要销毁的东西
}

void TRTWorker::run() {

    // 设置 TTL
    IpOption.Ttl = iTTL;

    // 标记为非停止
    isStopping = false;

    // 记录残余包数
    int remainPacks = maxTry;

    for (; (remainPacks > 0) && !isStopping; remainPacks--) {

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

            cout << "Current hop IP: "<< inet_ntoa(*(in_addr*)&pEchoReply->Address) << " "
                 << "Time: "     << pEchoReply->RoundTripTime << "ms" << endl;

            // 完成追踪，回报信息，退出线程
            emit reportHop(
                iTTL, pEchoReply->RoundTripTime, pEchoReply->Address, true
            );

            break;

        } else {
            emit reportHop(
                iTTL, 0, 0, false
            );
        }
    }

    emit fin(remainPacks);
}

void TRTWorker::requestStop() {
    isStopping = true;
}
