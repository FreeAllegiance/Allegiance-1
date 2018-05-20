#ifndef __Pig_h__
#define __Pig_h__

/////////////////////////////////////////////////////////////////////////////
// Pig.h : Declaration of the CPig class

#include "resource.h"       // main symbols
#include "PigEngine.h"
#include "PigSrv.h"
#include <..\TCLib\TCThread.h>
#include <..\TCLib\AutoHandle.h>
#include <..\TCLib\LookupTable.h>
#include <Allegiance.h>
#include "PigStaticSite.h"


/////////////////////////////////////////////////////////////////////////////
// Constants
const int MAX_USER_NAME_LENGTH     = 20;
const int MAX_USER_PASSWORD_LENGTH = 10;


/////////////////////////////////////////////////////////////////////////////
// Forward Declarations

class CPigBehavior;
class CPigBehaviorScript;
class CPigShip;


/////////////////////////////////////////////////////////////////////////////
// CPig
class ATL_NO_VTABLE CPig : 
  public IDispatchImpl<IPig, &IID_IPig, &LIBID_PigsLib>,
  public ISupportErrorInfo,
  public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPig, &CLSID_Pig>,
  public BaseClient,
  public IAutoUpdateSink,
  public IClientEventSink
{
// Declarations
public:
  DECLARE_REGISTRY_RESOURCEID(IDR_Pig)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

// Category Map
public:
  BEGIN_CATEGORY_MAP(CPig)
    IMPLEMENTED_CATEGORY(CATID_PigObjects)
  END_CATEGORY_MAP()

// Interface Map
public:
  BEGIN_COM_MAP(CPig)
    COM_INTERFACE_ENTRY(IPig)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
  END_COM_MAP()

// Types
protected:
  class XThreadParams;
  /////////////////////////////////////////////////////////////////////////////
  // Description: Properties of a chat command verb keyword.
  //
  struct XChatVerb
  {
    // Specifies the verb text used for matching to the beginning of the chat
    // message string, as further specified by m_bToken and m_cchSignificant.
    // The verb text matching is always case-insensitive.
    LPCSTR m_szVerb;

    // Indicates whether m_szVerb is to be matched against a token or
    // characters. If *true*, the match is performed against the first
    // space-delimited token of the chat message string, as further specified
    // with m_cchSignificant. Otherwise, the exact number of characters in
    // m_szVerb must match the beginning of the chat message string.
    bool m_bToken;

    // Indicates the number of significant characters in the verb to match on.
    // If -1, the entire verb token must match m_szVerb.
    int m_cchSignificant;
  };
  struct XChatCommand;
  typedef const XChatCommand& REFXChatCommand;
  typedef bool (CPig::*XChatCommandProc)(REFXChatCommand);
  struct XSectorChange;
  struct XShipDamaged;
  struct XShipKilled;
  struct XAlephHit;

  typedef std::vector<HANDLE>                 XHandles;
  typedef std::vector<CPigBehaviorScript*>    XBehaviors;
  typedef XBehaviors::iterator                XBehaviorIt;
  typedef TCObjectLock<CPig>                  XLock;
  typedef TCObjectLock<TCAutoCriticalSection> XLockStatic;

  // Description: Used for testing performance thresholds.
  struct XShipPerf
  {
    IshipIGC* pShip;
    Vector    vPosPrev;
    Vector    vFwdPrev;
  };
  typedef std::vector<XShipPerf> XHeavyShips;
  typedef XHeavyShips::iterator  XHeavyShipsIt;

  // Description: A structure containing a kernel event and a string.
  struct XEvent
  {
    XEvent() : m_hevt(::CreateEvent(NULL, false, false, NULL)) {}
    void Set()
    {
      m_bstr.Empty();
      ::SetEvent(m_hevt);
    }
    void Set(const CComBSTR& bstr)
    {
      m_bstr = bstr;
      ::SetEvent(m_hevt);
    }
    void Reset()
    {
      ::ResetEvent(m_hevt);
      m_bstr.Empty();
    }
    operator HANDLE()
    {
      return m_hevt;
    }
    operator BSTR()
    {
      return m_bstr;
    }
    TCHandle m_hevt;
    CComBSTR m_bstr;
  };

// Construction / Destruction
protected:
  CPig();
  HRESULT FinalConstruct();
  void FinalRelease();
public:
  static HRESULT Create(CPigBehaviorScriptType* pType, BSTR bstrCommandLine,
    DWORD* pdwGITCookie);

// Attributes
public:
  Time GetLastUpdateTime() const;
  PigState GetCurrentState();
  PigState SetCurrentState(PigState eNewState);
  CPigBehaviorScript* GetActiveBehavior() const;
  float GetUpdateFraction();
  SoundID GetOink();
  BSTR InternalGetName();

// Operations
public:
  void ReceiveSendAndUpdate();
  bool WaitInTimerLoop(HANDLE hObject, DWORD dwMilliseconds = INFINITE);
  void SendChat(ChatTarget chatTarget, const char* psz,
    IshipIGC* pshipRecipient = NULL, SoundID voiceOver = NA);

// Implementation
public:
  // Used by CPigBehaviorStack
  HRESULT Stack_get_Count(long* pnCount);
  HRESULT Stack_get__NewEnum(IUnknown** ppunkEnum);
  HRESULT Stack_Push(BSTR bstrType, BSTR bstrCommandLine,
    IPigBehavior** ppBehavior);
  HRESULT Stack_Pop();
  HRESULT Stack_get_Top(long nFromTop, IPigBehavior** ppBehavior);
protected:
  static CPigEngine& GetEngine();
  static unsigned CALLBACK ThreadThunk(void* pvParam);
  static HRESULT ThreadCreatePig(XThreadParams* ptp, CComObject<CPig>*& pPig);
  void ThreadProc();
  bool HandleThreadMessage(const MSG* pMsg, HANDLE hObject);
  HRESULT ProcessAppMessage(FEDMESSAGE* pfm);
  bool OnChat_AreYouAPig(REFXChatCommand chat);
  bool OnChat_Evaluate(REFXChatCommand chat);
  bool FindMissionName(char* pszMissionName);
  void CacheMaxShipAngleThresholds();
  void CacheMaxShipDistanceThresholds();
  void CacheMaxShipsUpdateLatencyThresholds();
  bool HasAnyPerformanceThresholds();
  HRESULT SendBytes(FedMessaging* pfm, VARIANT* pvBytes, bool bGuaranteed);

// ISupportErrorInfo Interface Methods
public:
  STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

// BaseClient Overrides
public:
  virtual HRESULT OnSessionLost(char* szReason, FedMessaging * pthis);
  virtual void ModifyShipData(DataShipIGC* pds);
  virtual void OnQuitMission(QuitSideReason reason, const char* szMessageParam);
  virtual void OnLogonAck(bool fValidated, bool bRetry, LPCSTR szFailureReason);
  virtual void OnLogonLobbyAck(bool fValidated, bool bRetry, LPCSTR szFailureReason);
  virtual const char* GetArtPath();
  virtual IAutoUpdateSink* OnBeginAutoUpdate();
  virtual bool ResetStaticData(char* szIGCStaticFile,
    ImissionIGC** ppStaticIGC, Time tNow, bool bEncrypt);
  virtual void StartLockDown(const ZString& strReason, LockdownCriteria criteria);
  virtual void EndLockDown(LockdownCriteria criteria);
protected:
  virtual void CreateDummyShip();
  virtual void DestroyDummyShip();

// IFedMessagingSite Overrides
public:
  virtual HRESULT OnAppMessage(FedMessaging * pthis, CFMConnection & cnxnFrom, FEDMESSAGE * pfm);
  virtual int     OnMessageBox(FedMessaging * pthis, const char* strText, const char* strCaption, UINT nType);
  virtual void    OnPreCreate (FedMessaging * pthis);
// KGJV TODO  virtual void    OnPostCreate(FedMessaging * pthis, IDirectPlayX* pdpIn, IDirectPlayX** pdpOut);

// IClientEventSink Overrides
public:
  virtual void OnAddMission(MissionInfo* pMissionDef);
  virtual void OnMissionStarted(MissionInfo* pMissionDef);
  virtual void OnAddPlayer(MissionInfo* pMissionDef, SideID sideID, PlayerInfo* pPlayerInfo);
  virtual void OnDelRequest(MissionInfo* pMissionInfo, SideID sideID, PlayerInfo* pPlayerInfo, DelPositionReqReason reason);

// IAutoUpdateSink Overrides
public:
  virtual void OnBeginRetrievingFileList();
  virtual bool ShouldFilterFile(const char * szFileName); // if returns true, then file is not downloaded
  virtual void OnAutoUpdateSystemTermination(bool bErrorOccurred, bool bRestarting);
  virtual void OnError(char* szErrorMessage);

// IIgcSite Overrides
public:
  virtual TRef<ThingSite> CreateThingSite(ImodelIGC* pModel);
  virtual TRef<ClusterSite> CreateClusterSite(IclusterIGC* pCluster);
  virtual void PlayNotificationSound(SoundID soundID, ImodelIGC* model);
  virtual int MessageBox(const ZString& strText, const ZString& strCaption,
    UINT nType);
  virtual void ChangeStation(IshipIGC* pship, IstationIGC* pstationOld,
    IstationIGC* pstationNew);
  virtual void ChangeCluster(IshipIGC*  pship, IclusterIGC* pclusterOld,
    IclusterIGC* pclusterNew);
  virtual void ReceiveChat(IshipIGC* pshipSender, ChatTarget ctRecipient,
    ObjectID oidRecipient, SoundID voiceOver, const char* szText,
    CommandID cid, ObjectType otTarget, ObjectID oidTarget,
    ImodelIGC* pmodelTarget = NULL, bool bObjectModel = false);
  virtual void SendChat(IshipIGC* pshipSender, ChatTarget chatTarget,
    ObjectID oidRecipient, SoundID soVoiceOver, const char* szText,
    CommandID cid = c_cidNone, ObjectType otTarget = NA,
    ObjectID oidTarget = NA, ImodelIGC* pmodelTarget = NULL);
  virtual void TerminateModelEvent(ImodelIGC * pModel);
  virtual void DamageShipEvent(Time now, IshipIGC* ship, ImodelIGC* launcher,
    DamageTypeID type, float amount, float leakage, const Vector& p1, const Vector& p2);
  virtual void KillShipEvent(Time timeCollision, IshipIGC* ship,
    ImodelIGC* launcher, float amount, const Vector& p1, const Vector& p2);
  virtual void HitWarpEvent(IshipIGC* ship, IwarpIGC* warp);
  virtual bool HitTreasureEvent(Time now, IshipIGC* ship, ItreasureIGC* treasure);
  virtual void GameOver(SideID iWinner); //imago 10/14
  //virtual void ActivateRipcord(IshipIGC* ship, bool activeF);

// IPig Interface Methods
public:
  STDMETHODIMP get_PigState(PigState* pePigState);
  STDMETHODIMP get_PigStateName(BSTR* pbstrPigState);
  STDMETHODIMP get_Stack(IPigBehaviorStack** ppStack);
  STDMETHODIMP get_Name(BSTR* pbstrName);
  STDMETHODIMP get_Ship(IPigShip** ppShip);
  STDMETHODIMP put_UpdatesPerSecond(long nPerSecond);
  STDMETHODIMP get_UpdatesPerSecond(long* pnPerSecond);
  STDMETHODIMP get_HullTypes(IPigHullTypes** ppHullTypes);
  STDMETHODIMP get_Money(AGCMoney* pnMoney);
  STDMETHODIMP Logon();
  STDMETHODIMP Logoff();
  STDMETHODIMP CreateMission(BSTR bstrServer, BSTR bstrAddr, IPigMissionParams* pMissionParams); //imago 10/14
  STDMETHODIMP JoinMission(BSTR bstrMissionOrPlayer);
  STDMETHODIMP JoinTeam(BSTR bstrCivName, BSTR bstrTeamOrPlayer);
  STDMETHODIMP QuitGame();
  STDMETHODIMP Launch();
  STDMETHODIMP Shutdown();
  STDMETHODIMP IsMissionOwner(BOOL* bOwner); //imago 10/14
  STDMETHODIMP IsTeamLeader(BOOL* bLeader);
  STDMETHODIMP SetSkills(float fShoot, float fTurn, float fGoto); //imago 10/14
  STDMETHODIMP get_Game(IAGCGame** ppGame);
  STDMETHODIMP get_Me(IAGCShip** ppShip);
  STDMETHODIMP get_ChatTarget(AGCChatTarget* peChatTarget);
  STDMETHODIMP get_Host(IPigBehaviorHost** ppHost);
  STDMETHODIMP StartCustomGame(IPigMissionParams* pMissionParams);
  STDMETHODIMP StartGame();
  STDMETHODIMP put_ShipAngleThreshold1(float rRadians);
  STDMETHODIMP get_ShipAngleThreshold1(float* prRadians);
  STDMETHODIMP put_ShipAngleThreshold2(float rRadians);
  STDMETHODIMP get_ShipAngleThreshold2(float* prRadians);
  STDMETHODIMP put_ShipAngleThreshold3(float rRadians);
  STDMETHODIMP get_ShipAngleThreshold3(float* prRadians);
  STDMETHODIMP put_ShipDistanceThreshold1(float rDistance);
  STDMETHODIMP get_ShipDistanceThreshold1(float* prDistance);
  STDMETHODIMP put_ShipDistanceThreshold2(float rDistance);
  STDMETHODIMP get_ShipDistanceThreshold2(float* prDistance);
  STDMETHODIMP put_ShipDistanceThreshold3(float rDistance);
  STDMETHODIMP get_ShipDistanceThreshold3(float* prDistance);
  STDMETHODIMP put_ShipsUpdateLatencyThreshold1(long nMilliseconds);
  STDMETHODIMP get_ShipsUpdateLatencyThreshold1(long* pnMilliseconds);
  STDMETHODIMP put_ShipsUpdateLatencyThreshold2(long nMilliseconds);
  STDMETHODIMP get_ShipsUpdateLatencyThreshold2(long* pnMilliseconds);
  STDMETHODIMP put_ShipsUpdateLatencyThreshold3(long nMilliseconds);
  STDMETHODIMP get_ShipsUpdateLatencyThreshold3(long* pnMilliseconds);
  STDMETHODIMP SendBytesToLobby(VARIANT* pvBytes, VARIANT_BOOL bGuaranteed);
  STDMETHODIMP SendBytesToGame(VARIANT* pvBytes, VARIANT_BOOL bGuaranteed);

// Enumerations
protected:
  // Message ID's
  enum
  {
    wm_TerminateBehavior = WM_APP,
    wm_Shutdown,
    wm_FireStateChange,
    wm_SignalEvent,

    wm_OnMissionStarted,
    wm_OnReceiveChat,
    wm_OnSectorChange,
    wm_OnShipDamaged,
    wm_OnShipKilled,
    wm_OnAlephHit,
  };

// Data Members
protected:
  // Threading
  static TCAutoCriticalSection s_csCreate, s_csUpdate, s_csLogon;
  TCPtr<TCThread*>             m_pth;
  DWORD                        m_dwGITCookie;
  DWORD                        m_dwSliceStart;
  XHandles                     m_Handles;
  // Object Model
  PigState                     m_eCurrentState;
  XBehaviors                   m_Behaviors;
  IPigBehaviorStackPtr         m_spBehaviorStack;
  TCPtr<CPigShip*>             m_pShip;
  IPigShipPtr                  m_spShip;
  IPigBehaviorHostPtr          m_spHost;
  // Allegiance
  Time                         m_timeLastUpdate;
  TRef<IClientEventSink>       m_ClientEventSink;
  long                         m_nTicksPerSecond;
  DWORD                        m_msPerTick;
  bool                         m_bDisconnected:1;
  CComBSTR                     m_bstrTryingToRipcord;
  // Messaging
  bool                         m_bInReceive:1;
  // Logon Parameters
  CComBSTR                     m_bstrName;
  BSTR						   m_bstrCdKey;
  IPigAccountPtr               m_spAccount;
  // Logon Acknowledgement status
  bool                         m_bLogonAck:1;
  // Team join Acknowlegdement status
  bool                         m_bTeamAccepted:1;
  // Event status
  XEvent                       m_evtQuitMission;
  XEvent                       m_evtLogonAck;
  XEvent                       m_evtLogonLobbyAck;
  XEvent                       m_evtJoiningTeam;
  XEvent                       m_evtJoiningMission;
  XEvent                       m_evtJoiningMission2;
  XEvent                       m_evtCreatingMission;
  XEvent                       m_evtLaunching;
  XEvent                       m_evtDocked;
  // Chat command processing
  IAGCShipPtr                  m_spShipChatOrigin;
  AGCChatTarget                m_eChatTarget;
  TCLookupTable_DECLARE(ChatCommands, XChatVerb, XChatCommandProc)
  // Performance Thresholds
  float                        m_rShipAngleThresholds          [3];
  float                        m_rShipDistanceThresholds       [3];
  DWORD                        m_dwShipsUpdateLatencyThresholds[3];
  float                        m_rMaxShipAngleThreshold;
  float                        m_rMaxShipDistanceThreshold;
  DWORD                        m_dwMaxShipsUpdateLatencyThreshold;

// Friends
  friend class CPigShip;
};


/////////////////////////////////////////////////////////////////////////////
// Attributes

inline Time CPig::GetLastUpdateTime() const
{
  return m_timeLastUpdate;
}

inline PigState CPig::GetCurrentState()
{
  XLock lock(this);
  return m_eCurrentState;
}

inline CPigBehaviorScript* CPig::GetActiveBehavior() const
{
  return m_Behaviors.empty() ? NULL : m_Behaviors.back();
}

inline float CPig::GetUpdateFraction()
{
  XLock lock(this);
  return 1.f / m_nTicksPerSecond;
}

inline BSTR CPig::InternalGetName()
{
  XLock lock(this);
  return m_bstrName;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

inline CPigEngine& CPig::GetEngine()
{
  return CPigEngine::GetEngine();
}

inline bool CPig::HasAnyPerformanceThresholds()
{
  XLock lock(this);
  return 
    0.f != m_rMaxShipAngleThreshold         ||
    0.f != m_rMaxShipDistanceThreshold      ||
    0   != m_dwMaxShipsUpdateLatencyThreshold;
}

/////////////////////////////////////////////////////////////////////////////
// Global Template Function

template<class T>
int findMaxThresholdExceeded(const T* pItems, int cItems, const T& value)
{
  T   tMax =  0;
  int iMax = -1;
  const T* end = pItems + cItems;
  for (const T* it = pItems; it < end; ++it)
  {
    if (value > *it && *it > tMax)
    {
      tMax = *it;
      iMax = it - pItems;
    }
  }
  return iMax;
}


/////////////////////////////////////////////////////////////////////////////

#endif //__Pig_h__
