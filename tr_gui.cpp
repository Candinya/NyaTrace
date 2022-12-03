#include "tr_gui.h"
#include "ui_tr_gui.h"

#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "tr_utils.h"
#include "tr_thread.h"

using namespace std;

// 用于路由追踪的子线程
TRThread * tracingThread;

TR_GUI::TR_GUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TR_GUI)
{
    ui->setupUi(this);

    // 初始化结果数据模型
    hopResultsModel = new QStandardItemModel();

    // 结果表调用模型
    ui->hopsTable->setModel(hopResultsModel);
    ui->hopsTable->show();

    // 初始化 UI
    Initialize();
    CleanUp();

    // 确认当前线程
    cout << "Main thread: " << QThread::currentThread() << endl;

    // 建立路由追踪线程
    tracingThread = new TRThread;

    connect(tracingThread, &TRThread::setHop, this, [=](
        const int ttl, const QString & timeComnsumption, const QString & ipAddress,
        const QString & cityName, const QString & countryName, const double latitude, const double longitude, const bool isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    ) {
        // 更新进度
        ui->tracingProgress->setValue(ttl);

        // 填充表格
        hopResultsModel->insertRow(ttl-1);

        hopResultsModel->setItem(ttl-1, 0, new QStandardItem(timeComnsumption));
        hopResultsModel->setItem(ttl-1, 1, new QStandardItem(ipAddress));

        hopResultsModel->setItem(ttl-1, 2, new QStandardItem(cityName));
        hopResultsModel->setItem(ttl-1, 3, new QStandardItem(countryName));

        if (isLocationValid) {
            // 仅在有效的情况下设置纬度和经度数据（之后为地图服务）
            hopResultsModel->setItem(ttl-1, 4, new QStandardItem(QString("%1").arg(latitude)));
            hopResultsModel->setItem(ttl-1, 5, new QStandardItem(QString("%1").arg(longitude)));
        }

        hopResultsModel->setItem(ttl-1, 6, new QStandardItem(isp));
        hopResultsModel->setItem(ttl-1, 7, new QStandardItem(org));

        if (asn != 0) {
            // 仅在有效的情况下设置 ASN 数据
            hopResultsModel->setItem(ttl-1, 8, new QStandardItem(QString("AS %1").arg(asn)));
        }

        hopResultsModel->setItem(ttl-1, 9, new QStandardItem(asOrg));
    });

    connect(tracingThread, &TRThread::setMessage, this, [=](QString msg) {
        // 更新信息
        ui->statusbar->showMessage(msg);
    });

    connect(tracingThread, &TRThread::finished, this, [=]() {
        // 完成追踪
        CleanUp();
    });
}

TR_GUI::~TR_GUI()
{
    delete ui;
    delete tracingThread;
}


void TR_GUI::on_startStopButton_clicked()
{

    if (tracingThread->isRunning()) {
        // 中止按钮
        cout << "Tracing process is running, abort it..." << endl;
        AbortTracing();
    } else {
        cout << "Tracing process is not running, starting it..." << endl;
        // 开始按钮
        StartTracing();
    }

}

void TR_GUI::Initialize() {
    // UI 相关初始化
    ui->startStopButton->setText("中止"); // 锁定按钮
    ui->hostInput->setDisabled(true);    // 锁定输入框
    ui->statusbar->showMessage("正在初始化..."); // 提示初始化信息

    // 清理表格数据
    hopResultsModel->clear();

    // 构建表头
    QStringList hopResultLables = QObject::trUtf8("时间,地址,城市,国,纬度,经度,ISP,组织,ASN,AS组织").simplified().split(",");
    hopResultsModel->setHorizontalHeaderLabels(hopResultLables);

    // 重置进度条
    ui->tracingProgress->setMaximum(DEF_MAX_HOP);
    ui->tracingProgress->setValue(0);

    // 更新状态
     ui->statusbar->showMessage("就绪"); // 提示初始化信息
}

void TR_GUI::StartTracing() {
    // 初始化
    Initialize();

    // 设置主机地址
    string hostStdString = ui->hostInput->text().toStdString();
    tracingThread->hostname = hostStdString.c_str();

    // 开始追踪
    tracingThread->start();
}

void TR_GUI::AbortTracing() {
    // 中止追踪进程
    //tracingThread->terminate();
    tracingThread->isStopping = true; // 正在中止进程
    ui->startStopButton->setDisabled(true); // 禁用按钮以防止多次触发

    // 设置提示信息
    ui->statusbar->showMessage("正在回收最后一包...");
}

void TR_GUI::CleanUp() {
    // UI 相关结束
    ui->tracingProgress->setValue(ui->tracingProgress->maximum()); // 完成进度条
    ui->startStopButton->setDisabled(false); // 解锁按钮
    ui->startStopButton->setText("开始"); // 设置功能提示
    ui->hostInput->setDisabled(false);   // 解锁输入框
}


void TR_GUI::on_hostInput_returnPressed()
{
    // 按下回车，启动追踪
    StartTracing();
}

