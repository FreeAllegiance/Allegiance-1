
#include "Logger.h"
#include <time.h>

std::shared_ptr<ILogger> CreateTimestampedFileLogger(std::string prefix) {
    //if prefix is too long this will be a problem
    if (prefix.length() > 400) {
        throw std::runtime_error("Logging location, file path too long: " + prefix);
    }

    char  bfr[512];

    time_t  longTime;
    time(&longTime);
    tm t = tm();

    localtime_s(&t, &longTime);

    snprintf(bfr, sizeof(bfr), "%s%04d%02d%02d_%02d%02d%02d.log",
        prefix.c_str(),
        t.tm_year + 1900,
        t.tm_mon + 1, 
        t.tm_mday, 
        t.tm_hour, 
        t.tm_min, 
        t.tm_sec
    );

    std::string target = std::string(bfr);

    return std::make_shared<FileLogger>(target, true);
}

void CreateDirectoryRecursivelyForFilepath(std::string filepath)
{
    int pos = 0;
    do
    {
        pos = filepath.find_first_of("\\", pos + 1);
        if (pos != std::string::npos) {
            CreateDirectory(filepath.substr(0, pos).c_str(), NULL);
        }
    } while (pos != std::string::npos);
}

FileLogger::FileLogger(std::string path, bool bAppend) :
    m_path(path)
{
    CreateDirectoryRecursivelyForFilepath(path);

    m_hFile = CreateFile(
        path.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        bAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
        nullptr
    );

    if (m_hFile == nullptr) {
        throw std::runtime_error("Unable to create log file: " + path);
    }
}

FileLogger::~FileLogger() {
    CloseHandle(m_hFile);
    m_hFile = nullptr;
}

void FileLogger::Log(std::string text) {
    const size_t size = 512;
    char         bfr[size];

    auto length = text.length();

    time_t  longTime;
    time(&longTime);
    tm t = tm();

    localtime_s(&t, &longTime);

    if (length > size - 30) {
        //if message to log is larger than the buffer size
        text = text.substr(0, size - 30);
    }

    sprintf(bfr, "%04d-%02d-%02d %02d:%02d:%02d %s\r\n",
        t.tm_year + 1900,
        t.tm_mon + 1, 
        t.tm_mday, 
        t.tm_hour, 
        t.tm_min, 
        t.tm_sec, 
        text.c_str()
    );

    uint32_t nBytes;
    ::WriteFile(m_hFile, bfr, strlen(bfr), LPDWORD(&nBytes), nullptr);
}

void OutputLogger::Log(std::string text) {
    ::OutputDebugStringA(text.c_str());
    ::OutputDebugStringA("\n");
}