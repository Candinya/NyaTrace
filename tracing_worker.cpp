#include <QDebug>

#include "tracing_worker.h"
#include "configs.h"

/**
 * TracingWorker
 * 具体的追踪线程，每个线程负责一跳，当得到当前跳的数据之后返回。
 */

TracingWorker::TracingWorker() {

    // 不能自动销毁，要等派发进程回收（避免脏写）
    setAutoDelete(false);

    // 分配当前跳地址的存储空间
    currentHopIPAddress = new sockaddr_storage;
    memset(currentHopIPAddress, 0, sizeof(sockaddr_storage));

}

TracingWorker::~TracingWorker() {
    // 销毁需要销毁的东西
    delete currentHopIPAddress;
}

void TracingWorker::run() {

    // 标记为非停止
    isStopping = false;

    // 标记 IP 簇
    currentHopIPAddress->ss_family = targetIPAddress->ss_family;

    // 执行第一项任务：获得 IP
    switch (targetIPAddress->ss_family) {
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

void TracingWorker::GetIPv4() {

    // 第一步：追踪
#ifdef Q_OS_WIN

    // ICMP 包发送缓冲区和接收缓冲区
    IP_OPTION_INFORMATION IpOption;
    char SendData[DEF_ICMP_DATA_SIZE];
    char ReplyBuf[sizeof(ICMP_ECHO_REPLY) + DEF_ICMP_DATA_SIZE];

    // ICMP 回复的结果
    PICMP_ECHO_REPLY pEchoReply;

    // 初始化内存区间
    memset(&IpOption, 0,sizeof(IP_OPTION_INFORMATION));
    memset(SendData, 0, sizeof(SendData));
    pEchoReply = (PICMP_ECHO_REPLY)ReplyBuf;

#endif
#ifdef Q_OS_UNIX

// TODO

#endif

    // 设置 TTL
    IpOption.Ttl = iTTL;

    // 设置有效值
    isIPValid = false;

    // 设置超时次数
    int timeoutCount = 0;

    while (!isStopping) {

        // 发送数据报并等待消息到达
        if (

#ifdef Q_OS_WIN
            IcmpSendEcho2(
                hIcmp, NULL, NULL, NULL,
                ((sockaddr_in*)targetIPAddress)->sin_addr.s_addr, SendData, sizeof (SendData), &IpOption,
                ReplyBuf, sizeof(ReplyBuf),
                gCfg->GetTraceTimeout()
            ) != 0

#endif
#ifdef Q_OS_UNIX

// TODO

#endif

        ) {
            // 得到返回
            ((sockaddr_in*)currentHopIPAddress)->sin_addr.s_addr = pEchoReply->Address;
            isIPValid = true;

            // 任务完成，退出线程
            break;
        } else {
            // 出现错误

#ifdef Q_OS_WIN

            auto err = GetLastError();

#endif
#ifdef Q_OS_UNIX

// TODO

#endif

            qWarning() << "[Trace Worker]"
                       << "ICMP send echo failed with error: "
                       << err;

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
    bool isTargetHost = ((sockaddr_in*)currentHopIPAddress)->sin_addr.s_addr == ((sockaddr_in*)targetIPAddress)->sin_addr.s_addr;

    // 完成追踪
    if (isIPValid && !isStopping) {
        // 仅在没有被请求停止的时候回报
        emit reportIPAndTimeConsumption(
            iTTL, pEchoReply->RoundTripTime, QString(printIPAddress), true, isTargetHost
        );
    }

}

void TracingWorker::GetIPv6() {

    // 第一步：追踪
    // ICMP 包发送缓冲区和接收缓冲区
    IP_OPTION_INFORMATION IpOption;
    char SendData[DEF_ICMP_DATA_SIZE];
    char ReplyBuf[sizeof(ICMPV6_ECHO_REPLY) + DEF_ICMP_DATA_SIZE];

    // ICMP 回复的结果
    PICMPV6_ECHO_REPLY pEchoReply;

    // 初始化内存区间
    memset(&IpOption, 0,sizeof(IP_OPTION_INFORMATION));
    memset(SendData, 0, sizeof(SendData));
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
                (sockaddr_in6*)sourceIPAddress, (sockaddr_in6*)targetIPAddress, SendData, sizeof (SendData), &IpOption,
                ReplyBuf, sizeof(ReplyBuf),
                gCfg->GetTraceTimeout()
            ) != 0
        ) {
            // 得到返回
            memcpy(&((sockaddr_in6*)currentHopIPAddress)->sin6_addr, &pEchoReply->Address.sin6_addr, sizeof(pEchoReply->Address.sin6_addr));
            isIPValid = true;

            // 任务完成，退出线程
            break;
        } else {
            // 出现错误
            qWarning() << "[Trace Worker]"
                       << "ICMP send echo failed with error: "
                       << GetLastError();

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
    bool isTargetHost = memcmp(&(*(sockaddr_in6*)currentHopIPAddress).sin6_addr, &(*(sockaddr_in6*)targetIPAddress).sin6_addr, sizeof((*(sockaddr_in6*)targetIPAddress).sin6_addr)) == 0;

    // 完成追踪
    if (isIPValid && !isStopping) {
        // 仅在没有被请求停止的时候回报
        emit reportIPAndTimeConsumption(
            iTTL, pEchoReply->RoundTripTime, QString(printIPAddress), true, isTargetHost
        );
    }

}

void TracingWorker::GetInfo() {

    // 查询 IP 对应的信息

    // 准备从 City 数据库中查询结果
    QString  cityName;
    QString  countryName;
    double   latitude       = 0.0;
    double   longitude      = 0.0;
    uint16_t accuracyRadius = 0;
    bool     isLocationValid = true;

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
        accuracyRadius,
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
        cityName, countryName, latitude, longitude, accuracyRadius, isLocationValid, // GeoIP2 City
        isp, org, asn, asOrg                                                         // GeoIP2 ISP
    );

}

void TracingWorker::GetHostname() {

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

void TracingWorker::requestStop() {
    isStopping = true;
}
