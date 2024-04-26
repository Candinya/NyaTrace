#include "nyatrace_map.h"
#include "ui_nyatrace_map.h"

#include <QMetaObject>
#include <QQuickWidget>

NyaTraceMap::NyaTraceMap(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NyaTraceMap)
{
    ui->setupUi(this);

    // 初始化追踪地图 （OSM）
    ui->tracingMap->setSource(QUrl("qrc:/tracing_map.qml"));
    ui->tracingMap->show();
}

NyaTraceMap::~NyaTraceMap()
{
    delete ui;
}

void NyaTraceMap::DrawPoint(const double & latitude, const double & longitude, const unsigned short & accuracyRadius) {
    // 根据纬度和经度在地图上画一个点和一个圆
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "drawHopPoint",
        Qt::DirectConnection,
        Q_ARG(QVariant, latitude),
        Q_ARG(QVariant, longitude),
        Q_ARG(QVariant, accuracyRadius)
    );
}

void NyaTraceMap::ConnectLine(const double & latitude, const double & longitude) {
    // 连一条线
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "connectLine",
        Qt::DirectConnection,
        Q_ARG(QVariant, latitude),
        Q_ARG(QVariant, longitude)
    );
}

void NyaTraceMap::SetTextAndGoto(const double & latitude, const double & longitude, QString & infoText) {
    // 设置文字并跳转到
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "gotoCoordinate",
        Qt::DirectConnection,
        Q_ARG(QVariant, latitude),
        Q_ARG(QVariant, longitude),
        Q_ARG(QVariant, 14), // 缩放等级
        Q_ARG(QVariant, infoText)
    );
}

void NyaTraceMap::FitMap() {
    // 自动调整地图
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "fitMap",
        Qt::DirectConnection
    );
}

void NyaTraceMap::ClearAll() {
    // 清空所有数据
    QMetaObject::invokeMethod(
        (QObject*)ui->tracingMap->rootObject(),
        "clearMap",
        Qt::DirectConnection
    );
}
