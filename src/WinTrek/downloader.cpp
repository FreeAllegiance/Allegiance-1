
#include "pch.h"
#include "downloader.h"
#include <chrono>
#include <thread>

std::shared_ptr<DownloadHandle> Downloader::Download(std::string path)
{
    auto handle = std::make_shared<DownloadHandle>(path);
    handle->Initiate();

    return handle;
}

DownloadHandle::DownloadHandle(const std::string& path) :
    m_strDownloadPath(path),
    m_bDone(false),
    m_hDownloadThreadHandle(nullptr),
    m_strErrorMessage("")
{
}

DownloadHandle::~DownloadHandle()
{
    if (m_hDownloadThreadHandle) {
        CloseHandle(m_hDownloadThreadHandle);
    }
}

void DownloadHandle::Initiate()
{
    m_pSession = std::unique_ptr<IHTTPSession>(CreateHTTPSession(this));

    const char * szFileList[] = { m_strDownloadPath.c_str(), "temp.cfg", NULL };
    PathString pathEXE(PathString::GetCurrentDirectory());

    m_pSession->InitiateDownload(szFileList, pathEXE); // . means EXE path

    m_hDownloadThreadHandle = CreateThread(NULL, 0, [](void* pData) {
        IHTTPSession* pSession = (IHTTPSession*)pData;

        bool bOk = true;
        while (bOk) {
            bOk = pSession->ContinueDownload();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return (DWORD)0;
    }, m_pSession.get(), 0, NULL);
}

void DownloadHandle::AddCallback(const std::function<void()>& callback)
{
    if (m_bDone) {
        callback();
    }
    else {
        m_callbacks.push_back(callback);
    }
}

const std::string& DownloadHandle::GetResultFilePath()
{
    if (!m_bDone) {
        throw DownloadException(std::string("Download failed: ") + m_strErrorMessage);
    }

    return m_strResultFilePath;
}

void DownloadHandle::Completed()
{
    CloseHandle(m_hDownloadThreadHandle);
    m_hDownloadThreadHandle = nullptr;
    for (std::function<void()> entry : m_callbacks) {
        entry();
    }
}

void DownloadHandle::OnError(char * szErrorMessage)
{
    m_strErrorMessage = szErrorMessage;
    Completed();
}

bool DownloadHandle::OnFileCompleted(char * szFileName)
{
    PathString pathEXE(PathString::GetCurrentDirectory());
    m_strResultFilePath = std::string(pathEXE) + "/" + szFileName;
    m_bDone = true;
    Completed();
    return true; // true means don't retry download
}
