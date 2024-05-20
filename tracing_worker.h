#ifndef TRACING_WORKER_H
#define TRACING_WORKER_H

#include <QObject>
#include <QRunnable>

#include "tracing_defs.h"
#include "ipdb.h"

class TracingWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit TracingWorker();
    ~TracingWorker();

public: // 共享变量区
    // 当前包的 TTL 值
    int iTTL;

    // （当前仅 IPv6 有效）发出请求的本机 Socket 地址
    sockaddr_storage * sourceIPAddress;

    // 目标主机地址
    sockaddr_storage * targetIPAddress;

#ifdef Q_OS_WIN

    // 定义动态链接库句柄
    HANDLE hIcmp;
    HANDLE hIcmp6;

#endif
#ifdef Q_OS_UNIX

// TODO

#endif

    // 用于读取 IP 对应数据的类操作接口
    IPDB * ipdb;

protected:
    void run() override;

private: // 私有变量区
    bool isStopping; // 是否中止
    bool isIPValid;  // IP 结果是否有效

    sockaddr_storage * currentHopIPAddress; // 初始化时创建，给父进程传参使用，所以需要在父进程使用完成后才能删除

private: // 成员函数区
    void GetIPv4();      // 第一步：得到目标 IP
    void GetIPv6();      // 第一步：得到目标 IP
    void GetInfo();      // 第二步：得到 IP 对应的数据库信息
    void GetHostname(); // 第三步：得到主机名

signals:
    // 回报信息
    void reportIPAndTimeConsumption(const int hop, const unsigned long timeConsumption, const QString & ipAddress, const bool isValid, const bool isTargetHost);
    void reportInformation(
        const int hop,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const unsigned short & accuracyRadius, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );
    void reportHostname(const int hop, const QString & hostname);

    // 增加进度
    void incProgress(const int progress);

    // 报告完成
    void fin(const int hop);

public slots:
    void requestStop();

};

#endif // TRACING_WORKER_H
