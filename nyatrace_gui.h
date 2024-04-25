#ifndef NYATRACE_GUI_H
#define NYATRACE_GUI_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QQuickWidget>

#include "tracing_defs.h"
#include "tracing_core.h"
#include "resolve_core.h"

QT_BEGIN_NAMESPACE
namespace Ui { class NyaTraceGUI; }
QT_END_NAMESPACE

class NyaTraceGUI : public QMainWindow
{
    Q_OBJECT

public:
    NyaTraceGUI(QWidget *parent = nullptr);
    ~NyaTraceGUI();

private slots:
    void on_hostInput_returnPressed();

    void on_resolveButton_clicked();
    void on_startStopTracingButton_clicked();

    void on_resolveTable_clicked(const QModelIndex &index);
    void on_resolveTable_doubleClicked(const QModelIndex &index);
    void on_traceTable_clicked(const QModelIndex &index);


    void on_openLogs_clicked();

private:
    // 界面 UI
    Ui::NyaTraceGUI *ui;

    // 一种数据结构。这个结构被用来存储被 WSAStartup 函数调用后返回的 Windows Sockets 数据
    WSADATA wsa;

    // 用于路由追踪的子线程
    TracingCore * tracingThread;

    // 用于解析的子线程
    ResolveCore * resolveThread;

    // 用于存储结果的模型
    QStandardItemModel * traceResultsModel;
    QStandardItemModel * resolveResultsModel;

    // 当前选中的解析结果 IP
    int currentSelectedIPNo;

    // 用于读取 IP 对应数据的类操作接口
    IPDB * ipdb;

    // 用于存储数据的数组
    struct GeoInfo {
        bool isValid;
        double latitude;
        double longitude;
        unsigned short accuracyRadius;
    };

    GeoInfo resolveGeoInfo[DEF_MAX_IPs];
    GeoInfo traceGeoInfo[DEF_MAX_HOP];

    // 开始时间计时器
    clock_t startTime;

    // 成员函数
    void ConnectResolveResults();
    void ConnectTracingResults();
    void InitializeResolving();
    void InitializeTracing();
    void StartResolving();
    void StartTracing();
    void AbortTracing();
    void CleanUpResolving(const bool isSucceeded);
    void CleanUpTracing(const bool isSucceeded);

};

#endif // NYATRACE_GUI_H
