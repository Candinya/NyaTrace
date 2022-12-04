#ifndef TR_GUI_H
#define TR_GUI_H

#include <QMainWindow>
#include <QStandardItemModel>

QT_BEGIN_NAMESPACE
namespace Ui { class TR_GUI; }
QT_END_NAMESPACE

class TR_GUI : public QMainWindow
{
    Q_OBJECT

public:
    TR_GUI(QWidget *parent = nullptr);
    ~TR_GUI();

private slots:
    void on_startStopButton_clicked();
    void on_hostInput_returnPressed();

private:
    // 界面 UI
    Ui::TR_GUI *ui;

    // 用于存储结果的模型
    QStandardItemModel * hopResultsModel;

    // 开始时间计时器
    clock_t startTime;

    // 成员函数
    void Initialize();
    void StartTracing();
    void AbortTracing();
    void CleanUp();

};

#endif // TR_GUI_H
