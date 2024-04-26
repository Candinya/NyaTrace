#include "configs.h"

Configs::Configs()
{
    // 初始化配置
    logLevel = 2;                  // 回报 Warn 和 Critical 的日志
    traceMaxHops = 30;             // 最大发出 30 跳路由追踪
    traceTimeout = 3000;           // 每个 ICMP 包的超时时间为 3000 毫秒
    traceThreadInterval = 100;     // 每后一个 ICMP 包的发送延迟为 100 毫秒
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


Configs * gCfg;
