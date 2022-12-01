#ifndef TR_THREAD_H
#define TR_THREAD_H

#include <QThread>

class TRThread : public QThread
{
    Q_OBJECT

public:
    TRThread(QObject *parent = nullptr);
    QString hostname;

protected:
    void run() override;

signals:
    void setTTL(const int ttl, const QString &timeComnsumption, const QString &ipAddress);
    void setMessage(const QString &msg);
    void finish();

public slots:

};

#endif // TR_THREAD_H
