#include "ipdb.h"

#include "mmdb_settings.h"

#include <iostream>
#include <direct.h>

using namespace std;

IPDB::IPDB() { // 构造函数
    // 输出当前工作目录
    cout << "Now working in: " << getcwd(NULL, 0) << endl;

    // 初始化 MMDB 数据库操作对象
    int openDatabaseStatus;
    openDatabaseStatus = MMDB_open(GEOIP2_CITY_MMDB, MMDB_MODE_MMAP, &CityDB);
    if (openDatabaseStatus != MMDB_SUCCESS) {
        // Open failed
        cerr << "Failed to open GeoIP2 City database from " << GEOIP2_CITY_MMDB
             << " with error: " << MMDB_strerror(openDatabaseStatus)
             << endl;
    }
    openDatabaseStatus = MMDB_open(GEOIP2_ISP_MMDB, MMDB_MODE_MMAP, &ISPDB);
    if (openDatabaseStatus != MMDB_SUCCESS) {
        // Open failed
        cerr << "Failed to open GeoIP2 ISP database from " << GEOIP2_ISP_MMDB
             << " with error: " << MMDB_strerror(openDatabaseStatus)
             << endl;
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
    const char * ip_address,
    QString & cityName,
    QString & countryName,
    double  & latitude,
    double  & longitude,
    bool    & isLocationValid
) {
    int getAddressInfoStatus, mmdbStatus;
    MMDB_lookup_result_s city_result = MMDB_lookup_string(&CityDB, ip_address, &getAddressInfoStatus, &mmdbStatus);
    if (getAddressInfoStatus != 0) {
        // 查询失败，地址无效
        cerr << "Failed to get address info with error: "
             << gai_strerror(getAddressInfoStatus)
             << endl;
        return false;
    }
    if (mmdbStatus != MMDB_SUCCESS) {
        cerr << "Failed to search from database with error: "
             << MMDB_strerror(mmdbStatus)
             << endl;
        return false;
    }

    // 似乎没有遇到大问题
    if (city_result.found_entry) {
//        // ！！！！调试：这里会输出所有的数据，方便调试，实际生产时候请注释掉这些
//        MMDB_entry_data_list_s * cityEntryDataList = NULL;
//        int getEntryDataListStatus = MMDB_get_entry_data_list(&city_result.entry, &cityEntryDataList);
//        if (getEntryDataListStatus != MMDB_SUCCESS) {
//            // 失败了
//            cerr << "Failed to retrieve data with error: "
//                 << MMDB_strerror(getEntryDataListStatus)
//                 << endl;
//        } else {
//            // 打印所有数据
//            MMDB_dump_entry_data_list(stdout, cityEntryDataList, 2);
//        }
//        // ！！！！结束调试

        // 成功查询，接收数据
        MMDB_entry_data_s
            cityEntryData_cityName,
            cityEntryData_countryName,
            cityEntryData_latitude,
            cityEntryData_longitude
        ;
        int getEntryDataStatus;

        // 城市名
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_cityName, "city", "names", GEOIP2_NAME_LANG, NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve city name data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            cityName = QString("未知");
        } else {
            cout << "Get city name successfully." << endl;
            auto cityNameStr = strndup(cityEntryData_cityName.utf8_string, cityEntryData_cityName.data_size);
            cityName = QString(cityNameStr);
            free(cityNameStr);
        }

        // 国名
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_countryName, "country", "names", GEOIP2_NAME_LANG, NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve country name data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            countryName = QString("未知");
        } else {
            cout << "Get country name successfully." << endl;
            auto countryNameStr = strndup(cityEntryData_countryName.utf8_string, cityEntryData_countryName.data_size);
            countryName = QString(countryNameStr);
            free(countryNameStr);
        }

        // 先认为经纬度信息是有效的
        isLocationValid = true;

        // 纬度
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_latitude, "location", "latitude", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve latitude data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            latitude = 0.0;
            // 但其实是无效的
            isLocationValid = false;
        } else {
            cout << "Get latitude successfully: " << cityEntryData_latitude.double_value << endl;
            latitude = cityEntryData_latitude.double_value;
        }

        // 经度
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_longitude, "location", "longitude", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve longitude data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            longitude = 0.0;
            // 但其实是无效的
            isLocationValid = false;
        } else {
            cout << "Get longitude successfully: " << cityEntryData_longitude.double_value << endl;
            longitude = cityEntryData_longitude.double_value;
        }

        return true;


    } else {
        // 查询失败，没找到结果，可能是本地地址
        cerr << "No entry found for IP: "
             << ip_address
             << endl;
        // return false;
        cityName    = QString("私有地址");
        countryName = QString("");
        // 其实是无效的
        isLocationValid = false;
        return true;
    }
}

// 成员函数：在 MMDB 中查询 IP 对应的ISP信息
bool IPDB::LookUpIPISPInfo(
    const char * ip_address,
    QString & isp,
    QString & org,
    uint    & asn,
    QString & asOrg
) {
    int getAddressInfoStatus, mmdbStatus;
    MMDB_lookup_result_s isp_result = MMDB_lookup_string(&ISPDB, ip_address, &getAddressInfoStatus, &mmdbStatus);
    if (getAddressInfoStatus != 0) {
        // 查询失败，地址无效
        cerr << "Failed to get address info with error: "
             << gai_strerror(getAddressInfoStatus)
             << endl;
        return false;
    }
    if (mmdbStatus != MMDB_SUCCESS) {
        cerr << "Failed to search from database with error: "
             << MMDB_strerror(mmdbStatus)
             << endl;
        return false;
    }

    // 似乎没有遇到大问题
    if (isp_result.found_entry) {
//        // ！！！！调试：这里会输出所有的数据，方便调试，实际生产时候请注释掉这些
//        MMDB_entry_data_list_s * ispEntryDataList = NULL;
//        int getEntryDataListStatus = MMDB_get_entry_data_list(&isp_result.entry, &ispEntryDataList);
//        if (getEntryDataListStatus != MMDB_SUCCESS) {
//            // 失败了
//            cerr << "Failed to retrieve data with error: "
//                 << MMDB_strerror(getEntryDataListStatus)
//                 << endl;
//        } else {
//            // 打印所有数据
//            MMDB_dump_entry_data_list(stdout, ispEntryDataList, 2);
//        }
//        // ！！！！结束调试

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
            cerr << "Failed to retrieve isp name data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            isp = QString("未知");
        } else {
            cout << "Get isp name successfully." << endl;
            auto ispStr = strndup(ispEntryData_isp.utf8_string, ispEntryData_isp.data_size);
            isp = QString(ispStr);
            free(ispStr);
        }

        // 组织
        getEntryDataStatus = MMDB_get_value(&isp_result.entry, &ispEntryData_org, "organization", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve organization data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            org = QString("未知");
        } else {
            cout << "Get organization successfully." << endl;
            auto orgStr = strndup(ispEntryData_org.utf8_string, ispEntryData_org.data_size);
            org = QString(orgStr);
            free(orgStr);
        }

        // ASN
        getEntryDataStatus = MMDB_get_value(&isp_result.entry, &ispEntryData_asn, "autonomous_system_number", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve autonomous system number data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            asn = 0;
        } else {
            cout << "Get autonomous system number successfully." << endl;
            asn = ispEntryData_asn.uint32;
        }

        // AS 组织
        getEntryDataStatus = MMDB_get_value(&isp_result.entry, &ispEntryData_asOrg, "autonomous_system_organization", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve autonomous system organization data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            asOrg = QString("未知");
        } else {
            cout << "Get autonomous system organization successfully." << endl;
            auto asOrgStr = strndup(ispEntryData_asOrg.utf8_string, ispEntryData_asOrg.data_size);
            asOrg = QString(asOrgStr);
            free(asOrgStr);
        }


        return true;
    } else {
        // 查询失败，没找到结果，可能是本地地址
        cerr << "No entry found for IP: "
             << ip_address
             << endl;
        // return false;
        isp = QString("私有地址");
        org  = QString("");
        asOrg   = QString("");
        return true;
    }

}
