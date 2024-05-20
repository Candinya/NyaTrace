#ifndef CONFIGS_H
#define CONFIGS_H


// 一些通用的设置
const int DEF_RESOLVE_MAX_IPs = 30;                        // resolve 最多解析得到的 IP 数
const int DEF_TRACE_MAX_HOPs = 100;                        // trace 最大跳站数
const unsigned long DEF_TRACE_TIMEOUT_MAX = 10000;         // trace 最大超时时间
const unsigned long DEF_TRACE_THREAD_INTERVAL_MAX = 3000; // 每个线程的最大启动延迟

// 一些预定义的枚举
enum ConfigLogLevel {
    ConfigLogLevelDebug,
    ConfigLogLevelInfo,
    ConfigLogLevelWarning,
    ConfigLogLevelCritical
};
enum ConfigResolveDoubleClickAction {
    ConfigResolveDoubleClickActionStartTrace,
    ConfigResolveDoubleClickActionOpenMap
};


class Configs
{
public:
    Configs();

    void Save();

    int  GetLogLevel();
    void SetLogLevel(int);

    int  GetTraceMaxHops();
    void SetTraceMaxHops(int);

    unsigned long GetTraceTimeout();
    void SetTraceTimeout(unsigned long);

    unsigned long GetTraceThreadInterval();
    void SetTraceThreadInterval(unsigned long);

    bool GetAutoOpenMap();
    void SetAutoOpenMap(bool);

    bool GetAutoStartTrace();
    void SetAutoStartTrace(bool);

    int  GetResolveDoubleClickAction();
    void SetResolveDoubleClickAction(int);

private:
    int logLevel;

    int           traceMaxHops;
    unsigned long traceTimeout;
    unsigned long traceThreadInterval;

    bool autoOpenMap;
    bool autoStartTrace;

    int resolveDoubleClickAction;

};

extern Configs * gCfg;

#endif // CONFIGS_H
