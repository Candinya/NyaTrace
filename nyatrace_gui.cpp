#include "nyatrace_gui.h"
#include "ui_nyatrace_gui.h"
#include "mode.h"

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

    // 调整分割线两边的大小
    ui->displaySplitter->setSizes(QList<int>{ 60, 240 });
    ui->traceSplitter->setSizes(QList<int>{ 60, 40 });

    // 初始化追踪地图 （OSM）
    ui->tracingMap->setSource(QUrl("qrc:/tracing_map.qml"));
    ui->tracingMap->show();

    // 初始化结果数据模型
    traceResultsModel = new QStandardItemModel();
    resolveResultsModel = new QStandardItemModel();

    // 结果表调用模型
    ui->traceTable->setModel(traceResultsModel);
    ui->traceTable->show();

    ui->resolveTable->setModel(resolveResultsModel);
    ui->resolveTable->show();

    // 设置结果表自动伸展
    ui->traceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->resolveTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    // 初始化 IPDB 实例
    ipdb = new IPDB;

    // 初始化 UI
    InitializeResolving();
    InitializeTracing();
    CleanUpResolving(false);
    CleanUpTracing(false);

    // WinSock2 相关初始化
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) { // 进行相应的socket库绑定,MAKEWORD(2,2)表示使用WINSOCK2版本
        //emit setMessage(QString("WinSock2 动态链接库初始化失败，错误代码： %1 。").arg(WSAGetLastError())); // 提示信息
        qFatal(QString("Failed to start winsocks2 library with error: %1").arg(WSAGetLastError()).toStdString().c_str());
    }

    // 建立路由追踪线程
    tracingThread = new TracingCore;
    tracingThread->ipdb = ipdb;

    // 建立解析线程
    resolveThread = new ResolveCore;
    resolveThread->ipdb = ipdb;

    // 连接结果
    ConnectTracingResults();
    ConnectResolveResults();

    // 禁用开始追踪按钮
    ui->startStopTracingButton->setDisabled(true);

    // 更新状态
    ui->statusbar->showMessage("就绪"); // 提示初始化信息

    // 把焦点放到输入框上
    ui->hostInput->QWidget::setFocus();
}

NyaTraceGUI::~NyaTraceGUI()
{

    // 销毁追踪主线程
    delete tracingThread;

    // 销毁解析主线程
    delete resolveThread;

    // 终止 Winsock 2 DLL (ws2_32.dll) 的使用
    WSACleanup();

    // 销毁结果模型
    delete traceResultsModel;
    delete resolveResultsModel;

    // 销毁 IPDB 实例
    delete ipdb;

    // 销毁主界面
    delete ui;
}

void NyaTraceGUI::ConnectTracingResults() {
    // 填充追踪结果表格
    connect(tracingThread, &TracingCore::setIPAndTimeConsumption, this, [=](const int hop, const QString & timeComnsumption, const QString & ipAddress) {
        traceResultsModel->setItem(hop-1, 0, new QStandardItem(timeComnsumption));
        traceResultsModel->setItem(hop-1, 1, new QStandardItem(ipAddress));
    });

    connect(tracingThread, &TracingCore::setInformation, this, [=](
        const int hop,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const unsigned short & accuracyRadius, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    ) {
        traceResultsModel->setItem(hop-1, 3, new QStandardItem(cityName));
        traceResultsModel->setItem(hop-1, 4, new QStandardItem(countryName));

        if (isLocationValid) {
            // 记录到表格数据中
            traceResultsModel->setItem(hop-1, 5, new QStandardItem(QString("%1").arg(latitude)));
            traceResultsModel->setItem(hop-1, 6, new QStandardItem(QString("%1").arg(longitude)));
            traceResultsModel->setItem(hop-1, 7, new QStandardItem(QString("%1").arg(accuracyRadius)));

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
                Q_ARG(QVariant, accuracyRadius)
            );

            // 存进数组
            traceGeoInfo[hop-1].isValid = true;
            traceGeoInfo[hop-1].latitude = latitude;
            traceGeoInfo[hop-1].longitude = longitude;
            traceGeoInfo[hop-1].accuracyRadius = accuracyRadius;
        }

        traceResultsModel->setItem(hop-1, 8, new QStandardItem(isp));
        traceResultsModel->setItem(hop-1, 9, new QStandardItem(org));


        if (asn != 0) {
            // 仅在有效的情况下设置 ASN 数据
            traceResultsModel->setItem(hop-1, 10, new QStandardItem(QString("AS %1").arg(asn)));
        }

        traceResultsModel->setItem(hop-1, 11, new QStandardItem(asOrg));
    });

    connect(tracingThread, &TracingCore::setHostname, this, [=](const int hop, const QString & hostName) {
        traceResultsModel->setItem(hop-1, 2, new QStandardItem(hostName));
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
        CleanUpTracing(isSucceeded);
    });
}

void NyaTraceGUI::ConnectResolveResults() {
    // 填充解析结果表格
    connect(resolveThread, &ResolveCore::setInformation, this, [=](
        const int id, const QString & ipAddress,
        const QString & cityName, const QString & countryName, const double & latitude, const double & longitude, const unsigned short & accuracyRadius, const bool & isLocationValid,
        const QString & isp, const QString & org, const uint & asn, const QString & asOrg
    ) {

        resolveResultsModel->setItem(id-1, 0, new QStandardItem(ipAddress));
        resolveResultsModel->setItem(id-1, 1, new QStandardItem(cityName));
        resolveResultsModel->setItem(id-1, 2, new QStandardItem(countryName));

        if (isLocationValid) {
            // 记录到表格数据中
            resolveResultsModel->setItem(id-1, 3, new QStandardItem(QString("%1").arg(latitude)));
            resolveResultsModel->setItem(id-1, 4, new QStandardItem(QString("%1").arg(longitude)));
            resolveResultsModel->setItem(id-1, 5, new QStandardItem(QString("%1").arg(accuracyRadius)));

            // 根据纬度和经度在地图上画一个点和一个圆
            qDebug() << "ID:"              << id
                     << "Latitude:"        << latitude
                     << "Longitude:"       << longitude
                     << "Accuracy Radius:" << accuracyRadius
            ;

            // 存进数组
            resolveGeoInfo[id-1].isValid = true;
            resolveGeoInfo[id-1].latitude = latitude;
            resolveGeoInfo[id-1].longitude = longitude;
            resolveGeoInfo[id-1].accuracyRadius = accuracyRadius;

            QMetaObject::invokeMethod(
                (QObject*)ui->tracingMap->rootObject(),
                "drawHopPoint",
                Qt::DirectConnection,
                Q_ARG(QVariant, latitude),
                Q_ARG(QVariant, longitude),
                Q_ARG(QVariant, accuracyRadius)
            );
        }

        resolveResultsModel->setItem(id-1, 6, new QStandardItem(isp));
        resolveResultsModel->setItem(id-1, 7, new QStandardItem(org));

        if (asn != 0) {
            // 仅在有效的情况下设置 ASN 数据
            resolveResultsModel->setItem(id-1, 8, new QStandardItem(QString("AS %1").arg(asn)));
        }

        resolveResultsModel->setItem(id-1, 9, new QStandardItem(asOrg));

        qDebug() << "ID:" << id << "'s data proceeded.";
    });

    // 更新 UI
    connect(resolveThread, &ResolveCore::setMessage, this, [=](QString msg) {
        // 更新信息
        ui->statusbar->showMessage(msg);
    });

    connect(resolveThread, &ResolveCore::end, this, [=](const bool isSucceeded) {
        // 完成解析
        CleanUpResolving(isSucceeded);

        if (isSucceeded) {
            // 解锁追踪功能
            ui->startStopTracingButton->setDisabled(false);

            if (ui->autoStartTrace->isChecked()) {
                // 开始追踪
                currentSelectedIPNo = 0;
                ui->resolveTable->selectRow(currentSelectedIPNo);
                StartTracing();
            }
        }
    });
}

void NyaTraceGUI::InitializeResolving() {
    // UI 相关初始化
    ui->hostInput->setDisabled(true); // 锁定输入框
    ui->statusbar->showMessage("正在初始化..."); // 提示初始化信息

    // 禁用解析按钮
    ui->resolveButton->setDisabled(true);

    // 禁用开始追踪按钮
    ui->startStopTracingButton->setDisabled(true);

    // 清理表格数据
    resolveResultsModel->clear();

    // 构建解析表头
    QStringList resolveResultLabels = { "地址", "城市", "国", "纬度", "经度", "误差半径", "ISP", "组织", "ASN", "AS组织" };
    resolveResultsModel->setHorizontalHeaderLabels(resolveResultLabels);

    // 清空地图
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "clearMap",
        Qt::DirectConnection
    );

    // 清空结果数组
    for (int i = 0; i < DEF_MAX_IPs; i++) {
        resolveGeoInfo[i].isValid = false;
    }

    qDebug () << "Resolve mode initialize complete.";
}

void NyaTraceGUI::InitializeTracing() {
    // UI 相关初始化
    ui->hostInput->setDisabled(true); // 锁定输入框
    ui->statusbar->showMessage("正在初始化..."); // 提示初始化信息

    // 禁用解析按钮
    ui->resolveButton->setDisabled(true);

    // 禁用解析结果表
    ui->resolveTable->setDisabled(true);

    // 清理表格数据
    traceResultsModel->clear();

    // 构建追踪表头
    QStringList hopResultLables = { "时间", "地址", "主机名", "城市", "国", "纬度", "经度", "误差半径", "ISP", "组织", "ASN", "AS组织" };
    traceResultsModel->setHorizontalHeaderLabels(hopResultLables);

    // 清空地图
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "clearMap",
        Qt::DirectConnection
    );

    // 清空结果数组
    for (int i = 0; i < DEF_MAX_HOP; i++) {
        traceGeoInfo[i].isValid = false;
    }

    // 重置进度条
    ui->tracingProgress->setMaximum(DEF_MAX_HOP * 3); // 三种任务，三倍进度
    ui->tracingProgress->setValue(0);

    qDebug () << "Trace mode initialize complete.";
}

void NyaTraceGUI::StartResolving() {

    // 初始化
    InitializeResolving();

    // 设置主机地址
    std::string hostStdString = ui->hostInput->text().toStdString();

    // 记录开始时间
    startTime = clock();

    // 解析
    ui->statusbar->showMessage("正在解析...");
    resolveThread->hostname = hostStdString.c_str();
    resolveThread->start();

    qDebug () << "Resolving started.";

}

void NyaTraceGUI::StartTracing() {

    qDebug() << "Start route tracing...";

    // 初始化
    InitializeTracing();

    // 设置主机地址
    std::string hostStdString = resolveResultsModel->item(currentSelectedIPNo, 0)->text().toStdString();

    // 记录开始时间
    startTime = clock();

    // 路由追踪
    ui->statusbar->showMessage("正在开始路由追踪...");
    tracingThread->hostname = hostStdString.c_str();
    tracingThread->start();

    // 设置按钮功能
    ui->startStopTracingButton->setText("中止"); // 更新按钮功能提示

    qDebug () << "Tracing started.";

}

void NyaTraceGUI::AbortTracing() {
    // 中止追踪进程
    tracingThread->requestStop(); // 中止进程
    ui->startStopTracingButton->setDisabled(true); // 禁用按钮以防止多次触发

    // 设置提示信息
    ui->statusbar->showMessage("正在回收最后一包...");

    qDebug() << "Abort tracing...";
}

void NyaTraceGUI::CleanUpResolving(const bool isSucceeded) {
    // UI 相关结束
    ui->resolveButton->setDisabled(false); // 解锁按钮
    ui->hostInput->setDisabled(false); // 解锁输入框

    // 记录结束时间
    clock_t endTime = clock();
    auto consumedClocks = endTime - startTime;
    bool isTimeMilliseconds = true;
    long consumedTime = 0;
    if (consumedClocks > CLOCKS_PER_SEC) {
        isTimeMilliseconds = false;
        consumedTime = consumedClocks / CLOCKS_PER_SEC;
    } else {
        isTimeMilliseconds = true;
        consumedTime = consumedClocks * 1000 / CLOCKS_PER_SEC;
    }

    if (isSucceeded) {

        qDebug() << "Work finished successfully in" << consumedTime << (isTimeMilliseconds ? "milliseconds" : "seconds") << ".";

        // 设置提示信息
        ui->statusbar->showMessage(QString("解析完成，耗时 %1 %2。").arg(consumedTime).arg(isTimeMilliseconds ? "毫秒" : "秒"));
    } // 否则失败了，不要去动失败的提示信息

    qDebug() << "CleanUp finished";

}

void NyaTraceGUI::CleanUpTracing(const bool isSucceeded) {
    // UI 相关结束
    ui->tracingProgress->setValue(ui->tracingProgress->maximum()); // 完成进度条
    ui->resolveButton->setDisabled(false); // 解锁按钮
    ui->startStopTracingButton->setDisabled(false); // 解锁按钮
    ui->startStopTracingButton->setText("追踪"); // 设置功能提示
    ui->hostInput->setDisabled(false); // 解锁输入框

    // 释放解析结果表
    ui->resolveTable->setDisabled(false);

    // 记录结束时间
    clock_t endTime = clock();
    auto consumedClocks = endTime - startTime;
    bool isTimeMilliseconds = true;
    long consumedTime = 0;
    if (consumedClocks > CLOCKS_PER_SEC) {
        isTimeMilliseconds = false;
        consumedTime = consumedClocks / CLOCKS_PER_SEC;
    } else {
        isTimeMilliseconds = true;
        consumedTime = consumedClocks * 1000 / CLOCKS_PER_SEC;
    }

    if (isSucceeded) {

        qDebug() << "Work finished successfully in" << consumedTime << (isTimeMilliseconds ? "milliseconds" : "seconds") << ".";

        // 设置提示信息
        ui->statusbar->showMessage(QString("路由追踪完成，耗时 %1 %2。").arg(consumedTime).arg(isTimeMilliseconds ? "毫秒" : "秒"));

        bool hasValidPoint = false;
        // 连线
        for (int i = 0; i < DEF_MAX_HOP; i++) {
            if (traceGeoInfo[i].isValid) {
                qDebug() << "Map: Connecting hop" << i+1;

                // 连一条线
                QMetaObject::invokeMethod(
                    (QObject*)ui->tracingMap->rootObject(),
                    "connectLine",
                    Qt::DirectConnection,
                    Q_ARG(QVariant, traceGeoInfo[i].latitude),
                    Q_ARG(QVariant, traceGeoInfo[i].longitude)
                );
                hasValidPoint = true;
            }
        }

        // 仅在存在有效点的情况下调整地图，不然就飞了
        if (hasValidPoint) {
            QMetaObject::invokeMethod(
                (QObject*)ui->tracingMap->rootObject(),
                "fitMap",
                Qt::DirectConnection
            );
        }
    } // 否则失败了，不要去动失败的提示信息

    qDebug() << "CleanUp finished";
}


void NyaTraceGUI::on_hostInput_returnPressed()
{
    // 按下回车，启动追踪
    StartResolving();
}

void NyaTraceGUI::on_traceTable_clicked(const QModelIndex &index)
{
    qDebug() << "Table index clicked:" << index;

    // 如果地址有效，就前往地址
    if (traceGeoInfo[index.row()].isValid) {
        QMetaObject::invokeMethod(
            (QObject*)ui->tracingMap->rootObject(),
            "gotoCoordinate",
            Qt::DirectConnection,
            Q_ARG(QVariant, traceGeoInfo[index.row()].latitude),
            Q_ARG(QVariant, traceGeoInfo[index.row()].longitude),
            Q_ARG(QVariant, 14), // 缩放等级
            Q_ARG(QVariant,
                QString("第 %1 跳 - %2\n%3 - %4")
                    .arg(index.row() + 1)
                    .arg(
                        traceResultsModel->item(index.row(), 1)->text(),
                        traceResultsModel->item(index.row(), 3)->text(),
                        traceResultsModel->item(index.row(), 4)->text()
                    )
                )
        );
    }
}


void NyaTraceGUI::on_resolveTable_clicked(const QModelIndex &index)
{
    qDebug() << "Table index clicked:" << index;

    currentSelectedIPNo = index.row();

    // 如果地址有效，就前往地址
    if (resolveGeoInfo[index.row()].isValid) {
        QMetaObject::invokeMethod(
            (QObject*)ui->tracingMap->rootObject(),
            "gotoCoordinate",
            Qt::DirectConnection,
            Q_ARG(QVariant, resolveGeoInfo[index.row()].latitude),
            Q_ARG(QVariant, resolveGeoInfo[index.row()].longitude),
            Q_ARG(QVariant, 14), // 缩放等级
            Q_ARG(QVariant,
                QString("第 %1 个 IP - %2\n%3 - %4")
                    .arg(index.row() + 1)
                    .arg(
                        resolveResultsModel->item(index.row(), 0)->text(),
                        resolveResultsModel->item(index.row(), 1)->text(),
                        resolveResultsModel->item(index.row(), 2)->text()
                    )
                )
        );
    }

}

void NyaTraceGUI::on_resolveTable_doubleClicked(const QModelIndex &index)
{
    StartTracing();
}


void NyaTraceGUI::on_resolveButton_clicked()
{
    StartResolving();
}

void NyaTraceGUI::on_startStopTracingButton_clicked()
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

