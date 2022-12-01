#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <direct.h>

#include "tr_utils.h"
#include "tr_thread.h"

#include "maxminddb.h"
//#include "maxminddb-compat-util.h"

#include "mmdb_settings.h"

#pragma comment(lib, "ws2_32")
using namespace std;

// 用于获得地址的 MMDB 数据库操作对象
MMDB_s CityDB;
MMDB_s ISPDB;

TRThread::TRThread(QObject *parent) {
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

TRThread::~TRThread() {
    // 关闭 MMDB
    MMDB_close(&CityDB);
    MMDB_close(&ISPDB);
}

// 工具函数：复制指定长度的字符串
// 我就纳闷了为什么直接用 "maxminddb-compat-util.h" 里面的会报指针类型错误，
// 是因为 C++ 调用了 C ，您可能是升级版的受害者么
char * strndup(const char *str, size_t n) {
    size_t len;
    char * copy;

    len = strnlen(str, n);
    if ((copy = (char*)malloc(len + 1)) == NULL)
        return (NULL);
    memcpy(copy, str, len);
    copy[len] = '\0';
    return (copy);
}


// 工具函数：在 MMDB 中查询 IP 对应的城市信息
bool LookUpIPCityInfo(
    const char * ip_address,
    QString & cityName,
    QString & countryName,
    double  & latitude,
    double  & longitude
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

        // 纬度
        getEntryDataStatus = MMDB_get_value(&city_result.entry, &cityEntryData_latitude, "location", "latitude", NULL);
        if (getEntryDataStatus != MMDB_SUCCESS) {
            // 还是失败了
            cerr << "Failed to retrieve latitude data with error: "
                 << MMDB_strerror(getEntryDataStatus)
                 << endl;
            latitude = 0.0;
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
        return true;
    }
}

// 工具函数：在 MMDB 中查询 IP 对应的ISP信息
bool LookUpIPISPInfo(
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

void TRThread::run() {

    // WinSock2 相关初始化
    WSADATA wsa; // 一种数据结构。这个结构被用来存储被WSAStartup函数调用后返回的Windows Sockets数据
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { // 进行相应的socket库绑定,MAKEWORD(2,2)表示使用WINSOCK2版本
        cerr << "Failed to start winsocks2 library with error: " << WSAGetLastError() << endl;
        emit setMessage(QString("WinSock2 动态链接库初始化失败，错误代码： %1 。").arg(WSAGetLastError())); // 提示信息
        return; // 结束
    }

    // 获得主机名字符串
    string hostStdString = hostname.toStdString();
    const char * hostCharString = hostStdString.c_str();

    cout << "Target host: " << hostCharString << endl;

    // 将命令行参数转换为IP地址
    u_long ulDestIP = inet_addr(hostCharString); // 初始化目标地址，将一个点分十进制的IP转换成一个长整数型数（u_long类型）

    if (ulDestIP == INADDR_NONE) { // INADDR_NONE 是个宏定义，代表IpAddress 无效的IP地址。
        // 转换不成功时按域名解析
        hostent *pHostent = gethostbyname(hostCharString); // 返回对应于给定主机名的包含主机名字和地址信息的hostent结构的指针
        if (pHostent) {
            ulDestIP = (*(in_addr *) pHostent->h_addr).s_addr;//in_addr 用来表示一个32位的IPv4地址.

            // 更新状态
            cout << "Tracing route to " << hostCharString
                 << " [" << inet_ntoa(*(in_addr *) (&ulDestIP)) << "] "
                 << "with maximun hops " << DEF_MAX_HOP
                 << endl;

            emit setMessage(
                QString("开始追踪路由 %1 [%2] ，最大跃点数为 %3 。")
                   .arg(hostCharString, inet_ntoa(*(in_addr *) (&ulDestIP)))
                   .arg(DEF_MAX_HOP)
            );

        } else { // 主机名解析失败
            cerr << "Failed to resolve host with error: " << WSAGetLastError() << endl;
            emit setMessage(QString("主机名解析失败，错误代码： %1 。").arg(WSAGetLastError()));
            WSACleanup();// 终止 Winsock 2 DLL (Ws2_32.dll) 的使用
            return; // 结束
        }
    } else {
        // 更新状态
        cout << "Tracing route to " << hostCharString
             << " with maximun hops " << DEF_MAX_HOP
             << endl;
        emit setMessage(
            QString("开始追踪路由 %1 ，最大跃点数为 %2 。")
               .arg(hostCharString)
               .arg(DEF_MAX_HOP)
        );
    }

    cout << "Target IP: " << inet_ntoa(*(in_addr *) (&ulDestIP)) << endl;

    sockaddr_in destSockAddr;// 用来处理网络通信的地址
    ZeroMemory(&destSockAddr, sizeof(sockaddr_in)); // 用0来填充一块内存区域
    destSockAddr.sin_family = AF_INET; //sin_family： 地址族，协议簇 AF_INET（TCP/IP – IPv4）
    destSockAddr.sin_addr.s_addr = ulDestIP; //s_addr： 32位IPv4地址

    // 使用 ICMP 协议创建 Raw Socket
    SOCKET sockRaw = WSASocket(
        AF_INET, SOCK_RAW,     // 套接字类型：原始套接字
        IPPROTO_ICMP, NULL, 0, // IPPROTO_ICMP表示ICMP报头由程序构造
        WSA_FLAG_OVERLAPPED    // iFlags：套接口属性描述
    );
    if (sockRaw == INVALID_SOCKET) { //无效套接字
        cerr << "Failed to create socket with error: " << WSAGetLastError() << endl;
        emit setMessage(QString("套接字创建失败，错误代码： %1 。").arg(WSAGetLastError()));
        WSACleanup();// 终止Winsock 2 DLL (Ws2_32.dll) 的使用
        return; // 结束
    }

    // 设置套接字属性
    int iTimeout = DEF_ICMP_TIMEOUT; // 设置超时
    if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char *) &iTimeout, sizeof(iTimeout)) == SOCKET_ERROR) {
        // 设置与某个套接字关联的选项。选项可能存在于多层协议中,为了操作套接字层的选项，应该将层的值指定为SOL_SOCKET
        // 将要被设置或者获取选项的套接字,选项所在的协议层,需要访问的选项名(套接字的操作都自动带有超时时间),指向包含新选项值的缓冲,现选项的长度

        cerr << "Failed to set socket timed out with error: " << WSAGetLastError() << endl;
        emit setMessage(QString("套接字超时设置失败，错误代码： %1 。").arg(WSAGetLastError()));
        closesocket(sockRaw);
        WSACleanup();
        return;
    }

    // 创建ICMP包发送缓冲区和接收缓冲区
    char IcmpSendBuf[sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE];
    memset(IcmpSendBuf, 0, sizeof(IcmpSendBuf));
    char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
    memset(IcmpRecvBuf, 0, sizeof(IcmpRecvBuf));

    // 填充待发送的ICMP包（头和数据部分）
    ICMP_HEADER *pIcmpHeader = (ICMP_HEADER *) IcmpSendBuf;
    pIcmpHeader->type = ICMP_ECHO_REQUEST;
    pIcmpHeader->code = 0;
    pIcmpHeader->id = (USHORT) GetCurrentProcessId();
    memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE); // 数据部分填充

    // 准备结果存储
    DECODE_RESULT stDecodeResult;
    BOOL bReachDestHost = FALSE;
    USHORT usSeqNo = 0;

    // 开始探测路由，当达到最大跳数或收到来自目标主机的报文时结束
    for (int iTTL = 1; (iTTL < DEF_MAX_HOP) && !bReachDestHost; iTTL++) {
        cout << "TTL: " << iTTL << endl;

        // 设置IP数据报头的ttl字段
        setsockopt(
            sockRaw, IPPROTO_IP, IP_TTL, (char *) &iTTL,
            sizeof(iTTL)
        ); // 为 0 （IPPROTO_IP) 的 raw socket 。用于接收任何的IP数据包。其中的校验和和协议分析由程序自己完成。

        // 填充表格中关于当前跳数的序号项
        //hopResultsModel->setItem(iTTL-1, 0, new QStandardItem(QString("%1").arg(iTTL)));

        // 填充 ICMP 数据报文的剩余字段
        ((ICMP_HEADER *) IcmpSendBuf)->cksum = 0;
        ((ICMP_HEADER *) IcmpSendBuf)->seq = htons(usSeqNo++); // 将无符号短整型主机字节序转换为网络字节序,将一个数的高低位互换, (如:12 34 --> 34 12)
        ((ICMP_HEADER *) IcmpSendBuf)->cksum = GenerateChecksum((USHORT *) IcmpSendBuf, sizeof(ICMP_HEADER) + DEF_ICMP_DATA_SIZE);

        // 记录序列号和当前时间
        stDecodeResult.usSeqNo = ((ICMP_HEADER *) IcmpSendBuf)->seq;
        stDecodeResult.dwRoundTripTime = GetTickCount(); // 返回从操作系统启动到当前所经过的毫秒数，常常用来判断某个方法执行的时间

        // 发送ICMP的EchoRequest数据报
        if (sendto(sockRaw, IcmpSendBuf, sizeof(IcmpSendBuf), 0, (sockaddr *) &destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR) {
            // 如果目的主机不可达则直接退出
            if (WSAGetLastError() == WSAEHOSTUNREACH) {
                cerr << "Failed to send package with error: " << WSAGetLastError() << endl;
                emit setMessage(QString("数据报文发送失败，错误代码： %1 。").arg(WSAGetLastError()));
            }
            closesocket(sockRaw);
            WSACleanup();
            return;
        }

        // 接收ICMP的EchoReply数据报
        // 因为收到的可能并非程序所期待的数据报，所以需要循环接收直到收到所要数据或超时
        sockaddr_in from;
        int iFromLen = sizeof(from);
        int iReadDataLen;

        // 准备用于返回的结果
        QString timeConsumption;
        QString ipAddress;

        // 准备从 City 数据库中查询结果
        QString cityName;
        QString countryName;
        double  latitude  = 0.0;
        double  longitude = 0.0;

        // 准备从 ISP 数据库中查询结果
        QString isp;
        QString org;
        uint    asn = 0;
        QString asOrg;


        while (true) {
            // 等待数据到达
            cout << "Waiting for data..." << endl;
            iReadDataLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (sockaddr *) &from, &iFromLen);
            if (iReadDataLen != SOCKET_ERROR) {
                // 有数据包到达
                // 解码得到的数据包，如果解码正确则跳出接收循环发送下一个EchoRequest包
                if (DecodeIcmpResponse(IcmpRecvBuf, iReadDataLen, stDecodeResult)) {

                    cout << "Current hop IP: "<< inet_ntoa(stDecodeResult.dwIPaddr) << " "
                         << "Target IP: "     << inet_ntoa(destSockAddr.sin_addr) << endl;

                    if (stDecodeResult.dwIPaddr.s_addr == destSockAddr.sin_addr.s_addr) {
                        cout << "Current IP match target, final target reached!" << endl;
                        bReachDestHost = TRUE;
                    }

                    // 填充表格中关于当前跳数的耗时
                    stDecodeResult.dwRoundTripTime = GetTickCount() - stDecodeResult.dwRoundTripTime;
                    if (stDecodeResult.dwRoundTripTime) {
                        timeConsumption = QString("%1 毫秒").arg(stDecodeResult.dwRoundTripTime);
                    } else {
                        timeConsumption = QString("小于 1 毫秒");
                    }

                    // 填充表格中关于当前跳数的地址
                    ipAddress = QString("%1").arg(inet_ntoa(stDecodeResult.dwIPaddr));

                    // 在 City 数据库中查询当前 IP 对应信息
                    if (!LookUpIPCityInfo(
                        inet_ntoa(stDecodeResult.dwIPaddr),
                        cityName,
                        countryName,
                        latitude,
                        longitude
                    )) {
                        // 查询失败，使用填充字符
                        cityName    = QString("未知");
                        countryName = QString("");
                    }

                    // 在 ISP 数据库中查询当前 IP 对应信息
                    if (!LookUpIPISPInfo(
                        inet_ntoa(stDecodeResult.dwIPaddr),
                        isp,
                        org,
                        asn,
                        asOrg
                    )) {
                        // 查询失败，使用填充字符
                        isp  = QString("未知");
                        org   = QString("");
                        asOrg = QString("未知");
                    }

                    break;
                }
            } else if (WSAGetLastError() == WSAETIMEDOUT) {
                // 填充表格中关于当前跳数的时间
                timeConsumption = QString("请求超时");
                // 其余置空
                ipAddress = QString("");

                cityName = QString("");
                countryName = QString("");

                isp  = QString("");
                org   = QString("");
                asOrg = QString("");
                break;
            } else {
                cerr << "Failed to call recvFrom with error: " << WSAGetLastError() << endl;
                emit setMessage(QString("套接字监听调用失败，错误代码： %1 。").arg(WSAGetLastError()));
                closesocket(sockRaw);
                WSACleanup();
                return;
            }
        }

        // 更新进度条数据
        emit setHop(
            iTTL, timeConsumption, ipAddress,           // 基础信息
            cityName, countryName, latitude, longitude, // GeoIP2 City
            isp, org, asn, asOrg                        // GeoIP2 ISP
        );

    }

    // 追踪完成，更新状态，回收资源
    closesocket(sockRaw);
    WSACleanup();

    cout << "Trace Route finish." << endl;
    emit setMessage("路由追踪完成。"); // 提示完成信息
    emit finish();


}
