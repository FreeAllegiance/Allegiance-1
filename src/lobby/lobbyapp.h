/*-------------------------------------------------------------------------
  LobbyApp.h
  
  Main class for the lobby
  
  Owner: 
  
  Copyright 1986-2000 Microsoft Corporation, All Rights Reserved
 *-----------------------------------------------------------------------*/
#ifndef _LOBBYAPP_H_ 
#define _LOBBYAPP_H_ 

// mmf added this include so client.cpp has HKLM_FedSrv defined
#include "regkey.h"

// BT - STEAM
#include "steam_gameserver.h"

// KGJV moved into regkey.h
//#define HKLM_AllLobby "SYSTEM\\CurrentControlSet\\Services\\AllLobby"

/////////////////////////////////////////////////////////////////////////////
// Forward Declarations

class ILobbyAppSite;

const int c_cReportServersMax = 20;

class LobbyClientSite : public IFedMessagingSite
{
public:
  // IFedMessagingSite
  virtual HRESULT OnAppMessage(FedMessaging * pthis, CFMConnection & cnxnFrom, FEDMESSAGE * pfm);
  virtual HRESULT OnSysMessage(FedMessaging * pthis) ;
  virtual void    OnMessageNAK(FedMessaging * pthis, DWORD dwTime, CFMRecipient * prcp) ;
  virtual HRESULT OnNewConnection(FedMessaging * pthis, CFMConnection & cnxn) ;
  virtual HRESULT OnDestroyConnection(FedMessaging * pthis, CFMConnection & cnxn) ;
  virtual HRESULT OnSessionLost(FedMessaging * pthis) ;
  virtual int     OnMessageBox(FedMessaging * pthis, const char * strText, const char * strCaption, UINT nType);
#ifndef NO_MSG_CRC
  virtual void    OnBadCRC(FedMessaging * pthis, CFMConnection & cnxn, BYTE * pMsg, DWORD cbMsg);
#endif
};
  

class LobbyServerSite : public IFedMessagingSite
{
public:
  // IFedMessagingSite
  virtual HRESULT OnAppMessage(FedMessaging * pthis, CFMConnection & cnxnFrom, FEDMESSAGE * pfm);
  virtual HRESULT OnSysMessage(FedMessaging * pthis) ;
  virtual void    OnMessageNAK(FedMessaging * pthis, DWORD dwTime, CFMRecipient * prcp) ;
  virtual HRESULT OnNewConnection(FedMessaging * pthis, CFMConnection & cnxn) ;
  virtual HRESULT OnDestroyConnection(FedMessaging * pthis, CFMConnection & cnxn) ;
  virtual HRESULT OnSessionLost(FedMessaging * pthis) ;
  virtual int     OnMessageBox(FedMessaging * pthis, const char * strText, const char * strCaption, UINT nType);
#ifndef NO_MSG_CRC
  virtual void    OnBadCRC(FedMessaging * pthis, CFMConnection & cnxn, BYTE * pMsg, DWORD cbMsg);
#endif
};

class CFLMission;
class CFLServer;

class CLobbyApp: public Win32App
#ifdef USECLUB
          , public ISQLSite2
#endif    
{
public:
  CLobbyApp(ILobbyAppSite * plas);
  ~CLobbyApp();

  // Win32App
  virtual bool    OnAssert(const char* psz, const char* pszFile, int line, const char* pszModule);
  virtual void    DebugOutput(const char *psz);
  
  HRESULT         Init();
  int             Run();
  bool            IsFMServers(FedMessaging * pfm) {return pfm == &m_fmServers;}

#ifdef USECLUB
  // ISQLSite2
  virtual void OnSQLErrorRecord(SSERRORINFO * perror, OLECHAR * postrError);
#endif

  ILobbyAppSite * GetSite()
  {
    return m_plas;
  }
  FedMessaging &  GetFMServers()
  {
    return m_fmServers;
  }
  FedMessaging &  GetFMClients()
  {
    return m_fmClients;
  }

  virtual int     OnMessageBox(const char * strText, const char * strCaption, UINT nType);

#ifdef USEAUTH
  TRef<IZoneAuthServer> GetZoneAuthServer()
  {
    return m_pzas;
  }
#endif

  PER_SERVER_COUNTERS * AllocatePerServerCounters(const char * szServername);

  void FreePerServerCounters(PER_SERVER_COUNTERS * pCounters)
  {
    m_perfshare.FreeCounters(pCounters);
  }
  
  LOBBY_COUNTERS * GetCounters()
  {
    return m_pCounters;
  }

  Time GetNow() // time at beginning of current cycle
  {
    return m_timeNow;
  }

  char * GetToken()
  {
    return m_szToken;
  }

  // Imago 9/14 // BT - STEAM
  CRITICAL_SECTION *GetLogonCS()
  {
	  return m_logonCS;
  }

  bool EnforceCDKey()
  {
    return m_fFreeLobby && m_fCheckCDKey;
  }

  //Orion ACSS : 2009 - Made more generic
  bool EnforceAuthentication()
  {
      return (m_dwAuthentication) ? true : false;
  }

  char* RetrieveAuthAddress()
  {
	  return m_szAuthenticationLocation;
  }

  char* RetrieveAuthBypassCDKey()
  {
	  return m_szAuthBypassCDKey;
  }



  // -KGJV: added
  bool IsFreeLobby()
  {
	  return m_fFreeLobby;
  }

  // BT - 12/21/2010 - ACSS integration
  bool CLobbyApp::GetRankForCallsign(const char* szPlayerName, int *rank, double *sigma, double *mu, int *commandRank, double *commandSigma, double *commandMu, char *rankName, int rankNameLen);
  //bool CDKeyIsValid(const char* szPlayerName, const char* szCDKey, const char* szAddress, char *resultMessage, int resultMessageLength);

  // BT - STEAM
  bool CDKeyIsValid(const char* szPlayerName, const char* szCDKey, const char* szAddress, char *resultMessage, int resultMessageLength, char *playerIdentifier);
  


  void SetPlayerMission(const char* szPlayerName, const char* szCDKey, CFLMission* pMission, const char* szAddress);
  void RemovePlayerFromMission(const char* szPlayerName, CFLMission* pMission);
  void RemoveAllPlayersFromMission(CFLMission* pMission);
  void RemoveAllPlayersFromServer(CFLServer* pServer);
  CFLMission* FindPlayersMission(const char* szPlayerName);

  bool BootPlayersByCDKey(const ZString& strCDKey, const ZString& strNameExclude = "", ZString& strOldPlayer = ZString());

#ifdef USECLUB
  CSQLCore & GetSQL() 
  {
    return m_sql;
  }
#endif

  bool ProcessMsgPump();
  //KGJV #114 - CoreInfoStuff
  int GetcStaticCoreInfo() { return m_cStaticCoreInfo; }
  StaticCoreInfo* GetvStaticCoreInfo() {return m_vStaticCoreInfo;}
  void BuildStaticCoreInfo();
  void FreeStaticCoreInfo()
  {
	  if (m_vStaticCoreInfo)
		  delete [] m_vStaticCoreInfo;
	  m_vStaticCoreInfo = NULL;
	  m_cStaticCoreInfo = 0;
  }

  // BT - STEAM
  void CheckAndUpdateDrmHashes(bool forceUpdate);
  
  // BT - STEAM
  char * GetApiKey() {
	  return m_szApiKey;
  }

  // BT - STEAM
  char * GetChatLogUploadUrl() {
	  return m_szChatlogUploadUrl;
  }

  // BT - STEAM
  char * GetBanCheckUrl() {
	  return m_szBanCheckUrl;
  }

private:
  const char *    SzFmMsgHeader(FedMessaging * pthis) {return IsFMServers(pthis) ? "Servers: " : "Clients: ";}
  void            SetNow()
  {
    m_timeNow = Time::Now();
  }
  ZGameServerInfoMsg* GetGameServerInfoMsg()
  {
    return (ZGameServerInfoMsg*) m_GameInfoBuf;
  }

  void SetConstantGameInfo();
  void SetVariableGameInfo();
  void SendGameInfo();
  void UpdatePerfCounters();
  void RollCall();

  // *** player list stuff *** 
  void BootPlayersByName(const ZString& strName);

  class PlayerLocInfo
  {
    ZString m_strName;
    CFLMission* m_pMission;
  public:
    PlayerLocInfo(const ZString& strName, CFLMission* pMission) 
        : m_strName(strName), m_pMission(pMission) {};

    const ZString&  GetName() { return m_strName; };
    CFLMission*     GetMission() { return m_pMission; };
  };

  struct StringCmpLess // sort by length, then content (faster)
  {
    bool operator () (const ZString& str1, const ZString& str2) const;
  };

  struct StringICmpLess // sort by length, then case insensitive content (faster)
  {
    bool operator () (const ZString& str1, const ZString& str2) const;
  };

// data
  ILobbyAppSite *   m_plas;
  FedMessaging      m_fmServers;
  FedMessaging      m_fmClients;
  LobbyClientSite   m_psiteClient;
  LobbyServerSite   m_psiteServer;
  char				m_szChatlogUploadUrl[MAX_PATH];   // BT - STEAM
  char				m_szBanCheckUrl[MAX_PATH];   // BT - STEAM
  char				m_szApiKey[MAX_PATH];   // BT - STEAM

  // *** Perfmon counter stuff ***
  CPerfShare        m_perfshare;
  LOBBY_COUNTERS *  m_pCounters;
// KG guard with USEAUTH for consistency 
#ifdef USEAUTH
  TRef<IZoneAuthServer> m_pzas;
#endif
  Time              m_timeNow;

  // BT - STEAM
  FILETIME			m_lastDrmHashUpdate;
  char				m_szDrmHashFilename[MAX_PATH];
  char				m_szDrmDownloadUrly[MAX_PATH];

  //ZGameInstanceInfoMsg is important info ZGameServerInfoMsg is trivial wrapper
  //structure designed with variable size elements and variable number of ZGameInstanceInfoMsg
  //but not necessary for game vendors to implement
  char              m_GameInfoBuf[sizeof(ZGameServerInfoMsg)+4096];
  int               m_cReportServers; // actual servers reporting to
  unsigned long     m_rgulIP[c_cReportServersMax]; // ip address(es) to report game info to
  DWORD             m_sGameInfoInterval;
  DWORD             m_sPort;				// port the lobby listens on for new clients
  bool              m_fProtocol;
  bool              m_fFreeLobby;
  bool              m_fCheckCDKey;
  char              m_szToken[24]; // sizeof(_ZONETICKET_TOKEN.szToken)
  DWORD             m_dwAuthentication;
  char				m_szAuthenticationLocation[261];
  char              m_szAuthBypassCDKey[MAX_PATH];

  // Player list stuff
  typedef std::multimap<ZString, PlayerLocInfo, StringCmpLess> PlayerByCDKey;
  typedef std::multimap<ZString, PlayerByCDKey::iterator, StringICmpLess> PlayerByName;
  PlayerByCDKey     m_playerByCDKey;
  PlayerByName      m_playerByName;

  // KGJV #114 - core stuff
  StaticCoreInfo   *m_vStaticCoreInfo;
  int               m_cStaticCoreInfo;

  //imago 9/16
  CRITICAL_SECTION  *m_logonCS;

#ifdef USECLUB
  // SQL Stuff
  CSQLCore          m_sql;
  DWORD             m_csqlSilentThreads;
  DWORD             m_csqlNotifyThreads;
  CComBSTR          m_strSQLConfig;
#endif
};

extern CLobbyApp * g_pLobbyApp;  

#endif
