#pragma once

class logger {
  public:
    enum LogLevel : int
    {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Assert = 5,
        Release = 6
    };
    static void logInfoW(const wchar_t *fmt, ...);
    static void logErrorW(const wchar_t *fmt, ...);
    static void logWarningW(const wchar_t *fmt, ...);

    static void logInfo(const char *fmt, ...);
    static void logError(const char *fmt, ...);
    static void logWarning(const char *fmt, ...);
    static void log(LogLevel level, const char *fmt, ...);
};

