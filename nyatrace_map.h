#ifndef NYATRACE_MAP_H
#define NYATRACE_MAP_H

#include <QMainWindow>

namespace Ui {
class NyaTraceMap;
}

class NyaTraceMap : public QMainWindow
{
    Q_OBJECT

public:
    explicit NyaTraceMap(QWidget *parent = nullptr);
    ~NyaTraceMap();

    void DrawPoint(const double & latitude, const double & longitude, const unsigned short & accuracyRadius);
    void ConnectLine(const double & latitude, const double & longitude);
    void SetTextAndGoto(const double & latitude, const double & longitude, QString & infoText);
    void FitMap();
    void ClearAll();

private:
    Ui::NyaTraceMap *ui;
};

#endif // NYATRACE_MAP_H
