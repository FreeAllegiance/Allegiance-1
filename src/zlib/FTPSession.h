
/*-------------------------------------------------------------------------
 * clintlib\FTPSession.h
 * 
 * Example usage of this code can be found at the bottom of this file.
 *
 * IFTPSession allows you to easily transfer files from an FTP 
 * site while hiding the low level aspects of the Windows FTP API.
 *
 * IFTPSessionUpdateSink is used to receiving information about a transfer.
 *
 * Currently only downloading is supported.
 *
 *
 * Owner: 
 * 
 * Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/

#include <cstdlib>

//
//  IFTPSessionUpdateSink is used to receive events about the transfer.  
//  You don't have to use it--not even for getting error messages.
//  GetLastErrorMessage() also can be used to get error messages.
//
class IInternetSessionSink
{
public:
    /*-------------------------------------------------------------------------
     * OnProgress()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Report the progress of a transfer.
     *
     * Paramters:
     *      cTotalBytes: Total bytes of the all files in the transfer.
     *
     *      szCurrentFile: Name of the current file being transfered.
     *
     *      cCurrentFileBytes: Count of bytes currently transfered for the current file.
     *
     * Remarks:
     *
     *      Updates occur at the start of the transfer, when a file finishes, and at 8k intervals
     *      during large file transfers.  The 8k interval is determined by Windows and subject to change.
     *
     */
    virtual void OnProgress(unsigned long cTotalBytes, const char* szCurrentFile, unsigned long cCurrentFileBytes) {}

    /*-------------------------------------------------------------------------
     * OnProgress()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Report error occuring during transfer.
     *
     * Paramters:
     *      szErrorMessage: Error message.  Could be long and detailed.
     */
    virtual void OnError(char *szErrorMessage) {}

    /*-------------------------------------------------------------------------
     * OnDataReceived()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Allow user to scan or change data as it comes in; and skip this 
     *      file if wanted.  
     *
     * Paramters:
     *      pData:  pointer to file data chunk
     *      cBytes: size of data chunk
     *
     * Returns:
     *      true: if you want this file's transfer to continue, false to start
     *      next file.
     */
    virtual bool OnDataReceived(void * pData, unsigned long cBytes) { return true; }

    /*-------------------------------------------------------------------------
     * OnFileCompleted()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Signal that another file is transfered and written to disk with no errors.  
     *
     * Paramters:
     *      szFileName: filename of transfered file with complete local path.  
     *
     * Returns:
     *      true:  if file downloaded okay
     *      false: if file should be re-downloaded (sometimes http downloads fail, so you might want to retry)
     */
    virtual bool OnFileCompleted(char *szFileName) 
    {
        return true;
    }

    /*-------------------------------------------------------------------------
     * OnTransferComplete()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Singal that the entire transfer is over.  This is also called in the event
     * of an error (or manual abort) stops the transfer.
     */
    virtual void OnTransferFinished() {}

};


class IHTTPSessionSink :
public IInternetSessionSink
{
};

class IFTPSessionUpdateSink :
public IInternetSessionSink
{
};



class IInternetSessionBase
{
public:
    virtual ~IInternetSessionBase() {}

    /*-------------------------------------------------------------------------
     * ContinueDownload()
     *-------------------------------------------------------------------------
     * Purpose:         
     *    Find out if any errors or events occured during the download.  
     *    Start downloading the next file if done with current one.
     *    Fire events as needed.
     *
     * Returns:
     *    false if aborted, or error, or successful download; true if download
     *    is incomplete and needs to be continued.
     *
     * Remarks:
     *    Errors (and other events) are reported to the Sink.
     */
    virtual bool  ContinueDownload() = 0;


    /*-------------------------------------------------------------------------
     * GetLastErrorMessage()
     *-------------------------------------------------------------------------
     * Returns:
     *    Last error message or NULL if no errors have occured.
     *
     * Remarks:
     *    Errors also are reported to the Sink.
     */               
    virtual const char* GetLastErrorMessage() = 0; // TODO: make this report the error code too


    /*-------------------------------------------------------------------------
     * GetDownloadPath()
     *-------------------------------------------------------------------------
     *
     * Returns a path of where the downloaded files are going.  Guarenteed to
     * end with a backslash.
     */
    virtual const char* GetDownloadPath() = 0; 

    /*-------------------------------------------------------------------------
     * Abort()
     *-------------------------------------------------------------------------
     * Purpose:
     *    Abort the download.
     *
     * Parameters:
     *    bDisconnect: if true, the disconnect is automatic
     */
    virtual void Abort(bool bAutoDisconnect = true) = 0; 
};

class IFTPSession :
    public IInternetSessionBase
{
public:
    virtual ~IFTPSession() {}

    /*-------------------------------------------------------------------------
     * ConnectToSite()
     *-------------------------------------------------------------------------
     * Paramters:
     *    szFTPSite:    FTP address of the server to connect to
     *    szDirectory:  The directory on the server where transfers will occur from
     *
     * Returns:
     *    true if success; false if error
     *
     * Remarks:
     *    Errors are reported to the Sink.
     */
    virtual bool  ConnectToSite(const char *szFTPSite, 
                                const char *szDirectory, 
                                const char *szUsername, 
                                const char *szPassword) = 0; 


    /*-------------------------------------------------------------------------
     * InitiateDownload()
     *-------------------------------------------------------------------------
     * Purpose:         
     *    Begin downloading a batch of files.  You must call ContinueDownload()
     *    periodically to ensure file(s) get downloaded.
     *
     * Paramters:
     *    pszFileList:  A list of files to download.  This is a pointer to a
     *                  NULL terminated array of NULL terminated char pointers.  
     *
     *    szDestFolder: The directory on the local machine of where the files go
     *
     *    bDisconnectWhenDone: if true, session automatically disconnects when done
     *
     *    nMaxBufferSize: maximum amount of memory allocated for download buffer
     *
     * Returns:
     *    true if success; false if error
     *
     * Remarks:
     *    Errors are reported to the Sink.
     */
    virtual bool  InitiateDownload(const char * const * pszFileList, 
                                   const char * szDestFolder, 
                                   bool bDisconnectWhenDone = true,
                                   int nMaxBufferSize = 1024*1024) = 0;

    /*-------------------------------------------------------------------------
     * Disconnect()
     *-------------------------------------------------------------------------
     * Purpose:
     *    Disconnect from FTP server.
     *
     * Returns:
     *    true if success; false if error
     *
     * Remarks:
     *    Errors are reported to the Sink.
     */
    virtual bool Disconnect() = 0; 
};

class IHTTPSession :
    public IInternetSessionBase
{
public:
    virtual ~IHTTPSession() {}

    /*-------------------------------------------------------------------------
     * InitiateDownload()
     *-------------------------------------------------------------------------
     * Purpose:         
     *    Begin downloading a batch of files.  You must call ContinueDownload()
     *    periodically to ensure file(s) get downloaded.
     *
     * Paramters:
     *    pszFileList:  A list of files to download.  This is a pointer to a
     *                  NULL terminated array of NULL terminated char pointers.  
     *                  Each entry is a pair: the first is the URL, the second
     *                  is the local filename
     *
     *    szDestFolder: The directory on the local machine of where the files go
     *
     *    nMaxBufferSize: maximum amount of memory allocated for download buffer
     *
     * Returns:
     *    true if success; false if error
     *
     * Remarks:
     *    Errors are reported to the Sink.
     */
    virtual bool  InitiateDownload(const char * const * pszFileList, 
                                   const char * szDestFolder, 
                                   int nMaxBufferSize = 1024*1024) = 0;

};


IFTPSession * CreateFTPSession(IFTPSessionUpdateSink * pUpdateSink = NULL); // use this to make a new IFTPTransfer object


IHTTPSession * CreateHTTPSession(IHTTPSessionSink * pUpdateSink = NULL);




/*         SAMPLE CODE:

#include "stdafx.h"
#include "FTPSession.h"


class CFTPStatusClass :
    public IFTPSessionUpdateSink
{
public:

    void OnProgress(unsigned long cTotalBytes, const char* szCurrentFile, unsigned long cCurrentFileBytes) 
    {
        char sz[80];
        const int cEstimatedSize = 900;

        sprintf(sz, "%2.2f%%   %i  %s  %i\n", 100.0f*float(cTotalBytes)/float(cEstimatedSize), cTotalBytes, szCurrentFile, cCurrentFileBytes);
        OutputDebugString(sz);
    }

    void OnError(char *szErrorMessage)
    {
        OutputDebugString("\nError:\n");
        OutputDebugString(szErrorMessage);
    }
};


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    CFTPStatusClass errorClass;

    IFTPSession *pFTP = CreateFTPSession(&errorClass);

    if (pFTP->ConnectToSite("a-markcu1", "Allegiance", "Anonymous", "Marco"))
    {
        char *pszFileList[] = {"test.vbs", "chat.vbs", "events.vbs", NULL};

        if (pFTP->InitiateDownload(pszFileList, "C:\\temp"))
        {
            while (pFTP->ContinueDownload())
            {
            }
        }
    }

    delete pFTP;

	return 0;
}

*/