#ifndef NYATRACE_LOGS_H
#define NYATRACE_LOGS_H

#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class NyaTraceLogs;
}

class NyaTraceLogs : public QDialog
{
    Q_OBJECT

public:
    explicit NyaTraceLogs(QWidget *parent = nullptr);
    ~NyaTraceLogs();
    void AppendLog(const QString &timestamp, const QString &level, const QString &message);
    void Initialize();

private slots:
    void on_clearAll_clicked();

private:
    Ui::NyaTraceLogs *ui;

    // 用于存储日志的模型
    QStandardItemModel * logsModel;
};

#endif // NYATRACE_LOGS_H
