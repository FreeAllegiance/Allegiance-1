/////////////////////////////////////////////////////////////////////////////
// PigBehaviorScript.h : Declaration of the CPigBehaviorScript class

#ifndef __PigBehaviorScript_h__
#define __PigBehaviorScript_h__

#include "PigSrv.h"
#include "resource.h"       // main symbols
#include "PigEngine.h"
#include "PigEventOwner.h"
#include "PigBehavior.h"
#include "PigBehaviorScriptMethods.h"
#include "IActiveScriptSiteImpl.h"
#include "PigMacro.h"


/////////////////////////////////////////////////////////////////////////////
// Forward Declarations

class CPig;
class CPigTimer;
class CPigBehaviorType;
class CPigBehaviorScriptType;


/////////////////////////////////////////////////////////////////////////////
// CPigBehaviorScript
class ATL_NO_VTABLE CPigBehaviorScript : 
  public IDispatchImpl<IPigBehavior, &IID_IPigBehavior, &LIBID_PigsLib>,
  public ISupportErrorInfo,
  public IActiveScriptSiteImpl<CPigBehaviorScript>,
  public CPigEventOwnerImpl<CPigBehaviorScript>,
  public CPigBehavior,
  public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPigBehaviorScript, &CLSID_PigBehaviorScript>
{
// Declarations
public:
  DECLARE_REGISTRY_RESOURCEID(IDR_PigBehaviorScript)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

// Category Map
public:
  BEGIN_CATEGORY_MAP(CPigBehaviorScript)
    IMPLEMENTED_CATEGORY(CATID_PigObjects)
  END_CATEGORY_MAP()

// Interface Map
public:
  BEGIN_COM_MAP(CPigBehaviorScript)
    COM_INTERFACE_ENTRY(IPigBehavior)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IActiveScriptSite)
  END_COM_MAP()

// Construction / Destruction
public:
  CPigBehaviorScript();
  HRESULT Init(CPigBehaviorScriptType* pType, CPig* pPig,
    BSTR bstrCommandLine, CPigBehaviorScript* pDerived = NULL);
  void Term();

// Attributes
public:
  HRESULT GetLastScriptError(IActiveScriptError** ppAse);
  IDispatchPtr GetScriptDispatch();
  CPigBehaviorScript* GetDerived();
  CPigBehaviorScript* GetMostDerived();

// Operations
public:
  HRESULT Eval(const char* pszExpression, VARIANT* pvResult);

// Overrides
public:
  // CPigEventOwner Overrides
  virtual void OnExpiration(CPigEvent* pEvent);
  virtual void OnInterruption(CPigEvent* pEvent);

// Implementation
protected:
  static CPigEngine& GetEngine();
  HRESULT CreateScriptingEngine();
  void ShutdownScriptingEngine();
  HRESULT ResolveScriptMethod(ITypeComp* ptc, LPCOLESTR pszMethodName,
    short cParams, DISPID* pid);
  HRESULT ResolveScriptMethod(LPCOLESTR pszMethodName,
    short cParams, DISPID* pid);
  HRESULT ResolveScriptMethods();
  void SetCurrentTimer(CPigTimer* pTimer);

// ISupportErrorInfo Interface Methods
public:
  STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

// IPigBehavior Interface Methods
public:
  // Infrastructure Properties
	STDMETHODIMP get_BehaviorType(IPigBehaviorType** ppType);
  STDMETHODIMP get_BaseBehavior(IDispatch** ppBaseBehavior);
	STDMETHODIMP get_Pig(IPig** ppPig);
  STDMETHODIMP get_Host(IPigBehaviorHost** ppHost);
  STDMETHODIMP get_IsActive(VARIANT_BOOL* pbIsActive);
  STDMETHODIMP get_Properties(IDictionary** ppProperties);
  STDMETHODIMP get_Timer(IPigTimer** ppTimer);
  STDMETHODIMP get_CommandLine(BSTR* pbstrCommandLine);
  // Infrastructure Methods
  STDMETHODIMP CreateTimer(float fInterval, BSTR bstrEventExpression,
    long nRepetitions, BSTR bstrName, IPigTimer** ppTimer);

  // Activation / Deactivation Response Methods
  CPigBehaviorScript_MethodImpl_1(OnActivate  , VT_DISPATCH, IPigBehavior*)
  CPigBehaviorScript_MethodImpl_1(OnDeactivate, VT_DISPATCH, IPigBehavior*)

  // State Transition Response Methods
  CPigBehaviorScript_MethodImpl_1(OnStateNonExistant      , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateLoggingOn        , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateLoggingOff       , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateMissionList      , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateCreatingMission  , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateJoiningMission   , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateQuittingMission  , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateTeamList         , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateJoiningTeam      , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateWaitingForMission, VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateDocked           , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateLaunching        , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateFlying           , VT_I4, PigState)
  CPigBehaviorScript_MethodImpl_1(OnStateTerminated       , VT_I4, PigState)

  // Game Response Methods
  CPigBehaviorScript_MethodImpl_0(OnMissionStarted)
  CPigBehaviorScript_MethodImpl_2R(OnReceiveChat, VT_BSTR, BSTR,
    VT_DISPATCH, IAGCShip*, VT_BOOL, VARIANT_BOOL)
  CPigBehaviorScript_MethodImpl_6(OnShipDamaged, VT_DISPATCH, IAGCShip*,
    VT_DISPATCH, IAGCModel*, VT_R4, float, VT_R4, float,
    VT_DISPATCH, IAGCVector*, VT_DISPATCH, IAGCVector*)
  CPigBehaviorScript_MethodImpl_5(OnShipKilled, VT_DISPATCH, IAGCShip*, VT_DISPATCH, IAGCModel*,
    VT_R4, float, VT_DISPATCH, IAGCVector*, VT_DISPATCH, IAGCVector*)
  CPigBehaviorScript_MethodImpl_2(OnSectorChange, VT_DISPATCH, IAGCSector*,
    VT_DISPATCH, IAGCSector*)
  CPigBehaviorScript_MethodImpl_1(OnAlephHit, VT_DISPATCH, IAGCAleph*);

// IActiveScriptSite Interface Methods
public:
  STDMETHODIMP GetItemInfo(LPCOLESTR, DWORD, IUnknown**, ITypeInfo**);
  STDMETHODIMP OnScriptError(IActiveScriptError* pScriptError);

// Types
protected:
  typedef TCObjectLock<CPigBehaviorScript> XLock;

// Data Members
protected:
  IActiveScript *          m_spAs;
  IActiveScriptParse *     m_spAsp;
  IActiveScriptError *     m_spAse;
  IDictionaryPtr            m_spProperties;
  IDispatchPtr              m_spdScript;
  IPigBehaviorPtr           m_spBehaviorBase;
  CPigBehaviorScript*       m_pBehaviorBase;
  CPigBehaviorScript*       m_pBehaviorDerived;
  CPigBehaviorScriptType*   m_pType;
  CPig*                     m_pPig;
  DWORD                     m_dwGITCookie;
  CPigBehaviorScriptMethods m_Methods;
  CPigTimer*                m_pTimerCurrent;
  CComBSTR                  m_bstrCommandLine;
};


/////////////////////////////////////////////////////////////////////////////
// Attributes

inline HRESULT CPigBehaviorScript::GetLastScriptError(IActiveScriptError** ppAse)
{
  if (NULL == m_spAse)
    return S_FALSE;
  *ppAse = m_spAse; //
  return S_OK;
}

inline IDispatchPtr CPigBehaviorScript::GetScriptDispatch()
{
  return m_spdScript;
}

inline CPigBehaviorScript* CPigBehaviorScript::GetDerived()
{
  return m_pBehaviorDerived;
}

inline CPigBehaviorScript* CPigBehaviorScript::GetMostDerived()
{
  CPigBehaviorScript* pDerivedPrev;
  CPigBehaviorScript* pDerived     = this;
  while (pDerived)
  {
    pDerivedPrev = pDerived;
    pDerived = pDerived->GetDerived();
  }
  return pDerivedPrev;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

inline CPigEngine& CPigBehaviorScript::GetEngine()
{
  return CPigEngine::GetEngine();
}


/////////////////////////////////////////////////////////////////////////////

#endif //__PigBehaviorScript_h__
