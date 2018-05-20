#pragma once;

#include <memory>
#include <string>
#include <vector>
#include <functional>

class DownloadException : public std::runtime_error {

public:
    using std::runtime_error::runtime_error;
};

class DownloadHandle : public IHTTPSessionSink {
private:
    std::vector<std::function<void()>> m_callbacks;
    bool m_bDone;
    std::string m_strErrorMessage;

    std::string m_strDownloadPath;
    std::string m_strResultFilePath;

    std::unique_ptr<IHTTPSession> m_pSession;
    HANDLE m_hDownloadThreadHandle;

public:
    DownloadHandle(const std::string& path);

    ~DownloadHandle();

    void Initiate();

    void AddCallback(const std::function<void()>& callback);

    const std::string& GetResultFilePath();

    void Completed();

    void OnError(char * szErrorMessage);
    bool OnFileCompleted(char * szFileName);
};

class Downloader {
public:
    std::shared_ptr<DownloadHandle> Download(std::string path);
};