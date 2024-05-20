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
    ui->checkAutoOpenMap->setChecked(      gCfg->GetAutoOpenMap()        );
    ui->checkAutoStartTrace->setChecked(   gCfg->GetAutoStartTrace()     );
    ui->radioResolveDoubleClickActionStartTrace->setChecked(gCfg->GetResolveDoubleClickAction() == ConfigResolveDoubleClickActionStartTrace);
    ui->radioResolveDoubleClickActionOpenMap->setChecked(   gCfg->GetResolveDoubleClickAction() == ConfigResolveDoubleClickActionOpenMap   );
}

void NyaTraceConfigs::setLogLevelValue(int logLevel) {
    // 根据日志等级映射
    QString targetLogLevel;
    switch(logLevel) {
    case ConfigLogLevelDebug:
        targetLogLevel = "Debug";
        break;
    case ConfigLogLevelInfo:
        targetLogLevel = "Info";
        break;
    case ConfigLogLevelWarning:
        targetLogLevel = "Warning";
        break;
    case ConfigLogLevelCritical:
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

void NyaTraceConfigs::Apply() {
    int           newLogLevel             = ui->sliderLogsLevel->value();
    int           newTraceMaxHops         = ui->spinTraceMaxHops->value();
    unsigned long newTraceTimeout         = ui->spinTraceTimeout->value();
    unsigned long newTraceThreadInterval = ui->spinTraceThreadInterval->value();
    bool          newAutoOpenMap     = ui->checkAutoOpenMap->isChecked();
    bool          newAutoStartTrace       = ui->checkAutoStartTrace->isChecked();
    int           newResolveDoubleClickAction = ConfigResolveDoubleClickActionStartTrace;

    if (ui->radioResolveDoubleClickActionStartTrace->isChecked()) {
        newResolveDoubleClickAction = ConfigResolveDoubleClickActionStartTrace;
    } else if (ui->radioResolveDoubleClickActionOpenMap->isChecked()) {
        newResolveDoubleClickAction = ConfigResolveDoubleClickActionOpenMap;
    }

    // 应用设置
    qDebug() << "[GUI Configs]"
             << "Apply new configs"
             << "log level"             << newLogLevel
             << "trace max hops"        << newTraceMaxHops
             << "trace timeout"         << newTraceTimeout
             << "trace thread interval" << newTraceThreadInterval
             << "auto open map"         << newAutoOpenMap
             << "auto start trace"      << newAutoStartTrace
    ;

    gCfg->SetLogLevel(           newLogLevel           );
    gCfg->SetTraceMaxHops(       newTraceMaxHops       );
    gCfg->SetTraceTimeout(       newTraceTimeout       );
    gCfg->SetTraceThreadInterval(newTraceThreadInterval);
    gCfg->SetAutoOpenMap(        newAutoOpenMap        );
    gCfg->SetAutoStartTrace(     newAutoStartTrace     );
    gCfg->SetResolveDoubleClickAction(newResolveDoubleClickAction);
}

void NyaTraceConfigs::on_btnApply_clicked()
{
    // 应用配置
    Apply();
}




void NyaTraceConfigs::on_btnSave_clicked()
{
    // 应用配置
    Apply();

    // 保存配置
    gCfg->Save();

    // 关闭窗口
    this->hide();

}

