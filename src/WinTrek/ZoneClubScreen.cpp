#include "pch.h"
#include <zreg.h>

// BT - STEAM
#include "atlenc.h"
#include <inttypes.h>

// #define NO_CLUB_SERVER_CONNECTION 1 // comment out before checkin


//////////////////////////////////////////////////////////////////////////////
//
// ZoneClub Screen
//
//////////////////////////////////////////////////////////////////////////////

bool g_fZoneAuth = 
#ifdef USEAUTH
  true;
#else
  false;
#endif
extern bool    g_bDownloadZoneMessage;
extern bool    g_bDownloadNewConfig;
extern bool    g_bDisableZoneClub;
extern bool    g_bSkipAutoUpdate;
bool g_bAutomaticallySkipMotdScreen = false;

const char * szValidCfg  = "THIS IS A VALID CONFIG FILE";
const char * szValidMotd = "THIS IS A VALID MESSAGE OF THE DAY FILE";

class ZoneClubScreen :
    public Screen,
    public LogonSite, 
    public TrekClientEventSink,
    public IIntegerEventSink,
    public IAutoUpdateSink,
    public EventTargetContainer<ZoneClubScreen>
{
private:
    TRef<Modeler>      m_pmodeler;
    TRef<Pane>         m_ppane;
    TRef<ButtonPane>   m_pbuttonGames;
    TRef<ButtonPane>   m_pbuttonGamesBig;
    TRef<ButtonPane>   m_pbuttonZoneEvents;
    TRef<ButtonPane>   m_pbuttonSquads;
    TRef<ButtonPane>   m_pbuttonMainMenu;
    TRef<ButtonPane>   m_pbuttonPlayerProfile;
    TRef<ButtonPane>   m_pbuttonLeaderboard;
    TRef<ButtonPane>   m_pbuttonWeb;
    TRef<MDLFileImage> m_pMDLFileImage;
    char               m_szName[c_cbPassportName];
    char               m_szPW[c_cbName];
    char               m_szPWOrig[c_cbName];
    char               m_szConfig[MAX_PATH];
    BOOL               m_fRememberPW;

    bool               m_bErrorOccured;
    bool               m_bMessageStage; // if true then we are at the Message of the day stage, if false then we are at the Config download stage
    bool               m_bTriedCurrentLogin;
    IHTTPSession *     m_pSession;

    bool               m_bConnectLobby;  // true means lobby server; false means Club server

    TRef<IMessageBox>  m_pmsgBox;

    ScreenID           m_screenPostConnect;

    static bool s_bWasAuthenticated;
 
public:
    ZoneClubScreen(Modeler* pmodeler, Number* ptime) :
        m_pSession(NULL),
        m_bErrorOccured(false),
        m_bMessageStage(false),
        m_pmsgBox(NULL),
        m_pmodeler(pmodeler),
        m_bTriedCurrentLogin(false)
    {
        TRef<INameSpace> pnsZoneClubData = GetModeler()->CreateNameSpace("zoneclubdata");

        pnsZoneClubData->AddMember("timeStart", ptime->MakeConstant());

        TRef<INameSpace> pns = pmodeler->GetNameSpace(
            trekClient.GetIsZoneClub() ? "zoneclubscreen" : "zonepublicscreen");
        CastTo(m_ppane, pns->FindMember("screen"));

        CastTo(m_pMDLFileImage, (Value*)pns->FindMember("messageImage"));

        CastTo(m_pbuttonGames, pns->FindMember("gamesButtonPane"));
        CastTo(m_pbuttonGamesBig, pns->FindMember("gamesBigButtonPane"));
        CastTo(m_pbuttonZoneEvents, pns->FindMember("zoneEventsButtonPane"));
        CastTo(m_pbuttonSquads, pns->FindMember("squadsButtonPane"));
        CastTo(m_pbuttonMainMenu, pns->FindMember("mainmenuButtonPane"));
        CastTo(m_pbuttonPlayerProfile, pns->FindMember("playerProfileButtonPane"));
        CastTo(m_pbuttonLeaderboard, pns->FindMember("leaderboardButtonPane"));
        CastTo(m_pbuttonWeb, pns->FindMember("webButtonPane"));
		// mdvalley: Lots of &ZoneClubScreen:: added
        AddEventTarget(&ZoneClubScreen::OnButtonGames, m_pbuttonGames->GetEventSource());
        AddEventTarget(&ZoneClubScreen::OnButtonGames, m_pbuttonGamesBig->GetEventSource());
        AddEventTarget(&ZoneClubScreen::OnButtonMainMenu, m_pbuttonMainMenu->GetEventSource());
        
        if (trekClient.GetIsZoneClub())
        {
            AddEventTarget(&ZoneClubScreen::OnButtonZoneEvents, m_pbuttonZoneEvents->GetEventSource());
            AddEventTarget(&ZoneClubScreen::OnButtonSquads, m_pbuttonSquads->GetEventSource());
            AddEventTarget(&ZoneClubScreen::OnButtonPlayerProfile, m_pbuttonPlayerProfile->GetEventSource());
            AddEventTarget(&ZoneClubScreen::OnButtonZoneWeb, m_pbuttonWeb->GetEventSource());
            AddEventTarget(&ZoneClubScreen::OnButtonLeaderBoard, m_pbuttonLeaderboard->GetEventSource());
        }
        
        //temporarily disable all buttons 
        m_pbuttonMainMenu->SetEnabled(false); // force download to finish before leaving this screen
        m_pbuttonGames->SetEnabled(false);
        m_pbuttonGamesBig->SetEnabled(false);

        if (trekClient.GetIsZoneClub())
        {
            m_pbuttonSquads->SetEnabled(false);
            m_pbuttonZoneEvents->SetEnabled(false);
            m_pbuttonPlayerProfile->SetEnabled(false);
            m_pbuttonLeaderboard->SetEnabled(false);
        }

        pmodeler->UnloadNameSpace(pns);

        BeginConfigDownload();

        if (g_bQuickstart)
			// mdvalley: &ZoneClubScreen:: needed.
            AddEventTarget(&ZoneClubScreen::OnButtonGames, GetWindow(), 0.01f);

        trekClient.FlushSessionLostMessage();
    }

    ~ZoneClubScreen()
    {
      //trekClient.m_pzac = NULL;

      if (m_pSession)
      {
         delete m_pSession;
         m_pSession = NULL;
      }
    }

    // connect to either Zone Lobby server or Zone Club (wrapper to provide appropriate prompt)
    void ConnectToZone(bool bConnectLobby, ScreenID screenid) 
    {
        ZString strPrompt;

#ifdef USEAUTH
        if (trekClient.GetCfgInfo().bUsePassport)
            strPrompt = "Sign in to Microsoft Passport";
        else
            strPrompt = "Sign in to the Microsoft Gaming Zone";
#else
        strPrompt = "Enter a call sign (player name) to use for this game.";
#endif
        ConnectToZone(bConnectLobby, screenid, strPrompt);
    }

    // connect to either Zone Lobby server or Zone Club
    void ConnectToZone(bool bConnectLobby, ScreenID screenid, const ZString& strPrompt) 
    {
        if (bConnectLobby)
        {
            if (trekClient.GetIsZoneClub())
            {
                if (trekClient.GetCfgInfo().strClubLobby.IsEmpty())
                {
                    TRef<IMessageBox> pmsgBox = CreateMessageBox(
                        "The Allegiance Zone lobby is not available at the moment.  "
                        + ZString(trekClient.GetCfgInfo().strPublicLobby.IsEmpty()
                            ? "Please try again later."
                            : "Please try the free lobby instead.")
                        );
                    GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                    return;
                }
            }
            else
            {
                if (trekClient.GetCfgInfo().strPublicLobby.IsEmpty())
                {

                	TRef<IMessageBox> pmsgBox = CreateMessageBox(
                    	"The free lobby is not available at the moment.  "
                    	+ ZString(trekClient.GetCfgInfo().strClubLobby.IsEmpty()
                    	    ? "Please try again later."
                    	    : "Please try the Allegiance Zone instead.")
                   	 );
                	GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                	return;			
                }
            }
        }
        else 
        {
            if (trekClient.GetCfgInfo().strClub.IsEmpty())
            {
                TRef<IMessageBox> pmsgBox = CreateMessageBox(
                    "The Club server is not available at the moment.  "
                    "Please try again later."
                    );
                GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
                return;
            }
        }

        m_bConnectLobby = bConnectLobby;
        m_screenPostConnect = screenid;

        assert (!trekClient.LoggedOn() && !trekClient.m_fm.IsConnected());
        HRESULT hr = E_FAIL;
#ifdef USEAUTH
        TRef<IZoneAuthClient> pzac;

        if (g_fZoneAuth)
        {
            pzac = trekClient.GetZoneAuthClient();
            if (!pzac)
                pzac = trekClient.CreateZoneAuthClient();
            
            if (trekClient.GetCfgInfo().bUsePassport 
                && !pzac->HasInterface(trekClient.GetCfgInfo().guidZoneAuth))
            {
                // take them to the web page...
                GetWindow()->ShowWebPage(trekClient.GetCfgInfo().strPassportUpdateURL);

                // shut down the client
                GetWindow()->PostMessage(WM_CLOSE);

                return;
            }

            if (s_bWasAuthenticated)
            {
                hr = S_OK;
            }
            else
            {
                pzac->SetAuthServer(trekClient.GetCfgInfo().strZAuth);
                
                if (!m_bTriedCurrentLogin)
                    hr = pzac->IsAuthenticated(5000);
            }
        }

        if (SUCCEEDED(hr)) // must be false if !g_fZoneAuth
        {
            BaseClient::ConnectInfo ci;
            DWORD cbZoneTicket;
            DWORD cbName = sizeof(ci.szName);
            ZSucceeded(pzac->GetTicket(&ci.pZoneTicket, &cbZoneTicket, ci.szName, &cbName));
            assert(cbName <= sizeof(ci.szName));
            ci.cbZoneTicket = cbZoneTicket;

            if(m_bConnectLobby)
                trekClient.ConnectToLobby(&ci);
            else
                trekClient.ConnectToClub(&ci);

            m_bTriedCurrentLogin = true;
        }
        else
#endif
        {
            m_szName[0] = '\0';
            m_szPWOrig[0] = '\0';
#ifdef USEAUTH
            if (g_fZoneAuth)
            pzac->GetDefaultLogonInfo(m_szName, m_szPWOrig, &m_fRememberPW);
#else
            lstrcpy(m_szName, trekClient.GetSavedCharacterName());

			// BT - Steam - User is logged into steam, and has a steam profile name
			// The steam reviewer was somehow launching the game with steam authorization but no persona name. If 
			// there is an player name, then the server rejects the user as a hacker with a DPlay error. 
			bool isUserLoggedIntoSteamWithValidPlayerName = SteamUser() != nullptr && strlen(m_szName) > 0;

#endif
		  // wlp - don't ask for callsign if it was on the command line
          if (!g_bAskForCallSign || isUserLoggedIntoSteamWithValidPlayerName == true) // BT - STEAM
		  {
			  // BT - STEAM - Add players callsign and token.
			  CallsignTagInfo callSignTagInfo;

			  ZString characterName = callSignTagInfo.Render(m_szName);

			  this->OnLogon(characterName, "", false);
	      } // wlp - end of dont ask for callsign 
		  else
		  {
            TRef<IPopup> plogonPopup = CreateLogonPopup(m_pmodeler, this, 
                (trekClient.GetIsZoneClub() ? 
                  LogonAllegianceZone :
#ifdef USEAUTH
                  LogonFreeZone
#else
                  LogonLAN
#endif
                ), strPrompt, m_szName, m_szPWOrig, m_fRememberPW);
            Point point(c_PopupX, c_PopupY);
            Rect rect(point, point);
            GetWindow()->GetPopupContainer()->OpenPopup(plogonPopup, rect, false);
		    }// wlp = end of else ask for callsign
        }
    }

    
    bool OnButtonMainMenu()
    {
        GetWindow()->screen(ScreenIDIntroScreen);

        return true;
    }

    bool OnButtonSquads()
    {
        if (trekClient.LoggedOnToClub())
        {
            GetWindow()->screen(ScreenIDSquadsScreen);
        }
        else
        {
            ConnectToZone(false, ScreenIDSquadsScreen);
        }

        return true;
    }

    bool OnButtonGames()
    {
		//imago 7/4/09 - uncommented the code here that was commented out by ? on ?   
        if (trekClient.LoggedOnToLobby())
        {
            GetWindow()->screen(ScreenIDGameScreen);
        }
        else
        {
			if (g_bQuickstart) { //imago 7/5/09
				if (trekClient.GetCfgInfo().strPublicLobby.GetLength()) {
					ConnectToZone(true, ScreenIDGameScreen);
					Sleep(1000); //woah, retail is fast!  wait a sec socket to be created
				} else {
					m_pbuttonMainMenu->SetEnabled(false); // force download to finish before leaving this screen
        			m_pbuttonGames->SetEnabled(false);
        			m_pbuttonGamesBig->SetEnabled(false);
					g_bQuickstart = false;
					GetWindow()->screen(ScreenIDGameScreen);  // if they don't have a cfg file yet
					return false;								// this will now be handled gracefully
				}
			} else {
				ConnectToZone(true, ScreenIDGameScreen);
			}
        }

        return true;
    }

    bool OnButtonZoneEvents()
    {
        GetWindow()->screen(ScreenIDZoneEvents);
        //ConnectToZone(true, ScreenIDZoneEvents);

        return true;
    }

    bool OnButtonPlayerProfile()
    {

#ifdef NO_CLUB_SERVER_CONNECTION
            GetWindow()->screen(ScreenIDCharInfo);
#else

        if (trekClient.LoggedOnToClub())
        {
            GetWindow()->screen(ScreenIDCharInfo);
        }
        else
        {
            ConnectToZone(false, ScreenIDCharInfo);
        }
#endif 

        return true;
    }

    bool OnButtonZoneWeb()
    {
        // note: users can also access web page from the introscreen
        GetWindow()->ShowWebPage();
        return true;
    }

    bool OnButtonLeaderBoard()
    {
        if (trekClient.LoggedOnToClub())
        {
            GetWindow()->screen(ScreenIDLeaderBoard);
        }
        else
        {
            ConnectToZone(false, ScreenIDLeaderBoard);
        }

        return true;
    }

    void BeginConfigDownload() 
    {
        std::string configPath = GetConfiguration()->GetStringValue(
            "Online.ConfigFile",
            GetConfiguration()->GetStringValue("CfgFile", "http://allegiance.zaphop.com/allegiance.txt")
        );
        lstrcpy(m_szConfig, configPath.c_str());

        if (!g_bDownloadNewConfig || g_bQuickstart)  //imago added quickstart and reordered 7/4/09
        {
            debugf("Skipping download of config file due to command-line switch.\n");
            OnConfigDownloadDone(true, false);
            return;
        }

        if (ZString(m_szConfig).Find("http://") == -1) 
        {
            debugf("Using local config file due to registry setting for ConfigFile because \"http://\" was missing from it. %s\n", m_szConfig);

            if (!IsFileValid(m_szConfig))
            {
                #ifndef DEBUG // don't give warning twice for debug users
                m_pmsgBox = CreateMessageBox("Warning, Local CFG missing validation string");
                GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
               #endif
            }

            OnConfigDownloadDone(false, false);
            return;
        }

        if (m_pSession)
            return;

        debugf("Beginning Config Download: %s.\n", m_szConfig);

        m_pSession = CreateHTTPSession(this);

        const char * szFileList[] = { m_szConfig, "temp.cfg", NULL };

        m_pSession->InitiateDownload(szFileList, "."); // . means EXE path

        return;
    }

    void BeginMessageOfDayDownload()
    {
        if (!g_bDownloadZoneMessage || g_bQuickstart) //imago 7/4/09
        {
            debugf("Skipping download of Message Of the Day due to command-line switch.\n");
            OnMessageOfDayDone(false);
            return;
        }

        PCC szMessageOfTheDayFileName = trekClient.GetIsZoneClub() 
            ? "clubmessageoftheday.mdl" : "publicmessageoftheday.mdl";

        PathString path(trekClient.GetArtPath());
        path = path + szMessageOfTheDayFileName;

        int crc = FileCRC(PCC(path), NULL);

        if (crc != 0 && crc == (trekClient.GetIsZoneClub() 
            ? trekClient.GetCfgInfo().crcClubMessageFile 
            : trekClient.GetCfgInfo().crcPublicMessageFile))
        {
            debugf("Existing messageoftheday file has the correct CRC, skipping download.\n");
            OnMessageOfDayDone(true);
            return;
        }
        else
        {
            debugf("Existing messageoftheday file has the wrong CRC (%X).\n", crc);
        }

        if (m_pSession)
            return;

        PCC szMessageOfTheDayURL = trekClient.GetIsZoneClub() 
            ? PCC(trekClient.GetCfgInfo().strClubMessageURL) 
            : PCC(trekClient.GetCfgInfo().strPublicMessageURL);

        debugf("Beginning messageoftheday download: %s.\n", szMessageOfTheDayURL);

        m_pSession = CreateHTTPSession(this);

        const char * szFileList[3] = { szMessageOfTheDayURL, szMessageOfTheDayFileName, NULL };

        m_pSession->InitiateDownload(szFileList, trekClient.GetArtPath());

        return;
    }

    void OnError(char * szErrorMessage) // on HTTP download error
    {
        // Errors are essentially ignored
        debugf("Error while downloading file.\n");
        debugf("%s\n", szErrorMessage);
        m_bErrorOccured = true;
    }

    bool IsFileValid(char * szFileName)
    {
        ZFile file(szFileName);

        int n = file.GetLength(); // -1 means error
        if (n != -1 && n != 0) 
        {
            char * pData = new char[n+1];
            memcpy(pData, file.GetPointer(), n);
            pData[n] = 0;

            if (m_bMessageStage)
            {
                if (strstr(pData, szValidMotd) == NULL)
                {
                    debugf("File %s is not a valid messageoftheday file.\n", szFileName);
#ifdef DEBUG
                    m_pmsgBox = CreateMessageBox("Warning, downloaded Motd file missing validation string");
                    GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
#endif
                    delete[] pData;
                    return false;
                }
            }
            else
            {
                if (strstr(pData, szValidCfg) == NULL)
                {
                    debugf("File %s is not a valid config file.\n", szFileName);
#ifdef DEBUG
                    m_pmsgBox = CreateMessageBox("Warning, CFG file missing validation string");
                    GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
#endif
                    delete[] pData;
                    return false;
                }
            }
            delete[] pData;
        }
        else
        {
             debugf("File %s error while trying to load downloaded config file.\n", szFileName);
             return false;
        }
        return true;
    }

    bool OnFileCompleted(char * szFileName)
    {
        debugf("Downloaded file: %s\n", szFileName);

        char szPath[MAX_PATH+20];
        strcpy(szPath, m_pSession->GetDownloadPath());
        strcat(szPath, szFileName);

        if (!IsFileValid(szPath))
        {
            m_bErrorOccured = true;
            debugf("Aborting either cfg or motd because file is invalid.");

            m_pmsgBox = CreateMessageBox("Unable to connect to Zone.  Please try again later.");
            GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
        }

        return true; // true means don't retry download
    }

    void OnFrame()
    {
        if (m_pSession)
        {
            if(m_bErrorOccured || !m_pSession->ContinueDownload())
            {
                if (m_bMessageStage)
                    OnMessageOfDayDone(!m_bErrorOccured);
                else
                    OnConfigDownloadDone(true, !m_bErrorOccured);
            }
        }
    }

    void OnConfigDownloadDone(bool bUseEXEFolder, bool bDownloadSuccesful) 
    {
        m_bMessageStage = true;

        if (m_pSession)
        {
            delete m_pSession;
            m_pSession = NULL;
        }

        if (bUseEXEFolder) 
        {
            PathString pathEXE(PathString::GetCurrentDirectory());
            PathString pathConfig(pathEXE + PathString(PathString(m_szConfig).GetFilename()));

            if (bDownloadSuccesful)
            {
                PathString pathTemp(pathEXE + "temp.cfg");
                trekClient.GetCfgInfo().Load(PCC(pathTemp));
                debugf("Loaded downloaded config file: %s\n", PCC(pathTemp));

                // rename file, so that next time if there is an error, there is 
                // a recent config file to use.
                debugf("Renaming %s to %s\n", PCC(pathTemp), PCC(pathConfig));
                DeleteFile(PCC(pathConfig));
                MoveFile(PCC(pathTemp), PCC(pathConfig));
				trekClient.GetCfgInfo().SetCfgFile(pathConfig); 
            }
            else 
            {
                debugf("Error detected, loading existing config file: %s\n", PCC(pathConfig));
                trekClient.GetCfgInfo().Load(PCC(pathConfig));
            }
        }
        else
        {
            debugf("Using local config file %s\n", m_szConfig);
            trekClient.GetCfgInfo().Load(m_szConfig);
        }

        if (!m_bErrorOccured || g_bQuickstart) //imago 7/4/09
        {
            if(!g_bSkipAutoUpdate)
            {
                if (trekClient.GetCfgInfo().crcFileList != 0 && 
                    trekClient.GetCfgInfo().nFilelistSize != 0 && 
                    trekClient.GetCfgInfo().strFilelistSite != "" && 
                    trekClient.GetCfgInfo().strFilelistDirectory != "")
                {
                    bool bForceFileCheck = trekClient.ShouldCheckFiles();

                    if (bForceFileCheck || trekClient.GetCfgInfo().crcFileList != FileCRC("Filelist.txt", NULL))
                    {
						DeleteFile(trekClient.GetArtPath() + ZString("/CommonTextures.Pack")); //#142 Imago 7/10
                        if (trekClient.m_pAutoDownload == NULL)
                            trekClient.m_pAutoDownload = CreateAutoDownload();

                        IAutoUpdateSink * pAutoUpdateSink = trekClient.OnBeginAutoUpdate(this, false);
                        assert(pAutoUpdateSink);

                        trekClient.m_pAutoDownload->SetFTPSite(PCC(trekClient.GetCfgInfo().strFilelistSite),
                                                               PCC(trekClient.GetCfgInfo().strFilelistDirectory),
                                                               "blah", // account
                                                               "blah"); // pw

                        trekClient.m_pAutoDownload->SetOfficialFileListAttributes(trekClient.GetCfgInfo().crcFileList, 
                                                                                  trekClient.GetCfgInfo().nFilelistSize);

                        trekClient.m_pAutoDownload->SetArtPath(trekClient.GetArtPath());

                        //
                        // Let's do it!
                        //
                        trekClient.m_pAutoDownload->BeginUpdate(pAutoUpdateSink, bForceFileCheck);
                        // m_pAutoDownload could be NULL at this point, if the autodownload system decided
                        // not to do a download after all.  This can happen if there is an error or if
                        // the client was already up-to-date.
                    }
                    else
                    {
                        // signal successful update/version already up-to-date
                        OnAutoUpdateSystemTermination(false, false);
                    }
                }
                else
                {
                    debugf("Your cfg file is missing one or more of the following:\nFilelistCRC, FilelistSize, FilelistSite, FilelistDirectory\nSkipping AutoUpdate.\n");
                    // signal successful update/version already up-to-date
                    OnAutoUpdateSystemTermination(false, false);

//                    m_pmsgBox = CreateMessageBox("Unable to connect to Zone.  CFG File is missing key components.  Please try again later.");
//                    GetWindow()->GetPopupContainer()->OpenPopup(m_pmsgBox, false);
                }
            }
            else
            {
                // signal successful update/version already up-to-date
                OnAutoUpdateSystemTermination(false, false);
            }

            BeginMessageOfDayDownload();
        }
        else
        m_pbuttonMainMenu->SetEnabled(true); // allow users to leave screen even if cfg file failed to download
    }

    void OnMessageOfDayDone(bool bSuccessful) 
    {
        if (m_pSession)
        {
            delete m_pSession;
            m_pSession = NULL;
        }

        m_pbuttonMainMenu->SetEnabled(true); // allow users to leave screen even if cfg file failed to download

        if (bSuccessful)
        {
            debugf("Loading messageoftheday file.\n");
            m_pMDLFileImage->Load(trekClient.GetIsZoneClub() 
                ? "clubmessageoftheday" : "publicmessageoftheday");
        }
        else 
        {
            debugf("Errors detected while downloading message of the day.  Skipping load.\n");
        }
    }


    ////////////////////////////////////////////////////////////////////
    //    
    // Events associated with IAutoDownloadSink
    //
    ////////////////////////////////////////////////////////////////////

    virtual void OnAutoUpdateSystemTermination(bool bErrorOccurred, bool bRestarting) 
    {
        m_pbuttonMainMenu->SetEnabled(true); // force download to finish before leaving this screen

        if (!bErrorOccurred && !bRestarting)
        {
            m_pbuttonGames->SetEnabled(true);
            m_pbuttonGamesBig->SetEnabled(true);

            if (trekClient.GetIsZoneClub())
            {
                m_pbuttonPlayerProfile->SetEnabled(true);
                m_pbuttonLeaderboard->SetEnabled(true);
                m_pbuttonSquads->SetEnabled(!g_bDisableZoneClub);
                m_pbuttonZoneEvents->SetEnabled(!trekClient.GetCfgInfo().strZoneEventsURL.IsEmpty());
            }

            // This is pretty ugly, but we don't like this screen.
            if (g_bAutomaticallySkipMotdScreen == true) {
                g_bAutomaticallySkipMotdScreen = false;
                OnButtonGames();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////
    //    
    // Events associated with IIntegerEventSink
    //
    ////////////////////////////////////////////////////////////////////

    bool OnEvent(IIntegerEventSource* pevent, int value)
    {
        //
        // User must have pressed Okay button after error
        //
        assert(m_pmsgBox);
        GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
        GetWindow()->RestoreCursor();

        return false;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // LogonSite methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void OnLogon(const ZString& strName, const ZString& strPassword, BOOL fRememberPW)
    {
        lstrcpy(m_szName, strName);
        lstrcpy(m_szPW, strPassword);
        m_fRememberPW = fRememberPW;
//#ifdef USEAUTH        
//#else
//        trekClient.SaveCharacterName(strName);
//#endif        
        GetWindow()->SetWaitCursor();
        TRef<IMessageBox> pmsgBox = CreateMessageBox("Connecting...", NULL, false);
        GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

        // pause to let the "connecting..." box draw itself
        AddEventTarget(&ZoneClubScreen::OnUsernameAndPassword, GetWindow(), 0.1f);
    }

    bool OnUsernameAndPassword()
    {
        HRESULT hr  = E_FAIL;
#ifdef USEAUTH
        TRef<IZoneAuthClient> pzac;
        if (g_fZoneAuth)
        {
            pzac = trekClient.GetZoneAuthClient();
            hr = pzac->Authenticate(m_szName, m_szPW, 
                    !!lstrcmp(m_szPW, m_szPWOrig), m_fRememberPW, 5000);
        }

        if (g_fZoneAuth && FAILED(hr))
        {
            bool bRetry;
            ZString strReason;

            if (ZT_E_AUTH_DENIED == hr)
            {
                if (trekClient.GetCfgInfo().bUsePassport)
                    strReason = "Either the username or the password you supplied was incorrect.  Be sure to sign in with your Microsoft Passport name (not your Zone ID).";
                else
                    strReason = "Either the username or the password you supplied was incorrect.  Please try again.";
                bRetry = true;
            }
            else
            {
                strReason = "Unable to contact the authentication server.  Please check your network connection.";
                bRetry = false;
            }

            // if the logon fails, close the connecting dialog box.
            GetWindow()->GetPopupContainer()->ClosePopup(m_pmsgBox);
            GetWindow()->RestoreCursor();

            if (m_bConnectLobby)
                OnLogonLobbyFailed(bRetry, strReason);
            else
                OnLogonClubFailed(bRetry, strReason);
        }
        else
#endif
        {
            s_bWasAuthenticated = true;
            
            BaseClient::ConnectInfo ci;
            DWORD cbZoneTicket = 0;
            ci.pZoneTicket = NULL;
            DWORD cbName = sizeof(ci.szName);
#ifdef USEAUTH
            if (g_fZoneAuth)
            {
                ZSucceeded(pzac->GetTicket(&ci.pZoneTicket, &cbZoneTicket, ci.szName, &cbName));
                assert(cbName <= sizeof(ci.szName));
                ci.cbZoneTicket = cbZoneTicket;
            }
            else
#endif
            lstrcpy(ci.szName, m_szName);
            
            ZeroMemory(&ci.ftLastArtUpdate, sizeof(ci.ftLastArtUpdate));
            
            if (m_bConnectLobby)
                trekClient.ConnectToLobby(&ci);
            else
                trekClient.ConnectToClub(&ci);
        }

        return false;
    }

    void OnAbort()
    {
    }

    void OnLogonLobby()
    {
        GetWindow()->screen(m_screenPostConnect);
    }

    void OnLogonLobbyFailed(bool bRetry, const char* szReason)
    {

		if (g_bQuickstart) {
			g_bQuickstart = false;
			GetWindow()->screen(ScreenIDIntroScreen);  //imago 7/13/09 this will make users able
														//  to retry and see the MOTD for outage info
		}

        if (bRetry)
        {
            s_bWasAuthenticated = false;
            ConnectToZone(true, m_screenPostConnect, szReason);
        }
        else
        {
            TRef<IMessageBox> pmsgBox = CreateMessageBox(szReason);
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);

        }
    }
    
    void OnLogonClub()
    {
        GetWindow()->screen(m_screenPostConnect);
    }

    void OnLogonClubFailed(bool bRetry, const char* szReason)
    {
        if (bRetry) 
        {
            s_bWasAuthenticated = false;
            ConnectToZone(false, m_screenPostConnect, szReason);
        }
        else
        {
            TRef<IMessageBox> pmsgBox = CreateMessageBox(szReason);
            GetWindow()->GetPopupContainer()->OpenPopup(pmsgBox, false);
        }
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Screen Methods
    //
    //////////////////////////////////////////////////////////////////////////////

    Pane* GetPane()
    {
        return m_ppane;
    }
};

bool ZoneClubScreen::s_bWasAuthenticated = false;

//////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
//////////////////////////////////////////////////////////////////////////////

TRef<Screen> CreateZoneClubScreen(Modeler* pmodeler, Number * ptime)
{
    return new ZoneClubScreen(pmodeler, ptime);
}

