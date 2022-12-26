#include <QDebug>

#include "tracing_utils.h"

addrinfo * ResolveAllAddress(const char * hostname) {
    addrinfo * resolveResult = NULL;
    addrinfo   resolveHints;

    ZeroMemory(&resolveHints, sizeof(resolveHints));

    resolveHints.ai_family   = AF_UNSPEC;
    resolveHints.ai_socktype = SOCK_STREAM;
    resolveHints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(hostname, NULL, &resolveHints, &resolveResult) == 0) {
        return resolveResult;
    } else {
        return NULL;
    }
}

bool ParseIPAddress(const char * ipStr, sockaddr_storage & targetIPAddress) {

    IN_ADDR  addr4; // IPv4 地址的暂存区域
    IN6_ADDR addr6; // IPv6 地址的暂存区域

    if (inet_pton(AF_INET, ipStr, &addr4) != 0) {
        // 输入是 IPv4 地址
        qDebug() << "It's IPv4";
        targetIPAddress.ss_family = AF_INET;
        (*(sockaddr_in*)&targetIPAddress).sin_addr = addr4;
    } else if (inet_pton(AF_INET6, ipStr, &addr6) != 0) {
        // 输入是 IPv6 地址
        qDebug() << "It's IPv6";
        targetIPAddress.ss_family = AF_INET6;
        (*(sockaddr_in6*)&targetIPAddress).sin6_addr = addr6;
    } else {
        // 什么都不是
        return false;
    }

    return true;
}

char * PrintIPAddress(sockaddr_storage & targetIPAddress) {

    char * printIPAddress = NULL;
    switch(targetIPAddress.ss_family) {
    case AF_INET:
        // 是 IPv4
        printIPAddress = new char(INET_ADDRSTRLEN);
        ZeroMemory(printIPAddress, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(*(sockaddr_in*)&targetIPAddress).sin_addr, printIPAddress, INET_ADDRSTRLEN);
        break;
    case AF_INET6:
        // 是 IPv6
        printIPAddress = new char(INET6_ADDRSTRLEN);
        ZeroMemory(printIPAddress, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &(*(sockaddr_in6*)&targetIPAddress).sin6_addr, printIPAddress, INET6_ADDRSTRLEN);
        break;
    }

    return printIPAddress;
}
