#ifndef NYATRACE_CONFIGS_H
#define NYATRACE_CONFIGS_H

#include <QDialog>

namespace Ui {
class NyaTraceConfigs;
}

class NyaTraceConfigs : public QDialog
{
    Q_OBJECT

public:
    explicit NyaTraceConfigs(QWidget *parent = nullptr);
    ~NyaTraceConfigs();

private slots:
    void setLogLevelValue(int);

    void on_sliderLogsLevel_valueChanged(int value);
    void on_saveButton_clicked();


private:
    Ui::NyaTraceConfigs *ui;
};

#endif // NYATRACE_CONFIGS_H
