#include <QDebug>

#include "resolve_core.h"
#include "tracing_utils.h"

ResolveCore::ResolveCore() {
    // 没什么需要准备的
}

ResolveCore::~ResolveCore() {
    // 没什么需要销毁的
}

void ResolveCore::run() {

    // 获得主机名字符串
    std::string hostStdStr = hostname.toStdString();
    const char * hostCharStr = hostStdStr.c_str();

    qDebug() << "[Resolve Core]"
             << "Target host: " << hostCharStr;

    sockaddr_storage targetIPAddress; // 用于存储目标地址

    // 清空目标地址
    memset(&targetIPAddress, 0, sizeof(sockaddr_storage));

    if (ParseIPAddress(hostCharStr, targetIPAddress)) {
        // 解析成功，更新状态
        GetInfo(1, &targetIPAddress);
    } else {
        // 按照主机名解析
        qDebug() << "[Resolve Core]"
                 << "Target host is not IP address, resolving...";

        auto resolveResult = ResolveAllAddress(hostCharStr);

        if (resolveResult != NULL) {
            int idCounter = 1;
            while (resolveResult != NULL) {
                bool isAddressValid = true;
                switch (resolveResult->ai_family) {
                case AF_INET:
                    // 是 IPv4
                    targetIPAddress.ss_family = AF_INET;
                    (*(sockaddr_in*)&targetIPAddress).sin_addr.s_addr = (*(sockaddr_in*)resolveResult->ai_addr).sin_addr.s_addr;
                    break;
                case AF_INET6:
                    // 是 IPv6
                    targetIPAddress.ss_family = AF_INET6;
                    memcpy((*(sockaddr_in6*)&targetIPAddress).sin6_addr.s6_addr, (*(sockaddr_in6*)resolveResult->ai_addr).sin6_addr.s6_addr, resolveResult->ai_addrlen);
                    break;
                default:
                    // 不是 IPv4 也不是 IPv6
                    isAddressValid = false;
                }

                if (isAddressValid) {
                    GetInfo(idCounter, &targetIPAddress);
                    idCounter++;
                }

                resolveResult = resolveResult->ai_next;
            }
        } else {
            // 解析失败

#ifdef Q_OS_WIN

            auto err = WSAGetLastError();

#endif
#ifdef Q_OS_UNIX

            QString err;

            switch (h_errno) {
            case HOST_NOT_FOUND:
                err = "找不到主机";
                break;
            case NO_ADDRESS:
                err = "没有查询到地址";
                break;
            case NO_RECOVERY:
                err = "无法恢复的解析错误";
                break;
            case TRY_AGAIN:
                err = "名称解析暂时不可用";
                break;
            default:
                err = "未知问题";
                break;
            }

#endif

            qWarning() << "[Resolve Core]"
                        << "Failed to resolve host with error: " << err;
            emit setMessage(QString("主机名解析失败，错误代码： %1 。").arg(err));
            emit end(false);
            return; // 结束
        }
    }

    emit end(true);
}

void ResolveCore::GetInfo(int id, sockaddr_storage * targetIPAddress) {

    // 查询 IP 对应的信息

    char printIPAddress[INET6_ADDRSTRLEN]; // INET6_ADDRSTRLEN 大于 INET_ADDRSTRLEN ，所以可以兼容（虽然可能有点浪费）
    memset(printIPAddress, 0, sizeof(printIPAddress));
    PrintIPAddress(targetIPAddress, printIPAddress);

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
        (sockaddr *)targetIPAddress,
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
        (sockaddr *)targetIPAddress,
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
    emit setInformation(
        id, QString(printIPAddress),
        cityName, countryName, latitude, longitude, accuracyRadius, isLocationValid, // GeoIP2 City
        isp, org, asn, asOrg                                                         // GeoIP2 ISP
    );

}
