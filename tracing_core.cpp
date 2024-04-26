#include <cstdio>
#include <QThreadPool>
#include <QDebug>

#include "tracing_core.h"
#include "tracing_utils.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

/**
 * TracingCore
 * 执行追踪程序的核心，涉及到基础数据的转化、追踪线程的管理和对 UI 状态的更新
 */

TracingCore::TracingCore() {

    qDebug() << "[Trace Core]"
             << "Tracing Core start constructing...";

    // 载入依赖的动态链接库
    hIcmpDll = LoadLibraryA("IPHLPAPI.DLL");
    if (hIcmpDll == NULL) {
        //emit setMessage(QString("icmp.dll 动态链接库加载失败"));
        WSACleanup();
        qCritical() << "[Trace Core]"
                    << "Failed to load ICMP module";
    }

    // 从动态链接库中获取所需的函数入口地址
    IcmpCreateFile  = (lpIcmpCreateFile )GetProcAddress(hIcmpDll,"IcmpCreateFile" );
    Icmp6CreateFile = (lpIcmpCreateFile )GetProcAddress(hIcmpDll,"Icmp6CreateFile");
    IcmpCloseHandle = (lpIcmpCloseHandle)GetProcAddress(hIcmpDll,"IcmpCloseHandle");

    // 打开 ICMP 句柄
    if ((hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE) {
        //emit setMessage(QString("ICMP 句柄打开失败"));
        WSACleanup();
        qCritical() << "[Trace Core]"
                    << "Failed to open ICMP handle";
    }
    if ((hIcmp6 = Icmp6CreateFile()) == INVALID_HANDLE_VALUE) {
        //emit setMessage(QString("ICMP 句柄打开失败"));
        WSACleanup();
        qCritical() << "[Trace Core]"
                    << "Failed to open ICMP6 handle";
    }

    // 新建一个线程池
    tracingPool = new QThreadPool;

    // 清空子线程数组
    for (int i = 0; i < gCfg->GetTraceMaxHops(); i++) {
        workers[i] = NULL;
    }
}

TracingCore::~TracingCore() {
    // 停止所有子线程
    requestStop();

    // 销毁线程池
    delete tracingPool;

    // 回收资源
    IcmpCloseHandle(hIcmp);
    IcmpCloseHandle(hIcmp6);

    // 释放动态链接库
    FreeLibrary(hIcmpDll);
}

void TracingCore::run() {

    isStopping = false;

    // 获得主机名字符串
    std::string hostStdStr = hostname.toStdString();
    const char * hostCharStr = hostStdStr.c_str();

    qDebug() << "[Trace Core]"
             << "Target host: " << hostCharStr;

    sockaddr_storage targetIPAddress; // 用于存储目标地址

    // 清空目标地址
    ZeroMemory(&targetIPAddress, sizeof(sockaddr_storage));

    char printIPAddress[INET6_ADDRSTRLEN]; // INET6_ADDRSTRLEN 大于 INET_ADDRSTRLEN ，所以可以兼容（虽然可能有点浪费）
    ZeroMemory(printIPAddress, sizeof(printIPAddress));

    if (ParseIPAddress(hostCharStr, targetIPAddress)) {
        // 解析成功，更新状态

        PrintIPAddress(&targetIPAddress, printIPAddress);

        qDebug() << "[Trace Core]"
                 << "Tracing route to " << printIPAddress
                 << " with maximun hops " << gCfg->GetTraceMaxHops();
        emit setMessage(
            QString("开始追踪路由 %1 ，最大跃点数为 %2 。")
               .arg(printIPAddress)
               .arg(gCfg->GetTraceMaxHops())
        );
    }

    // 初始化最大跳数据
    maxHop    = gCfg->GetTraceMaxHops();
    oldMaxHop = gCfg->GetTraceMaxHops();

    // 置线程池最大线程计数为跳数上限，让所有的线程能一起运行
    tracingPool->setMaxThreadCount(gCfg->GetTraceMaxHops());

    // 用于存储当前地址
    sockaddr_storage sourceIPAddress;

    // 清空当前地址
    ZeroMemory(&sourceIPAddress, sizeof(sockaddr_storage));

    // 相同传输协议栈
    sourceIPAddress.ss_family = targetIPAddress.ss_family;

    // 使用任意出站 IP ，如果设计成可以从列表中选取可能会更好（这是一个可以优化的点）
    switch(sourceIPAddress.ss_family) {
    case AF_INET:
        qDebug() << "[Trace Core]"
                 << "Binding any outbound IPv4 address";
        ((sockaddr_in*)&sourceIPAddress)->sin_addr = in4addr_any;
        break;
    case AF_INET6:
        qDebug() << "[Trace Core]"
                 << "Binding any outbound IPv6 address";
        ((sockaddr_in6*)&sourceIPAddress)->sin6_addr = in6addr_any;
        break;
    }

    // 使用子线程开始追踪路由
    for (int i = 0; (i < maxHop) && !isStopping; i++) {
        qDebug() << "[Trace Core]"
                 << "TTL: " << i + 1;

        workers[i] = new TracingWorker;

        workers[i]->iTTL = i + 1;
        workers[i]->sourceIPAddress = &sourceIPAddress;
        workers[i]->targetIPAddress = &targetIPAddress;
        workers[i]->hIcmp  = hIcmp;
        workers[i]->hIcmp6 = hIcmp6;
        workers[i]->ipdb = ipdb;

        connect(workers[i], &TracingWorker::reportIPAndTimeConsumption, this, [=](const int hop, const unsigned long timeConsumption, const QString & ipAddress, const bool isValid, const bool isTargetHost) {

            if (hop > maxHop) {
                // 超过目标主机，丢弃
                return;
            }

            // 检查是否为目标主机
            if (isTargetHost && hop <= maxHop) {
                // 是目标主机，并且比最大跳还要靠前

                // 设定为新的最大跳
                maxHop = hop;

                // 终止所有更高跳数的线程（从 maxHop 到 DEF_MAX_HOP 之间的已经在上一轮被清理掉了）
                for (int j = hop; j < oldMaxHop; j++) {
                    if (workers[j] != NULL) {
                        workers[j]->requestStop();
                    }
                }

                // 更新旧的最大跳数据
                oldMaxHop = maxHop;

                // 发出状态命令，删除表中的多余行（暂时好像不需要？）

                // 调试输出
                qDebug() << "[Trace Core]"
                         << "Max hop found:" << hop;

            }

            // 基础信息
            QString ipAddressStr;
            QString timeConsumptionStr;

            if (isValid) {
                ipAddressStr = ipAddress;

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

            emit setIPAndTimeConsumption(hop, timeConsumptionStr, ipAddressStr);
        });

        connect(workers[i], &TracingWorker::reportInformation, this, [=](
            const int hop,
            const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const unsigned short & accuracyRadius, const bool & isLocationValid,
            const QString & isp, const QString & org, const uint & asn, const QString & asOrg
        ) {

            if (hop <= maxHop) {
                emit setInformation(hop, cityName, countryName, latitude, longitude, accuracyRadius, isLocationValid, isp, org, asn, asOrg);
            }

        });

        connect(workers[i], &TracingWorker::reportHostname, this, [=](const int hop, const QString & hostname) {

            if (hop <= maxHop) {
                emit setHostname(hop, hostname);
            }

        });

        connect(workers[i], &TracingWorker::incProgress, this, [=](const int progress) {
            // 发送进度
            emit incProgress(progress);
        });

        connect(workers[i], &TracingWorker::fin, this, [=](const int hop) {

            // 子线程运行完成，标记当前 worker 为 NULL
            qDebug() << "[Trace Core]"
                     << "Hop " << hop << " finished.";

            // 回收子线程
            workers[hop - 1]->deleteLater();

            // 标记为运行完成
            workers[hop - 1] = NULL;

        });

        // 将任务分配到线程池并启动
        tracingPool->start(workers[i]);

        // 休息一会，来尽可能创造到达目标主机的首包时间差
        usleep(gCfg->GetTraceThreadInterval());

    }

    // 等待所有的线程运行结束
    tracingPool->waitForDone();

    // 追踪完成，更新状态
    qDebug() << "[Trace Core]"
             << "Trace Route finish.";

    emit end(true);

}

void TracingCore::requestStop() {
    // 设置自身需要停止
    isStopping = true;

    // 对每一个子线程发出停止信号
    for (int i = 0; i < gCfg->GetTraceMaxHops(); i++) {
        if (workers[i] != NULL) {
            workers[i]->requestStop();
        }
    }
}
