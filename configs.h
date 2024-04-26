#ifndef CONFIGS_H
#define CONFIGS_H


// 一些通用的设置
const int DEF_RESOLVE_MAX_IPs = 30;          // resolve 最多解析得到的 IP 数
const int DEF_TRACE_MAX_HOPs = 100;          // trace 最大跳站数
const unsigned long DEF_TRACE_TIMEOUT_MAX = 10000; // trace 最大超时时间
const unsigned long DEF_TRACE_THREAD_INTERVAL_MAX = 10000; // 每个线程的最大启动延迟

class Configs
{
public:
    Configs();

    int GetLogLevel();
    void SetLogLevel(int);

    int GetTraceMaxHops();
    void SetTraceMaxHops(int);

    unsigned long GetTraceTimeout();
    void SetTraceTimeout(unsigned long);

    unsigned long GetTraceThreadInterval();
    void SetTraceThreadInterval(unsigned long);

private:
    int logLevel;
    int traceMaxHops;
    unsigned long traceTimeout;
    unsigned long traceThreadInterval;

};

extern Configs * gCfg;

#endif // CONFIGS_H
