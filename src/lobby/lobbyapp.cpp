/*-------------------------------------------------------------------------
  LobbyApp.cpp
  
  Implementation of the lobby
  
  Owner: 
  
  Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/

#include "pch.h"
#include <conio.h>
#include <zreg.h>
#include "client.h"

 // BT - STEAM
#include "atlenc.h"
#include <inttypes.h>

ALLOC_MSG_LIST;

CLobbyApp * g_pLobbyApp = NULL;

#ifdef USECLUB
void CLobbyApp::OnSQLErrorRecord(SSERRORINFO * perror, OLECHAR * postrError)
{
  // don't make the event an error event, because this may or may not be fatal. 
  // But we certainly want to see them all in any case.
  m_plas->LogEvent(EVENTLOG_WARNING_TYPE, LE_DatabaseError, perror->pwszMessage,
    perror->pwszProcedure, perror->lNative, perror->wLineNumber, postrError);
}
#endif

/*-------------------------------------------------------------------------
 * CLobbyApp.ProcessMsgPump
 *-------------------------------------------------------------------------
  Purpose:
      Process all thread messages, which so far are only sql query completion notifications

  Returns:
      Whether we received a WM_QUIT
 */
bool CLobbyApp::ProcessMsgPump()
{
  static CTimer timerMsgPump("in message pump", 0.1f);
  timerMsgPump.Start();
  bool fQuit = false;

  // BT - STEAM 
  SteamGameServer_RunCallbacks();

  // Process the message queue, if any messages were received
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    // dispatch Windows Messages to allow for the admin tool's COM to work
    TranslateMessage(&msg);
    switch (msg.message)
    {
//Imago removed
//#ifdef USECLUB
      case wm_sql_querydone:
      {
        CSQLQuery * pQuery = (CSQLQuery *) msg.lParam;
        pQuery->DataReady();
        break;
      }
//#endif

      case WM_QUIT:
        fQuit = true;
        break;

      default:
        DispatchMessage(&msg);
    }
  }

  timerMsgPump.Stop();
  
  return fQuit;
}


void CLobbyApp::SetConstantGameInfo()
{
  ZGameServerInfoMsg* gameInfo = GetGameServerInfoMsg();
  gameInfo->protocolSignature = zGameInfoSignature;
  gameInfo->protocolVersion = zGameInfoCurrentProtocolVersion;
  gameInfo->numEntries = 1;
  ZGameServerInfoMsgEndian(gameInfo);

  lstrcpy(gameInfo->info[0].gameInternalName, m_fFreeLobby ? "sOBLI_xx_x02" : "sOBLI_xx_x01");

  lstrcpy(gameInfo->info[0].gameFriendlyName, "Allegiance");
  lstrcpy(gameInfo->info[0].gameRoomDescription, "Am I supposed to put something here?");
  lstrcpy(gameInfo->info[0].setupToken, ""); //reserverd for internal Zone use
}


void CLobbyApp::SetVariableGameInfo()
{
  ZGameServerInfoMsg* gameInfo = GetGameServerInfoMsg();

  gameInfo->info[0].order = 1;  //used for Web page order of view
  gameInfo->info[0].blobsize = 0; //reserved for internal Zone use
  gameInfo->info[0].maxPopulation = 1000;

  gameInfo->info[0].gameAddr = 0; //inet_addr ("127.0.0/1");
  gameInfo->info[0].gamePort = 0; //2803; //your game port 
  gameInfo->info[0].serviceType = GAMEINFO_SERVICE_TYPE_GAME;
  gameInfo->info[0].gameState = zGameStateActive;
  gameInfo->info[0].gameVersion = 1;

  SYSTEMTIME systime;
  GetSystemTime (&systime);
  SystemTimeToFileTime(&systime, &(gameInfo->info[0].timeGameStart));

  gameInfo->info[0].numPlayers = m_pCounters->cPlayersMissions + m_pCounters->cPlayersLobby;
  gameInfo->info[0].numSysops = 0;
  gameInfo->info[0].numNotPlaying = m_pCounters->cPlayersLobby;
  gameInfo->info[0].numGamesServed = m_pCounters->cMissions; // relies on perf counters being updated

  // can not use interlocked calls here b/c they're 16bit values
  gameInfo->info[0].numTables = 1;
  gameInfo->info[0].numTablesInUse = 1;
}


void CLobbyApp::SendGameInfo()
{
  ZGameServerInfoMsg* gameInfo = GetGameServerInfoMsg();
  SetVariableGameInfo();
  //must do endian so both side of network get correct numbers
  //gameinfo used on Unix and Intel boxes
  //note that doing this once will invalidate all numbers
  //so set gameInfo->info[0] numbers again.
  ZGameInstanceInfoMsgEndian( gameInfo->info );

  //send, usually to many Ip addresses which are Zone Web servers, so add this to your game configuration
  for (int i = 0; i < m_cReportServers; i++)
    ZGameInfoSendTo(m_rgulIP[i], 2000, GetGameServerInfoMsg(), sizeof(m_GameInfoBuf));
}

CLobbyApp::CLobbyApp(ILobbyAppSite * plas) :
  m_plas(plas),
  m_fmServers(&m_psiteServer),
  m_fmClients(&m_psiteClient),
  m_cReportServers(0),
  m_sGameInfoInterval(0), // doesn't really matter, but...
  m_fProtocol(true),
  m_cStaticCoreInfo(0),
  m_vStaticCoreInfo(NULL),
  m_dwAuthentication(0)
#ifdef USECLUB
  ,
  m_csqlSilentThreads(0),
  m_csqlNotifyThreads(0),
  m_sql(this)
#endif
{
	// BT - STEAM
	m_lastDrmHashUpdate.dwHighDateTime = 0;
	m_lastDrmHashUpdate.dwLowDateTime = 0;
	strcpy(m_szDrmHashFilename, "");

  assert(m_plas);
  m_plas->LogEvent(EVENTLOG_INFORMATION_TYPE, LE_Creating);

  // Imago 9/14 // BT - STEAM
  m_logonCS = new CRITICAL_SECTION;

#ifdef USECLUB
  m_strSQLConfig.Empty();
#endif
  // see if we're setup to report to any web servers
  HKEY  hk;
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, HKLM_AllLobby, 0, "", REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hk, NULL) == ERROR_SUCCESS)
  {
    // read AutoUpdate portion of registry
    char szServers[c_cReportServersMax * 17];
    bool bSuccess = _Module.ReadFromRegistry(hk, true, "GameInfoServers", szServers, 0, true);
    if(bSuccess)
    {
      char * token;
      token = strtok((char *) szServers, " ");
      while(token)
      {
        unsigned long ip = inet_addr(token);
        if (INADDR_NONE == ip) // then try to resolve by name
        {
          HOSTENT * phe = gethostbyname(token);
          if (phe)
            ip = * (unsigned long *) phe->h_addr_list[0];
        }
        
        if (INADDR_NONE != ip)
          m_rgulIP[m_cReportServers++] = ip;
        else
          m_plas->LogEvent(EVENTLOG_INFORMATION_TYPE, LE_BadGameInfoSrv, token);
          
        token = strtok(NULL, " ");
      }
      bSuccess = _Module.ReadFromRegistry(hk, false, "GameInfoInterval", &m_sGameInfoInterval, 25);
    }

    DWORD dwPort;
	bSuccess = _Module.ReadFromRegistry(hk, false, "ClientPort", &dwPort, 2302);		// Mdvalley: defaults to port 2302
	m_sPort = dwPort;

    m_szToken[0] = '\0';
    bSuccess = _Module.ReadFromRegistry(hk, true, "Token", m_szToken, NULL);

	// BT - STEAM - AWeb API urls.
	m_szChatlogUploadUrl[0] = '\0';
	bSuccess = _Module.ReadFromRegistry(hk, true, "ChatLogUploadUrl", m_szChatlogUploadUrl, NULL);

	m_szBanCheckUrl[0] = '\0';
	bSuccess = _Module.ReadFromRegistry(hk, true, "BanCheckUrl", m_szBanCheckUrl, NULL);

	m_szApiKey[0] = '\0';
	bSuccess = _Module.ReadFromRegistry(hk, true, "ApiKey", m_szApiKey, NULL);

    DWORD dwProtocol;
    bSuccess = _Module.ReadFromRegistry(hk, false, "fProtocol", &dwProtocol, (unsigned long) true);
    m_fProtocol = !!dwProtocol;

    DWORD dwFreeLobby;
    bSuccess = _Module.ReadFromRegistry(hk, false, "fFreeLobby", &dwFreeLobby, (unsigned long) 
#ifdef USECLUB
    false
#else    
    true
#endif    
    );
    m_fFreeLobby = !!dwFreeLobby;

    DWORD dwCheckKey;
    bSuccess = _Module.ReadFromRegistry(hk, false, "fCheckCDKey", &dwCheckKey, (unsigned long) 
#ifdef USECLUB
      true
#else      
      false
#endif
    );
    m_fCheckCDKey = !!dwCheckKey;

#ifdef USECLUB
    bSuccess = _Module.ReadFromRegistry(hk, false, "SQLThreadsNotify", &m_csqlNotifyThreads, (unsigned long) 5);
    bSuccess = _Module.ReadFromRegistry(hk, false, "SQLThreadsSilent", &m_csqlSilentThreads, (unsigned long) 1);

    if (FAILED(LoadRegString(hk, "SQLConfig", m_strSQLConfig)))
    {
      m_strSQLConfig.Empty(); 
      _Module.LogEvent(EVENTLOG_ERROR_TYPE, LE_RegStrMissingNoDef, "SQLConfig");
    }
#endif

  }

  g_pLobbyApp = this;

  // stuff for reporting population to zone
  //imago, only if we have gameinfoservers to report to
  if (m_cReportServers) {
	  WSAData data;
	  WSAStartup(MAKEWORD(1,0),&data);
	  //initialize UDP send API
	  ZGameInfoInit(0);
	  //initialize structure
	  SetConstantGameInfo();
  }

  // if zone club lobby
#ifdef USEAUTH  
  m_pzas = CreateZoneAuthServer();
#endif
}

CLobbyApp::~CLobbyApp()
{
  m_plas->LogEvent(EVENTLOG_INFORMATION_TYPE, LE_ShuttingDown);
// KG guard with USEAUTH for consistency 
#ifdef USEAUTH
  m_pzas = NULL;
#endif
  m_perfshare.FreeCounters(m_pCounters);
  ZGameInfoClose();
  FreeStaticCoreInfo(); // KGJV #114
  
  //imago, only if we have gameinfoservers to report to
  if (m_cReportServers) {
	  ZGameInfoClose();
	  WSACleanup();
  }

  // BT - STEAM 
  SteamGameServer_Shutdown();

}



HRESULT CLobbyApp::Init()
{
  HRESULT hr = E_FAIL;
  m_plas->LogEvent(EVENTLOG_INFORMATION_TYPE, LE_Initializing);
  ZVerify(m_perfshare.Initialize());
  m_pCounters = (LOBBY_COUNTERS *)m_perfshare.AllocateCounters(
                      "AllLobby", "0",    // if there are ever multiple lobbies running, change this
                      sizeof(LOBBY_COUNTERS));
  ZeroMemory(m_pCounters, sizeof(LOBBY_COUNTERS));

#ifdef USECLUB
  hr = m_sql.Init(m_strSQLConfig.m_str, GetCurrentThreadId(), m_csqlSilentThreads, m_csqlNotifyThreads);
  if (FAILED(hr))
  {
    m_plas->LogEvent(EVENTLOG_ERROR_TYPE, LE_SQLInitFailed);
    return hr;
  }
#endif

  // TODO: Make keep-alives an option
  if (FAILED(hr = m_fmServers.HostSession(m_fFreeLobby ? FEDFREELOBBYSERVERS_GUID : FEDLOBBYSERVERS_GUID, false, 0, m_fProtocol, m_sPort + 1)) ||	// Mdvalley: I don't know what happens if you try to host 2 servers on one port. Let's not find out.
      FAILED(hr = m_fmClients.HostSession(m_fFreeLobby ? FEDFREELOBBYCLIENTS_GUID : FEDLOBBYCLIENTS_GUID, true, 0, m_fProtocol, m_sPort)))			// The first session doesn't need the port fixed, but I don't want to risk having it take the one the second is going to use.
  {
    m_plas->LogEvent(EVENTLOG_ERROR_TYPE, LE_HostSessionFailure);
    return hr;
  }

  //
  // Read Registry
  //
  DWORD dw; // Gets result of whether it opened or created...
  HKEY  hk;
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, HKLM_AllLobby, 0, "", REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hk, &dw) == ERROR_SUCCESS)
  {
    // read AutoUpdate portion of registry
    DWORD dwWantAutoDownload;
    bool bSuccess = _Module.ReadFromRegistry(hk, false, "AutoUpdateActive", &dwWantAutoDownload, 0);
    if(bSuccess && dwWantAutoDownload)
    {
      char szFileName[MAX_PATH+16];
      strcpy(szFileName, _Module.GetModulePath());
      Strcat(szFileName, "FileList.txt");
      CreateAutoUpdate(hk, szFileName);
    }
    else 
      g_pAutoUpdate = NULL;

    RegCloseKey(hk);

    //Imago 8/6/09 We can't use any of these other handy registry functions
    // because we have to be different and read from Allsrv's registry key ;-/
    HKEY  hk;
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, HKLM_AllLobby, 0, "", 
      REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hk, NULL) == ERROR_SUCCESS)
    {
		//Orion ACSS : 2009
        _Module.ReadFromRegistry(hk, false, "AUTH_ON", &m_dwAuthentication, 0, true);
    }
    RegCloseKey(hk);

    //Orion ACSS : 2009 - Retrieve url of auth server from registry
	if (m_dwAuthentication)
	{
		HKEY  hk;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, HKLM_AllLobby, 0, "", 
		  REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hk, NULL) == ERROR_SUCCESS)
		{
			_Module.ReadFromRegistry(hk, true, "AUTH_ADDRESS", m_szAuthenticationLocation, NULL);
		}
		RegCloseKey(hk);
	}

	//Load Pig KEY
	if (m_dwAuthentication)
	{
		HKEY  hk;
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, HKLM_AllLobby, 0, "",
			REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hk, NULL) == ERROR_SUCCESS)
		{
			if (!_Module.ReadFromRegistry(hk, true, "AUTH_BYPASS_CDKEY", m_szAuthBypassCDKey, NULL)) {
				uint32_t now = m_timeNow.Now().clock();
				
				ZString hashInput = ZString("{481CB2C0-B27D-40D1-A032-7C9A8764D47E}{BAB57D25-A1F2-4F6B-AFBC-5D7226E46113}");
				int inputlen = hashInput.GetLength();
				for (int idx = 0; idx < inputlen; idx++) {
					// char overflow expected
					m_szAuthBypassCDKey[idx] = hashInput[idx] * now * 17 * 23;
					// if overflow results in a null, fix it.
					if (m_szAuthBypassCDKey[idx] == '\0') { 
						m_szAuthBypassCDKey[idx] = '!';
					}
				}
				// make sure we are still null terminated. 
				m_szAuthBypassCDKey[MAX_PATH - 1] = '\0'; 
			}
		}
		RegCloseKey(hk);
	}
  }

  return hr;
}


void CLobbyApp::UpdatePerfCounters()
{
  static CTempTimer timerPerfCounters("assembling perf info", .05f);
  timerPerfCounters.Start();
  m_fmClients.GetSendQueue(&(m_pCounters->cOutboundQueueLength),
                           &(m_pCounters->cOutboundQueueSize));
  m_fmClients.GetReceiveQueue(&(m_pCounters->cInboundQueueLength),
                              &(m_pCounters->cInboundQueueSize));

  m_pCounters->cPlayersLobby = m_fmClients.GetCountConnections();
  m_pCounters->cServers = m_fmServers.GetConnectionCount();

  // Get all the per server stuff, and agregate the count of missions and players
  ListConnections::Iterator iterCnxn(*m_fmServers.GetConnections());
  int cMissions = 0;
  DWORD cPlayers = 0;
  while (!iterCnxn.End())
  {
    CFLServer * pServerT = CFLServer::FromConnection(*iterCnxn.Value());
    cMissions += (pServerT->GetCounters()->cMissions = pServerT->GetMissions()->GetCount());
    cPlayers  += (pServerT->GetCounters()->cPlayers    = pServerT->GetPlayerCount());
    pServerT->GetCounters()->percentLoad = pServerT->GetPercentLoad();
    iterCnxn.Next();
  }
  m_pCounters->cMissions = cMissions;
  m_pCounters->cPlayersMissions = cPlayers;
  timerPerfCounters.Stop();
}


void CLobbyApp::RollCall()
{
  ListConnections::Iterator iterCnxn(*m_fmServers.GetConnections());
  while (!iterCnxn.End())
  {
    CFMConnection & cnxn = *iterCnxn.Value();
    iterCnxn.Next(); // have to move iterator FIRST, because we might kill the current node
    CFLServer * pServerT = CFLServer::FromConnection(cnxn);
    if (pServerT->GetHere())
    {
      // not combining ifs, since order would matter
      if (cnxn.IncAbsentCount() > 4) // dead--nuke 'em
      {
        m_plas->LogEvent(EVENTLOG_WARNING_TYPE, LE_ServerMissedRollCall, cnxn.GetName());
        m_fmServers.DeleteConnection(cnxn);
      }
    }
  }
}

// BT - STEAM
void CLobbyApp::CheckAndUpdateDrmHashes(bool forceUpdate)
{
	if (strlen(m_szDrmHashFilename) == 0)
	{
		HKEY hKey;
		char drmHashFilename[MAX_PATH];
		char drmDownloadUrl[MAX_PATH];

		if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, HKLM_AllLobby, 0, KEY_READ, &hKey))
		{
			DWORD cbValue = MAX_PATH;

			drmHashFilename[0] = '\0';
			drmDownloadUrl[0] = '\0';

			::RegQueryValueExA(hKey, "DrmHashFile", NULL, NULL, (LPBYTE)&drmHashFilename, &cbValue);

			cbValue = MAX_PATH;
			::RegQueryValueExA(hKey, "DrmDownloadUrl", NULL, NULL, (LPBYTE)&drmDownloadUrl, &cbValue);

			if (strlen(drmHashFilename) == 0)
			{
				ZDebugOutput("The DrmHashFile entry is not set. Drm Hash Files will not be relayed to game servers correctly. If you are not using Steam integration, then it is safe to ignore the message.");
				strcpy(drmHashFilename, "NOTSET");
			}

			if (strlen(drmDownloadUrl) == 0)
			{
				ZDebugOutput("The DrmDownloadUrl entry is not set. Drm Hash Files will not be relayed to game servers correctly. If you are not using Steam integration, then it is safe to ignore the message.");
				strcpy(drmDownloadUrl, "NOTSET");
			}

			strcpy(m_szDrmHashFilename, drmHashFilename);
			strcpy(m_szDrmDownloadUrly, drmDownloadUrl);

			RegCloseKey(hKey);
		}
	}

	if (strcmp(m_szDrmHashFilename, "NOTSET") == 0)
		return;

	if (strcmp(m_szDrmDownloadUrly, "NOTSET") == 0)
		return;

	FILETIME lastModifiedTime = ZFile::GetMostRecentFileModificationTime(ZString(m_szDrmHashFilename));
	if (CompareFileTime(&m_lastDrmHashUpdate, &lastModifiedTime) != 0 || forceUpdate == true)
	{
		// Push update message to all servers.
		FedMessaging * pfm = &g_pLobbyApp->GetFMServers();
		int count = pfm->GetConnectionCount();
		ListConnections::Iterator iterCnxn(*pfm->GetConnections());
		while (!iterCnxn.End()) {
			BEGIN_PFM_CREATE(*pfm, pfmUpdateDrmHashes, L, UPDATE_DRM_HASHES)
				FM_VAR_PARM(m_szDrmDownloadUrly, CB_ZTS)
				END_PFM_CREATE

				pfm->SendMessages(iterCnxn.Value(), FM_GUARANTEED, FM_FLUSH);
			iterCnxn.Next();
		}

		m_lastDrmHashUpdate = lastModifiedTime;
	}

}

int CLobbyApp::Run()
{
  const DWORD c_dwUpdateInterval = 200; // milliseconds
  DWORD dwSleep = c_dwUpdateInterval;
  DWORD dwWait = WAIT_TIMEOUT;

  // BT - STEAM
  uint32 unIP = INADDR_ANY;
  uint32 steamAuthenticationPort = 8766;
  uint32 gamePort = 27015; // We will manage our own network communications with game clients, so this will not be used directly by us.
  uint32 masterUpdaterPort = 27016;
  EServerMode eMode = eServerModeAuthenticationAndSecure;
  const char *lobbyVersion = "1.0.0.4";

  if (SteamGameServer_Init(unIP, steamAuthenticationPort, gamePort, masterUpdaterPort, eMode, lobbyVersion) == false)
  {
	  printf("failed on steam server init.\n");
	  exit(-1);
  }

  SteamGameServer()->LogOnAnonymous();
  // End Steam


  m_plas->LogEvent(EVENTLOG_INFORMATION_TYPE, LE_Running);
  _putts("---------Press Q to exit---------");
   printf("Ready for clients/servers.\n");
  CTempTimer timerIterations("between iterations", .25f);
  timerIterations.Start();
  CTempTimer timerReceiveClientsMessages("in clients ReceiveMessages()", .05f);
  CTempTimer timerReceiveServersMessages("in servers ReceiveMessages()", .05f);
  Time timeLastQueueCheck = Time::Now();
  Time timeLastGameInfo = Time::Now();

  while (true)
  {
    timerIterations.Stop();
    timerIterations.Start();

    if (ProcessMsgPump() || (_kbhit() && toupper(_getch()) == 'Q')) {
		//Imago #111 7/10
		if(g_pAutoUpdate)
		{ 
			char szFileName[MAX_PATH+16];
			strcpy(szFileName, _Module.GetModulePath());
			Strcat(szFileName, "FileList.txt");
			g_pAutoUpdate->LoadCRC(szFileName);
			FedMessaging * pfm = &g_pLobbyApp->GetFMClients();
			int count = pfm->GetConnectionCount();
			ListConnections::Iterator iterCnxn(*pfm->GetConnections());
			while (!iterCnxn.End()) {
				BEGIN_PFM_CREATE(*pfm, pfmAutoUpdate, L, AUTO_UPDATE_INFO)
				  FM_VAR_PARM(g_pAutoUpdate->GetFTPServer(), CB_ZTS)
				  FM_VAR_PARM(g_pAutoUpdate->GetFTPInitialDir(), CB_ZTS)
				  FM_VAR_PARM(g_pAutoUpdate->GetFTPAccount(), CB_ZTS)
				  FM_VAR_PARM(g_pAutoUpdate->GetFTPPassword(), CB_ZTS)
				END_PFM_CREATE
				pfmAutoUpdate->crcFileList = g_pAutoUpdate->GetFileListCRC();
				pfmAutoUpdate->nFileListSize = g_pAutoUpdate->GetFileListSize();
				pfm->SendMessages(iterCnxn.Value(), FM_GUARANTEED, FM_FLUSH);
				iterCnxn.Next();
			}
		}
      return 0;
	}

    SetNow();

    m_pCounters->timeInnerLoop = timerIterations.LastInterval();

    // receive any messages in the queue
    timerReceiveClientsMessages.Start();
    m_fmClients.ReceiveMessages();
    timerReceiveClientsMessages.Stop();

    timerReceiveServersMessages.Start();
    m_fmServers.ReceiveMessages();
    timerReceiveServersMessages.Stop();

    if (GetNow() - timeLastQueueCheck >= 1.0f)
    {
      // count the fairly expensive stuff no more than once a second
      UpdatePerfCounters();
      timeLastQueueCheck = GetNow();
      if (GetNow() - timeLastGameInfo >= (float) m_sGameInfoInterval)
      {
        SendGameInfo();
        timeLastGameInfo = GetNow();
      }

      // Do a periodic roll call. If we haven't heard from anyone for two roll calls in a row, waste 'em
      static Time timeRollCall = Time::Now();
      if (GetNow() - timeRollCall >= 5.0f)
      {
        RollCall();
		CheckAndUpdateDrmHashes(false); // BT - STEAM
        timeRollCall = GetNow();
      }
    }
    Sleep(1);
  }
  return 0;
}


int CLobbyApp::OnMessageBox(const char * strText, const char * strCaption, UINT nType)
{
  char sz[256];
  if (strCaption && *strCaption)
  {
    Strcpy(sz, strCaption);
    Strcat(sz, ": ");
  }
  Strcat(sz, strText);
  return m_plas->LogEvent(EVENTLOG_ERROR_TYPE, LE_ODBC_Error, strText);
}


PER_SERVER_COUNTERS * CLobbyApp::AllocatePerServerCounters(const char * szServername)
{
  PER_SERVER_COUNTERS * pPerServerCounters = (PER_SERVER_COUNTERS *) 
        m_perfshare.AllocateCounters((CHAR *) "AllLobbyPerServer", (CHAR*) szServername, sizeof(PER_SERVER_COUNTERS));
  ZeroMemory(pPerServerCounters, sizeof(*pPerServerCounters));  
  return pPerServerCounters;
}


bool CLobbyApp::OnAssert(const char* psz, const char* pszFile, int line, const char* pszModule)
{
  m_plas->LogEvent(EVENTLOG_ERROR_TYPE, LE_Assert, ZString("'")
            + psz
            + "' ("
            + pszFile
            + ":"
            + ZString(line)
            + ")\n"
  );
  return true;
}


void CLobbyApp::DebugOutput(const char *psz)
{
    //Imago had to modify this because of the debugging changes in Win32App by mmf/radar
#ifdef DEBUG
  ::OutputDebugString("AllLobby: ");
  ::OutputDebugString(psz);
#endif
}

void CLobbyApp::BootPlayersByName(const ZString& strName)
{
  PlayerByName::iterator iterPlayer = m_playerByName.find(strName);

  // if we think that player is already logged on...
  while (iterPlayer != m_playerByName.end()
      && (*iterPlayer).first == strName)
  {
    // boot all old copies
    CFLMission * pMissionOld = (*(*iterPlayer).second).second.GetMission();
    
    BEGIN_PFM_CREATE(m_fmServers, pfmRemovePlayer, L, REMOVE_PLAYER)
      FM_VAR_PARM((PCC)strName, CB_ZTS)
      FM_VAR_PARM(NULL, 0)
    END_PFM_CREATE
    pfmRemovePlayer->dwMissionCookie = pMissionOld->GetCookie();
    pfmRemovePlayer->reason = RPR_duplicateName;
    m_fmServers.SendMessages(pMissionOld->GetServer()->GetConnection(), 
      FM_GUARANTEED, FM_FLUSH);

    ++iterPlayer;
  }
}

bool CLobbyApp::BootPlayersByCDKey(const ZString& strCDKey, const ZString& strNameExclude, ZString& strOldPlayer)
{
  PlayerByCDKey::iterator iterPlayerByCDKey = m_playerByCDKey.find(strCDKey);
  bool bBootedSomeone = false;

  // if we think that player is already logged on...
  while (iterPlayerByCDKey != m_playerByCDKey.end()
      && (*iterPlayerByCDKey).first == strCDKey)
  {
    // boot all old copies
    if ((*iterPlayerByCDKey).second.GetName() != strNameExclude)
    {
        CFLMission * pMissionOld = (*iterPlayerByCDKey).second.GetMission();
    
        BEGIN_PFM_CREATE(m_fmServers, pfmRemovePlayer, L, REMOVE_PLAYER)
          FM_VAR_PARM((PCC)(*iterPlayerByCDKey).second.GetName(), CB_ZTS)
          FM_VAR_PARM((PCC)(strNameExclude), CB_ZTS)
        END_PFM_CREATE
        pfmRemovePlayer->dwMissionCookie = pMissionOld->GetCookie();
        pfmRemovePlayer->reason = RPR_duplicateCDKey;    
        m_fmServers.SendMessages(pMissionOld->GetServer()->GetConnection(), 
          FM_GUARANTEED, FM_FLUSH);

        // note: only returns the name of the last player we booted.
        strOldPlayer = (*iterPlayerByCDKey).second.GetName();
        bBootedSomeone = true;
    }

    ++iterPlayerByCDKey;
  }

  return bBootedSomeone;
}

// BT - 12/21/2010 - CSS - Get all rank details from the lobby web service
bool CLobbyApp::GetRankForCallsign(const char* szPlayerName, int *rank, double *sigma, double *mu, int *commandRank, double *commandSigma, double *commandMu, char *rankName, int rankNameLen)
{
	char resultMessage[1024];
	int contentLen = 0; 
    char *content;
    char szResponse[MAX_PATH];    
	char szURL[MPR_HTTP_MAX_URL]; 
	char szName[c_cbName];

	// the player callsign has to be urlencoded, because it may contain '+', '?', etc.
	char callsign[128];
	char playername[128];
	Strcpy(playername, szPlayerName);
	strcpy(callsign, "");
	encodeURL(callsign, playername);

	char* baseUrl = g_pLobbyApp->RetrieveAuthAddress();
	sprintf(szURL, "%s?Action=GetRank&Callsign=%s", baseUrl, callsign);

	MaUrl maUrl;
	maUrl.parse(szURL);

	// First make sure we can write to a socket
    MprSocket* socket = new MprSocket();
	socket->openClient(maUrl.host, maUrl.port, 0);
    int iwrite = socket->_write("GET /\r\n");
    delete socket;

    MaClient* client = new MaClient();
    client->setTimeout(3000);
    client->setRetries(1);
    client->setKeepAlive(0);

	strcpy(resultMessage, "Rank Retrieve Failed.\n\nPlease contact system admin.");

	// make sure we wrote 7 bytes
    if (iwrite == 7) 
	{ 
		debugf("retrieving rank: %s\r\n", szURL);

        client->getRequest(szURL);
        if (client->getResponseCode() == 200) // check for HTTP OK 8/3/08
	        content = client->getResponseContent(&contentLen);
		else
		{
			char msg[1024];
			sprintf(resultMessage, "Lobby GetRank Service Failed (%i)", client->getResponseCode());
		}
    }

	debugf("GetRankForCallsign(): contentLen = %ld, content = %s\r\n", contentLen, content);
	
	int resultCode = -1;

	char localRankName[50];
	if(sscanf(content, "%ld|%ld|%s|%f|%f|%ld|%f|%f", &resultCode, rank, localRankName, sigma, mu, commandRank, commandSigma, commandMu) == EOF)
		resultCode = -1;

	strncpy(rankName, localRankName, rankNameLen);

	// Delete this only after you are done with the content that came back from client->getResponseContent, or that 
	// pointer will get fried.
	delete client;

	if(resultCode == 0)
		strcpy(resultMessage, "Rank retrieved.");

	debugf(resultMessage);

	return resultCode == 0;
}

// BT - 9/11/2010 - CD Key check will call back to the lobby service to ensure the authentication token is valid.
bool CLobbyApp::CDKeyIsValid(const char* szPlayerName, const char* szCDKey, const char* szAddress, char *resultMessage, int resultMessageLength, char * playerIdentifier)
{
	// BT - STEAM
	ZString combinedKey = szCDKey;
	ZString steamIDString = combinedKey.LeftOf(":");
	ZString steamTokenString = combinedKey.RightOf(steamIDString.GetLength() + 1);

	uint64 steamUID = strtoull(steamIDString, NULL, 0);
	CSteamID steamID = CSteamID(steamUID);

	strcpy(playerIdentifier, steamIDString);

	char steamAuthTicket[1024];
	int steamAuthTicketLen = sizeof(steamAuthTicket);
	char steamAuthTicketEncoded[2064];
	strcpy(steamAuthTicketEncoded, steamTokenString);
	UUDecode((BYTE *)steamAuthTicketEncoded, steamTokenString.GetLength(), (BYTE *)steamAuthTicket, &steamAuthTicketLen);


	EBeginAuthSessionResult result = SteamGameServer()->BeginAuthSession(steamAuthTicket, steamAuthTicketLen, steamID);

	bool returnValue = false;


	switch (result)
	{
	case k_EBeginAuthSessionResultOK:
		strcpy(resultMessage, "Authentication In Progress.");
		returnValue = true;
		break;

	case k_EBeginAuthSessionResultInvalidTicket:
		strcpy(resultMessage, "Couldn't begin authorization: Invalid Ticket.");
		returnValue = false;
		break;

	case k_EBeginAuthSessionResultDuplicateRequest:
		strcpy(resultMessage, "Couldn't begin authorization: Duplicate Request.");
		returnValue = false;
		break;

	case k_EBeginAuthSessionResultInvalidVersion:
		strcpy(resultMessage, "Couldn't begin authorization: Invalid Version.");
		returnValue = false;
		break;

	case k_EBeginAuthSessionResultGameMismatch:
		strcpy(resultMessage, "Couldn't begin authorization: Game Mismatch.");
		returnValue = false;
		break;

	case k_EBeginAuthSessionResultExpiredTicket:
		strcpy(resultMessage, "Couldn't begin authorization: Expired Ticket.");
		returnValue = false;
		break;

	default:
		sprintf(resultMessage, "Couldn't begin authorization: unknown error code returned: %ld", result);
		returnValue = false;
		break;
	}

	return returnValue;

	/*
	int contentLen = 0; 
    char *content;
    char szResponse[MAX_PATH];    
	char szURL[MPR_HTTP_MAX_URL]; 
	char szName[c_cbName];

	//Orion : 2009 - Retrieve base lobby authentication service URL
	char* baseUrl = g_pLobbyApp->RetrieveAuthAddress();

	Strcpy(szURL, baseUrl);
 	
	Strcat(szURL,"?Callsign=");
	
 	// one thread per connecting player
 	Strcpy(szName, szPlayerName);

	
	// the player callsign has to be urlencoded, because it may contain '+', '?', etc.
	char callsign[128];
	char playername[128];
	Strcpy(playername, szPlayerName);
	strcpy(callsign, "");
	encodeURL(callsign, playername);
	Strcat(szURL, callsign);
	
	// add the IP to the url
	Strcat(szURL,"&IP=");
	Strcat(szURL,szAddress);

	// add the ticket to the url
	Strcat(szURL,"&Ticket=");

	char cdkey[2048];
	char escapedCdKey[2048];

	Strcpy(cdkey, szCDKey);
	strcpy(escapedCdKey, "");
    encodeURL(escapedCdKey, cdkey); 
	Strcat(szURL,escapedCdKey);

	// BT - Get rid of the hardcoded reference to the auth url.
	MaUrl maUrl;
	maUrl.parse(szURL);

    // First make sure we can write to a socket
    MprSocket* socket = new MprSocket();
	socket->openClient(maUrl.host, maUrl.port, 0); // BT - Get rid of the hardcoded reference to the auth url.
    int iwrite = socket->_write("GET /\r\n");
    delete socket;

    MaClient* client = new MaClient();
    client->setTimeout(3000);
    client->setRetries(1);
    client->setKeepAlive(0);

	bool succeeded = false;
	strcpy(resultMessage, "Authentication Failure.\n\nPlease restart the game using the Authentication System.");

	// make sure we wrote 7 bytes
    if (iwrite == 7) 
	{ 
		debugf("authenticating: %s\r\n", szURL);

        client->getRequest(szURL);
        if (client->getResponseCode() == 200) // check for HTTP OK 8/3/08
	        content = client->getResponseContent(&contentLen);
		else
		{
			char msg[1024];
			sprintf(resultMessage, "Lobby Verification Service Failed (%i)", client->getResponseCode());
			
		}
    }

	debugf("CDKeyIsValid(): contentLen = %ld, content = %s\r\n", contentLen, content);
	
	if (contentLen > 0 && (Strcmp(content, "1") == 0)) { // there's POSITIVE content, we expect it a certain way...
		succeeded = true;
	}

	// Delete this only after you are done with the content that came back from client->getResponseContent, or that 
	// pointer will get fried.
	delete client;

	if(succeeded == true)
		strcpy(resultMessage, "Authentication Succeeded.");

	return succeeded;
	*/
}


void CLobbyApp::SetPlayerMission(const char* szPlayerName, const char* szCDKey, CFLMission* pMission, const char* szAddress)
{

	debugf("SetPlayerMission(): Setting player mission for: %s", szPlayerName);

  ZString strPlayerName = szPlayerName;
  ZString strCDKey = szCDKey;
  ZString strAddress = szAddress;

  // boot any old copies of this player
#ifdef USECLUB
  BootPlayersByName(strPlayerName);
#endif

  // BT - STEAM - Check with the AWeb service for any bans on this user's SteamID. 
  MaClient client;

  ZString url = ZString(g_pLobbyApp->GetBanCheckUrl()) + "?apiKey=" + ZString(g_pLobbyApp->GetApiKey()) + "&steamID=" + ZString(szCDKey);

  int result = client.getRequest((char*) (PCC) url);
  int responseCode = client.getResponseCode();

  if (responseCode == 200)
  {
	  char buffer[2064];
	  int bufferLen = sizeof(buffer);
	  strncpy(buffer, client.getResponseContent(&bufferLen), sizeof(buffer));
	  if (bufferLen > sizeof(buffer) - 1)
		  buffer[sizeof(buffer) - 1] = '\0';
	  else
		  buffer[bufferLen] = '\0';

	  if (strlen(buffer) > 0)
	  {
		  BEGIN_PFM_CREATE(m_fmServers, pfmRemovePlayer, L, REMOVE_PLAYER)
			  FM_VAR_PARM(szPlayerName, CB_ZTS)
			  FM_VAR_PARM(buffer, CB_ZTS)
			  END_PFM_CREATE
			  pfmRemovePlayer->dwMissionCookie = pMission->GetCookie();
		  pfmRemovePlayer->reason = RPR_bannedBySteam;
		  m_fmServers.SendMessages(pMission->GetServer()->GetConnection(),
			  FM_GUARANTEED, FM_FLUSH);
		  GetSite()->LogEvent(EVENTLOG_WARNING_TYPE, LE_BadCDKey, szCDKey,
			  pMission->GetServer()->GetConnection()->GetName(), szPlayerName);
	  }
  }


  // BT - No more ACSS.
 // if(EnforceAuthentication() == true)
 // {
	//  // BT - 9/11/2010 - Readding CD Key Auth on player join to the Allegiance server.
	//int resultMessageLength = 1024;
	//char resultMessage[1024];

	//debugf("SetPlayerMission(): checking valid key for: %s, cdKey: %s, IP: %s\r\n", szPlayerName, szCDKey, szAddress);
	//
	//// BT - STEAM
	//char playerIdentifier[64];
	//bool cdKeyIsValid = CDKeyIsValid(szPlayerName, szCDKey, szAddress, resultMessage, resultMessageLength, playerIdentifier);

	//debugf("SetPlayerMission(): keycheck for: %s, key: %s, address: %s, result: %s, succeeded: %s\r\n", szPlayerName, szCDKey, szAddress, resultMessage, (cdKeyIsValid == true) ? "true" : "false");

 //   if (cdKeyIsValid == false)
 //   {
 //     BEGIN_PFM_CREATE(m_fmServers, pfmRemovePlayer, L, REMOVE_PLAYER)
 //       FM_VAR_PARM(szPlayerName, CB_ZTS)
	//	FM_VAR_PARM(resultMessage, CB_ZTS)
 //     END_PFM_CREATE
 //     pfmRemovePlayer->dwMissionCookie = pMission->GetCookie();
 //     pfmRemovePlayer->reason = RPR_duplicateCDKey;    
 //     m_fmServers.SendMessages(pMission->GetServer()->GetConnection(), 
 //       FM_GUARANTEED, FM_FLUSH);
 //     GetSite()->LogEvent(EVENTLOG_WARNING_TYPE, LE_BadCDKey, szCDKey,
 //         pMission->GetServer()->GetConnection()->GetName(), szPlayerName);
 //   }
 //   else // BT - 9/11/2010 - End.
	//{
		/*ZString strOldPlayer;

		if (BootPlayersByCDKey(strCDKey, szPlayerName, strOldPlayer))
		{
		  BEGIN_PFM_CREATE(m_fmServers, pfmRemovePlayer, L, REMOVE_PLAYER)
			FM_VAR_PARM(szPlayerName, CB_ZTS)
			FM_VAR_PARM((PCC)strOldPlayer, CB_ZTS)
		  END_PFM_CREATE
		  pfmRemovePlayer->dwMissionCookie = pMission->GetCookie();
		  pfmRemovePlayer->reason = RPR_duplicateCDKey;    
		  m_fmServers.SendMessages(pMission->GetServer()->GetConnection(), 
			FM_GUARANTEED, FM_FLUSH);
		}*/
	//}
 // }

  // create a new player by creating entries in the maps
  PlayerByCDKey::iterator iterPlayerByCDKey = 
      m_playerByCDKey.insert(PlayerByCDKey::value_type(strCDKey, PlayerLocInfo(strPlayerName, pMission)));
  m_playerByName.insert(PlayerByName::value_type(strPlayerName, iterPlayerByCDKey));

  pMission->AddPlayer();
}

void CLobbyApp::RemovePlayerFromMission(const char* szPlayerName, CFLMission* pMission)
{
  ZString strPlayerName = szPlayerName;
  PlayerByName::iterator iterPlayer = m_playerByName.find(strPlayerName);

  // we better have found the player
  if(iterPlayer != m_playerByName.end())
  {
    // find the entry coresponding to this mission
    while (iterPlayer != m_playerByName.end()
        && (*iterPlayer).first == strPlayerName)
    {
      if ((*(*iterPlayer).second).second.GetMission() == pMission)
      {
        pMission->RemovePlayer();

        // delete the player from the maps
        m_playerByCDKey.erase((*iterPlayer).second);
        m_playerByName.erase(iterPlayer);

        return;
      }
        
      ++iterPlayer;
    }
  }

  m_plas->LogEvent(EVENTLOG_WARNING_TYPE, LE_CantRemovePlayer, szPlayerName, pMission->GetCookie());
}

void CLobbyApp::RemoveAllPlayersFromMission(CFLMission* pMission)
{
  // remove all players playing in this mission
  if (pMission->GetPlayerCount() != 0)
  {
    // REVIEW: O(AllPlayers).  We could do this in O(ln(AllPlayers)) by
    // storing a set of players on each mission, but that requires extra
    // work for the common case (inserting or removing players) to speed
    // up the uncommon case (removing a server with players).  

    // loop through all of the players
    PlayerByName::iterator iterPlayer = m_playerByName.begin();

    while (iterPlayer != m_playerByName.end())
    {
      // if this player was playing in the mission to be deleted...
      if ((*(*iterPlayer).second).second.GetMission() == pMission)
      {
        pMission->RemovePlayer();

        // delete the player from the maps
        m_playerByCDKey.erase((*iterPlayer).second);
        iterPlayer = m_playerByName.erase(iterPlayer);
      }
      else
        ++iterPlayer;
    }

    assert(pMission->GetPlayerCount() == 0);
  }
}

void CLobbyApp::RemoveAllPlayersFromServer(CFLServer* pServer)
{
  // remove all players playing in this server
  
  if (pServer->GetPlayerCount() != 0)
  {
    // REVIEW: O(AllPlayers).  We could do this in O(ln(AllPlayers)) by
    // storing a set of players on each mission, but that requires extra
    // work for the common case (inserting or removing players) to speed
    // up the uncommon case (removing a server with players).  

    // loop through all of the players
    PlayerByName::iterator iterPlayer = m_playerByName.begin();

    while (iterPlayer != m_playerByName.end())
    {
      // if this player was playing in the mission to be deleted...
      if ((*(*iterPlayer).second).second.GetMission()->GetServer() == pServer)
      {
        (*(*iterPlayer).second).second.GetMission()->RemovePlayer();

        // delete the player from the map
        m_playerByCDKey.erase((*iterPlayer).second);
        iterPlayer = m_playerByName.erase(iterPlayer);
      }
      else
        ++iterPlayer;
    }

    assert(pServer->GetPlayerCount() == 0);
  }
}

CFLMission* CLobbyApp::FindPlayersMission(const char* szPlayerName)
{
  PlayerByName::iterator iterPlayerName = m_playerByName.find(szPlayerName);

  if (iterPlayerName != m_playerByName.end())
  {
    CFLMission* pmission = (*(*iterPlayerName).second).second.GetMission();
    return pmission->GetServer()->GetPaused() ? NULL : pmission;
  }
  else
    return NULL;
}

bool CLobbyApp::StringCmpLess::operator () (const ZString& str1, const ZString& str2) const
{ 
  // comparing lengths first is faster than a strcmp and still creates a strong ordering
  int nLength1 = str1.GetLength(); 
  int nLength2 = str2.GetLength();

  if (nLength1 == nLength2)
    return memcmp((PCC)str1, (PCC)str2, nLength1) < 0; 
  else
    return nLength1 < nLength2;
};

bool CLobbyApp::StringICmpLess::operator () (const ZString& str1, const ZString& str2) const
{ 
  // comparing lengths first is faster than a strcmp and still creates a strong ordering
  int nLength1 = str1.GetLength(); 
  int nLength2 = str2.GetLength();

  if (nLength1 == nLength2)
    return _stricmp(str1, str2) < 0; 
  else
    return nLength1 < nLength2;
};

// KGJV #114
class StaticCoreInfoEquals {
public:
    bool operator () (const StaticCoreInfo* value1, const StaticCoreInfo* value2)
    {
        return (stricmp(value1->cbIGCFile,value2->cbIGCFile) == 0);
    }
};
void CLobbyApp::BuildStaticCoreInfo()
{
	// build the master core list
	// then set coremask for each server
	// 1. get ride of the old list
	FreeStaticCoreInfo();
	// 2. loop thru unpaused servers and build a TList of StaticCoreInfo and the coremask
	ListConnections::Iterator iterCnxn(*GetFMServers().GetConnections());
	TList<StaticCoreInfo*,StaticCoreInfoEquals> CoreList;
	while (!iterCnxn.End())
	{
		CFLServer * pServerT = CFLServer::FromConnection(*iterCnxn.Value());
		if (pServerT) // skip lost/terminating server
		{
			pServerT->SetStaticCoreMask(0); // clear the core mask, not really needed here but it doesnt hurt
			int c = pServerT->GetcStaticCoreInfo();
			if (!pServerT->GetPaused()) // skip paused serveR
				for (int i=0; i<c; i++)
				{
					if (!CoreList.Find(&(pServerT->GetvStaticCoreInfo()[i])))
						CoreList.PushFront(&(pServerT->GetvStaticCoreInfo()[i]));
				}
		}
		iterCnxn.Next();
	}

	// 3. Allocate mem 
	m_cStaticCoreInfo = CoreList.GetCount();
	if (m_cStaticCoreInfo)
		m_vStaticCoreInfo =  new StaticCoreInfo[m_cStaticCoreInfo];
	else
		return; // no core, all done


    // 4. transform the TList into an array

	for (int i = 0; i < m_cStaticCoreInfo; i++)
		Strcpy(m_vStaticCoreInfo[i].cbIGCFile,CoreList[i]->cbIGCFile);
	CoreList.SetEmpty();

	// 5. loop thru unpaused servers and build the coremask
	ListConnections::Iterator iterCnxn2(*GetFMServers().GetConnections());
	while (!iterCnxn2.End())
	{
		CFLServer * pServerT = CFLServer::FromConnection(*iterCnxn2.Value());
		if (pServerT) // skip lost/terminating server
		{
			int c = pServerT->GetcStaticCoreInfo();
			pServerT->SetStaticCoreMask(0); // clear the core mask
			if (!pServerT->GetPaused()) // skip paused server
				for (int i=0; i<c; i++)
				{
					for (int j = 0; j < m_cStaticCoreInfo; j++)
						if (strcmp(pServerT->GetvStaticCoreInfo()[i].cbIGCFile,m_vStaticCoreInfo[j].cbIGCFile) == 0)
							pServerT->SetStaticCoreMask(pServerT->GetStaticCoreMask() | 1<<j);
						
				}
		}
		iterCnxn2.Next();
	}
}
