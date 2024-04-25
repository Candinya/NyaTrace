#include "nyatrace_logs.h"
#include "ui_nyatrace_logs.h"
#include "nyatrace_window.h"

NyaTraceLogs::NyaTraceLogs(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NyaTraceLogs)
{
    ui->setupUi(this);

    // 初始化日志数据模型
    logsModel = new QStandardItemModel();

    // 日志表调用模型
    ui->logsTable->setModel(logsModel);
    ui->logsTable->show();

    // 初始化 UI
    Initialize();

    // 自动扩展最后一段
    ui->logsTable->horizontalHeader()->setStretchLastSection(true);
}

NyaTraceLogs::~NyaTraceLogs()
{

    // 销毁日志模型
    delete logsModel;

    // 销毁主界面
    delete ui;
}

void NyaTraceLogs::Initialize() {

    // 清空表格数据
    logsModel->clear();

    // 构建日志表头
    QStringList logsLables = { "时间", "等级", "消息" };
    logsModel->setHorizontalHeaderLabels(logsLables);

}

void NyaTraceLogs::AppendLog(const QString &timestamp, const QString &level, const QString &message)
{
    // 添加新的日志
    QList<QStandardItem*> list = { new QStandardItem(timestamp), new QStandardItem(level), new QStandardItem(message) };
    logsModel->appendRow(list);
}

void NyaTraceLogs::on_clearAll_clicked()
{
    // 清空日志（重新初始化）
    Initialize();
}

