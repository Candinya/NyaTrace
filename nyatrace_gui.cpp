#include "nyatrace_gui.h"
#include "ui_nyatrace_gui.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <QDebug>
#include <QMetaObject>

NyaTraceGUI::NyaTraceGUI(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NyaTraceGUI)
{
    // 初始化界面 UI
    ui->setupUi(this);

    // 初始化追踪地图 （OSM）
    ui->tracingMap->setSource(QUrl("qrc:/tracing_map.qml"));
    ui->tracingMap->show();

    // 初始化结果数据模型
    hopResultsModel = new QStandardItemModel();

    // 结果表调用模型
    ui->hopsTable->setModel(hopResultsModel);
    ui->hopsTable->show();

    // 设置结果表自动伸展
    ui->hopsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 初始化 UI
    Initialize();
    CleanUp(false);

    // 建立路由追踪线程
    tracingThread = new TracingCore;

    // 填充表格
    connect(tracingThread, &TracingCore::setIPAndTimeConsumption, this, [=](const int hop, const QString & timeComnsumption, const QString & ipAddress) {
        hopResultsModel->setItem(hop-1, 0, new QStandardItem(timeComnsumption));
        hopResultsModel->setItem(hop-1, 1, new QStandardItem(ipAddress));
    });

    connect(tracingThread, &TracingCore::setInformation, this, [=](
        const int hop,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const unsigned short & accuracyRadius, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    ) {
        hopResultsModel->setItem(hop-1, 3, new QStandardItem(cityName));
        hopResultsModel->setItem(hop-1, 4, new QStandardItem(countryName));

        hopResultsModel->setItem(hop-1, 5, new QStandardItem(isp));
        hopResultsModel->setItem(hop-1, 6, new QStandardItem(org));

        if (isLocationValid) {
            // 根据纬度和经度在地图上画一个点和一个圆
            qDebug() << "Hop:"             << hop
                     << "Latitude:"        << latitude
                     << "Longitude:"       << longitude
                     << "Accuracy Radius:" << accuracyRadius
            ;

            QMetaObject::invokeMethod(
                (QObject*)ui->tracingMap->rootObject(),
                "drawHopPoint",
                Qt::DirectConnection,
                Q_ARG(QVariant, latitude),
                Q_ARG(QVariant, longitude),
                Q_ARG(QVariant, accuracyRadius),
                Q_ARG(QVariant, hop),
                Q_ARG(QVariant, QString("第 %1 跳").arg(hop))
            );

            // 存进数组
            hopGeoInfo[hop-1].isValid = true;
            hopGeoInfo[hop-1].latitude = latitude;
            hopGeoInfo[hop-1].longitude = longitude;
            hopGeoInfo[hop-1].accuracyRadius = accuracyRadius;
        }


        if (asn != 0) {
            // 仅在有效的情况下设置 ASN 数据
            hopResultsModel->setItem(hop-1, 7, new QStandardItem(QString("AS %1").arg(asn)));
        }

        hopResultsModel->setItem(hop-1, 8, new QStandardItem(asOrg));
    });

    connect(tracingThread, &TracingCore::setHostname, this, [=](const int hop, const QString & hostName) {
        hopResultsModel->setItem(hop-1, 2, new QStandardItem(hostName));
    });

    // 更新 UI
    connect(tracingThread, &TracingCore::setMessage, this, [=](QString msg) {
        // 更新信息
        ui->statusbar->showMessage(msg);
    });


    connect(tracingThread, &TracingCore::incProgress, this, [=](const int packs) {
        // 完成一跳，进度条更新
        ui->tracingProgress->setValue(ui->tracingProgress->value() + packs);
    });

    connect(tracingThread, &TracingCore::end, this, [=](const bool isSucceeded) {
        // 完成追踪
        CleanUp(isSucceeded);
    });

    // 更新状态
    ui->statusbar->showMessage("就绪"); // 提示初始化信息
}

NyaTraceGUI::~NyaTraceGUI()
{
    // 销毁追踪主线程
    delete tracingThread;

    // 销毁结果模型
    delete hopResultsModel;

    // 销毁主界面
    delete ui;
}


void NyaTraceGUI::on_startStopButton_clicked()
{

    if (tracingThread->isRunning()) {
        // 中止按钮
        qDebug() << "Tracing process is running, abort it...";
        AbortTracing();
    } else {
        qDebug() << "Tracing process is not running, starting it...";
        // 开始按钮
        StartTracing();
    }

}

void NyaTraceGUI::Initialize() {
    // UI 相关初始化
    ui->startStopButton->setText("中止"); // 更新按钮功能提示
    ui->hostInput->setDisabled(true); // 锁定输入框
    ui->statusbar->showMessage("正在初始化..."); // 提示初始化信息

    // 清理表格数据
    hopResultsModel->clear();

    // 构建表头
    QStringList hopResultLables = { "时间", "地址", "主机名", "城市", "国", "ISP", "组织", "ASN", "AS组织" };
    hopResultsModel->setHorizontalHeaderLabels(hopResultLables);

    // 重置进度条
    ui->tracingProgress->setMaximum(DEF_MAX_HOP * 3); // 三种任务，三倍进度
    ui->tracingProgress->setValue(0);

    // 清空地图
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "clearMap",
        Qt::DirectConnection
    );

    // 清空结果数组
    for (int i = 0; i < DEF_MAX_HOP; i++) {
        hopGeoInfo[i].isValid = false;
    }
}

void NyaTraceGUI::StartTracing() {
    // 初始化
    Initialize();

    // 设置提示信息
    ui->statusbar->showMessage("正在开始路由追踪...");

    // 设置主机地址
    std::string hostStdString = ui->hostInput->text().toStdString();
    tracingThread->hostname = hostStdString.c_str();

    // 记录开始时间
    startTime = clock();

    // 开始追踪
    tracingThread->start();
}

void NyaTraceGUI::AbortTracing() {
    // 中止追踪进程
    //tracingThread->terminate();
    tracingThread->requestStop(); // 中止进程
    ui->startStopButton->setDisabled(true); // 禁用按钮以防止多次触发

    // 设置提示信息
    ui->statusbar->showMessage("正在回收最后一包...");
}

void NyaTraceGUI::CleanUp(const bool isSucceeded) {
    // UI 相关结束
    ui->tracingProgress->setValue(ui->tracingProgress->maximum()); // 完成进度条
    ui->startStopButton->setDisabled(false); // 解锁按钮
    ui->startStopButton->setText("开始"); // 设置功能提示
    ui->hostInput->setDisabled(false); // 解锁输入框

    // 记录结束时间
    clock_t endTime = clock();
    auto consumedSeconds = (endTime - startTime) / CLOCKS_PER_SEC;

    qDebug() << "Tracing finished in " << consumedSeconds << " seconds.";

    if (isSucceeded) {
        // 设置提示信息
        ui->statusbar->showMessage(QString("路由追踪完成，耗时 %1 秒。").arg(consumedSeconds));

        // 连线
        for (int i = 0; i < DEF_MAX_HOP; i++) {
            if (hopGeoInfo[i].isValid) {
                qDebug() << "Map: Connecting hop" << i+1;
                QMetaObject::invokeMethod(
                    (QObject*)ui->tracingMap->rootObject(),
                    "connectLine",
                    Qt::DirectConnection,
                    Q_ARG(QVariant, hopGeoInfo[i].latitude),
                    Q_ARG(QVariant, hopGeoInfo[i].longitude)
                );
            }
        }


        // 调整地图
        QMetaObject::invokeMethod(
            (QObject*)ui->tracingMap->rootObject(),
            "fitMap",
            Qt::DirectConnection
        );
    } // 否则失败了，不要去动失败的提示信息
}


void NyaTraceGUI::on_hostInput_returnPressed()
{
    // 按下回车，启动追踪
    StartTracing();
}
