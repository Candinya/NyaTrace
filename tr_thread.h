#ifndef TR_THREAD_H
#define TR_THREAD_H

#include <QThread>

class TRThread : public QThread
{
    Q_OBJECT

public:
    TRThread(QObject *parent = nullptr);
    ~TRThread();
    QString hostname;

protected:
    void run() override;

signals:
    void setHop(
        const int ttl, const QString & timeComnsumption, const QString & ipAddress,
        const QString & cityName, const QString & countryName, const double latitude, const double longitude,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );
    void setMessage(const QString &msg);

public slots:

};

#endif // TR_THREAD_H
