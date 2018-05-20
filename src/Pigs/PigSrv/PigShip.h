#ifndef __PigShip_h__
#define __PigShip_h__

/////////////////////////////////////////////////////////////////////////////
// PigShip.h : Declaration of the CPigShip class

#include "resource.h"       // main symbols
#include "PigEngine.h"
#include "PigSrv.h"
#include "Pig.h"
#include <IAGCShipImpl.h>
#include <..\TCAtl\ErrorFormatter.h>
#include <Allegiance.h>


/////////////////////////////////////////////////////////////////////////////
// CPigShip
class ATL_NO_VTABLE CPigShip : 
  public IAGCShipImpl<CPigShip, IshipIGC, IPigShip, &LIBID_PigsLib>,
  public ISupportErrorInfo,
  public TCErrorFormatter<CPigShip, &IID_IPigShip>,
	public CComCoClass<CPigShip, &CLSID_PigShip>
{
// Declarations
public:
  DECLARE_REGISTRY_RESOURCEID(IDR_PigShip)
  DECLARE_PROTECT_FINAL_CONSTRUCT()
  DECLARE_AGC_TYPE(pig)

// Category Map
public:
  BEGIN_CATEGORY_MAP(CPigShip)
    IMPLEMENTED_CATEGORY(CATID_PigObjects)
  END_CATEGORY_MAP()

// Interface Map
public:
  BEGIN_COM_MAP(CPigShip)
    COM_INTERFACE_ENTRY(IPigShip)
    COM_INTERFACE_ENTRIES_IAGCShipImpl()
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
  END_COM_MAP()

// Construction / Destruction
public:
  CPigShip();
  HRESULT FinalConstruct();
  void FinalRelease();
  HRESULT Init(CPig* pPig);

// Attributes
public:
  CPig* GetPig() const;
  void SetIGC(IshipIGC* pShipIGC);

// Operations
public:
  bool OnNewChatMessage();
  void KillAutoAction();

// Implementation
public:
	static void WINAPI ObjectMain(bool bStarting);
protected:
  static CPigEngine& GetEngine();
  static void LoadShipResponse(BSTR* pbstrResponse, UINT id = IDS_SHIP_RESPONSE);
  static HRESULT ValidateThrustBit(PigThrust eThrust);
  static int InterpretThrustBit(PigThrust eThrust);
  float GetAxis(Axis eAxis) const;
  void SetAxis(Axis eAxis, float fNewValue);
  HRESULT CreateCommandWrapper(Command cmd, IAGCCommand** ppCommand);
  void ReplaceAutoAction(IPigShipEvent* pAutoAction);
  IclusterIGC* CPigShip::FindCluster(BSTR bstrSector);
  ImodelIGC* CPigShip::FindModel(BSTR bstrModel);

// ISupportErrorInfo Interface Methods
public:
  STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

// IAGCShip Interface Methods
public:
  STDMETHODIMP put_WingID(short Val);
  STDMETHODIMP put_AutoDonate(IAGCShip* pShip);

// IPigShip Interface Methods
public:
  STDMETHODIMP SellAllParts();
  STDMETHODIMP BuyHull(IAGCHullType* pHullType);
  STDMETHODIMP BuyDefaultLoadout();
  STDMETHODIMP CommitSuicide(BSTR* pbstrResponse);
  STDMETHODIMP AllStop(BSTR* pbstrResponse);
  STDMETHODIMP Thrust(PigThrust e1, PigThrust e2, PigThrust e3, PigThrust e4,
    PigThrust e5, PigThrust e6, PigThrust e7, BSTR* pbstrResponse);
  STDMETHODIMP Boost(VARIANT_BOOL bOn, BSTR* pbstrResponse);
  STDMETHODIMP FireWeapon(VARIANT_BOOL bBegin, BSTR* pbstrResponse);
  STDMETHODIMP FireMissile(VARIANT_BOOL bBegin, BSTR* pbstrResponse);
  STDMETHODIMP DropMine(VARIANT_BOOL bBegin, BSTR* pbstrResponse);
  STDMETHODIMP LockVector(VARIANT_BOOL bLockVector, BSTR* pbstrResponse);
  STDMETHODIMP RipCord(VARIANT* pvParam, BSTR* pbstrResponse);
  STDMETHODIMP Cloak(VARIANT_BOOL bCloak, BSTR* pbstrResponse);
  STDMETHODIMP AcceptCommand(BSTR* pbstrResponse);
  STDMETHODIMP ClearCommand(BSTR* pbstrResponse);
  STDMETHODIMP Face(VARIANT* pvarObject, BSTR bstrExpirationExpr,
    BSTR bstrInterruptionExpr, VARIANT_BOOL bLevel, BSTR* pbstrResponse);
  STDMETHODIMP Defend(BSTR bstrObject, BSTR* pbstrResponse);
  STDMETHODIMP AttackShip(IAGCShip* pTarget, BSTR* pbstrResponse);
  STDMETHODIMP AttackStation(IAGCStation* pTarget, BSTR* pbstrResponse);
  STDMETHODIMP Goto(BSTR bstrObject, VARIANT_BOOL bFriendly, BSTR* pbstrResponse);
  STDMETHODIMP GotoStationID(ObjectID oid, BSTR* pbstrResponse);
  STDMETHODIMP put_AutoPilot(VARIANT_BOOL bEngage);
  STDMETHODIMP get_AutoPilot(VARIANT_BOOL* pbEngaged);
  STDMETHODIMP get_AutoAction(IPigShipEvent** ppAutoAction);
  STDMETHODIMP get_IsThrusting(PigThrust eThrust, VARIANT_BOOL* pbThrusting);
  STDMETHODIMP put_ActiveWeapon(long iWeapon);
  STDMETHODIMP get_ActiveWeapon(long* piWeapon);
  STDMETHODIMP get_IsFiringWeapon(VARIANT_BOOL* pbFiring);
  STDMETHODIMP get_IsFiringMissile(VARIANT_BOOL* pbFiring);
  STDMETHODIMP get_IsDroppingMine(VARIANT_BOOL* pbDropping);
  STDMETHODIMP get_IsVectorLocked(VARIANT_BOOL* pbVectorLocked);
  STDMETHODIMP get_IsRipCording(VARIANT_BOOL* pbRipCording);
  STDMETHODIMP get_IsCloaked(VARIANT_BOOL* pbCloaked);
  STDMETHODIMP get_MissileRange(float* pfRange);
  STDMETHODIMP get_MissileLock(float* pfLock);
  STDMETHODIMP put_Throttle(float fThrottle);
  STDMETHODIMP get_Throttle(float* pfThrottle);
  STDMETHODIMP put_Pitch(float fPitch);
  STDMETHODIMP get_Pitch(float* pfPitch);
  STDMETHODIMP put_Yaw(float fYaw);
  STDMETHODIMP get_Yaw(float* pfYaw);
  STDMETHODIMP put_Roll(float fRoll);
  STDMETHODIMP get_Roll(float* pfRoll);
  STDMETHODIMP get_QueuedCommand(IAGCCommand** ppCommand);
  STDMETHODIMP get_AcceptedCommand(IAGCCommand** ppCommand);
  STDMETHODIMP put_AutoAcceptCommands(VARIANT_BOOL bAutoAccept);
  STDMETHODIMP get_AutoAcceptCommands(VARIANT_BOOL* pbAutoAccept);
  STDMETHODIMP get_RipCordTimeLeft(float* pfRipCordTimeLeft);

// Types
protected:
  typedef TCObjectLock<CPigShip> XLock;

// Data Members
protected:
  static const int c_nThrustmask;
  CPig*            m_pPig;
  IPigShipEventPtr m_spDummyEvent;
  IPigShipEventPtr m_spAutoAction;
  bool             m_bAllWeapons : 1;
  bool             m_bAutoAcceptCommands : 1;
};


/////////////////////////////////////////////////////////////////////////////
// Attributes

inline CPig* CPigShip::GetPig() const
{
  return m_pPig;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

inline void WINAPI CPigShip::ObjectMain(bool bStarting)
{
  // Do not register a creator function for IAGCShip or IPigShip.
  // This is because we create this object directly from CPig.
  // By not registering a creator function for IAGCShip, the AGCShip object
  // will be created for all ships that are not Pig ships.
}

inline CPigEngine& CPigShip::GetEngine()
{
  return CPigEngine::GetEngine();
}

inline void CPigShip::LoadShipResponse(BSTR* pbstrResponse, UINT id)
{
  // Load the ship's method response string
  if (pbstrResponse)
  {
    CComBSTR bstr;
    _VERIFYE(bstr.LoadString(id));
    *pbstrResponse = bstr.Detach();
  }
}

inline HRESULT CPigShip::ValidateThrustBit(PigThrust eThrust)
{
  return (ThrustNone <= eThrust && eThrust <= ThrustBooster) ?
    S_OK : E_INVALIDARG;
}

inline int CPigShip::InterpretThrustBit(PigThrust eThrust)
{
  static int nBits[] =
  {
    0,                    // ThrustNone
    leftButtonIGC,        // ThrustLeft
    rightButtonIGC,       // ThrustRight
    upButtonIGC,          // ThrustUp
    downButtonIGC,        // ThrustDown
    forwardButtonIGC,     // ThrustForward
    backwardButtonIGC,    // ThrustBackward
    afterburnerButtonIGC, // ThrustBooster
  };
  return nBits[eThrust];
}

inline float CPigShip::GetAxis(Axis eAxis) const
{
  assert(c_axisYaw <= eAxis && eAxis < c_axisMax);
  return m_pPig->BaseClient::GetShip()->GetControls().jsValues[eAxis];
}

inline void CPigShip::SetAxis(Axis eAxis, float fNewValue)
{
  assert(c_axisYaw <= eAxis && eAxis < c_axisMax);
  assert(-1.f <= fNewValue && fNewValue <= 1.f);

  ControlData cd = m_pPig->BaseClient::GetShip()->GetControls();
  cd.jsValues[eAxis] = fNewValue;
  m_pPig->BaseClient::GetShip()->SetControls(cd);
}


/////////////////////////////////////////////////////////////////////////////

#endif //__PigShip_h__
