#ifndef TR_THREAD_H
#define TR_THREAD_H

#include <QThread>
#include <QThreadPool>
#include <QRunnable>

#include "tr_utils.h"
#include "ipdb.h"

class TRTWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit TRTWorker();
    ~TRTWorker();

public: // 共享变量区
    // 当前包的 TTL 值
    int iTTL;

    // （当前仅 IPv6 有效）发出请求的本机 Socket 地址
    sockaddr_storage * sourceIPAddress;

    // 目标主机地址
    sockaddr_storage * targetIPAddress;

    // 定义动态链接库句柄
    HANDLE hIcmp;
    HANDLE hIcmp6;

    // 用于读取 IP 对应数据的类操作接口
    IPDB * ipdb;

protected:
    void run() override;

private: // 私有变量区
    bool isStopping; // 是否中止
    bool isIPValid;  // IP 结果是否有效

    sockaddr_storage * currentHopIPAddress; // 初始化时创建，给父进程传参使用，所以需要在父进程使用完成后才能删除

private: // 成员函数区
    void GetIPv4();        // 第一步：得到目标 IP
    void GetIPv6();        // 第一步：得到目标 IP
    void GetInfo();      // 第二步：得到 IP 对应的数据库信息
    void GetHostname(); // 第三步：得到主机名

signals:
    // 回报信息
    void reportIPAndTimeConsumption(const int ttl, const unsigned long timeConsumption, const QString & ipAddress, const bool isValid, const bool isTargetHost);
    void reportInformation(
        const int ttl,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );
    void reportHostname(const int ttl, const QString & hostname);

    // 增加进度
    void incProgress(const int progress);

    // 报告完成
    void fin(const int ttl);

public slots:
    void requestStop();

};

class TRThread : public QThread
{
    Q_OBJECT

public:
    explicit TRThread();
    ~TRThread();

public: // 共享变量区
    QString hostname;   // 用户输入的主机名，需要这个作为传入参数

private: // 私有变量区

    // 一种数据结构。这个结构被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据
    WSADATA wsa;

    // 定义动态链接库
    HMODULE hIcmpDll;

    // 定义 3 个 icmp.dll 的函数指针
    lpIcmpCreateFile  IcmpCreateFile;
    lpIcmpCreateFile  Icmp6CreateFile;
    lpIcmpCloseHandle IcmpCloseHandle;

    // 定义动态链接库句柄
    HANDLE hIcmp;
    HANDLE hIcmp6;

    // 用于读取 IP 对应数据的类操作接口
    IPDB * ipdb;

    // 追踪线程的线程池
    QThreadPool * tracingPool;

    // 追踪线程的子线程组
    TRTWorker * workers[DEF_MAX_HOP];

    // 当前的最大跳，超过这一跳的数据都应该被丢弃
    int maxHop;
    int oldMaxHop; // 旧的最大跳，用空间换时间（这样可以把更新操作放到最后了）

    bool isStopping; // 是否中止

private: // 工具函数
    bool TRThread::parseIPAddress(const char * ipStr, sockaddr_storage & targetHostIPAddress);

protected:
    void run() override;

signals:
    // 填充表格
    void setIPAndTimeConsumption(const int ttl, const QString & timeComnsumption, const QString & ipAddress);
    void setInformation(
        const int ttl,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );
    void setHostname(const int ttl, const QString & hostname);

    void end(const bool isSucceeded);

    // 变更 UI 组件
    void setMessage(const QString &msg);
    void incProgress(const int progress = 1);

public slots:
    void requestStop();

};

#endif // TR_THREAD_H
