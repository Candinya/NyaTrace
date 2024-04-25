#ifndef CONFIGS_H
#define CONFIGS_H


// 一些通用的设置
const int DEF_RESOLVE_MAX_IPs = 30;           // resolve 最多解析得到的 IP 数
const int DEF_TRACE_MAX_HOPs = 100;          // trace 最大跳站数

class Configs
{
public:
    Configs();

    int GetLogLevel();
    void SetLogLevel(int);

    int GetTraceMaxHops();
    void SetTraceMaxHops(int);

    unsigned long GetTraceICMPTimeout();
    void SetTraceICMPTimeout(unsigned long);

    unsigned long GetTraceIntervalPerThread();
    void SetTraceIntervalPerThread(unsigned long);

private:
    int logLevel;
    int traceMaxHops;
    unsigned long traceICMPTimeout;
    unsigned long traceIntervalPerThread;

};

extern Configs * gCfg;

#endif // CONFIGS_H
