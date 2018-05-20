// AllSrvUp.cpp : Implementation of WinMain

#include "pch.h"
#include "resource.h"
#include "AutoUpdate.h"

//imago enhanced /w new operation (Graceful shutdown) 6/10
#include <agc.h>
#include <AllSrvModuleIDL.h>

#include "AdminSessionSecure.h"
#include "AdminSessionSecureHost.h"


//imago 6/10
IAdminGamesPtr  m_spAdminGamesPtr = 0;
IAdminGamePtr   m_spAdminGamePtr = 0;
IAdminUsersPtr  m_spAdminUsersPtr = 0;

/////////////////////////////////////////////////////////////////////////////

void MsgBox(bool bService, const char* format, ...)
{
    const size_t size = 256;
    char         bfr[size];

    va_list vl;
    va_start(vl, format);
    _vsnprintf(bfr, size, format, vl);
    va_end(vl);

    OutputDebugString(bfr);

	//Imago #105 7/10
	if (bService) {
        // write to file
        char szFileData[sizeof(bfr) + 200];
        SYSTEMTIME st;
        GetLocalTime(&st);
        sprintf(szFileData, "\r\n\r\nUpdate attempt at: %d/%d/%02d at %d:%02d:%02d  (local time)\r\n%s\r\n\r\n", st.wMonth, st.wDay, st.wYear % 100, st.wHour, st.wMinute, st.wSecond, bfr);
        UTL::AppendFile("UpdateError.txt", szFileData, strlen(szFileData));
	} else {
		MessageBox(NULL, bfr, "AutoUpdate", MB_SERVICE_NOTIFICATION);
	}
}

/////////////////////////////////////////////////////////////////////////////

class CAutodownloadSink :
    public IAutoUpdateSink
{
public:

    void OnAutoUpdateSystemTermination(bool bErrorOccurred, bool bRestarting) 
    {
        if (!bErrorOccurred)
        {
            // update succeeded
            if (m_pdlg->m_szPostUpdateEXE[0] != 0)
            {
                char * szEXE = m_pdlg->m_szPostUpdateEXE;

                // look for signal that this is an Service
                if ((szEXE[0] == 'S' || szEXE[0] == 's') && szEXE[1] == ':')
                {
                  // Start the post update program utility as an service
                  bool bFailed = false;

                  szEXE += 2;

                  SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, GENERIC_READ);

                  if (hSCM)
                  {
                    SC_HANDLE hService = ::OpenService(hSCM, szEXE, SERVICE_START);

                    if (hService)
                    {
                      if (!StartService(hService, NULL, NULL))
                        bFailed = true;
                    }
                    else bFailed = true;
                  }
                  else bFailed = true;

                  if (bFailed)
                  {
                    MsgBox(1,"Failed to relaunch %s as a service.", szEXE);
                  }
                }
                else
                {
                  // Start the post update program utility as an EXE
                  if((int)ShellExecute(0, 
                                       "Open", 
                                       szEXE,
                                       "", 
                                       NULL,
                                       SW_SHOWNORMAL
                                       ) <= 32)
                  {
                      MsgBox(0,"After a seemingly successful update, there was an error launching %s", m_pdlg->m_szPostUpdateEXE);
                  }
                }
            }
        }

        m_pdlg->DestroyWindow();
    }

    virtual void OnError(char *szErrorMessage) 
    {
		char * szEXE = m_pdlg->m_szPostUpdateEXE;
		bool bService = false;
		// look for signal that this is an Service
		if ((szEXE[0] == 'S' || szEXE[0] == 's') && szEXE[1] == ':')
			bService = true;

        debugf("\n\nAutoUpdate Error\n\n%s\n\n", szErrorMessage);
        MsgBox(bService,"\n\nAutoUpdate Error\n\n%s\n\n", szErrorMessage);
    }

    virtual void OnBeginRetrievingFileList() 
    {
        debugf("\nBeginning Filelist Download (Step 1 of 3)\n\n");
    }

    virtual void OnRetrievingFileListProgress(unsigned long nFileSize, unsigned long cCurrentBytes) 
    {
        m_pdlg->m_ProgressFileList.SendMessage(PBM_SETPOS, int(100.0f*cCurrentBytes/nFileSize));

        debugf("\rFilelist Download Progress %d out of %d (%2.2f%%)", cCurrentBytes, nFileSize, 100.0f*cCurrentBytes/nFileSize);
    }

    virtual void OnUserAbort()
    {
        m_pdlg->DestroyWindow();
    }

    virtual void OnBeginAnalysis() 
    {
        debugf("\n\nBeginning Local Files Verification Progress (Step 2 of 3)\n\n");
    }

    virtual void OnAnalysisProgress(float fPercentDone) 
    {
        m_pdlg->m_ProgressAnalyze.SendMessage(PBM_SETPOS, int(100.0f * fPercentDone));

        debugf("Verification Progress (%2.2f%%)\n", fPercentDone * 100.0f);
    }

    void OnBeginDownloadProgressBar(unsigned cTotalBytes, int cFiles) 
    {
        debugf("\nBeginning File Download Process (Step 3 of 3)\n\n");
        m_cGrandTotalBytes = cTotalBytes;
        m_cTotalFiles = cFiles;
    };

    virtual void OnProgress(unsigned long cTotalBytes, const char* szCurrentFile, unsigned long cCurrentFileBytes, unsigned cEstimatedSecondsLeft) 
    {
        static const char*   szPrevCurrentFile = NULL; // last file transfered
        static unsigned      cFilesCompleted = 0;

        //
        // Detect current file change
        //
        if (szCurrentFile != NULL && szCurrentFile != szPrevCurrentFile)
        {
            cFilesCompleted++;
            debugf("\rStarting next file: %s (%i out of %i)\n", szCurrentFile, cFilesCompleted, m_cTotalFiles);
            szPrevCurrentFile = szCurrentFile;
            m_pdlg->m_staticFileName.SetWindowText(szCurrentFile);
        }
        else
        {
            m_pdlg->m_ProgressDownload.SendMessage(PBM_SETPOS, int(100.0f*float(cTotalBytes)/float(m_cGrandTotalBytes)));
            debugf("\r%2.2f%%   %i  %s  %i  ", 100.0f*float(cTotalBytes)/float(m_cGrandTotalBytes), cTotalBytes, szCurrentFile, cCurrentFileBytes);
        }
    }

    /*-------------------------------------------------------------------------
     * OnMoveError()
     *-------------------------------------------------------------------------
     * Purpose:
     *      Handles MoveFiles() Errors.  Give user option to try again.
     *
     * Returns: true if the user wants to try again.
     */
    bool OnMoveError(char *szErrorMessage)
    {
        static int cCalled = 0;

        //
        // The first few time this function is called, we ignore it and keep
        // trying to move the files.  It's possbile that Allegiance.exe isn't
        // done exiting and has some files open.
        //
        if (++cCalled > 25) // 25 secs
        {
            //
            // Give User the option to wait longer
            //
            int nErrorCode = GetLastError();

            // Give time for Allegiance to minimize
            ::Sleep(1000);

            char szTitle[100];
            sprintf(szTitle, "Error occured during auto update (Code = %d)", nErrorCode);

            char szMsg[500];

            sprintf(szMsg, "%s\r\n\r\n\r\n\r\nDo you wish to retry moving the files?", szErrorMessage);

			//Imago commented out...7/10 #105
            //if (::MessageBox(NULL, szMsg, szTitle, MB_YESNO | MB_SERVICE_NOTIFICATION) == IDYES)
            //    return true;
            //else
                return false;
        }
        else
        {
            Sleep(1000); // wait a bit for exe's files to close and try again.

            return true; // signal to try again
        }
    }

    char * GetBaseFileName (char * szFullFileName)
    {
      // find pre filename
      char * szFileName = strrchr(szFullFileName, '\\');
      if (!szFileName)
      {
        szFileName = strrchr(szFullFileName, '/');
        if (!szFileName)
          szFileName = szFullFileName;
        else szFileName++;
      }
      else szFileName++;
      return szFileName;
    }

    // returns true if file should be registered
    bool ShouldRegister(char * szFullFileName) // path is included in szFullFileName
    { 
      char * szFileName = GetBaseFileName(szFullFileName);

      if (_stricmp(szFileName, "AGC.DLL") == 0 ||
          _stricmp(szFileName, "AllSrv.EXE") == 0 ||
          _stricmp(szFileName, "AllSrv32.EXE") == 0)
      {
        return true; // true means register this
      }

      return false; // false means don't register this
    }

    // returns exit code (0 means success)
    int RegisterFile(char * szFullFileName) // path is included in szFullFileName
    {
      char * szFileName = GetBaseFileName(szFullFileName);

      if (_stricmp(szFileName, "AGC.DLL") == 0)
      {
        char szPath[MAX_PATH];
        GetSystemDirectory(szPath, MAX_PATH);

        PathString strPath(PathString(szPath) + PathString("regsvr32"));
        
        ZString strCommand = strPath;

        strCommand += " /s ";
        strCommand += szFullFileName;

        return system(PCC(strCommand));
      }
      else
      if (_stricmp(szFileName, "AllSrv.EXE") == 0 ||
          _stricmp(szFileName, "AllSrv32.EXE") == 0)
      {
        char szPath[MAX_PATH+20];

        strncpy(szPath, szFullFileName, sizeof(szPath) - 10);
        strcat(szPath, " -reregister");

        // call szFullFileName -reregserver
        return system(szPath);
      }
      return -1; // unhandled file
    }

    // if returns true, then file is not downloaded
    bool ShouldFilterFile(const char * szFileName)  // path not included
    { 
      if (_stricmp(szFileName, "AutoUpdate.exe") == 0)
        return true;

      return false; 
    } 

    int m_cGrandTotalBytes, m_cTotalFiles;

    CAutoUpdate * m_pdlg;

} g_AutoDownloadSink;

/////////////////////////////////////////////////////////////////////////////

int CAutoUpdate::Init(int argc, char* argv[])
{
    /* example commmand line:
     AllSrv32.exe FFFFFFFF 120000 "http://a-markcu1" "/artcomp/1850" "c:/fed/objs/artwork"
    */
    if (argc != 7)
        return -2;

    if (argv[1][0] != '-' && argv[1][0] != 0) // - means execute nothing after update
    {
      strncpy(m_szPostUpdateEXE, argv[1], sizeof(m_szPostUpdateEXE));
    }
    else
      m_szPostUpdateEXE[0] = 0;

    char szCRC[30];
    strncpy(szCRC, argv[2], sizeof(szCRC));
    _strupr(szCRC);

    int nFilelistCRC = UTL::hextoi(szCRC);

    if (nFilelistCRC == 0)
        return -3;

    int nFilelistSize = atoi(argv[3]);

    if (nFilelistSize == 0)
        return -4;

    m_pAutoDownload = CreateAutoDownload();

    m_pAutoDownload->SetFTPSite(argv[4], 
                                argv[5], 
                                "blah", 
                                "blah");

    m_pAutoDownload->SetOfficialFileListAttributes(nFilelistCRC, 
                                                   nFilelistSize);

    m_pAutoDownload->SetArtPath(argv[6]);

    m_pAutoDownload->SetFilelistSubDir("standalone");

    //
    // Let's do it!
    //
    m_pAutoDownload->BeginUpdate(&g_AutoDownloadSink, false);

    return 0;
}

/////////////////////////////////////////////////////////////////////////////

int CAutoUpdate::Run()
{
    MSG msg;
    do
    {
        if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) && WM_QUIT != msg.message)
        {
            if (!IsDialogMessage(&msg))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }
        else
        {
            if(::IsWindow(*this))
            {
                m_pAutoDownload->HandleAutoDownload(1000);
            }
        }
    } while (WM_QUIT != msg.message);
    return msg.wParam;
}

/////////////////////////////////////////////////////////////////////////////


//Imago 6/10 (adapted from training)
bool    Error (HRESULT hr)
{
    if (FAILED (hr))
    {
        IErrorInfoPtr spErr;
        ::GetErrorInfo(0, &spErr);
        _com_error e (hr, spErr, true);
        _bstr_t strError (e.Description().length() ? e.Description() : _bstr_t (e.ErrorMessage()));

        // help trace the problem?
       MsgBox(1,"Fail Code: %x",hr);
	   return true;
	}
	return false;
}

//Imago returns the index of the game with the most players in it or -1 if none - 6/10
long lBigGameIndex (long lGameCount,IAdminSessionPtr spSession,IAdminServerPtr spServer) {
	long index = -1;
	long lastCount = 0;
	for (long l = 0;l <= lGameCount;l++) {
		m_spAdminGamesPtr->get_Item (& (_variant_t) l, &m_spAdminGamePtr);
		if (!m_spAdminGamePtr) {
			continue;
		}
		IAGCShipsPtr pAGCShips;
		m_spAdminGamePtr->get_Ships(&pAGCShips);
		long lCount;
		pAGCShips->get_Count(&lCount);
		if (lCount > lastCount) {
			index = l;
			lastCount = lCount;
		}
	}
	return index;
}

//int main(int argc, char* argv[])
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR lpCmdLine, int)
{
	char szModule[_MAX_PATH];
    GetModuleFileName(NULL, szModule, sizeof(szModule));
    _Module.Init(NULL, hinst);
    InitCommonControls();

	//Imago 6/10 a graceful shutdown of a server (used to push an update)
	if (_stricmp(lpCmdLine, "shutdown") == 0) {
		
		CoInitialize(NULL);

		HRESULT                 hr;
		IAdminSessionClassPtr   spClass;
		hr = CoGetClassObject (__uuidof(AdminSession), CLSCTX_LOCAL_SERVER, NULL, __uuidof(spClass), (void**)&spClass);
		if (!Error (hr))
		{
			AdminSessionSecureHost    xHost;
			IAdminSessionPtr spSession;
			hr = spClass->CreateSession(&xHost, &spSession);
			::CoDisconnectObject(&xHost, 0);
			if (!Error (hr))
			{
				IAdminServerPtr spServer;
				if (!Error (hr = spSession->get_Server(&spServer)))
				{
					if (!Error (hr = spServer->get_Games(&m_spAdminGamesPtr)))
					{
						long lGameCount;
						spServer->get_MissionCount(&lGameCount);
						long index = lBigGameIndex(lGameCount,spSession,spServer);
						if (index != -1) {
							if (lGameCount == 0) {
								spSession->Stop();
							} else {
								if (!Error (hr = spServer->get_Users(&m_spAdminUsersPtr))) {
									long lUsersCount;
									m_spAdminUsersPtr->get_Count(&lUsersCount);
									if (lUsersCount > 0) {
										m_spAdminGamesPtr->get_Item (& (_variant_t) index, &m_spAdminGamePtr);
										if (!m_spAdminGamePtr) {
											index = lBigGameIndex(lGameCount,spSession,spServer);
											m_spAdminGamesPtr->get_Item (& (_variant_t) index, &m_spAdminGamePtr);
										}
										IAGCTeamsPtr pAGCTeams;
										m_spAdminGamePtr->get_Teams(&pAGCTeams);
										long lCount;
										pAGCTeams->get_Count(&lCount);
										if (lCount >= 2) { 
											IAGCTeamPtr pAGCTeam;
											pAGCTeams->get_Item(& (_variant_t) (0L), &pAGCTeam);
											IAGCStationsPtr pAGCStations;
											pAGCTeam->get_Stations(&pAGCStations);
											long lStations;
											pAGCStations->get_Count(&lStations);
											if (lStations > 0) {
												BSTR pBName;
												m_spAdminGamePtr->get_Name(&pBName);
												CComBSTR zMessage = L"Shutting down after ";
												zMessage += pBName;
												zMessage += L" is over";
												spServer->SendMsg(zMessage);
												int i = 0;
												while (lStations > 0) {
													pAGCStations->get_Count(&lStations);
													Sleep(5 * 1000);
													i++;
													if (i >= 2160) {
														zMessage = L"Aborting shutdown, epic game!";
														spServer->SendMsg(zMessage);
														return 1;
													}
													if (i % 84 == 0) {
														spServer->SendMsg(zMessage);
													}
												}
												zMessage = L"Shutting down in four minutes.";
												spServer->SendMsg(zMessage);
												Sleep(120 * 1000);
												zMessage = L"Shutting down in two minutes.";
												spServer->SendMsg(zMessage);
												Sleep(90 * 1000);
												zMessage = L"Shutting down in 30 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(25 * 1000);
												zMessage = L"Shutting down in 5 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(2 * 1000);
												zMessage = L"Shutting down in 3 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(1 * 1000);
												zMessage = L"Shutting down in 2 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(1 * 1000);
												zMessage = L"Shutting down in 1 second.";
												spServer->SendMsg(zMessage);
												Sleep(1 * 1000);
												spSession->Stop();
											} else {
												CComBSTR zMessage = L"Shutting down in two minutes.";
												spServer->SendMsg(zMessage);
												Sleep(90 * 1000);
												zMessage = L"Shutting down in 30 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(25 * 1000);
												zMessage = L"Shutting down in 5 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(2 * 1000);
												zMessage = L"Shutting down in 3 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(1 * 1000);
												zMessage = L"Shutting down in 2 seconds.";
												spServer->SendMsg(zMessage);
												Sleep(1 * 1000);
												zMessage = L"Shutting down in 1 second.";
												spServer->SendMsg(zMessage);
												Sleep(1 * 1000);
												spSession->Stop();
											}
										} else {
											CComBSTR zMessage = L"Shutting down in one minute.";
											spServer->SendMsg(zMessage);
											Sleep(30 * 1000);
											zMessage = L"Shutting down in 30 seconds.";
											spServer->SendMsg(zMessage);
											Sleep(25 * 1000);
											zMessage = L"Shutting down in 5 seconds.";
											spServer->SendMsg(zMessage);
											Sleep(2 * 1000);
											zMessage = L"Shutting down in 3 seconds.";
											spServer->SendMsg(zMessage);
											Sleep(1 * 1000);
											zMessage = L"Shutting down in 2 seconds.";
											spServer->SendMsg(zMessage);
											Sleep(1 * 1000);
											zMessage = L"Shutting down in 1 second.";
											spServer->SendMsg(zMessage);
											Sleep(1 * 1000);
											spSession->Stop();
										} 
									}
								}
							}
						} else {
							spSession->Stop();
						}
					}
				}
			}
		}
		// and we terminate the entire process here
		m_spAdminUsersPtr = 0;
		m_spAdminGamePtr = 0;
		m_spAdminGamesPtr = 0;
		spClass = 0;

		CoUninitialize();
		
		return 0;
	}

    CAutoUpdate dlg;

    if (!dlg.Create(NULL))
    {
      return -5;
    }

    g_AutoDownloadSink.m_pdlg = &dlg;

    int nResult = dlg.Init(__argc, __argv); //Imago 6/10
    if (nResult != 0)
    {
        MsgBox(0,"AutoUpdate was given an invalid commmand-line.");
        dlg.DestroyWindow();
		
        return nResult;
    }

    nResult = dlg.Run();

    _Module.Term();

    return nResult;
}

