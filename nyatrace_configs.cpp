#include "nyatrace_configs.h"
#include "ui_nyatrace_configs.h"
#include "configs.h"

#include <QDebug>

NyaTraceConfigs::NyaTraceConfigs(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NyaTraceConfigs)
{
    // 初始化界面 UI
    ui->setupUi(this);

    // 初始化配置项极值
    ui->sliderLogsLevel->setMinimum(0);
    ui->sliderLogsLevel->setMaximum(3);

    ui->spinTraceMaxHops->setMinimum(1);
    ui->spinTraceMaxHops->setMaximum(DEF_TRACE_MAX_HOPs);

    ui->spinTraceTimeout->setMinimum(0);
    ui->spinTraceTimeout->setMaximum(DEF_TRACE_TIMEOUT_MAX);
    ui->spinTraceTimeout->setSingleStep(1000);

    ui->spinTraceThreadInterval->setMinimum(0);
    ui->spinTraceThreadInterval->setMaximum(DEF_TRACE_THREAD_INTERVAL_MAX);
    ui->spinTraceThreadInterval->setSingleStep(100);

    // 初始化现有配置
    initializeCurrentValue();
}

NyaTraceConfigs::~NyaTraceConfigs()
{
    delete ui;
}

void NyaTraceConfigs::showEvent(QShowEvent * event) {
    // 重载 showEvent 以追加启动时的功能事件
    qDebug() << "[GUI Configs]"
             << "Execute custom showEvent"
    ;

    // 先调用官方事件
    QWidget::showEvent( event );

    // 追加自定义函数
    initializeCurrentValue();
}

void NyaTraceConfigs::initializeCurrentValue() {
    // 使用现有设置初始化
    qDebug() << "[GUI Configs]"
             << "Initialize with current configurations"
    ;
    ui->sliderLogsLevel->setSliderPosition(gCfg->GetLogLevel()           );
    ui->spinTraceMaxHops->setValue(        gCfg->GetTraceMaxHops()       );
    ui->spinTraceTimeout->setValue(        gCfg->GetTraceTimeout()       );
    ui->spinTraceThreadInterval->setValue( gCfg->GetTraceThreadInterval());
    ui->checkAutoOpenMap->setChecked(      gCfg->GetMapAutoOpen()   );
}

void NyaTraceConfigs::setLogLevelValue(int logLevel) {
    // 根据日志等级映射
    QString targetLogLevel;
    switch(logLevel) {
    case 0:
        targetLogLevel = "Debug";
        break;
    case 1:
        targetLogLevel = "Info";
        break;
    case 2:
        targetLogLevel = "Warning";
        break;
    case 3:
        targetLogLevel = "Critical";
        break;
    default:
        targetLogLevel = "Unknown";
        break;
    }

    qDebug() << "[GUI Configs]"
             << "New log level"
             << logLevel << targetLogLevel;

    // 设置
    ui->labelLogsLevelValue->setText(targetLogLevel);
}

void NyaTraceConfigs::on_sliderLogsLevel_valueChanged(int value)
{
    qDebug() << "[GUI Configs]"
             << "sliderLogsLevel value changed" << value;
    setLogLevelValue(value);
}

void NyaTraceConfigs::on_saveButton_clicked()
{
    int           newLogLevel             = ui->sliderLogsLevel->value();
    int           newTraceMaxHops         = ui->spinTraceMaxHops->value();
    unsigned long newTraceTimeout         = ui->spinTraceTimeout->value();
    unsigned long newTraceThreadInterval = ui->spinTraceThreadInterval->value();
    bool          newTraceAutoOpenMap     = ui->checkAutoOpenMap->isChecked();

    // 保存设置
    qDebug() << "[GUI Configs]"
             << "Save new configs"
             << "log level"             << newLogLevel
             << "trace max hops"        << newTraceMaxHops
             << "trace timeout"         << newTraceTimeout
             << "trace thread interval" << newTraceThreadInterval
    ;

    gCfg->SetLogLevel(           newLogLevel           );
    gCfg->SetTraceMaxHops(       newTraceMaxHops       );
    gCfg->SetTraceTimeout(       newTraceTimeout       );
    gCfg->SetTraceThreadInterval(newTraceThreadInterval);
    gCfg->SetMapAutoOpen(   newTraceAutoOpenMap   );

    // 关闭窗口
    this->hide();
}



