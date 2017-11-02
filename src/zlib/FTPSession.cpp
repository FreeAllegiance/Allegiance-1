
/*-------------------------------------------------------------------------
 * clintlib\FTPSession.cpp
 * 
 * The implementation of IFTPSession
 *
 * See clintlib\FTPSession.h for descriptions of all public functions.
 *
 * Owner: 
 * 
 * Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/


#include "FTPSession.h"

#include <Windows.h>
#include <WinInet.h>
#include <cstdio>
#include <cstdint>

#include "zassert.h"


template<class TInterfaceClass>
class CInternetSessionImpl :
    public TInterfaceClass
{
public:

    CInternetSessionImpl(IInternetSessionSink * pSink) : // pSink can be NULL
      m_buffer(NULL),
      m_hFile(NULL),
      m_pszFileList(NULL),
      m_pszFileListData(NULL),
      m_nLastErrorCode(0),
      m_nLastErrorCodeInThread(0),
      m_cBytesRead(0),
      m_bAbortDownload(false),
      m_hFTPSession(NULL),
      m_hFileConnection(NULL),
      m_pUpdateSink(pSink) 
    {
      m_szLastError[0] = 0;
      m_szErrorInThread[0] = 0;

      m_eventResumeDownload = CreateEvent(NULL, FALSE, FALSE, NULL);
      m_eventKillDownload = CreateEvent(NULL, TRUE, FALSE, NULL);
      m_eventProgress = CreateEvent(NULL, FALSE, FALSE, NULL);
      m_eventDownloadTerminated = CreateEvent(NULL, TRUE, FALSE, NULL);
      m_eventFileCompleted = CreateEvent(NULL, FALSE, FALSE, NULL);

      debugf("Creating download thread.\n");
      uint32_t dum;
      m_threadDownload = CreateThread(NULL, 0, LPTHREAD_START_ROUTINE(DownloadThread), (void*)this, 0, LPDWORD(&dum));

      if (m_pUpdateSink && m_threadDownload == NULL)
          debugf("Failed to create thread.\n");
    }

    virtual ~CInternetSessionImpl() 
    {
        KillDownload();
        Disconnect();

        CloseHandle(m_eventResumeDownload);
        CloseHandle(m_eventKillDownload);
        CloseHandle(m_eventProgress);
        CloseHandle(m_eventDownloadTerminated);
        CloseHandle(m_eventFileCompleted);

        CloseHandle(m_threadDownload);

        if (m_pszFileList)
        {
            char * * psz = m_pszFileListData + 1; // first one is 0xFFFFFFFF, so skip it

            while (psz && *psz)
                free(*(psz++));

            delete[] m_pszFileListData;
        }
    }

    virtual int GetFileListIncrement() = 0;

    void SetSink(IInternetSessionSink * pUpdateSink)
    {
      m_pUpdateSink = pUpdateSink;
    }


    bool InitiateDownload(const char * const * pszFileList, 
                          const char * szDestFolder, 
                          bool bDisconnectWhenDone,
                          int nMaxBufferSize)
    {
        //
        // Prepare a memory buffer for the filelist, and copy pszFileList to m_pszFileList
        //
        {
            // iterate once to find number of entries
            char * * psz = const_cast<char * *>(pszFileList);
            int i = 0;
            while (psz && *psz)
            {
                i++;
                psz++;
            }
            m_pszFileListData = new char*[i+2];
            m_pszFileListData[0] = (char*)-1; // means not started yet;
            // StartNextFile() increments first thing; so (char*)-1 is not downloaded, (I chose -1 because it's different than NULL)

            // iterate again and allocate memory for each entry
            psz = const_cast<char * *>(pszFileList);
            i = 1;
            while (psz && *psz)
            {
                m_pszFileListData[i] = _strdup(*psz);
                i++;
                psz++;
            }
            m_pszFileListData[i] = NULL;
            m_pszFileList = m_pszFileListData;
        }

        strcpy_s(m_szDestFolder, _MAX_PATH, szDestFolder);
        if (m_szDestFolder[0] == '\0' ||  // prevent access of not our memory        
           m_szDestFolder[strlen(m_szDestFolder)-1] != '\\')
        {
           strcat_s(m_szDestFolder, _MAX_PATH, "\\");
        }

        m_bAutoDisconnect = bDisconnectWhenDone;

        m_nBufferSize = nMaxBufferSize;

        if(m_buffer == NULL)
            m_buffer = (char*)::VirtualAlloc(NULL, m_nBufferSize, MEM_COMMIT, PAGE_READWRITE);

        assert(m_buffer);

        m_cTotalBytesRead = 0;

        ResetEvent(m_eventResumeDownload);
        ResetEvent(m_eventKillDownload);
        ResetEvent(m_eventProgress);
        ResetEvent(m_eventDownloadTerminated);
        ResetEvent(m_eventFileCompleted);

        return true;
    }



    bool Disconnect()
    {
        FinishCurrentFile(false); 

        if (m_hInternetSession)
        {
            if (!InternetCloseHandle(m_hInternetSession))
            {    
                DoError("Disconnect Failed");
                return false;
            }
            m_hInternetSession = NULL;
        }

        if (m_buffer)
        {
            ::VirtualFree((void*)m_buffer, 0, MEM_RELEASE);
            m_buffer = NULL;
        }

        return true;
    }

    const char* GetDownloadPath()
    {
        return m_szDestFolder;
    }

    const char* GetLastErrorMessage() 
    {
        if (m_szLastError[0] != '\0')
        {
            return m_szLastError;
        }
        else return NULL; // no error has occured
    }

    void Abort(bool bAutoDisconnect) 
    {
        KillDownload();
        m_bAbortDownload = true;
        if (bAutoDisconnect)
            Disconnect();
    }


    bool ContinueDownload()
    {
        if (m_szLastError[0] != '\0')  // if aborted or previous error
            return false;

        // if error within download thread
        if (WaitForSingleObject(m_eventKillDownload, 0) == WAIT_OBJECT_0)
        {
            if (m_szErrorInThread[0] != 0)
            {
                SetLastError(m_nLastErrorCodeInThread);
                DoError(m_szErrorInThread);
            }
            else
            if (m_bAutoDisconnect)
                Disconnect();

            return false; // signal that we are done
        }

        // Note: *m_pszFileList is 0xFFFFFFFF if download thread hasn't started downloading yet
        if (*m_pszFileList != NULL && !m_bAbortDownload) // if not done
        {
            if (WaitForSingleObject(m_eventProgress, 0) == WAIT_OBJECT_0)
            {
                //
                // Flush buffer as if needed
                //
                if (m_cBytesRead == m_nBufferSize)
                {
                    if (!FlushDownloadBuffer())
                        return false;
                }
    
                // If progress updates are wanted
                if(m_pUpdateSink)
                {
                    // Fire update
                    m_pUpdateSink->OnProgress(m_cTotalBytesRead, *m_pszFileList, m_cCurrentFileBytesRead);
                }

                SetEvent(m_eventResumeDownload);
            }
            else if(WaitForSingleObject(m_eventFileCompleted, 0) == WAIT_OBJECT_0)
            {
                FinishCurrentFile(true);
                SetEvent(m_eventResumeDownload);
            }
            return true;  // signal NOT yet done with entire transfer

        }
        else  // if done
        {
            //
            //  Close up
            //

            // signal complete transfer
            if (m_pUpdateSink)
                m_pUpdateSink->OnTransferFinished();

            if (m_bAutoDisconnect)
                Disconnect();

            return false;
        }
    }

protected:


    enum DOWNLOAD_RESULT
    {
        DOWNLOAD_ERROR,
        DOWNLOAD_PROGRESS,
        FILE_COMPLETED,
    };

     static uint32_t WINAPI DownloadThread(LPVOID pThreadParameter)
    {
		debugf("DownloadThread(): thread started.\r\n");

        CInternetSessionImpl * pSession = (CInternetSessionImpl *) pThreadParameter;

        HANDLE pHandles[] = { pSession->m_eventKillDownload, pSession->m_eventResumeDownload };

        //
        // Wait for file download to resume or for abort
        //
		debugf("DownloadThread(): beginning wait cycle.\r\n");
        while (WaitForMultipleObjects(2, pHandles, FALSE, INFINITE) != WAIT_OBJECT_0)
        {
			debugf("DownloadThread(): Checking for pSession->m_hFile: %ld\r\n", pSession->m_hFile);

            if (pSession->m_hFile == NULL)
            {
				debugf("DownloadThread(): pSession->m_hFile was null, calling start next file.\r\n");

                if (!pSession->StartNextFile())
                {
					debugf("DownloadThread(): pSession->StartNextFile() failed, killing download.\r\n");
                    SetEvent(pSession->m_eventKillDownload);
                    break;
                }
            }

			debugf("DownloadThread(): checking pSession->m_hFile: %ld\r\n", pSession->m_hFile);

            if (pSession->m_hFile != NULL) 
            {
				debugf("DownloadThread(): pSession->m_hFile was not null, downloading file block.\r\n");

                DOWNLOAD_RESULT result = DOWNLOAD_ERROR; // default to error, until we know better
                __try
                {
                    result = pSession->DownloadFileBlock();
					debugf("DownloadThread(): pSession->DownloadFileBlock() result: %ld.\r\n", result);
                }
                __except(1)
                {
                    result = DOWNLOAD_ERROR;
					debugf("DownloadThread(): pSession->DownloadFileBlock() = threw exception, DOWNLOAD_ERROR.\r\n");
                }

                if (result == DOWNLOAD_PROGRESS)
                {
					debugf("DownloadThread(): pSession->DownloadFileBlock() returned: DOWNLOAD_PROGRESS\r\n");
                    SetEvent(pSession->m_eventProgress);
					debugf("DownloadThread(): pSession->m_eventProgress - event set.\r\n");
                }
                else
                if (result == FILE_COMPLETED)
                {
					debugf("DownloadThread(): pSession->DownloadFileBlock() returned: FILE_COMPLETED\r\n");
                    SetEvent(pSession->m_eventFileCompleted);
					debugf("DownloadThread(): pSession->m_eventFileCompleted - event set.\r\n");
                }
                else
                if (result == DOWNLOAD_ERROR)
                {
					debugf("DownloadThread(): pSession->DownloadFileBlock() returned: DOWNLOAD_ERROR\r\n");
                    SetEvent(pSession->m_eventKillDownload);
					debugf("DownloadThread(): pSession->m_eventKillDownload - event set.\r\n");
                    break;
                }
            }
        }
        debugf("Download thread exiting...\n");
        SetEvent(pSession->m_eventDownloadTerminated);
        ExitThread(0);
        return 0;
    }

    virtual bool StartNextFile()
    {
        m_cBytesRead = 0;
        m_cCurrentFileBytesRead = 0;

        ++m_pszFileList;

        unsigned cTries = 0;

        if (*m_pszFileList)
        {
            //
            // Open file for download
            //
            while (!(m_hFileConnection = FtpOpenFile(m_hFTPSession, *m_pszFileList, GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_RELOAD, 0)))
            {
                //
                // Okay, it failed.  On NT5, this seems to sometimes happen for no apparent reason, so lets try a 
                // few more things just to be sure.
                //
                cTries++;
                debugf("Failed to open file via FTP for download, try #%d; error code: %d\n", cTries, GetLastError());
                Sleep(500);
                if(cTries >= 10) // okay, it must be more than a glich--must be an error, like the file not existing
                {
                   DoErrorInThread("Failed to open file (%s) for download.", *m_pszFileList);
                   return false;
                }
            }

            return OpenDownloadFile();
        }

        return true;
    }

    bool FinishCurrentFile(bool bCompleted) // finish as in close, without any downloading
    {
        if (m_hFileConnection)
        {
            if (!InternetCloseHandle(m_hFileConnection))
            {    
                DoError("InternetCloseHandle() Failed for download file");
                return false;
            }
            m_hFileConnection = NULL;
        }

        //
        // If done, write file and exit
        //
        if (!FlushDownloadBuffer())
            return false;

        if (!CloseDownloadFile(bCompleted))
            return false;

        return true;
    }

    DOWNLOAD_RESULT DownloadFileBlock()
    {
        unsigned long cBytesAvail, cBytesJustRead;

        //
        // Find out is data is available for receiving
        //

        if (!InternetQueryDataAvailable(m_hFileConnection, &cBytesAvail, 0, 0))
        {
            DoErrorInThread("InternetQueryDataAvailable() Failed.");

            return DOWNLOAD_ERROR;
        }

//        cBytesAvail = min(cBytesAvail, nMaxBlockSize);

        if (cBytesAvail == 0)
        {
            return FILE_COMPLETED;
            //return FinishCurrentFile();
        }

        //
        // Get the available data
        //
        unsigned long cBytesAttempted = min(cBytesAvail, m_nBufferSize-m_cBytesRead);

        if (!InternetReadFile((void*)m_hFileConnection, (void*)(m_buffer+m_cBytesRead), cBytesAttempted, &cBytesJustRead))
        {
           DoErrorInThread("InternetReadFile() Failed.");

           return DOWNLOAD_ERROR;
        }

        if (cBytesJustRead == 0) // testing proves this additional check is needed.
        {
            return FILE_COMPLETED;
//            return FinishCurrentFile();
        }

/*
        INTERNET_BUFFERS buffer;
        buffer.lpvBuffer = m_buffer+m_cBytesRead;
        buffer.dwBufferLength = m_nBufferSize-m_cBytesRead;
        buffer.dwBufferTotal = buffer.dwBufferLength;

        buffer.dwStructSize = sizeof(INTERNET_BUFFERS);
        buffer.lpcszHeader = NULL;
        buffer.dwHeadersLength = 0;
        buffer.dwHeadersTotal = 0;
        buffer.Next = NULL;
        buffer.dwOffsetLow = 0;
        buffer.dwOffsetHigh = 0;

        if (!InternetReadFileEx(m_hFileConnection, &buffer, IRF_SYNC | IRF_NO_WAIT, 0))
        {
            DoError("FTP Read Failed.");
            return false;
        }

        unsigned cBytesJustRead = buffer.dwBufferTotal;
*/
        m_cBytesRead += cBytesJustRead;
        m_cCurrentFileBytesRead += cBytesJustRead;
        m_cTotalBytesRead += cBytesJustRead;

        return DOWNLOAD_PROGRESS;
    }


    
    bool OpenDownloadFile()
    {
       //
       // Make Path
       //
       char szFilename[MAX_PATH+20];
       strcpy(szFilename, m_szDestFolder);
       strcat(szFilename, *m_pszFileList);

       m_hFile = CreateFile(szFilename, 
                                 GENERIC_WRITE, 
                                 FILE_SHARE_READ, 
                                 NULL, 
                                 CREATE_ALWAYS, 
                                 FILE_ATTRIBUTE_TEMPORARY, // don't write to disk right away (for better performance).
                                 NULL);

       if (m_hFile == INVALID_HANDLE_VALUE)
       {
           DoErrorInThread("Failed create file (%s) on local drive.", szFilename);
           return false;
       }

       return true;
    }


    bool FlushDownloadBuffer()
    {
       unsigned long cBytesWritten;

       if (m_cBytesRead != 0)
       {
           if(m_pUpdateSink)
           {
               if(m_pUpdateSink->OnDataReceived((void*)m_buffer, m_cBytesRead) == false)
               {
                    // after looking at the data, user has decided to 
                    // skip this file.  
                    // NOTE: This hasn't been tested.
                    ::InternetCloseHandle(m_hFileConnection);
                    m_hFileConnection = NULL;
                    ::CloseHandle(m_hFile);
                    m_hFile = NULL;
               }
           }

           if (!WriteFile(m_hFile, (void*)m_buffer, m_cBytesRead, &cBytesWritten, NULL))
           {
               DoError("Failed to write the file (%s) to local drive : ", *m_pszFileList);
               return false;
           }

           m_cBytesRead = 0;
       }

       return true;
    }


    bool CloseDownloadFile(bool bCompleted)
    {
       if (m_hFile == NULL)
           return true;

       if (!::CloseHandle(m_hFile))
           DoError("Failed to close file %s", *m_pszFileList);

       m_hFile = NULL;

      //
      // If progress updates are wanted
      // 
      if(m_pUpdateSink && bCompleted)
      {
        // Fire update
        if(!m_pUpdateSink->OnFileCompleted(*m_pszFileList))
        {
            m_pszFileList-= GetFileListIncrement();
            m_cTotalBytesRead -= m_cCurrentFileBytesRead;
        }
      }

      return true;
    }



    /*-------------------------------------------------------------------------
     * FormatErrorMessage()
     *-------------------------------------------------------------------------
     * Paramters:
     *    dwErrorCode: take a dwErrorCode and print what it means as text    
     * 
     */
    void FormatErrorMessage(char *szBuffer, uint32_t dwErrorCode)
    {

      sprintf(szBuffer,"(%d) ", dwErrorCode);

      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | 
                    FORMAT_MESSAGE_IGNORE_INSERTS, 
                    NULL, 
                    dwErrorCode, 
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                    szBuffer + strlen(szBuffer),
                    128,
                    NULL 
                    );
      strcat(m_szLastError, "   ");
      unsigned long dummy, size = sizeof(m_szLastError) - strlen(szBuffer) - 2;
      InternetGetLastResponseInfo(&dummy, szBuffer + strlen(szBuffer), &size);
    }


    /*-------------------------------------------------------------------------
     * DoError()
     *-------------------------------------------------------------------------
     */
    void DoError(char * szFormat, ...) 
    {
        if (m_szLastError[0] != 0) // don't erase over first error which can cause other errors
            return;              

        m_nLastErrorCode = GetLastError();

        char szMsg[sizeof(m_szLastError) - 50];
        va_list pArg;
        va_start(pArg, szFormat);
        _vsnprintf_s(szMsg, sizeof(szMsg), sizeof(szMsg), szFormat, pArg);
        va_end(pArg);

        strcpy(m_szLastError, szMsg);
        FormatErrorMessage(m_szLastError + strlen(m_szLastError), m_nLastErrorCode);

        CloseDownloadFile(false);

        // by now GetLastError() has been cleared
        SetLastError(m_nLastErrorCode); // preserve this for external users

        if(m_pUpdateSink)
            m_pUpdateSink->OnError(m_szLastError);

    }

    void DoErrorInThread(char * szFormat, ...) 
    {
        if (m_szErrorInThread[0] != 0) // don't erase over first error which can cause other errors
            return;              

        m_nLastErrorCodeInThread = GetLastError();

        char szMsg[sizeof(m_szErrorInThread) - 50];
        va_list pArg;
        va_start(pArg, szFormat);
        _vsnprintf(szMsg, sizeof(szMsg), szFormat, pArg);
        va_end(pArg);

        strcpy(m_szErrorInThread, szMsg);
    }

    void KillDownload()
    {
        if (m_threadDownload)
        {
            SetEvent(m_eventKillDownload); // tell thread to exit
            int nAwaker = WaitForSingleObject(m_eventDownloadTerminated, 5000); // wait up to 5 seconds
            if (nAwaker == WAIT_TIMEOUT)
            {
                // Ideally, the thread would have it's memory instead of using the InternetSession's memory.
                // Then we wouldn't need to Terminate it like this.  
                TerminateThread(m_threadDownload, 0);
            }
            CloseHandle(m_threadDownload);
            m_threadDownload = NULL;
        }
    }


protected:

    char * *              m_pszFileList;
    char * *              m_pszFileListData;
    char                  m_szDestFolder[MAX_PATH];

    volatile HINTERNET    m_hInternetSession;
    volatile HINTERNET    m_hFTPSession;
    volatile HINTERNET    m_hFileConnection;

    volatile HANDLE       m_hFile; 

    // events for controlling download thread
    HANDLE                m_eventResumeDownload;      // ready to resume downloading because either we are starting a new file or a progress sink has finished being fired 
    HANDLE                m_eventKillDownload;        // download was killed by either error or abort
    HANDLE                m_eventDownloadTerminated;  // download thread was terminated
    HANDLE                m_eventProgress;            // download progress was made, so time to fire an update event to the sink
    HANDLE                m_eventFileCompleted;       // file was completed

    HANDLE                m_threadDownload;

    volatile char *       m_buffer;
    volatile unsigned     m_cBytesRead;  // bytes read into m_buffer for current file
    volatile unsigned     m_nBufferSize; // alloc-ed size of m_buffer for current file

    char                  m_szLastError[1024];
    char                  m_szErrorInThread[MAX_PATH+100];
    int                   m_nLastErrorCode;
    int                   m_nLastErrorCodeInThread;
    bool                  m_bAutoDisconnect; // if true, auto disconnects after D/L
    bool                  m_bAbortDownload; // if true, this class quits downloading ASAP

    volatile unsigned long  m_cTotalBytesRead; // bytes read for all files
    volatile unsigned long  m_cCurrentFileBytesRead; // bytes read for current file

    IInternetSessionSink * m_pUpdateSink;
};


class CFTPSessionImpl:
    public CInternetSessionImpl<IFTPSession>
{
public:

    CFTPSessionImpl(IFTPSessionUpdateSink * pSink) :
        CInternetSessionImpl<IFTPSession>(pSink) 
    {
    }
        
    virtual ~CFTPSessionImpl()
    {
    }

    virtual bool ConnectToSite(const char * szFTPSite, const char * szDirectory, const char * szUsername, const char * szPassword) 
    {
       m_szLastError[0] = '\0';

       m_hInternetSession = ::InternetOpen(
                           "Microsoft Internet Explorer",   // agent
                            INTERNET_OPEN_TYPE_PROXY,       // access
                            "ftp-gw",                       // proxy server
                            NULL,                           // defaults
                            0);                             // synchronous

       //
       // Connect to remote FTP server.
       //
       m_hFTPSession = ::InternetConnect(
           m_hInternetSession,              // Handle from a previous
           // call to InternetOpen.
           szFTPSite,                       // Server we want to connect to
           INTERNET_INVALID_PORT_NUMBER,    // Use appropriate port
           szUsername,                      // Username, can be NULL
           szPassword,                      // Password, can be NULL
           INTERNET_SERVICE_FTP,            // Flag to use FTP services
           0,                               // Flags (see SDK docs)
           (uint32_t) this);                   // Context for this connection

        if(m_hFTPSession== NULL)
        {
            DoError("Failed to log onto FTP site (%s) : ", szFTPSite);
            return false;
        }

        if (!FtpSetCurrentDirectory(m_hFTPSession, szDirectory))
        {
            DoError("Failed to enter the proper FTP directory (%s) : ", szDirectory);
            return false;
        }

        return true;
    }

    virtual int GetFileListIncrement()
    {
        return 1;
    }

    virtual bool  InitiateDownload(const char * const * pszFileList, 
                                   const char * szDestFolder, 
                                   bool bDisconnectWhenDone = true,
                                   int nMaxBufferSize = 1024*1024)
    {
        bool bRet = CInternetSessionImpl<IFTPSession>::InitiateDownload(pszFileList, szDestFolder, false, nMaxBufferSize);

        // If progress updates are wanted
        if(m_pUpdateSink && *pszFileList != NULL)
            // Fire starting point
                m_pUpdateSink->OnProgress(0, *(m_pszFileList+1), 0);

        SetEvent(m_eventResumeDownload);  // startup the download thread

        return bRet;
    }

};

class CHTTPSessionImpl :
 public CInternetSessionImpl<IHTTPSession>
{
public:


    CHTTPSessionImpl(IHTTPSessionSink * pSink) :
        CInternetSessionImpl<IHTTPSession>(pSink) 
    {

       m_hInternetSession = ::InternetOpen(
                           "Microsoft Internet Explorer",   // agent
                            INTERNET_OPEN_TYPE_PRECONFIG, //INTERNET_OPEN_TYPE_PROXY,       // access
                            NULL,//"ftp-gw",                       // proxy server
                            NULL,                           // defaults
                            0);                             // synchronous  //Fix memory leak -Imago 8/2/09

       if (m_hInternetSession == NULL)
           DoError("Failed to initialize HTTP stuff.");
    }

    virtual ~CHTTPSessionImpl() 
    {
    }

    bool ConstructionSuccess()
    {
        return m_hInternetSession != NULL;
    }

    virtual int GetFileListIncrement()
    {
        return 2;
    }

    virtual bool StartNextFile()
    {
        m_cBytesRead = 0;
        m_cCurrentFileBytesRead = 0;

        ++m_pszFileList;

        unsigned cTries = 0;

        if (*m_pszFileList && strlen(*m_pszFileList) > 0)
        {
            //
            // Open file for download
            //
            while (!(m_hFileConnection = InternetOpenUrl(m_hInternetSession, *m_pszFileList, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0))) //Fix memory leak -Imago 8/2/09
            {
                cTries++;
                debugf("Failed to open URL(%s) for download, try #%d\n", *m_pszFileList, cTries);
                Sleep(500);
                if(cTries >= 5) 
                {
                   DoErrorInThread("Failed to open file for download.");
                   return false;
                }
            }

            ++m_pszFileList;

            if (*m_pszFileList == NULL) // todo, verify the memory is ours
            {
                DoErrorInThread("FileList has bad format");
                return false;
            }

            return OpenDownloadFile();
        }

        return true;
    }

    virtual bool  InitiateDownload(const char * const * pszFileList, 
                                   const char * szDestFolder, 
                                   int nMaxBufferSize = 1024*1024)
    {
        bool bRet = CInternetSessionImpl<IHTTPSession>::InitiateDownload(pszFileList, szDestFolder, false, nMaxBufferSize);

        if(m_pUpdateSink && *(m_pszFileList+2) != NULL)
            // Fire starting point
            m_pUpdateSink->OnProgress(0, *(m_pszFileList+2), 0);

        SetEvent(m_eventResumeDownload);  // startup the download thread

        return bRet;
    }

};


IHTTPSession * CreateHTTPSession(IHTTPSessionSink * pUpdateSink /*= NULL*/)
{
    CHTTPSessionImpl * pNew = new CHTTPSessionImpl(pUpdateSink);

    if (pNew && pNew->ConstructionSuccess())
        return pNew; 
    else
        return NULL;
}

IFTPSession * CreateFTPSession(IFTPSessionUpdateSink * pUpdateSink /*= NULL*/)
{
    CFTPSessionImpl * pNew = new CFTPSessionImpl(pUpdateSink);

    return pNew; 
}                                                         
