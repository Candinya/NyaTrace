#ifndef TR_THREAD_H
#define TR_THREAD_H

#include <QThread>

#include "tr_utils.h"

class TRThread : public QThread
{
    Q_OBJECT

public:
    TRThread(QObject *parent = nullptr);
    ~TRThread();

public: // 共享变量区
    QString hostname;   // 用户输入的主机名，需要这个作为传入参数
    bool    isStopping; // 对进程发出中止信号，回收最后一包再结束

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

protected:
    void run() override;

signals:
    void setHop(
        const int ttl, const QString & timeComnsumption, const QString & ipAddress,
        const QString & cityName, const QString & countryName, const double latitude, const double longitude, const bool isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );
    void setMessage(const QString &msg);

public slots:

};

#endif // TR_THREAD_H
