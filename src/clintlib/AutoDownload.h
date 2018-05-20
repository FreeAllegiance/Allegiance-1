#pragma once
/*-------------------------------------------------------------------------
 * clintlib\AutoDownload.h
 * 
 *
 * Owner: 
 * 
 * Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/

#include <cstdio>
#include <FTPSession.h>
#include <regkey.h>
#include <windows.h>
#include <zassert.h>

class IAutoUpdateSink;
/*      AutoDownload incepts the logon ack, delaying it until the download
 *      is complete.  Use this to tell Autodown what to Ack with, once the
 *      transfer is complete.
 */





class IAutoDownload
{
public:

    /*-------------------------------------------------------------------------
     * SetFTPSite()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Set the site info so we know what sever to connect to.
     */
    virtual void SetFTPSite(const char * szFTPSite, const char * szInitialDirectory, const char * szUsername, const char * szPassword) = 0;

    /*-------------------------------------------------------------------------
     * SetOfficialFileListAttributes()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Set the CRC we can verify that the FTP site has the correct FileList.txt
     *
     * Parameters:
     *      nCRC: official CRC for filelist.txt at FTP site
     *      nFileSize: size of filelist.txt at FTP Site
     */
    virtual void SetOfficialFileListAttributes(int nCRC, unsigned nFileSize) = 0;

    /*-------------------------------------------------------------------------
     * SetOfficialTime()
     *-------------------------------------------------------------------------
     */
    virtual void SetArtPath(const char * pArtPath) = 0;

    /*-------------------------------------------------------------------------
     * SetFilelistSubDir()
     * Since the filename Filelist.txt is hardcoded everywhere, and sometimes
     * we want differentiate between two filelists, we have this.
     * This is the sub dir of where the filelist is downloaded from; if not
     * set, then the regular base directory is used.
     *-------------------------------------------------------------------------
     */
    virtual void SetFilelistSubDir(const char * pszPath) = 0;

    /*-------------------------------------------------------------------------
     * BeginUpdate()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Setup variables and download the file list.
     *
     * Parameters:
     *      pSink: pointer to the object receiving events about the download
     *      bForceCRCCheck: if true, file times are ignored and CRC are always checked
     *      bSkipReloader: if true, then reloader is not launch: an error is displayed
     */
    virtual void BeginUpdate(IAutoUpdateSink * pSink, bool bForceCRCCheck) = 0;

    /*-------------------------------------------------------------------------
     * HandleAutoDownload()
     *-------------------------------------------------------------------------
     * Parameters:
     *      dwTimeAlloted: time slice that we have to get or process data.
     *                     HandleAutoDownload() will usually take AT LEAST this much
     *                     time, and on rare occasions less.
     * Purpose:
     *      Check to see if AutoUpdate is done from within the main thread.  If so,
     *    restart or relogon as needed.
     *
     * Remarks:
     *      This is intended to be called periodically during the download.
     */
     virtual void HandleAutoDownload(DWORD dwTimeAlloted) = 0;

    /*-------------------------------------------------------------------------
     * Abort()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Abort the autodownload.
     */
     virtual void Abort() = 0;
};


IAutoDownload * CreateAutoDownload();






/*---------------------------------------------------------------------------------

  IAutoUpdateSink:

  This receives events about the Autodownload system

 ---------------------------------------------------------------------------------*/

class IAutoUpdateSink :
    public IHTTPSessionSink  
{
public:
    virtual void OnBeginRetrievingFileList() {}

    virtual void OnRetrievingFileListProgress(unsigned long nFileSize, unsigned long cCurrentBytes) {}

    // once we've got the right filelist, we begin analysis of local files
    virtual void OnBeginAnalysis() {}

    virtual void OnAnalysisProgress(float fPercentDone) {}

    virtual bool ShouldFilterFile(const char * szFileName) // szFileName is base filename (not including path)
    { 
      return false; // if returns true, then file is not downloaded
    } 

    virtual void OnProgress(unsigned long cTotalBytes, const char* szCurrentFile, unsigned long cCurrentFileBytes, unsigned cEstimatedSecondsLeft) {}
    
    virtual void OnBeginDownloadProgressBar(unsigned cTotalBytes, int cFiles) {}

    virtual void OnUserAbort() {}

    /*-------------------------------------------------------------------------
     * SetErrorFunction()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Set a callback function for when an error occurs during moving files.
     *
     * Returns: true if the autoupdate system should try again
     */
    virtual bool OnMoveError(char * szErrorMsg) 
    {
      return false;
    }

    // returns true if file should be registered
    virtual bool ShouldRegister(char * szFullFileName) // path is included in szFullFileName
    { 
      return false;
    }

    // returns registration exit code (0 means success)
    virtual int RegisterFile(char * szFullFileName) // path is included in szFullFileName
    {
      return -1;
    }

    // this is called in the AutoUpdate system destructor
    virtual void OnAutoUpdateSystemTermination(bool bErrorOccurred, bool bRestarting) {}
};







class CAutoDownloadUtil  // Shared between Reloader.exe and IAutoDownload's privates
{
public:

    /*-------------------------------------------------------------------------
     * MoveFiles()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Copies temp files into ArtPath.  There's a hardcoded check for 
     *      Allegiance.exe. Assumes current folder is where Allegiance.exe 
     *      should go (not ArtPath).
     *
     * Paramters:
     *      szTempPath:  where the all the files come from
     *      szArtPath:   where the art files go
     *      bSkipSharingViolation: if true then we don't quit when a sharing violation occurs.
     *                             if false then we quit with error on such cases.
     *      pbFilesWereSkipped: is used at only if bSkipSharingViolation is true -AND- pbFilesWereSkipped != NULL.
     *                          *pbFilesWereSkipped is set to true if files were indeed skipped due to a
     *                          sharing violation.
     *      bNoRegistryWrite: do not update registry
     *      pSink : AutoUpdate sink where move errors are reported
     * Returns:
     *      true only on success
     */
    static bool MoveFiles(const char * szTempPath, const char * szArtPath_, bool bSkipSharingViolation, 
                          bool * pbFilesWereSkipped, char * szErrorMsg, IAutoUpdateSink * pSink)
    {

      WIN32_FIND_DATA finddata;
      HANDLE hsearchFiles = 0;

      char szSourceSpec[MAX_PATH+20];
      char szArtPath[MAX_PATH+20];
      strcpy(szSourceSpec, szTempPath);
      strcat(szSourceSpec, "*.*");

      strcpy(szArtPath, szArtPath_);

      int cLen = strlen(szArtPath);
      if (cLen == 0 || szArtPath[cLen-1] != '\\')
      {
          szArtPath[cLen++] = '\\';
          szArtPath[cLen] = 0;
      }

      bool bFilesWereSkipped = false;

      // count the files in the file path
      hsearchFiles = FindFirstFile(szSourceSpec, &finddata);
      if (INVALID_HANDLE_VALUE == hsearchFiles)
      {
          return false;
      }

      char szSource[MAX_PATH+20];
      char szDest[MAX_PATH+20];

      while (INVALID_HANDLE_VALUE != hsearchFiles)
      {
        // skip directory listings "." and ".."; and filelist (filelist is moved last)
        if (finddata.cFileName[0] != '.' && 
            _stricmp(finddata.cFileName, "filelist.txt") != 0 &&
            (!pSink || !pSink->ShouldFilterFile(finddata.cFileName)))
        {
            // setup move paths
            strcpy(szSource, szTempPath);
            strcat(szSource, finddata.cFileName);
            GetFileNameWithPath(szDest, finddata.cFileName, szArtPath, ".\\");

            // Move files to their dest
            if (!MoveFilePrivate(szSource, szDest, bSkipSharingViolation, &bFilesWereSkipped, szErrorMsg, pSink))
                return false;

            // consider registering special files
            if(pSink && pSink->ShouldRegister(szDest))
            {
              int nExitCode = pSink->RegisterFile(szDest);

              if (nExitCode != 0)
              {
                // registration failed; move file back so autoupdate attempts to register later; then abort
                MoveFile(szDest, szSource);

                if (szErrorMsg)
                    sprintf(szErrorMsg, "Failed to Register file %s; registration exit code(%d)", finddata.cFileName, nExitCode);

                return false;
              }
            }
        }

        if (!FindNextFile(hsearchFiles, &finddata))
        {
          FindClose(hsearchFiles);
          hsearchFiles = INVALID_HANDLE_VALUE;
        }
      }

      //
      // finish off with moving the filelist; if no files were skipped; if files were skipped, then reloader
      // needs to move the filelist
      //
      if(!bFilesWereSkipped)
      {
          strcpy(szSource, szTempPath);
          strcat(szSource, "filelist.txt");

          if (CAutoDownloadUtil::GetFileLength(szSource) != -1) // check for existance
          {
            GetFileNameWithPath(szDest, "filelist.txt", szArtPath, ".\\");

            if (!MoveFilePrivate(szSource, szDest, bSkipSharingViolation, &bFilesWereSkipped, szErrorMsg, pSink))
                return false;
          }
      }

      FindClose(hsearchFiles);

      if (bSkipSharingViolation && pbFilesWereSkipped)
      *pbFilesWereSkipped = bFilesWereSkipped;

      return true;
    }

    static const char * GetEXEFileName(int nIndex)
    {
        static const char * pszEXEFiles[] =
        {
            "CliConfig.exe",
            "fsmon.exe",
            "readme.txt",
            "patcher.exe", 
            "FileList.txt",
            "Reloader.exe",
            "msrgbits.inf",
            "msrgtran.dll",
            "msrgip.dll"
            // the file muse be at least 8 characters (including ext)  For example: fsmon.exe
            // increment g_cEXEFiles, if you add to this
        };
        #define g_cEXEFiles 10 //imago dur 6/10

        return pszEXEFiles[nIndex];
    }

    /*-------------------------------------------------------------------------
     * GetFileNameWithPath()
     *-------------------------------------------------------------------------
     * Returns:
     *      The full path of where the file belongs.
     */
    static void GetFileNameWithPath(OUT char * szFileNameWithPath, 
                             IN  const char * szRawFileName, 
                             IN  const char * szArtPath,
                             IN  const char * szEXEPath)
    {
        //
        // Move special files to current directory
        //

        //
        // NOTE: if you add special files, try adding them to the GetEXEFileName()
        // list to ensure get special care for the PC Gamer Build (Beta 1) bug.
        //   ///////////////////////////////////////////////////////
        if (_stricmp(szRawFileName, "AllegianceRetail.exe") == 0 ||
            _stricmp(szRawFileName, "AllegianceDebug.exe") == 0 ||
            _stricmp(szRawFileName, "AllegianceTest.exe") == 0 ||
            _stricmp(szRawFileName, "Allegiance.exe") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "Allegiance.exe");
        }
        else
        if (_stricmp(szRawFileName, "AllegianceRetail.pdb") == 0 ||
            _stricmp(szRawFileName, "AllegianceDebug.pdb") == 0 ||
            _stricmp(szRawFileName, "AllegianceTest.pdb") == 0 ||
            _stricmp(szRawFileName, "Allegiance.pdb") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "Allegiance.pdb");
        }
        else
        if (_stricmp(szRawFileName, "AllegianceRetail.sym") == 0 ||
            _stricmp(szRawFileName, "AllegianceDebug.sym") == 0 ||
            _stricmp(szRawFileName, "AllegianceTest.sym") == 0 ||
            _stricmp(szRawFileName, "Allegiance.sym") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "Allegiance.sym");
        }
        else
        if (_stricmp(szRawFileName, "AllegianceRetail.map") == 0 ||
            _stricmp(szRawFileName, "AllegianceDebug.map") == 0 ||
            _stricmp(szRawFileName, "AllegianceTest.map") == 0 ||
            _stricmp(szRawFileName, "Allegiance.map") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "Allegiance.map");
        }
        else ///////////////////////////////////////////////////////
        if (_stricmp(szRawFileName, "AllSrvRetail.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrv.exe") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.exe");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvRetail.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrv.pdb") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.pdb");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvRetail.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrv.sym") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.sym");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvRetail.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.map") == 0 ||
            _stricmp(szRawFileName, "AllSrv.map") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.map");
        }
        else ///////////////////////////////////////////////////////
        if (_stricmp(szRawFileName, "AllSrvRetail.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrv.exe") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.exe");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvRetail.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrv.pdb") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.pdb");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvRetail.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrv.sym") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.sym");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvRetail.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvDebug.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvTest.map") == 0 ||
            _stricmp(szRawFileName, "AllSrv.map") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrv.map");
        }
        else ///////////////////////////////////////////////////////
        if (_stricmp(szRawFileName, "AGCRetail.dll") == 0 ||
            _stricmp(szRawFileName, "AGCDebug.dll") == 0 ||
            _stricmp(szRawFileName, "AGCTest.dll") == 0 ||
            _stricmp(szRawFileName, "AGC.dll") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AGC.dll");
        }
        else
        if (_stricmp(szRawFileName, "AGCRetail.pdb") == 0 ||
            _stricmp(szRawFileName, "AGCDebug.pdb") == 0 ||
            _stricmp(szRawFileName, "AGCTest.pdb") == 0 ||
            _stricmp(szRawFileName, "AGC.pdb") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AGC.pdb");
        }
        else
        if (_stricmp(szRawFileName, "AGCRetail.sym") == 0 ||
            _stricmp(szRawFileName, "AGCDebug.sym") == 0 ||
            _stricmp(szRawFileName, "AGCTest.sym") == 0 ||
            _stricmp(szRawFileName, "AGC.sym") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AGC.sym");
        }
        else
        if (_stricmp(szRawFileName, "AGCRetail.map") == 0 ||
            _stricmp(szRawFileName, "AGCDebug.map") == 0 ||
            _stricmp(szRawFileName, "AGCTest.map") == 0 ||
            _stricmp(szRawFileName, "AGC.map") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AGC.map");
        }
        else ///////////////////////////////////////////////////////
        if (_stricmp(szRawFileName, "AllSrvUIRetail.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.exe") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.exe");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvUIRetail.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.pdb") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.pdb");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvUIRetail.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.sym") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.sym");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvUIRetail.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.map") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.map");
        }
        else ///////////////////////////////////////////////////////
        if (_stricmp(szRawFileName, "AllSrvUIRetail.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.exe") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.exe") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.exe");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvUIRetail.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.pdb") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.pdb") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.pdb");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvUIRetail.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.sym") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.sym") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.sym");
        }
        else
        if (_stricmp(szRawFileName, "AllSrvUIRetail.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvUIDebug.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvUITest.map") == 0 ||
            _stricmp(szRawFileName, "AllSrvUI.map") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AllSrvUI.map");
        }
        else ///////////////////////////////////////////////////////
        if (_stricmp(szRawFileName, "AutoUpdateRetail.exe") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateDebug.exe") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateTest.exe") == 0 ||
            _stricmp(szRawFileName, "AutoUpdate.exe") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AutoUpdate.exe");
        }
        else
        if (_stricmp(szRawFileName, "AutoUpdateRetail.pdb") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateDebug.pdb") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateTest.pdb") == 0 ||
            _stricmp(szRawFileName, "AutoUpdate.pdb") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AutoUpdate.pdb");
        }
        else
        if (_stricmp(szRawFileName, "AutoUpdateRetail.sym") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateDebug.sym") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateTest.sym") == 0 ||
            _stricmp(szRawFileName, "AutoUpdate.sym") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AutoUpdate.sym");
        }
        else
        if (_stricmp(szRawFileName, "AutoUpdateRetail.map") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateDebug.map") == 0 ||
            _stricmp(szRawFileName, "AutoUpdateTest.map") == 0 ||
            _stricmp(szRawFileName, "AutoUpdate.map") == 0)
        {
            strcpy(szFileNameWithPath, szEXEPath);
            strcat(szFileNameWithPath, "AutoUpdate.map");
        }
        else
        {
            for (int i = 0; i < g_cEXEFiles; ++i)
            {
                if (_stricmp(szRawFileName, GetEXEFileName(i)) == 0)
                {
                    strcpy(szFileNameWithPath, szEXEPath);
                    strcat(szFileNameWithPath, szRawFileName);
                    return;
                }
            }
            //
            // Must be an ArtFile!
            //
            strcpy(szFileNameWithPath, szArtPath);
            strcat(szFileNameWithPath, szRawFileName);
        }
    }


    //////////////////////////////////////////////////////////////////////////

    static unsigned GetFileLength(char *szFileName)
    {
        HANDLE hFile = CreateFile(szFileName, 
                                 0/*GENERIC_READ*/,  // 0 == query only
                                 FILE_SHARE_READ, 
                                 NULL, 
                                 OPEN_EXISTING, 
                                 FILE_ATTRIBUTE_NORMAL, 
                                 NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            return (unsigned)-1;
        }

        unsigned nSize = ::GetFileSize(hFile, NULL);

        ::CloseHandle(hFile);

        return nSize;
    }

private:


    /*-------------------------------------------------------------------------
     * Move File considering bSkipSharingViolation
     *
     * returns false if error, true on success
     */
    static bool MoveFilePrivate(char * szSource, char * szDest, bool bSkipSharingViolation, 
                                bool * pbFilesWereSkipped, char * szErrorMsg, IAutoUpdateSink * pSink)
    {
        bool bErrorOccured;
        bool bTryAgain;
        
        do
        {
            ::DeleteFile(szDest);

            // Note: considered using MoveFileEx, but win95/98 doesn't support it
            BOOL bResult = ::MoveFile(szSource, szDest);
        
            if(!bResult)
            {
                if (bSkipSharingViolation)
                {
                    int nErr = GetLastError();

                    bool bSkip = (nErr == ERROR_ALREADY_EXISTS || nErr == ERROR_ACCESS_DENIED || nErr == ERROR_SHARING_VIOLATION);

                    if (bSkip) 
                    {
                        *pbFilesWereSkipped = true;
                        bErrorOccured = false;
                    }
                    else 
                        bErrorOccured = true;
                }
                else 
				{
                    g_pDebugLogger->Log("Moving Files Error:" + GetLastError());

                    bErrorOccured = true;
                }
            }
            else 
                bErrorOccured = false;

            bTryAgain = false;

            if (bErrorOccured && (pSink || szErrorMsg))
            {
                char szErrorMessage[2*MAX_PATH+50];

                FormatErrorMessage(szErrorMessage, GetLastError());

                strcat(szErrorMessage, "\n\r\n\r   Source: ");

                strcat(szErrorMessage, szSource);

                strcat(szErrorMessage, "\n\r\n\r   Dest: ");
                
                strcat(szErrorMessage, szDest);

                if (szErrorMsg)
                    strcpy(szErrorMsg, szErrorMessage);

                if (pSink)
                   bTryAgain = pSink->OnMoveError(szErrorMessage);
            }

        } while(bTryAgain);

        return !bErrorOccured; // true if successful move
    }

    /*-------------------------------------------------------------------------
     * FormatErrorMessage()
     *-------------------------------------------------------------------------
     * Paramters:
     *    dwErrorCode: take a dwErrorCode and print what it means as text    
     * 
     */
    static void FormatErrorMessage(char *szBuffer, DWORD dwErrorCode)
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
    }

};





bool LaunchReloaderAndExit(bool bReLaunchAllegianceAsMinimized);
