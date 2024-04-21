#include "nyatrace_gui.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

    // 标记上下文这个变量没有被使用到
    Q_UNUSED(context);

    QString level;

    switch (type) {
    case QtDebugMsg:
        level = QString("[Debug]");
        break;
    case QtInfoMsg:
        level = QString("[Info]");
        break;
    case QtWarningMsg:
        level = QString("[Warning]");
        break;
    case QtCriticalMsg:
        level = QString("[Critical]");
        break;
    case QtFatalMsg:
        level = QString("[Fatal]");
        break;
    }


    QTextStream textStream(type == QtDebugMsg || type == QtInfoMsg ? stdout : stderr);
    textStream 
        << QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss]") << "\t" 
        << level << "\t" 
        << msg << Qt::endl
    ;

    if (type == QtFatalMsg) {
        // 现在才可以退出
        abort();
    }
}

int main(int argc, char *argv[])
{
#ifndef QT_DEBUG
    // 控制日志输出
    qInstallMessageHandler(customMessageHandler);
#endif

    // 定义版本号
    auto version = QString("NyaTrace %1").arg(APP_VERSION);

    // 打印版本号
    qDebug() << "Booting" << version << "...";

    QApplication app(argc, argv);
    NyaTraceGUI w;

    // 样式表文件
    QFile styleFile("theme/nyatrace.qss");

    // 如果文件存在则应用主题
    if (styleFile.exists()) {

        // 添加图标目录
        QDir::addSearchPath("icon", "theme/icon");
        styleFile.open(QFile::ReadOnly);

        // 读取所有内容
        QString styleSheet { styleFile.readAll() };

        // 读取完成后关闭
        styleFile.close();

        // 应用样式表
        app.setStyleSheet(styleSheet);
    }

    // 修改窗口标题
    w.setWindowTitle(version);

    w.show();
    return app.exec();
}
