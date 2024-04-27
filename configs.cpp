#include "configs.h"

#include <QSettings>

Configs::Configs()
{
    // 强制使用 ini 格式的配置文件以避免弄乱系统底层数据（编辑注册表是非常危险的操作）
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // 初始化一个 QSettings 对象用于读配置
    QSettings settings;

    // 初始化配置
    logLevel = settings.value("LogLevel", 2).toInt(); // 回报 Warn 和 Critical 的日志

    settings.beginGroup("Trace");
    traceMaxHops        = settings.value("MaxHops", 30).toInt();               // 最大发出 30 跳路由追踪
    traceTimeout        = settings.value("Timeout", 3000).toLongLong();        // 每个 ICMP 包的超时时间为 3000 毫秒
    traceThreadInterval = settings.value("ThreadInterval", 100).toLongLong();  // 每后一个 ICMP 包的发送延迟为 100 毫秒
    settings.endGroup();

    settings.beginGroup("Auto");
    autoOpenMap         = settings.value("OpenMap", true).toBool();     // 解析或追踪时自动打开地图
    autoStartTrace      = settings.value("StartTrace", false).toBool(); // 解析完成后自动开始追踪
    settings.endGroup();
}

void Configs::Save() {
    // 保存配置

    // 初始化一个 QSettings 对象用于写配置
    QSettings settings;

    // 转换不支持的类型
    long long traceTimeoutLL         = traceTimeout;
    long long traceThreadIntervalLL = traceThreadInterval;

    // 设置值
    settings.setValue("LogLevel", logLevel);

    settings.beginGroup("Trace");
    settings.setValue("MaxHops",        traceMaxHops         );
    settings.setValue("Timeout",        traceTimeoutLL       );
    settings.setValue("ThreadInterval", traceThreadIntervalLL);
    settings.endGroup();

    settings.beginGroup("Auto");
    settings.setValue("OpenMap",    autoOpenMap   );
    settings.setValue("StartTrace", autoStartTrace);
    settings.endGroup();
}

int Configs::GetLogLevel() {
    return logLevel;
}

void Configs::SetLogLevel(int newLogLevel) {
    // Debug    0
    // Info     1
    // Warn     2
    // Critical 3
    if (0 <= newLogLevel && newLogLevel <= 3) {
        // 有效的
        logLevel = newLogLevel;
    } // 否则是无效的
}

int Configs::GetTraceMaxHops() {
    return traceMaxHops;
}

void Configs::SetTraceMaxHops(int newTraceMaxHops) {
    if (0 < newTraceMaxHops && newTraceMaxHops <= DEF_TRACE_MAX_HOPs) {
        // 有效的
        traceMaxHops = newTraceMaxHops;
    } // 否则是无效的
}

unsigned long Configs::GetTraceTimeout() {
    return traceTimeout;
}

void Configs::SetTraceTimeout(unsigned long newTraceTimeout) {
    if (0 < newTraceTimeout && newTraceTimeout <= DEF_TRACE_TIMEOUT_MAX) {
        traceTimeout = newTraceTimeout;
    }

}

unsigned long Configs::GetTraceThreadInterval() {
    return traceThreadInterval;
}

void Configs::SetTraceThreadInterval(unsigned long newTraceThreadInterval) {
    if (0 < newTraceThreadInterval && newTraceThreadInterval <= DEF_TRACE_THREAD_INTERVAL_MAX) {
        traceThreadInterval = newTraceThreadInterval;
    }
}

bool Configs::GetAutoOpenMap() {
    return autoOpenMap;
}

void Configs::SetAutoOpenMap(bool newAutoOpenMap) {
    autoOpenMap = newAutoOpenMap;
}

bool Configs::GetAutoStartTrace() {
    return autoStartTrace;
}

void Configs::SetAutoStartTrace(bool newAutoStartTrace) {
    autoStartTrace = newAutoStartTrace;
}


Configs * gCfg;
