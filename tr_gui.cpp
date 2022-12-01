#include "tr_gui.h"
#include "ui_tr_gui.h"

#include <iostream>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "tr_utils.h"
#include "tr_thread.h"

// #include <QThread>

using namespace std;

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
    tracingThread = new TRThread(this);

    connect(tracingThread, &TRThread::setTTL, this, [=](int ttl, QString timeComnsumption, QString ipAddress) {
        // 更新进度
        ui->tracingProgress->setValue(ttl);

        // 填充表格
        hopResultsModel->insertRow(ttl-1);
        hopResultsModel->setItem(ttl-1, 0, new QStandardItem(QString("%1").arg(ttl)));
        hopResultsModel->setItem(ttl-1, 1, new QStandardItem(timeComnsumption));
        hopResultsModel->setItem(ttl-1, 2, new QStandardItem(ipAddress));
    });

    connect(tracingThread, &TRThread::setMessage, this, [=](QString msg) {
        // 更新信息
        ui->statusbar->showMessage(msg);
    });

    connect(tracingThread, &TRThread::finish, this, [=]() {
        // 完成追踪
        CleanUp();
    });

//    connect(ui->startButton, &QPushButton::clicked, this, [=]() {
//        // 初始化
//        Initialize();

//        // 设置主机地址
//        string hostStdString = ui->hostInput->text().toStdString();
//        tracingThread->hostCharString = hostStdString.c_str();

//        // 开始追踪
//        tracingThread->start();
//    });
}

TR_GUI::~TR_GUI()
{
    delete ui;
    delete tracingThread;
}


void TR_GUI::on_startButton_clicked()
{

    // 初始化
    Initialize();

    // 设置主机地址
    string hostStdString = ui->hostInput->text().toStdString();
    tracingThread->hostname = hostStdString.c_str();

    // 开始追踪
    tracingThread->start();

}

void TR_GUI::Initialize() {
    // UI 相关初始化
    ui->startButton->setDisabled(true); // 锁定按钮
     ui->statusbar->showMessage("正在初始化..."); // 提示初始化信息

    // 清理表格数据
    hopResultsModel->clear();

    // 构建表头
    QStringList hopResultLables = QObject::trUtf8("跳数,时间,地址").simplified().split(",");
    hopResultsModel->setHorizontalHeaderLabels(hopResultLables);

    // 重置进度条
    ui->tracingProgress->setMaximum(DEF_MAX_HOP);
    ui->tracingProgress->setValue(0);

    // 更新状态
     ui->statusbar->showMessage("就绪"); // 提示初始化信息
}

void TR_GUI::CleanUp() {
    // UI 相关结束
    ui->tracingProgress->setValue(ui->tracingProgress->maximum()); // 完成进度条
    ui->startButton->setDisabled(false); // 解锁按钮
}

