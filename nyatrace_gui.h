#ifndef NYATRACE_GUI_H
#define NYATRACE_GUI_H

#include <QMainWindow>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class NyaTraceGUI; }
QT_END_NAMESPACE

class NyaTraceGUI : public QMainWindow
{
    Q_OBJECT

public:
    NyaTraceGUI(QWidget *parent = nullptr);
    ~NyaTraceGUI();

private slots:
    void on_startStopButton_clicked();
    void on_hostInput_returnPressed();

private:
    // 界面 UI
    Ui::NyaTraceGUI *ui;

    // 用于存储结果的模型
    QStandardItemModel * hopResultsModel;

    // 开始时间计时器
    clock_t startTime;

    // 成员函数
    void Initialize();
    void StartTracing();
    void AbortTracing();
    void CleanUp(const bool isSucceeded);

};

#endif // NYATRACE_GUI_H
