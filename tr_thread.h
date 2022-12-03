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

    // 共享变量区
    QString hostname;   // 用户输入的主机名，需要这个作为传入参数
    bool    isStopping; // 对进程发出中止信号，回收最后一包再结束

protected:
    void run() override;

#ifndef SETTING_USE_ICMPDLL

private:
    // 工具函数
    USHORT GenerateChecksum(USHORT* pBuf, int iSize);
    BOOL DecodeIcmpResponse(char* pBuf, int iPacketSize, DECODE_RESULT& stDecodeResult);

#endif

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
