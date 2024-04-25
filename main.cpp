#include "nyatrace_gui.h"

#include "nyatrace_window.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QMutex>

// 使用互斥锁以避免多线程异步写入日志导致冲突
QMutex logMutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

    // 标记 context 这个变量没有被使用到
    Q_UNUSED(context);

    QString level;

    switch (type) {
    case QtDebugMsg:
        level = QString("Debug");
        break;
    case QtInfoMsg:
        level = QString("Info");
        break;
    case QtWarningMsg:
        level = QString("Warning");
        break;
    case QtCriticalMsg:
        level = QString("Critical");
        break;
    case QtFatalMsg:
        level = QString("Fatal");
        break;
    }

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");

    // 输出调试日志
    qDebug() << "新的日志到来" << level << msg;

    // 检查日志窗口是否开启
    if (ntlw != nullptr) {
        if (!ntlw->isActiveWindow()) {
            // 没有开启
            if (type == QtFatalMsg) {
                // 因为是 fatal 日志，所以必须让用户感知到
                qDebug() << "显示日志窗口以让用户看到日志";
                ntlw->show();
            }
        }

        qDebug() << "向日志窗口输出日志";

        // 启用互斥锁
        logMutex.lock();

        // 输出日志
        ntlw->AppendLog(ts, level, msg);

        // 释放互斥锁
        logMutex.unlock();
    }

    if (type == QtFatalMsg) {
        // 销毁主窗口，仅保留日志窗口
        delete ntgw;
    }
}

int main(int argc, char *argv[])
{

    // 定义版本号
    auto version = QString("NyaTrace %1").arg(APP_VERSION);

    // 打印版本号
    qDebug() << "Booting" << version << "...";

    // 初始化主程序
    QApplication app(argc, argv);

    // 初始化日志窗口
    ntlw = new NyaTraceLogs();

    // 切换日志输出
    qInstallMessageHandler(customMessageHandler);

    // 样式表文件
    QFile styleFile("theme/nyatrace.qss");

    // 如果文件存在则应用主题
    if (styleFile.exists()) {
        // 应用主题
        qInfo() << "Applying theme stylesheets...";

        // 添加图标目录
        QDir::addSearchPath("icon", "theme/icon");
        styleFile.open(QFile::ReadOnly);

        // 读取所有内容
        QString styleSheet { styleFile.readAll() };

        // 读取完成后关闭
        styleFile.close();

        // 应用样式表
        app.setStyleSheet(styleSheet);
    } else {
        // 没有主题
        qInfo() << "No theme found, start with default UI.";
    }

    // 初始化主窗口
    ntgw = new NyaTraceGUI();

    // 修改窗口标题
    ntgw->setWindowTitle(version);

    // 显示主窗口
    ntgw->show();

    return app.exec();
}
