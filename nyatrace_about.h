#ifndef NYATRACE_ABOUT_H
#define NYATRACE_ABOUT_H

#include <QDialog>

namespace Ui {
class NyaTraceAbout;
}

class NyaTraceAbout : public QDialog
{
    Q_OBJECT

public:
    explicit NyaTraceAbout(QWidget *parent = nullptr);
    ~NyaTraceAbout();
    void SetVersion(QString &);

private:
    Ui::NyaTraceAbout *ui;
};

#endif // NYATRACE_ABOUT_H
