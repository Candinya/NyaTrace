#ifndef TR_THREAD_H
#define TR_THREAD_H

#include <QThread>
#include <QRunnable>
#include <QMutex>

#include "tr_utils.h"
#include "ipdb.h"

class TRTWorker : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit TRTWorker(QObject *parent = nullptr);
    ~TRTWorker();

public: // 共享变量区
    int iTTL;        // 当前包的 TTL 值
    int maxTry;      // 最大重试次数
    u_long ulDestIP; // 目标的 IP 地址

    // 用于发送 ICMP ECHO 的调用程序入口地址
    lpIcmpSendEcho IcmpSendEcho;

    // 定义动态链接库句柄
    HANDLE hIcmp;

protected:
    void run() override;

private: // 私有变量区
    bool isStopping; // 是否中止

    // ICMP 包发送缓冲区和接收缓冲区
    IP_OPTION_INFORMATION IpOption;
    char SendData[DEF_ICMP_DATA_SIZE];
    char ReplyBuf[sizeof(ICMP_ECHO_REPLY) + DEF_ICMP_DATA_SIZE];

    // ICMP 回复的结果指针
    PICMP_ECHO_REPLY pEchoReply;

signals:
    void reportHop(
        const int ttl, const unsigned long timeConsumption, const unsigned long ipAddress, const bool isValid
    );
    void fin(const int remainPacks); // 运行结束

public slots:
    void requestStop();

};

class TRThread : public QThread
{
    Q_OBJECT

public:
    explicit TRThread(QObject *parent = nullptr);
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
    lpIcmpCloseHandle IcmpCloseHandle;
    lpIcmpSendEcho    IcmpSendEcho;

    // 定义动态链接库句柄
    HANDLE hIcmp;

    // 用于读取 IP 对应数据的类操作接口
    IPDB * ipdb;

    // 追踪线程的线程池
    QThreadPool * tracingPool;

    // 追踪线程的子线程组
    TRTWorker * workers[DEF_MAX_HOP];

    // 当前的最大跳，超过这一跳的数据都应该被丢弃
    int maxHop;

    // 用于顺序管理最大跳的线程锁
    QMutex * maxHopMutexLock;

protected:
    void run() override;

signals:
    void setHop(
        const int ttl, const QString & timeComnsumption, const QString & ipAddress,
        const QString & cityName, const QString & countryName, const double latitude, const double longitude, const bool isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );
    void setMessage(const QString &msg);
    void incProgress(const int packs = DEF_MAX_TRY);

public slots:
    void requestStop();

};

#endif // TR_THREAD_H
