#include "nyatrace_gui.h"

#include "nyatrace_window.h"

#include "configs.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QMutex>
#include <QDebug>

// 使用互斥锁以避免多线程异步写入日志导致冲突
QMutex logMutex;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

    // 标记 context 这个变量没有被使用到
    Q_UNUSED(context);

    QString level;
    bool printToLogsTable = false;

    switch (type) {
    case QtDebugMsg:
        level = QString("Debug");
        printToLogsTable = gCfg->GetLogLevel() <= 0;
        break;
    case QtInfoMsg:
        level = QString("Info");
        printToLogsTable = gCfg->GetLogLevel() <= 1;
        break;
    case QtWarningMsg:
        level = QString("Warning");
        printToLogsTable = gCfg->GetLogLevel() <= 2;
        break;
    case QtCriticalMsg:
        level = QString("Critical");
        printToLogsTable = gCfg->GetLogLevel() <= 3;
        break;
    case QtFatalMsg:
        // 特殊处理： Fatal 是完全无法恢复、需要立刻停止级别的错误，所以不应该进入正常处理流程，而应该尽快结束程序。
        // 应用程序级别的致命问题请使用 qCritical 来输出。
        // 并且也不应该使用程序本身的 UI 来报错，而应该创建崩溃转储信息。

        // 创建时间戳
        QString crashTs = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");

        // 创建以时间戳命名的崩溃日志，存储到系统临时目录中
        QString crashReportFileName = QDir::cleanPath(QDir::tempPath() + QDir::separator() +QString("NyaTrace_CrashReport_%1.log").arg(crashTs));

        // 打开文件，创建写入流
        QFile crashReportFile(crashReportFileName);
        QTextStream crashReportTextStream(&crashReportFile);

        // 写入数据
        crashReportTextStream << msg;

        // 终止一切执行
        abort();

        // 立刻返回
        return;
    }

    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");

    // 输出调试日志
    qDebug() << "New log arrive:" << level << msg;

    // 检查日志窗口是否开启
    if (ntlw != nullptr) {
        if (!ntlw->isActiveWindow()) {
            // 没有开启
            if (type == QtCriticalMsg) {
                // 因为是 fatal 日志，所以必须让用户感知到
                qDebug() << "Show log dialog to let users know";
                ntlw->show();
            }
        }

        // 写出日志
        if (printToLogsTable) {

            // 启用互斥锁
            logMutex.lock();

            // 输出日志
            ntlw->AppendLog(ts, level, msg);

            // 释放互斥锁
            logMutex.unlock();

        }
    }

    if (type == QtCriticalMsg) {
        // 尝试销毁主窗口
        if (ntgw != nullptr) {
            delete ntgw;
        }
    }
}

int main(int argc, char *argv[])
{

    // 定义版本号
    auto tagVersion = QString("v%1").arg(APP_VERSION);
    auto fullVersion = QString("NyaTrace %1").arg(tagVersion);

    // 打印版本号
    qDebug() << "Booting" << fullVersion << "...";

    // 初始化配置文件
    gCfg = new Configs();

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

    // 初始化地图窗口
    ntmw = new NyaTraceMap();

    // 初始化配置窗口
    ntcw = new NyaTraceConfigs();

    // 初始化关于窗口
    ntaw = new NyaTraceAbout();

    // 设置关于窗口的版本号
    ntaw->SetVersion(tagVersion);

    // 初始化主窗口
    ntgw = new NyaTraceGUI();

    // 修改主窗口标题
    ntgw->setWindowTitle(fullVersion);

    // 显示主窗口
    ntgw->show();

    return app.exec();
}
