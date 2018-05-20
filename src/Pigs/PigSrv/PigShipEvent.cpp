/////////////////////////////////////////////////////////////////////////////
// PigShipEvent.cpp : Implementation of the CPigShipEvent class.

#include "pch.h"
#include <TCLib.h>
#include <..\TCAtl\ObjectMap.h>
#include "Pig.h"
#include "PigBehaviorScript.h"
#include "PigShip.h"
#include "PigShipEvent.h"


/////////////////////////////////////////////////////////////////////////////
// CPigShipEvent

TC_OBJECT_EXTERN_NON_CREATEABLE_IMPL(CPigShipEvent)


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

HRESULT CPigShipEvent::Init(CPigShip* pPigShip, BSTR bstrTarget)
{
  XLock lock(this);

  // Save the specified pig ship pointer
  m_pPigShip = pPigShip;

  // Perform default processing
  CPigEvent::Init(m_pPigShip->GetPig()->GetActiveBehavior()->GetMostDerived());

  // Set the specified target
  RETURN_FAILED(SetTarget(bstrTarget));

  // Generate an event name
  CComBSTR bstrFmt;
  _VERIFYE(bstrFmt.LoadString(IDS_FMT_SHIPEVENT_NAME));
  OLECHAR szName[_MAX_PATH];
  swprintf(szName, bstrFmt, m_pOwner->GetNextEventNumber());
  RETURN_FAILED(put_Name(CComBSTR(szName)));

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Attributes

HRESULT CPigShipEvent::SetTarget(BSTR bstrTarget)
{
  // Resolve the specified object name into a model in the current cluster
  XLock lock(this);
  m_pTarget = FindTargetName(m_pPigShip->GetPig(), bstrTarget);
  if (m_pTarget)
    return S_OK;

  // Format the error message
  return FormattedError(IDS_E_FMT_OBJNAME, bstrTarget ? bstrTarget : L"");
}


/////////////////////////////////////////////////////////////////////////////
// Operations

ImodelIGC* CPigShipEvent::FindTargetName(CPig* pPig, BSTR bstrTarget, bool bFriendsOnly)
{
  ImodelIGC* pTarget = NULL;
  if (pPig && BSTRLen(bstrTarget))
  {
    // Only look in the current cluster
    IclusterIGC* pCluster = pPig->BaseClient::GetCluster();
    if (pCluster)
    {
      // Convert the BSTR to ANSI
      USES_CONVERSION;
      LPCSTR pszTarget = OLE2CA(bstrTarget);

      // Get the pig ship's position
      Vector vPig = pPig->GetShip()->GetPosition();

      // Find the nearest model with the specified name
      float               fNearest = 0.f;
      const ModelListIGC* pModels = pCluster->GetModels();
      for (ModelLinkIGC* it = pModels->first(); it; it = it->next())
      {
        ImodelIGC* pModel = it->data();
				if (pModel) { //imago 10/14 (model can disappear here)
        if (0 == _stricmp(GetModelName(pModel), pszTarget))
        {
						if (bFriendsOnly && pModel->GetSide() != pPig->BaseClient::GetSide()) {
							//Imago skip! 10/14
						} else {
          float fDistance = (pModel->GetPosition() - vPig).LengthSquared();
          if (!pTarget || fDistance < fNearest)
          {
            fNearest = fDistance;
            pTarget = pModel;
          }
        }
      }
    }
  }
		}
	}
  return pTarget;
}


/////////////////////////////////////////////////////////////////////////////
// Overrides

bool CPigShipEvent::Pulse(bool bExpired)
{
  // Have the ship kill us when expired
  if (bExpired)
    m_pPigShip->KillAutoAction();

  // Always terminate the event when it expires
  return bExpired;
}

bool CPigShipEvent::ModelTerminated(ImodelIGC* pModelIGC)
{
  // Ignore if the specified model is not our action target
  if (m_pTarget != pModelIGC)
    return false;

  // Reset the model pointer
  m_pTarget = NULL;

  // Indicate that we should be terminated
  return true;
}


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigShipEvent::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigShipEvent,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// IPigShipEvent Interface Methods

STDMETHODIMP CPigShipEvent::get_Target(BSTR* pbstrTarget)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrTarget, (BSTR)NULL);

  // Copy the target name, if any
  XLock lock(this);
  if (m_pTarget)
    *pbstrTarget = CComBSTR(m_pTarget->GetName()).Detach();

  // Indicate success
  return S_OK;
}

//
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CPigFaceEvent
//


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

HRESULT CPigFaceEvent::Init(CPigShip* pPigShip, VARIANT* pvarTarget,
  bool bMatchUpVector)
{
  XLock lock(this);

  // Save the specified pig ship pointer
  m_pPigShip = pPigShip;

  // Perform default processing
  CPigEvent::Init(m_pPigShip->GetPig()->GetActiveBehavior()->GetMostDerived());

  // Determine which type of VARIANT was specified
  if (!pvarTarget)
    return E_POINTER;
  VARTYPE vt = V_VT(pvarTarget);
  switch (vt)
  {
    case VT_BSTR:
    {
      // Set the specified target as a named model
      RETURN_FAILED(SetTarget(V_BSTR(pvarTarget)));
      break;
    }
    case VT_DISPATCH:
    case VT_UNKNOWN:
    {
      // See if it's an IAGCModel
      IAGCModelPtr spModel(V_UNKNOWN(pvarTarget));
      if (NULL != spModel)
      {
        IAGCPrivatePtr spPrivate(spModel);
        if (NULL == spPrivate)
          return E_INVALIDARG;
        m_pTarget = reinterpret_cast<ImodelIGC*>(spPrivate->GetIGCVoid());
      }
      else
      {
        // See if it's an IAGCVector
        IAGCVectorPtr spVector(V_UNKNOWN(pvarTarget));
        IAGCVectorPrivatePtr spPrivate(spVector);
        if (NULL == spPrivate)
          return E_INVALIDARG;
        RETURN_FAILED(spPrivate->CopyVectorTo(&m_vector));
        m_bVector = true;
      }
      break;
    }
    default:
      return DISP_E_TYPEMISMATCH;
  }

  // Get the target's radius, if it's a model
  if (m_pTarget)
    m_fTargetRadius = m_pTarget->GetRadius() * 0.25f;

  // Generate an event name
  CComBSTR bstrFmt;
  _VERIFYE(bstrFmt.LoadString(IDS_FMT_SHIPEVENT_NAME));
  OLECHAR szName[_MAX_PATH];
  swprintf(szName, bstrFmt, m_pOwner->GetNextEventNumber());
  RETURN_FAILED(put_Name(CComBSTR(szName)));

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Overrides

bool CPigFaceEvent::IsExpired()
{
  // Get the ship object
  assert(m_pPigShip->GetPig());
  IshipIGC* pShip = m_pPigShip->GetPig()->BaseClient::GetShip();
  assert(pShip);

  // We are definitely expired if we are not flying
  if (PigState_Flying != m_pPigShip->GetPig()->GetCurrentState())
    return true;

  // We are definitely expired if we have no target
  if (!m_pTarget && !m_bVector)
    return true;

  // Get the ship's current controls
  ControlData cd = pShip->GetControls();

  // Determine the vector to face
  Vector vectorFace(m_bVector ? m_vector : m_pTarget->GetPosition());

  // Compute the ship's new controls
  Vector vectorDelta(vectorFace - pShip->GetPosition());
  float deltaAngle = turnToFace(vectorDelta,
    m_pPigShip->GetPig()->GetUpdateFraction(), pShip, &cd);

  // Set the ship's new controls
  pShip->SetControls(cd);

  // We expire when we are (close to) facing our target
//  _TRACE1("CPigFaceEvent::IsExpired(): deltaAngle = %f\n", deltaAngle);
  const static float fMarginStatic = fabs(RadiansFromDegrees(1.f));
  float fMargin;
  if (m_pTarget)
  {
    fMargin = fabs(m_fTargetRadius / vectorDelta.Length());
//    _TRACE2("CPigFaceEvent::IsExpired(): fabs(deltaAngle) = %f, fMargin = %f\n",
//      fabs(deltaAngle), fMargin);
  }
  else
  {
    fMargin = fMarginStatic;
  }
  return fabs(deltaAngle) <= fMargin;
}

void CPigFaceEvent::DoDefaultExpiration()
{
  m_pPigShip->AllStop(NULL);
}


/////////////////////////////////////////////////////////////////////////////
// IPigShipEvent Interface Methods

STDMETHODIMP CPigFaceEvent::get_Target(BSTR* pbstrTarget)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrTarget, (BSTR)NULL);

  // Copy the target name, if any
  XLock lock(this);
  if (m_pTarget)
    *pbstrTarget = CComBSTR(m_pTarget->GetName()).Detach();
  else if (m_bVector)
    *pbstrTarget = CComBSTR(m_vector.GetString()).Detach();

  // Indicate success
  return S_OK;
}

