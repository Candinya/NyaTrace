#include "nyatrace_gui.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NyaTraceGUI w;
    w.show();
    return a.exec();
}
