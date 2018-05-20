/////////////////////////////////////////////////////////////////////////////
// PigShip.cpp : Implementation of the CPigShip class.

#include "pch.h"
#include <AGC.h>
#include <TCLib.h>
#include <..\TCAtl\ObjectMap.h>
#include "PigEventOwner.h"
#include "PigShipEvent.h"
#include "PigShip.h"


/////////////////////////////////////////////////////////////////////////////
// CPigShip

TC_OBJECT_EXTERN_NON_CREATEABLE_IMPL(CPigShip)


/////////////////////////////////////////////////////////////////////////////
// Static Initialization

const int CPigShip::c_nThrustmask = leftButtonIGC | rightButtonIGC
  | upButtonIGC | downButtonIGC | forwardButtonIGC | backwardButtonIGC
  | afterburnerButtonIGC;


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPigShip::CPigShip() :
  m_pPig(NULL),
  m_bAllWeapons(true),
  m_bAutoAcceptCommands(true)
{
}

HRESULT CPigShip::FinalConstruct()
{
  // Create a dummy event so that AutoAction always returns an object
  CComObject<CPigShipDummyEvent>* pDummyEvent = NULL;
  RETURN_FAILED(pDummyEvent->CreateInstance(&pDummyEvent));
  m_spDummyEvent = pDummyEvent;
  assert(NULL != m_spDummyEvent);

  // Indicate success
  return S_OK;
}

void CPigShip::FinalRelease()
{
}

HRESULT CPigShip::Init(CPig* pPig)
{
  // Save the specified pig pointer
  assert(pPig);
  assert(!m_pPig);
  m_pPig = pPig;

  // Set the IshipIGC pointer of the object
  SetIGC(m_pPig->GetShip());

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Attributes

void CPigShip::SetIGC(IshipIGC* pShipIGC)
{
  // Remove the AGC mapping of this object
  if (GetIGC())
    GetAGCGlobal()->RemoveAGCObject(GetIGC(), false);

  // Set the base IGC pointer of the object
  IAGCShipImplBase::Init(pShipIGC);

  // Add this as the AGC wrapper to the map
  if (pShipIGC)
    GetAGCGlobal()->AddAGCObject(pShipIGC, GetUnknown());
}


/////////////////////////////////////////////////////////////////////////////
// Operations

bool CPigShip::OnNewChatMessage()
{

	ImodelIGC* pModel = GetIGC()->GetCommandTarget(c_cmdQueued);
  // Return if the chat message was not a command or no command is queued
	if (!pModel)
    return false;

	CommandID idCmd = GetIGC()->GetCommandID(c_cmdQueued);
	const char* pszVerb = (0 <= idCmd && idCmd < c_cidMax) ? c_cdAllCommands[idCmd].szVerb : "";
	debugf("...got a <%s> command to %s!!!\n",pszVerb,pModel->GetName());
  // Accept the queued command if auto accept is enabled
  return m_bAutoAcceptCommands ? SUCCEEDED(AcceptCommand(NULL)) : true;
}


void CPigShip::KillAutoAction()
{
  // Reset the AutoPilot flag
  m_pPig->BaseClient::SetAutoPilot(false);

  // Disconnect any current action event object
  if (NULL != m_spAutoAction && m_spDummyEvent != m_spAutoAction)
  {
    // Release external references to the object
    CoDisconnectObject(m_spAutoAction, 0);

    // Kill the action event object
    m_spAutoAction->Kill();

    // Release the object and replace it with the dummy event
    m_spAutoAction = m_spDummyEvent;
  }
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

HRESULT CPigShip::CreateCommandWrapper(Command cmd, IAGCCommand** ppCommand)
{
  // Initialize the [out] parameter
  CLEAROUT(ppCommand, (IAGCCommand*)NULL);

  // Create a new AGC.Command object
  IAGCCommandPrivatePtr spCmd;
  RETURN_FAILED(spCmd.CreateInstance(CLSID_AGCCommand));

  // Get the command target and id
  ImodelIGC* pModel = GetIGC()->GetCommandTarget(cmd);
  const CommandID idCmd = GetIGC()->GetCommandID(cmd);

  // Get the strings for the command target and id
  const char* pszTarget = pModel ? GetModelName(pModel) : NULL;
  const char* pszVerb = (0 <= idCmd && idCmd < c_cidMax) ?
    c_cdAllCommands[idCmd].szVerb : "";

  // Initialize the new object
  spCmd->Init(pszTarget, pszVerb);

  // Query for the IAGCCommand interface
  _SVERIFYE(spCmd->QueryInterface(IID_IAGCCommand, (void**)ppCommand));

  // Indicate success
  return S_OK;
}

void CPigShip::ReplaceAutoAction(IPigShipEvent* pAutoAction)
{
  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Set the new action event object
  m_spAutoAction = pAutoAction;
}


IclusterIGC* CPigShip::FindCluster(BSTR bstrSector)
{
  if (!BSTRLen(bstrSector))
    return NULL;

  USES_CONVERSION;
  LPCSTR pszSector = OLE2CA(bstrSector);

  const ClusterListIGC* pClusters = GetIGC()->GetMission()->GetClusters();
  for (ClusterLinkIGC* it = pClusters->first(); it; it = it->next())
  {
    IclusterIGC* pCluster = it->data();
    if (0 == _stricmp(pCluster->GetName(), pszSector))
      return pCluster;
  }
  return NULL;
}


ImodelIGC* CPigShip::FindModel(BSTR bstrModel)
{
  if (!BSTRLen(bstrModel))
    return NULL;

  USES_CONVERSION;
  LPCSTR pszModel = OLE2CA(bstrModel);

  const ClusterListIGC* pClusters = GetIGC()->GetMission()->GetClusters();
  for (ClusterLinkIGC* itCluster = pClusters->first(); itCluster; itCluster = itCluster->next())
  {
    IclusterIGC* pCluster = itCluster->data();
    const ModelListIGC* pModels = pCluster->GetModels();
    for (ModelLinkIGC* it = pModels->first(); it; it = it->next())
    {
      ImodelIGC* pModel = it->data();
      if (0 == _stricmp(GetModelName(pModel), pszModel))
        return pModel;
    }
  }

  return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigShip::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigShip,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// IAGCShip Interface Methods


STDMETHODIMP CPigShip::put_WingID(short Val)
{
  // Validate the specified WingID
  if (0 > Val || Val > 9)
    return E_INVALIDARG;

  if (m_pPig->BaseClient::GetNetwork()->IsConnected())
  {
    m_pPig->BaseClient::SetMessageType(BaseClient::c_mtGuaranteed);
    BEGIN_PFM_CREATE(*m_pPig->BaseClient::GetNetwork(), pfm, CS, SET_WINGID)
    END_PFM_CREATE
    pfm->shipID = GetIGC()->GetObjectID();
    pfm->wingID = Val;
  }
  return S_OK;
}

STDMETHODIMP CPigShip::put_AutoDonate(IAGCShip* pShip)
{
  if (m_pPig->BaseClient::GetNetwork()->IsConnected())
  {
    AGCObjectID shipID;
    if (!pShip)
      shipID = NA;
    else
      RETURN_FAILED(pShip->get_ObjectID(&shipID))

    if (GetIGC() && GetIGC()->GetObjectID() == shipID)
      shipID = NA;

    m_pPig->BaseClient::SetMessageType(BaseClient::c_mtGuaranteed);
    BEGIN_PFM_CREATE(*m_pPig->BaseClient::GetNetwork(), pfm, C, AUTODONATE)
    END_PFM_CREATE
    pfm->sidDonateTo = shipID;
    pfm->amount = m_pPig->BaseClient::GetMoney();
    m_pPig->BaseClient::SetMoney(0);

    //Do not set the ship's autodonate ... happens on the echo from the server.
  }
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IPigShip Interface Methods

STDMETHODIMP CPigShip::SellAllParts()
{
  // Validate the current state
  if (PigState_Docked != m_pPig->GetCurrentState())
    return Error(IDS_E_BUYSELL_DOCKED, IID_IPigShip);

  // make a copy of the current ship with no parts.
  // create a new ship with that hull type
  IshipIGC*   pshipNew = m_pPig->BaseClient::CreateEmptyShip();
  pshipNew->SetBaseHullType(m_pPig->BaseClient::GetShip()->GetBaseHullType());

  // request the new ship
  m_pPig->BaseClient::BuyLoadout(pshipNew, false);

  // clean up the ship
  pshipNew->Terminate();
  pshipNew->Release();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::BuyHull(IAGCHullType* pHullType)
{
  // Validate the current state
  if (PigState_Docked != m_pPig->GetCurrentState())
    return Error(IDS_E_BUYSELL_DOCKED, IID_IPigShip);

  BSTR name;
  pHullType->get_Name(&name);

  BSTR model;
  pHullType->get_ModelName(&model);

  BSTR pigName;
  m_pPig->get_Name(&pigName);

  printf("Pig: %S requesting hull type: %S, model: %S\n", pigName, name, model);
  

  // Validate the specified pointer
  if (!pHullType)
    return E_POINTER;

  // TODO: Validate the specified pointer some more

  // Get the IGC pointer of the specified AGCHullType
  IAGCPrivatePtr spPrivate(pHullType);
  assert(spPrivate->GetIGCVoid());

  // Sell all parts
  RETURN_FAILED(SellAllParts());

  // Get the specified hull type's IGC pointer
  IhullTypeIGC* pIGC = reinterpret_cast<IhullTypeIGC*>(spPrivate->GetIGCVoid());

  // create a new ship with that hull type
  IshipIGC*   pshipNew = m_pPig->BaseClient::CreateEmptyShip();
  IstationIGC* pstation = m_pPig->BaseClient::GetShip()->GetStation();

  Money       budget = GetIGC()->GetValue() + m_pPig->BaseClient::GetMoney();
  assert (pshipNew);
  m_pPig->BaseClient::BuyDefaultLoadout(pshipNew, pstation, pIGC, &budget);

  IhullTypeIGC * baseHull = pshipNew->GetBaseHullType();
  IbuyableIGC * successor = pstation->GetSuccessor(pIGC);

  printf("baseHull: %s, successorBuyableType: %s\n", baseHull->GetName(), successor->GetName());

  // TODO: BT / YP Figure out why the starting ship can't be bought here. 
  // Don't need this as it turns out. The budget check is above in BuyDefaultLoadout.
  // This doesn't even work, if the pig tries to get a gunship at $200, this doesn't block that action. 
  /*if (pshipNew->GetBaseHullType() != pstation->GetSuccessor(pIGC))
    return Error(IDS_E_TOO_POOR, IID_IPigShip);*/

  // request the new ship
  m_pPig->BaseClient::BuyLoadout(pshipNew, false);

  // clean up the ship
  pshipNew->Terminate();
  pshipNew->Release();

  // Buy the default loadout for the new hull type
//  RETURN_FAILED(BuyDefaultLoadout()); // Already done (above) at the BaseClient level

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::BuyDefaultLoadout()
{
  // Validate the current state
  if (PigState_Docked != m_pPig->GetCurrentState())
    return Error(IDS_E_BUYSELL_DOCKED, IID_IPigShip);

  // Delegate to BaseClient method
  //m_pPig->BaseClient::BuyDefaultLoadout();

  // HACK: This will buy the default loadout asynchonusly; it should let pigs
  // work for the moment, and I'll let John figure out how to make this work 
  // in the long run.  
  {
    IshipIGC*   pshipNew = m_pPig->BaseClient::CreateEmptyShip();
    IshipIGC*   pship = m_pPig->BaseClient::GetShip();
    Money       budget = pship->GetValue() + m_pPig->BaseClient::GetMoney();

    assert (pshipNew);
    m_pPig->BaseClient::BuyDefaultLoadout(pshipNew, pship->GetStation(), 
        pship->GetBaseHullType(), &budget);

    //NYI disable loadout till we get an ack
    m_pPig->BaseClient::BuyLoadout(pshipNew, false);

    pshipNew->Terminate();
    pshipNew->Release();
  }


  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::CommitSuicide(BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Send the SUICIDE message if connected to the server
  if (m_pPig->BaseClient::GetNetwork()->IsConnected())
  {
    m_pPig->BaseClient::SetMessageType(BaseClient::c_mtGuaranteed);
    BEGIN_PFM_CREATE(*m_pPig->BaseClient::GetNetwork(), pfm, C, SUICIDE)
    END_PFM_CREATE
  }
  else
  {
    m_pPig->BaseClient::KillShipEvent(Time::Now(),
      GetIGC(), NULL, 1.0f,
      GetIGC()->GetPosition(), Vector::GetZero());
  }

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::AllStop(BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Stop the ship
  ControlData cd;
  cd.Reset();
  GetIGC()->SetControls(cd);
  GetIGC()->SetStateBits(c_nThrustmask, 0);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::Thrust(PigThrust e1, PigThrust e2, PigThrust e3,
  PigThrust e4, PigThrust e5, PigThrust e6, PigThrust e7, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Validate the specified thrust value
  RETURN_FAILED(ValidateThrustBit(e1));
  RETURN_FAILED(ValidateThrustBit(e2));
  RETURN_FAILED(ValidateThrustBit(e3));
  RETURN_FAILED(ValidateThrustBit(e4));
  RETURN_FAILED(ValidateThrustBit(e5));
  RETURN_FAILED(ValidateThrustBit(e6));
  RETURN_FAILED(ValidateThrustBit(e7));

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Interpret the specified thrust values
  int nBits = 0;
  nBits |= InterpretThrustBit(e1);
  nBits |= InterpretThrustBit(e2);
  nBits |= InterpretThrustBit(e3);
  nBits |= InterpretThrustBit(e4);
  nBits |= InterpretThrustBit(e5);
  nBits |= InterpretThrustBit(e6);
  nBits |= InterpretThrustBit(e7);

  // Set the ship's control state bits
  GetIGC()->SetStateBits(c_nThrustmask, nBits);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}

//imago 10/14
STDMETHODIMP CPigShip::Boost(VARIANT_BOOL bOn, BSTR* pbstrResponse)
{
	// Initialize the [out] parameters
	CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

	GetIGC()->SetWantBoost(bOn);

	// Set the ship's control state bits
	GetIGC()->SetStateBits(buttonsMaskIGC, (bOn) ? GetIGC()->GetStateM() | afterburnerButtonIGC : GetIGC()->GetStateM() & ~afterburnerButtonIGC);

	// Load the ship's method response string
	LoadShipResponse(pbstrResponse);

	// Indicate success
	return S_OK;
}


STDMETHODIMP CPigShip::FireWeapon(VARIANT_BOOL bBegin, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Determine which ship state bit to set/reset
  int nBit = (m_bAllWeapons ? allWeaponsIGC : oneWeaponIGC) & -!!(long)bBegin;

  // Set/reset the ship state bit
  GetIGC()->SetStateBits(weaponsMaskIGC, nBit);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::FireMissile(VARIANT_BOOL bBegin, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Determine the new value of the ship state bit
  int nBit = missileFireIGC & -!!(long)bBegin;

  // Set/reset the ship state bit
  GetIGC()->SetStateBits(missileFireIGC, nBit);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::DropMine(VARIANT_BOOL bBegin, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Determine the new value of the ship state bit
  int nBit = mineFireIGC & -!!(long)bBegin;

  // Set/reset the ship state bit
  GetIGC()->SetStateBits(mineFireIGC, nBit);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::LockVector(VARIANT_BOOL bLockVector, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Determine the new value of the ship state bit
  int nBit = coastButtonIGC & -!!(long)bLockVector;

  // Set/reset the ship state bit
  GetIGC()->SetStateBits(coastButtonIGC, nBit);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::RipCord(VARIANT* pvParam, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // pvParam could be an AGCSector or AGCModel object
  CComVariant var;
  IclusterIGC* pCluster = NULL;
  IAGCPrivatePtr spPrivate(V_OBJECT(pvParam));
  if (spPrivate != NULL)
  {
    IbaseIGC* pBase = reinterpret_cast<IbaseIGC*>(spPrivate->GetIGCVoid());
    ObjectType ot = pBase->GetObjectType();
    if (OT_cluster == ot)
      pCluster = reinterpret_cast<IclusterIGC*>(pBase);
    else if (OT_modelBegin <= ot && ot <= OT_modelEnd)
      pCluster = reinterpret_cast<ImodelIGC*>(pBase)->GetCluster();
    else
    {
      if (FAILED(VariantChangeType(&var, pvParam, 0, VT_BSTR)))
        return E_INVALIDARG;
      pvParam = &var;
    }
  }

  // pvParam could be the name of a sector or model
  if (VT_BSTR == V_VT(pvParam))
  {
    pCluster = FindCluster(V_BSTR(pvParam));
    if (!pCluster)
    {
      ImodelIGC* pModel = FindModel(V_BSTR(pvParam));
      if (pModel)
        pCluster = pModel->GetCluster();
    }
  }

  // Otherwise, it must be interpreted as a boolean
  bool bRipCord = !!pCluster;
  if (!bRipCord)
  {
    if (FAILED(VariantChangeType(&var, pvParam, 0, VT_BOOL)))
      return E_INVALIDARG;
    bRipCord = !!V_BOOL(&var);
  }

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  /* NYI -- fix with a correct call to RequestRipcord
  // Determine the new value of the ship state bit
  int nBit = ripcordActiveIGC & -!!(long)bRipCord;

  // Set/reset the ship state bit
  m_pPig->m_bstrTryingToRipcord.Empty();
  GetIGC()->SetStateBits(ripcordActiveIGC, nBit);
  if (BSTRLen(m_pPig->m_bstrTryingToRipcord))
  {
    GetIGC()->SetStateBits(ripcordActiveIGC, 0);
    if (pbstrResponse)
    {
      CLEAROUT(pbstrResponse, (BSTR)m_pPig->m_bstrTryingToRipcord);
      m_pPig->m_bstrTryingToRipcord.Detach();
    }
    return S_FALSE;
  }

  // Set the ripord cluster, if any
  if (pCluster)
    m_pPig->BaseClient::SetRipcordCluster(pCluster);
  */

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::Cloak(VARIANT_BOOL bCloak, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Determine the new value of the ship state bit
  int nBit = cloakActiveIGC & -!!(long)bCloak;

  // Set/reset the ship state bit
  GetIGC()->SetStateBits(cloakActiveIGC, nBit);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::AcceptCommand(BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Get the id and model of the queued command
  ImodelIGC* pmodel = GetIGC()->GetCommandTarget(c_cmdQueued);
  CommandID  idCmd  = GetIGC()->GetCommandID(c_cmdQueued);

  // Check for a queued command
  if (idCmd < 0)
    return Error(IDS_E_ACCEPT_COMMAND, IID_IPigShip);

  // Move the queued command to the accepted command
  GetIGC()->SetCommand(c_cmdAccepted, pmodel, idCmd);

  // Clear the queued command
  GetIGC()->SetCommand(c_cmdQueued, NULL, c_cidNone);

  // Activate the AutoPilot
  put_AutoPilot(VARIANT_TRUE);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::ClearCommand(BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Get the current accepted command, if any
  CommandID idCmd = GetIGC()->GetCommandID(c_cmdAccepted);

  // Clear the accepted command
  GetIGC()->SetCommand(c_cmdAccepted, NULL, c_cidNone);

  // Kill any current automatic action, including AutoPilot
  if (idCmd >= 0)
    KillAutoAction();

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::Face(VARIANT* pvarObject, BSTR bstrExpirationExpr,
  BSTR bstrInterruptionExpr, VARIANT_BOOL bLevel, BSTR* pbstrResponse)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(pbstrResponse, (BSTR)NULL);

  // Create a CPigFaceEvent object
  CComObject<CPigFaceEvent>* pAutoAction = NULL;
  RETURN_FAILED(pAutoAction->CreateInstance(&pAutoAction));
  RETURN_FAILED(pAutoAction->Init(this, pvarObject, !!bLevel));
  IPigShipEventPtr spAutoAction(pAutoAction);

  // Set the properties of the CPigFaceEvent object
  RETURN_FAILED(spAutoAction->put_ExpirationExpression(bstrExpirationExpr));
  RETURN_FAILED(spAutoAction->put_InterruptionExpression(bstrInterruptionExpr));

  // Replace any current automatic action, including AutoPilot
  ReplaceAutoAction(spAutoAction);

  // Load the ship's method response string
  LoadShipResponse(pbstrResponse, IDS_SHIP_FACE_RESPONSE);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::Defend(BSTR bstrObject, BSTR* pbstrResponse)
{
  // Resolve the specified target name


  ImodelIGC* pTarget = CPigShipEvent::FindTargetName(m_pPig, bstrObject);
  if (!pTarget)
    return FormattedError(IDS_E_FMT_OBJNAME, bstrObject ? bstrObject : L"");

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

	m_pPig->BaseClient::SetAutoPilot(true); //imago 10/14

  // Set the queued command
  GetIGC()->SetCommand(c_cmdPlan, pTarget, c_cidDefend);

  return S_OK;
}

STDMETHODIMP CPigShip::AttackShip(IAGCShip* pTargetShip, BSTR* pbstrResponse)
{
	IAGCModel* pModel = (IAGCModel*)pTargetShip;
	
	// Validate the specified pointer
	if (!pModel)
		return E_POINTER;

	// Get the IGC pointer of the specified AGCHullType
	IAGCPrivatePtr spPrivate(pModel);
	assert(spPrivate->GetIGCVoid());

	// Get the specified hull type's IGC pointer
	ImodelIGC* pTarget = reinterpret_cast<ImodelIGC*>(spPrivate->GetIGCVoid());
	// Kill any current automatic action, including AutoPilot
	KillAutoAction();

	// Set the target so that missile lock can be aquired.
	GetIGC()->SetCommand(c_cmdCurrent, pTarget, c_cidDefault);

	m_pPig->BaseClient::SetAutoPilot(true);

	// Set the Plan command 
	// This was queueing a command, but the queued command didn't end up as a Plan,
	// And it seems that only Plan commands are evaluated by autopilot.
	GetIGC()->SetCommand(c_cmdPlan, pTarget, c_cidAttack);

	return S_OK;
}

STDMETHODIMP CPigShip::AttackStation(IAGCStation* pTargetStation, BSTR* pbstrResponse)
{
	IAGCModel* pModel = (IAGCModel*)pTargetStation;

	// Validate the specified pointer
	if (!pModel)
		return E_POINTER;

	// Get the IGC pointer of the specified AGCHullType
	IAGCPrivatePtr spPrivate(pModel);
	assert(spPrivate->GetIGCVoid());

	// Get the specified hull type's IGC pointer
	ImodelIGC* pTarget = reinterpret_cast<ImodelIGC*>(spPrivate->GetIGCVoid());
	// Kill any current automatic action, including AutoPilot
	KillAutoAction();

	// Set the target so that missile lock can be aquired.
	GetIGC()->SetCommand(c_cmdCurrent, pTarget, c_cidDefault);

	m_pPig->BaseClient::SetAutoPilot(true);

	// Set the Plan command 
	// This was queueing a command, but the queued command didn't end up as a Plan,
	// And it seems that only Plan commands are evaluated by autopilot.
	GetIGC()->SetCommand(c_cmdPlan, pTarget, c_cidAttack);

	// Accept the queued command
	return S_OK;
}

//imago 10/14
STDMETHODIMP CPigShip::Goto(BSTR bstrObject, VARIANT_BOOL bFriendly, BSTR* pbstrResponse)
{
	// Resolve the specified target name
	ImodelIGC* pTarget = CPigShipEvent::FindTargetName(m_pPig, bstrObject, bFriendly);
	if (!pTarget)
		return FormattedError(IDS_E_FMT_OBJNAME, bstrObject ? bstrObject : L"");

	// Kill any current automatic action, including AutoPilot
	KillAutoAction();

	m_pPig->BaseClient::SetAutoPilot(true);

	// Set the queued command
	GetIGC()->SetCommand(c_cmdPlan, pTarget, c_cidGoto);

	return S_OK;
}

//imago 10/14
STDMETHODIMP CPigShip::GotoStationID(ObjectID oid, BSTR* pbstrResponse)
{
	// Resolve the specified object ID
	ImodelIGC* pTarget = GetIGC()->GetMission()->GetStation(oid);
	if (!pTarget)
		return FormattedError(IDS_E_FMT_STATIONOBJID, oid ? oid : -1);

	// Kill any current automatic action, including AutoPilot
	KillAutoAction();

	m_pPig->BaseClient::SetAutoPilot(true);

	// Set the queued command
	GetIGC()->SetCommand(c_cmdPlan, pTarget, c_cidGoto);

	return S_OK;
}


STDMETHODIMP CPigShip::put_AutoPilot(VARIANT_BOOL bEngage)
{
  // Stop everything
  AllStop(NULL);

  // Set the AutoPilot flag
  m_pPig->BaseClient::SetAutoPilot(!!bEngage);

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::get_AutoPilot(VARIANT_BOOL* pbEngaged)
{
  CLEAROUT(pbEngaged, VARBOOL(m_pPig->autoPilot()));
  return S_OK;
}


STDMETHODIMP CPigShip::get_AutoAction(IPigShipEvent** ppAutoAction)
{
  // Initialize the [out] parameter
  CLEAROUT(ppAutoAction, (IPigShipEvent*)NULL);

  // Copy the action event object to the [out] parameter
  if (NULL != m_spAutoAction)
    (*ppAutoAction = m_spAutoAction)->AddRef();

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::get_IsThrusting(PigThrust eThrust,
  VARIANT_BOOL* pbThrusting)
{
  // Validate the specified thrust value
  RETURN_FAILED(ValidateThrustBit(eThrust));

  // Get the ship's control state bits
  int nBits = GetIGC()->GetStateM();

  // If Thrust_None was specified, all bits must be off
  bool bThrusting = (ThrustNone != eThrust) ?
    !!(nBits & InterpretThrustBit(eThrust)) : !(nBits & c_nThrustmask);

  // Copy the result to the [out] parameter
  CLEAROUT(pbThrusting, VARBOOL(bThrusting));

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::put_ActiveWeapon(long iWeapon)
{
  // Validate the specified weapon index
  long cWeaponsMax = GetIGC()->GetHullType()->GetMaxWeapons();
  if (!cWeaponsMax)
    return Error(IDS_E_NO_WEAPONS, IID_IPigShip);
  if (-1 > iWeapon || iWeapon >= cWeaponsMax)
    return FormattedError(IDS_E_FMT_INVALID_WEAPON_INDEX, iWeapon, cWeaponsMax - 1);
  if (!GetIGC()->GetMountedPart(ET_Weapon, (Mount)iWeapon))
    return FormattedError(IDS_E_FMT_NO_WEAPON_AT_INDEX, iWeapon);

  // Save the specified weapon index
  XLock lock(this);
  if (!(m_bAllWeapons = (-1 == iWeapon)))
    GetIGC()->SetStateBits(selectedWeaponMaskIGC,
      iWeapon << selectedWeaponShiftIGC);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_ActiveWeapon(long* piWeapon)
{
  // Get the active weapon
  XLock lock(this);
  CLEAROUT(piWeapon, m_bAllWeapons ? -1L :
    (long)(GetIGC()->GetStateM() & selectedWeaponMaskIGC)
    >> selectedWeaponShiftIGC);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_IsFiringWeapon(VARIANT_BOOL* pbFiring)
{
  // Determine which ship state bit to test
  XLock lock(this);
  int nBit = m_bAllWeapons ? allWeaponsIGC : oneWeaponIGC;

  // Test the ship state bit
  CLEAROUT(pbFiring, VARBOOL(GetIGC()->GetStateM() & nBit));

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_IsFiringMissile(VARIANT_BOOL* pbFiring)
{
  // Test the ship state bit
  XLock lock(this);
  CLEAROUT(pbFiring, VARBOOL(GetIGC()->GetStateM() & missileFireIGC));

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_MissileRange(float* pfRange)
{
	// Test the ship state bit
	XLock lock(this);
	float maxRange = 0;
	CLEAROUT(pfRange, maxRange);

	ImagazineIGC* pmagazine = (ImagazineIGC*)m_pPig->BaseClient::GetShip()->GetMountedPart(ET_Magazine, 0);
	if (pmagazine) {
		//bool isLocked = (pmagazine && pmagazine->GetLock() == 1.0f);
		ImissileTypeIGC* missileType = pmagazine->GetMissileType();
		if (missileType) {
			float lifespan = missileType->GetLifespan();
			maxRange = missileType->GetInitialSpeed() * lifespan + 0.5f * missileType->GetAcceleration() * lifespan * lifespan;
			CLEAROUT(pfRange, maxRange);
		}
	}

	return S_OK;
}

STDMETHODIMP CPigShip::get_MissileLock(float* pfLock)
{
	// Test the ship state bit
	XLock lock(this);
	float missilelock = 0;
	CLEAROUT(pfLock, missilelock);

	ImagazineIGC* pmagazine = (ImagazineIGC*)m_pPig->BaseClient::GetShip()->GetMountedPart(ET_Magazine, 0);
	if (pmagazine) {
		missilelock = pmagazine->GetLock();
		CLEAROUT(pfLock, missilelock);
	}

	return S_OK;
}

STDMETHODIMP CPigShip::get_IsDroppingMine(VARIANT_BOOL* pbDropping)
{
  // Test the ship state bit
  XLock lock(this);
  CLEAROUT(pbDropping, VARBOOL(GetIGC()->GetStateM() & mineFireIGC));

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::get_IsVectorLocked(VARIANT_BOOL* pbVectorLocked)
{
  // Get the vector lock bit
  XLock lock(this);
  CLEAROUT(pbVectorLocked,
    VARBOOL(GetIGC()->GetStateM() & coastButtonIGC));

  // Indicate success
  return S_OK;
}


STDMETHODIMP CPigShip::get_IsRipCording(VARIANT_BOOL* pbRipCording)
{
  // Get the rip cording bit
  XLock lock(this);
  CLEAROUT(pbRipCording,
	  VARBOOL(GetIGC()->fRipcordActive()));//KGJV TODO added (), check if ok

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_IsCloaked(VARIANT_BOOL* pbCloaked)
{
  // Get the rip cording bit
  XLock lock(this);
  CLEAROUT(pbCloaked,
    VARBOOL(GetIGC()->GetStateM() & cloakActiveIGC));

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::put_Throttle(float fThrottle)
{
  // Validate the specified throttle rate
  if (0.f > fThrottle || fThrottle > 100.f)
    return Error(IDS_E_THROTTLE_RANGE, IID_IPigShip, E_INVALIDARG);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Change the throttle value
  SetAxis(c_axisThrottle, (fThrottle / 50.f) - 1.f);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_Throttle(float* pfThrottle)
{
  CLEAROUT(pfThrottle, (GetAxis(c_axisThrottle) + 1.f) * 50.f);
  return S_OK;
}

STDMETHODIMP CPigShip::put_Pitch(float fPitch)
{
  // Validate the specified throttle rate
  if (-100.f > fPitch || fPitch > 100.f)
    return Error(IDS_E_PITCH_RANGE, IID_IPigShip, E_INVALIDARG);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Change the pitch value
  SetAxis(c_axisPitch, fPitch / 100.f);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_Pitch(float* pfPitch)
{
  CLEAROUT(pfPitch, GetAxis(c_axisPitch) * 100.f);
  return S_OK;
}

STDMETHODIMP CPigShip::put_Yaw(float fYaw)
{
  // Validate the specified throttle rate
  if (-100.f > fYaw || fYaw > 100.f)
    return Error(IDS_E_YAW_RANGE, IID_IPigShip, E_INVALIDARG);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Change the yaw value
  SetAxis(c_axisYaw, fYaw / 100.f);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_Yaw(float* pfYaw)
{
  CLEAROUT(pfYaw, GetAxis(c_axisYaw) * 100.f);
  return S_OK;
}

STDMETHODIMP CPigShip::put_Roll(float fRoll)
{
  // Validate the specified throttle rate
  if (-100.f > fRoll || fRoll > 100.f)
    return Error(IDS_E_ROLL_RANGE, IID_IPigShip, E_INVALIDARG);

  // Kill any current automatic action, including AutoPilot
  KillAutoAction();

  // Change the roll value
  SetAxis(c_axisRoll, fRoll / 100.f);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigShip::get_Roll(float* pfRoll)
{
  CLEAROUT(pfRoll, GetAxis(c_axisRoll) * 100.f);
  return S_OK;
}


STDMETHODIMP CPigShip::get_QueuedCommand(IAGCCommand** ppCommand)
{
  return CreateCommandWrapper(c_cmdQueued, ppCommand);
}


STDMETHODIMP CPigShip::get_AcceptedCommand(IAGCCommand** ppCommand)
{
  return CreateCommandWrapper(c_cmdAccepted, ppCommand);
}


STDMETHODIMP CPigShip::put_AutoAcceptCommands(VARIANT_BOOL bAutoAccept)
{
  m_bAutoAcceptCommands = !!bAutoAccept;
  return S_OK;
}


STDMETHODIMP CPigShip::get_AutoAcceptCommands(VARIANT_BOOL* pbAutoAccept)
{
  CLEAROUT(pbAutoAccept, VARBOOL(m_bAutoAcceptCommands));
  return S_OK;
}


STDMETHODIMP CPigShip::get_RipCordTimeLeft(float* pfRipCordTimeLeft)
{
  CLEAROUT(pfRipCordTimeLeft, GetIGC()->GetRipcordTimeLeft());
  return S_OK;
}

