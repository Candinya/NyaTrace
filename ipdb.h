#ifndef IPDB_H
#define IPDB_H

#include <QString>

#include "maxminddb.h"

// IPDB 类：操作基于 MMDB 的数据读取工作，
//         以后可能会改成基于 web 请求的格式以优化封装体积
class IPDB {
// 构造与销毁
public:
    IPDB();
    ~IPDB();

// 私有变量
private:
    // 用于获得地址的 MMDB 数据库操作对象
    MMDB_s CityDB;
    MMDB_s ISPDB;

// 私有工具函数
    char * strndup(const char *str, size_t n);

// 公共接口
public:
    bool LookUpIPCityInfo(
        const char * ip_address,
        QString & cityName,
        QString & countryName,
        double  & latitude,
        double  & longitude,
        bool    & isLocationValid
    );

    bool LookUpIPISPInfo(
        const char * ip_address,
        QString & isp,
        QString & org,
        uint    & asn,
        QString & asOrg
    );
};

#endif // IPDB_H
