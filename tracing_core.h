#ifndef TRACING_CORE_H
#define TRACING_CORE_H

#include <QThread>
#include <QThreadPool>

#include "tracing_worker.h"
#include "configs.h"

class TracingCore : public QThread
{
    Q_OBJECT

public:
    explicit TracingCore();
    ~TracingCore();

public: // 共享变量区
    QString hostname;   // 用户输入的主机名，需要这个作为传入参数
    IPDB * ipdb; // 用于读取 IP 对应数据的类操作接口

private: // 私有变量区

#ifdef Q_OS_WIN

    // 定义动态链接库
    HMODULE hIcmpDll;

    // 定义 3 个 icmp.dll 的函数指针
    lpIcmpCreateFile  IcmpCreateFile;
    lpIcmpCreateFile  Icmp6CreateFile;
    lpIcmpCloseHandle IcmpCloseHandle;

    // 定义动态链接库句柄
    HANDLE hIcmp;
    HANDLE hIcmp6;

#endif
#ifdef Q_OS_UNIX

// TODO

#endif

    // 追踪线程的线程池
    QThreadPool * tracingPool;

    // 追踪线程的子线程组
    TracingWorker * workers[DEF_TRACE_MAX_HOPs];

    // 当前的最大跳，超过这一跳的数据都应该被丢弃
    int maxHop;
    int oldMaxHop; // 旧的最大跳，用空间换时间（这样可以把更新操作放到最后了）

    bool isStopping; // 是否中止

protected:
    void run() override;

signals:
    // 填充表格
    void setIPAndTimeConsumption(const int hop, const QString & timeComnsumption, const QString & ipAddress);
    void setInformation(
        const int hop,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const unsigned short & accuracyRadius, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );
    void setHostname(const int hop, const QString & hostname);

    void end(const bool isSucceeded);

    // 变更 UI 组件
    void setMessage(const QString &msg);
    void incProgress(const int progress = 1);

public slots:
    void requestStop();

};

#endif // TRACING_CORE_H
