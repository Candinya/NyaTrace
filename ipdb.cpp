#include "ipdb.h"

#include "ipdb_settings.h"
#include "tracing_utils.h"

#include <direct.h>
#include <QDebug>

IPDB::IPDB() { // 构造函数
    // 输出当前工作目录
    qDebug() << "[IPDB]"
             << "Now working in: "
             << _getcwd(NULL, 0);

    // 初始化 MMDB 数据库操作对象
    int openDatabaseStatus;
    openDatabaseStatus = MMDB_open(GEOIP2_CITY_MMDB, MMDB_MODE_MMAP, &CityDB);
    if (openDatabaseStatus != MMDB_SUCCESS) {
        // Open failed
        qCritical() << "[IPDB]"
                    << QString("Failed to open GeoIP2 City database from %1 with error: %2").arg(GEOIP2_CITY_MMDB, MMDB_strerror(openDatabaseStatus));
    }
    openDatabaseStatus = MMDB_open(GEOIP2_ISP_MMDB, MMDB_MODE_MMAP, &ISPDB);
    if (openDatabaseStatus != MMDB_SUCCESS) {
        // Open failed
        qCritical() << "[IPDB]"
                    << QString("Failed to open GeoIP2 ISP database from %1 with error: %2").arg(GEOIP2_ISP_MMDB, MMDB_strerror(openDatabaseStatus));
    }

}

IPDB::~IPDB() { // 销毁函数
    // 关闭 MMDB
    MMDB_close(&CityDB);
    MMDB_close(&ISPDB);

}

// 工具函数：复制指定长度的字符串
// 我就纳闷了为什么直接用 "maxminddb-compat-util.h" 里面的会报指针类型错误，
// 是因为 C++ 调用了 C ，您可能是升级版的受害者么
char * IPDB::strndup(const char *str, size_t n) {
    size_t len;
    char * copy;

    len = strnlen(str, n);
    if ((copy = (char*)malloc(len + 1)) == NULL)
        return (NULL);
    memcpy(copy, str, len);
    copy[len] = '\0';
    return (copy);
}


// 成员函数：在 MMDB 中查询 IP 对应的城市信息
bool IPDB::LookUpIPCityInfo(
    const sockaddr * ip_address,
    QString  & cityName,
    QString  & countryName,
    double   & latitude,
    double   & longitude,
    uint16_t & accuracyRadius,
    bool     & isLocationValid
) {
    int mmdbStatus;

    MMDB_lookup_result_s city_result = MMDB_lookup_sockaddr(&CityDB, ip_address, &mmdbStatus);
    if (mmdbStatus != MMDB_SUCCESS) {
        qWarning() << "[IPDB]"
                   << "Failed to search from City database with error: "
                   << MMDB_strerror(mmdbStatus);
        return false;
    }

    // 似乎没有遇到大问题
    if (city_result.found_entry) {
#ifdef IPDB_PRINT_ALL
        // ！！！！调试：这里会输出所有的数据，方便调试，实际生产时候请注释掉这些
        MMDB_entry_data_list_s * cityEntryDataList = NULL;
        int getEntryDataListStatus = MMDB_get_entry_data_list(&city_result.entry, &cityEntryDataList);
        if (getEntryDataListStatus != MMDB_SUCCESS) {
            // 失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve data with error: "
                       << MMDB_strerror(getEntryDataListStatus);
        } else {
            // 打印所有数据
            MMDB_dump_entry_data_list(stdout, cityEntryDataList, 2);
        }
        // ！！！！结束调试
#endif
        // 成功查询，接收数据
        MMDB_entry_data_s
            cityEntryData_cityName,
            cityEntryData_countryName,
            cityEntryData_latitude,
            cityEntryData_longitude,
            cityEntryData_accuracyRadius
        ;
        int getEntryDataStatus;

        // 城市名
        if ((getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_cityName, "city", "names", "zh-CN", NULL)) == MMDB_SUCCESS) {
            // 获得中文名
            qDebug() << "[IPDB]"
                     << "Get city zh-CN name successfully.";
            auto cityNameStr = strndup(cityEntryData_cityName.utf8_string, cityEntryData_cityName.data_size);
            cityName = QString(cityNameStr);
            free(cityNameStr);
        } else if ((getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_cityName, "city", "names", "en", NULL)) == MMDB_SUCCESS) {
            // 获得英文名
            qDebug() << "[IPDB]"
                     << "Get city en name successfully.";
            auto cityNameStr = strndup(cityEntryData_cityName.utf8_string, cityEntryData_cityName.data_size);
            cityName = QString(cityNameStr);
            free(cityNameStr);
        } else {
            // 失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve city name data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            cityName = QString("未知");
        }

        // 国名
        if ((getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_countryName, "country", "names", "zh-CN", NULL)) == MMDB_SUCCESS) {
            // 获得中文名
            qDebug() << "[IPDB]"
                     << "Get country zh-CN name successfully.";
            auto countryNameStr = strndup(cityEntryData_countryName.utf8_string, cityEntryData_countryName.data_size);
            countryName = QString(countryNameStr);
            free(countryNameStr);
        } else if ((getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_countryName, "country", "names", "en", NULL)) == MMDB_SUCCESS) {
            // 获得英文名
            qDebug() << "[IPDB]"
                     << "Get country en name successfully.";
            auto countryNameStr = strndup(cityEntryData_countryName.utf8_string, cityEntryData_countryName.data_size);
            countryName = QString(countryNameStr);
            free(countryNameStr);
        } else {
            // 失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve country name data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            countryName = QString("未知");
        }

        // 先认为经纬度信息是有效的
        isLocationValid = true;

        // 纬度
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_latitude, "location", "latitude", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve latitude data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            latitude = 0.0;
            // 但其实是无效的
            isLocationValid = false;
        } else {
            qDebug() << "[IPDB]"
                     << "Get latitude successfully: "
                     << cityEntryData_latitude.double_value;
            latitude = cityEntryData_latitude.double_value;
        }

        // 经度
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_longitude, "location", "longitude", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve longitude data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            longitude = 0.0;
            // 但其实是无效的
            isLocationValid = false;
        } else {
            qDebug() << "[IPDB]"
                     << "Get longitude successfully: "
                     << cityEntryData_longitude.double_value;
            longitude = cityEntryData_longitude.double_value;
        }

        // 准确半径
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_accuracyRadius, "location", "accuracy_radius", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve accuracy_radius data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            accuracyRadius = 0.0;
            // 但其实是无效的
            isLocationValid = false;
        } else {
            qDebug() << "[IPDB]"
                     << "Get accuracy_radius successfully: "
                     << cityEntryData_accuracyRadius.uint16;
            accuracyRadius = cityEntryData_accuracyRadius.uint16;
        }

    } else {
        // 查询失败，没找到结果，可能是本地地址
        char ipAddressPrintBuf[INET6_ADDRSTRLEN];
        PrintIPAddress((sockaddr_storage *)ip_address, ipAddressPrintBuf);
        qWarning() << "[IPDB]"
                   << "No City entry found for IP:"
                   << ipAddressPrintBuf;
        cityName    = QString("私有地址");
        countryName = QString("");
        // 其实是无效的
        isLocationValid = false;
    }

    return true;
}

// 成员函数：在 MMDB 中查询 IP 对应的ISP信息
bool IPDB::LookUpIPISPInfo(
    const sockaddr * ip_address,
    QString & isp,
    QString & org,
    uint    & asn,
    QString & asOrg
) {
    int mmdbStatus;
    MMDB_lookup_result_s isp_result = MMDB_lookup_sockaddr(&ISPDB, ip_address, &mmdbStatus);
    if (mmdbStatus != MMDB_SUCCESS) {
        qWarning() << "[IPDB]"
                   << "Failed to search from ISP database with error: "
                   << MMDB_strerror(mmdbStatus);
        return false;
    }

    // 似乎没有遇到大问题
    if (isp_result.found_entry) {
#ifdef IPDB_PRINT_ALL
        // ！！！！调试：这里会输出所有的数据，方便调试，实际生产时候请注释掉这些
        MMDB_entry_data_list_s * ispEntryDataList = NULL;
        int getEntryDataListStatus = MMDB_get_entry_data_list(&isp_result.entry, &ispEntryDataList);
        if (getEntryDataListStatus != MMDB_SUCCESS) {
            // 失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve data with error: "
                       << MMDB_strerror(getEntryDataListStatus);
        } else {
            // 打印所有数据
            MMDB_dump_entry_data_list(stdout, ispEntryDataList, 2);
        }
        // ！！！！结束调试
#endif
        // 成功查询，接收数据
        MMDB_entry_data_s
            ispEntryData_isp,
            ispEntryData_org,
            ispEntryData_asn,
            ispEntryData_asOrg
        ;
        int getEntryDataStatus;

        // ISP 名字
        getEntryDataStatus = MMDB_get_value(&isp_result.entry, &ispEntryData_isp, "isp", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve isp name data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            isp = QString("未知");
        } else {
            qDebug() << "[IPDB]"
                     << "Get isp name successfully.";
            auto ispStr = strndup(ispEntryData_isp.utf8_string, ispEntryData_isp.data_size);
            isp = QString(ispStr);
            free(ispStr);
        }

        // 组织
        getEntryDataStatus = MMDB_get_value(&isp_result.entry, &ispEntryData_org, "organization", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve organization data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            org = QString("未知");
        } else {
            qDebug() << "[IPDB]"
                     << "Get organization successfully.";
            auto orgStr = strndup(ispEntryData_org.utf8_string, ispEntryData_org.data_size);
            org = QString(orgStr);
            free(orgStr);
        }

        // ASN
        getEntryDataStatus = MMDB_get_value(&isp_result.entry, &ispEntryData_asn, "autonomous_system_number", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve autonomous system number data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            asn = 0;
        } else {
            qDebug() << "[IPDB]"
                     << "Get autonomous system number successfully.";
            asn = ispEntryData_asn.uint32;
        }

        // AS 组织
        getEntryDataStatus = MMDB_get_value(&isp_result.entry, &ispEntryData_asOrg, "autonomous_system_organization", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            qWarning() << "[IPDB]"
                       << "Failed to retrieve autonomous system organization data with error: "
                       << MMDB_strerror(getEntryDataStatus);
            asOrg = QString("未知");
        } else {
            qDebug() << "[IPDB]"
                     << "Get autonomous system organization successfully.";
            auto asOrgStr = strndup(ispEntryData_asOrg.utf8_string, ispEntryData_asOrg.data_size);
            asOrg = QString(asOrgStr);
            free(asOrgStr);
        }

    } else {
        // 查询失败，没找到结果，可能是本地地址
        char ipAddressPrintBuf[INET6_ADDRSTRLEN];
        PrintIPAddress((sockaddr_storage *)ip_address, ipAddressPrintBuf);
        qWarning() << "[IPDB]"
                   << "No ISP entry found for IP:"
                   << ipAddressPrintBuf;
        isp = QString("私有地址");
        org  = QString("");
        asOrg   = QString("");
    }

    return true;
}
