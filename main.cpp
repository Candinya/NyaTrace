#include "nyatrace_gui.h"

#include <QApplication>
#include <QDir>
#include <QFile>

int main(int argc, char *argv[])
{
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
