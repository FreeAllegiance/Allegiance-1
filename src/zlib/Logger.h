#pragma once

#include <memory>
#include <string>
#include <vector>
#include <Windows.h>

class ILogger {
public:
    virtual void Log(std::string text) = 0;
};

class FileLogger : public ILogger {
private:
    std::string m_path;
    HANDLE m_hFile;

public:
    FileLogger(std::string path, bool bAppend);
    ~FileLogger();

    void Log(std::string text) override;
};

class NullLogger : public ILogger {
private:
    static std::shared_ptr<NullLogger> m_pStaticInstance;

    NullLogger() {}

public:

    static const std::shared_ptr<NullLogger>& GetInstance() {
        static std::shared_ptr<NullLogger> instance = std::shared_ptr<NullLogger>(new NullLogger());

        return instance;
    }

    void Log(std::string text) override {}
};

class OutputLogger : public ILogger {
public:
    void Log(std::string text) override;
};

class SettableLogger : public ILogger {
private:
    std::shared_ptr<ILogger> m_logger;

public:
    SettableLogger(const std::shared_ptr<ILogger>& logger) :
        m_logger(logger)
    {
    }

    std::shared_ptr<ILogger> SetLogger(const std::shared_ptr<ILogger>& logger) {
        auto old = m_logger;
        m_logger = logger;

        return old;
    }

    void Log(std::string text) override {
        m_logger->Log(text);
    }
};

class MultiLogger : public ILogger {
private:
    std::vector<std::shared_ptr<ILogger>> m_loggers;

public:
    MultiLogger(const std::vector<std::shared_ptr<ILogger>>& loggers) :
        m_loggers(loggers)
    {}

    void Log(std::string text) override {
        for (auto it = m_loggers.begin(); it != m_loggers.end(); ++it) {
            (*it)->Log(text);
        }
    }
};

std::shared_ptr<ILogger> CreateTimestampedFileLogger(std::string prefix);