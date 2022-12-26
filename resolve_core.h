#ifndef RESOLVECORE_H
#define RESOLVECORE_H

#include <QThread>

#include "ipdb.h"

class ResolveCore : public QThread
{
    Q_OBJECT

public:
    explicit ResolveCore();
    ~ResolveCore();

public: // 共享变量区
    QString hostname;   // 用户输入的主机名，需要这个作为传入参数
    IPDB * ipdb; // 用于读取 IP 对应数据的类操作接口

protected:
    void run() override;

private: // 成员函数区
    void GetInfo(int id, sockaddr_storage * targetIPAddress); // 得到 IP 对应的数据库信息

signals:
    // 填充表格
    void setInformation(
        const int id, const QString & ipAddress,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const unsigned short & accuracyRadius, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    );

    // 解析结束
    void end(const bool isSucceeded);

    // 变更 UI 组件
    void setMessage(const QString &msg);

};

#endif // RESOLVECORE_H
