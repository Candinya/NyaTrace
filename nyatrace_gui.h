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
    void on_startStopButton_clicked();
    void on_hostInput_returnPressed();

    void on_hopsTable_clicked(const QModelIndex &index);

    void on_resolveTable_clicked(const QModelIndex &index);

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
    QStandardItemModel * hopResultsModel;
    QStandardItemModel * resolveResultsModel;

    // 用于读取 IP 对应数据的类操作接口
    IPDB * ipdb;

    // 用于存储数据的数组
    struct {
        bool isValid;
        double latitude;
        double longitude;
        unsigned short accuracyRadius;
    } geoInfo[DEF_MAX_HOP];

    // 开始时间计时器
    clock_t startTime;

    // 当前工作模式
    int workMode;

    // 成员函数
    void ConnectTracingResults();
    void ConnectResolveResults();
    void Initialize();
    void StartTracing();
    void AbortTracing();
    void CleanUp(const bool isSucceeded);

};

#endif // NYATRACE_GUI_H
