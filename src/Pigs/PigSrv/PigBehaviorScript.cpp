/////////////////////////////////////////////////////////////////////////////
// PigBehaviorScript.cpp : Implementation of the CPigBehaviorScript class.

#include "pch.h"
#include <TCLib.h>
#include <..\TCLib\ExcepInfo.h>
#include <..\TCAtl\ObjectMap.h>

#include "PigTimer.h"
#include "Pig.h"
#include "PigBehaviorType.h"
#include "PigBehaviorScript.h"
#include "PigBehaviorScriptType.h"


/////////////////////////////////////////////////////////////////////////////
// CPigBehaviorScript

TC_OBJECT_EXTERN_NON_CREATEABLE_IMPL(CPigBehaviorScript)


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPigBehaviorScript::CPigBehaviorScript() :
  m_pType(NULL),
  m_pPig(NULL),
  m_dwGITCookie(0),
  m_pTimerCurrent(NULL),
  m_pBehaviorBase(NULL),
  m_pBehaviorDerived(NULL),
  m_spAs(NULL),
  m_spAse(NULL),
  m_spAsp(NULL)
{
}

HRESULT CPigBehaviorScript::Init(CPigBehaviorScriptType* pType, CPig* pPig,
  BSTR bstrCommandLine, CPigBehaviorScript* pDerived)
{
  // Save the specified type and pig references
  assert(!m_pType);
  assert(!m_pPig);
  m_pType            = pType;
  m_pPig             = pPig;
  m_bstrCommandLine  = bstrCommandLine;
  m_pBehaviorDerived = pDerived;

  // Get the script's filename from the type
  IActiveScriptSiteImplBase::SetScriptFileName(m_pType->GetScriptFileName());

  // Create the base behavior, if any
  tstring strBaseBehavior(m_pType->GetBaseBehaviorName());
  if (!strBaseBehavior.empty())
  {
    // Get the base behavior type
    CPigBehaviorScriptType* pType =
      GetEngine().GetBehaviorType(CComBSTR(strBaseBehavior.c_str()));
    assert(pType);

    // Create an instance of a behavior object
    CComObject<CPigBehaviorScript>* pBehavior = NULL;
    RETURN_FAILED(pBehavior->CreateInstance(&pBehavior));
    IPigBehaviorPtr spBehavior(pBehavior->GetUnknown());
    assert(NULL != spBehavior);

    // Initialize the behavior
    RETURN_FAILED(pBehavior->Init(pType, m_pPig, NULL, this));

    // Save the base behavior object native and interface pointers
    m_pBehaviorBase = pBehavior;
    m_spBehaviorBase = spBehavior;
  }

  // Create the scripting engine
  RETURN_FAILED(CreateScriptingEngine());
  assert(NULL != m_spAsp);

  // Populate the scripting engine with script text
  long cStrings;
  ITCStringsPtr spScriptStrings;
  RETURN_FAILED(m_pType->get_ScriptText(&spScriptStrings));
  RETURN_FAILED(spScriptStrings->get_Count(&cStrings));
  for (_variant_t i(0L); V_I4(&i) < cStrings; ++V_I4(&i))
  {
    // Get the script text string
    CComBSTR bstrScript;
    RETURN_FAILED(spScriptStrings->get_Item(&i, &bstrScript));

    // Add the script text string to the scripting engine
    TCEXCEPINFO ei;
    HRESULT hr = m_spAsp->ParseScriptText(bstrScript, NULL, NULL, NULL, 0,
      0, SCRIPTTEXT_ISVISIBLE, NULL, &ei);
    if (FAILED(hr))
    {
      _SVERIFYE(ei.SetErrorInfo());
      return hr;
    }
  }

  // Begin executing the script
  RETURN_FAILED(m_spAs->SetScriptState(SCRIPTSTATE_CONNECTED));

  // Resolve the interface methods to script method DISPID's
  RETURN_FAILED(m_Methods.ResolveMethods(GetObjectCLSID(), m_spdScript,
    m_pType));

  // Add ourself to the Global Interface Table (GIT)
  assert(!m_dwGITCookie);
  RETURN_FAILED(GetEngine().RegisterInterfaceInGlobal(GetUnknown(),
    IID_IUnknown, &m_dwGITCookie));

  // Add our GIT reference to our type object
  m_pType->AddBehavior(m_dwGITCookie);

  // AddRef our type object
  m_pType->AddRef();

  // Indicate success
  return S_OK;
}

void CPigBehaviorScript::Term()
{
  XLock lock(this);

  // Release all timer (and event) objects
  CPigEventOwnerImplBase::RemoveEvents();

  // Shutdown the scripting engine
  ShutdownScriptingEngine();

  // Disconnect external references
  CoDisconnectObject(GetUnknown(), 0);

  // Terminate our base behavior, if any
  if (m_pBehaviorBase)
    m_pBehaviorBase->Term();

  // Remove our GIT reference from our type object
  assert(m_pType);
  m_pType->RemoveBehavior(m_dwGITCookie);

  // Release our type object
  m_pType->Release();

  // Remove ourself from the Global Interface Table (GIT)
  assert(m_dwGITCookie);
  _SVERIFYE(GetEngine().RevokeInterfaceFromGlobal(m_dwGITCookie));
  m_dwGITCookie = 0;

  // Detach our pig and type object references
  m_pPig = NULL;
  m_pType = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Operations

HRESULT CPigBehaviorScript::Eval(const char* pszExpression, VARIANT* pvResult)
{
  // Do nothing with an empty string
  assert(pvResult);
  if (!pszExpression || '\0' == *pszExpression)
    return S_FALSE;

  // Keep a reference on ourself while evaluating the script text
  IPigBehaviorPtr spBehavior(GetUnknown());

  // Evaluate the event expression
  TCEXCEPINFO ei;
  USES_CONVERSION;
  HRESULT hr = m_spAsp->ParseScriptText(A2COLE(pszExpression), NULL, NULL,
    NULL, 0, 0, SCRIPTTEXT_ISEXPRESSION, pvResult, &ei);
  if (FAILED(hr))
  {
    // Get the last script error
    ei.Release();
    _SVERIFYE(m_spAse->GetExceptionInfo(&ei));
    return ei.SetErrorInfo(), hr;
  }

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Overrides

void CPigBehaviorScript::OnExpiration(CPigEvent* pEvent)
{
  // Do nothing for non-active behaviors
  VARIANT_BOOL bActive;
  ZSucceeded(get_IsActive(&bActive));
  if (!bActive)
    return;

  // Set the current timer, if this event is a CPigTimer event type
  SetCurrentTimer(CPigTimer::DynamicCast(pEvent));

  // Get the completion expression
  CComBSTR bstrExpr;
  _SVERIFYE(pEvent->get_ExpirationExpression(&bstrExpr));
  if (bstrExpr.Length() && NULL != m_spAsp)
  {
    // Save the current script file name
    tstring strSave = IActiveScriptSiteImplBase::GetScriptFileName();
    IActiveScriptSiteImplBase::SetScriptFileName(tstring("(Internal Script Text)"));

    // Evaluate the completion expression
    TCEXCEPINFO ei;
    CComVariant varResult;
    HRESULT hr = m_spAsp->ParseScriptText(bstrExpr, NULL, NULL, NULL, 0, 0,
      /*SCRIPTTEXT_ISEXPRESSION*/0, &varResult, &ei);

    // Restore the script file name
    IActiveScriptSiteImplBase::SetScriptFileName(strSave);
  }
  else
  {
    pEvent->DoDefaultExpiration();
  }

  // Reset the current timer, if any
  SetCurrentTimer(NULL);
}


void CPigBehaviorScript::OnInterruption(CPigEvent* pEvent)
{
  // Do nothing for non-active behaviors
  VARIANT_BOOL bActive;
  ZSucceeded(get_IsActive(&bActive));
  if (!bActive)
    return;

  // Set the current timer, if this event is a CPigTimer event type
  SetCurrentTimer(CPigTimer::DynamicCast(pEvent));

  // Get the interruption expression
  CComBSTR bstrExpr;
  _SVERIFYE(pEvent->get_InterruptionExpression(&bstrExpr));

  // Process as a completion, if interruption expression is NULL
  if (!(BSTR)bstrExpr)
  {
    OnExpiration(pEvent);
    return;
  }
  if (bstrExpr.Length() && NULL != m_spAsp)
  {
    // Save the current script file name
    tstring strSave = IActiveScriptSiteImplBase::GetScriptFileName();
    IActiveScriptSiteImplBase::SetScriptFileName(tstring("(Internal Script Text)"));

    // Evaluate the completion expression
    TCEXCEPINFO ei;
    CComVariant varResult;
    HRESULT hr = m_spAsp->ParseScriptText(bstrExpr, NULL, NULL, NULL, 0, 0,
      /*SCRIPTTEXT_ISEXPRESSION*/0, &varResult, &ei);

    // Restore the script file name
    IActiveScriptSiteImplBase::SetScriptFileName(strSave);
  }

  // Reset the current timer, if any
  SetCurrentTimer(NULL);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

HRESULT CPigBehaviorScript::CreateScriptingEngine()
{
  // Get the ProgID of the Active Scripting language
  CComBSTR bstrProgID;
  RETURN_FAILED(m_pType->get_ScriptEngineProgID(&bstrProgID));

  // Create the scripting engine
  assert(NULL == m_spAs && NULL == m_spAsp);
  //RETURN_FAILED(m_spAsp.CreateInstance(bstrProgID));
  GUID appid;
  CLSIDFromProgID(bstrProgID,&appid);
  RETURN_FAILED(CoCreateInstance(appid, 0, CLSCTX_ALL, 
                 IID_IActiveScript, 
                 (void **)&m_spAs));
  //m_spAs = m_spAsp;
  m_spAs->QueryInterface(IID_IActiveScriptParse, (void **)&m_spAsp);
  assert(NULL != m_spAsp);

  // Set the script site
  RETURN_FAILED(m_spAs->SetScriptSite(this));

  // Initialize the script parse
  RETURN_FAILED(m_spAsp->InitNew());

  // Add our named objects
  RETURN_FAILED(m_spAs->AddNamedItem(L"This",
    SCRIPTITEM_ISVISIBLE /* | SCRIPTITEM_ISSOURCE*/ | SCRIPTITEM_GLOBALMEMBERS));
  RETURN_FAILED(m_spAs->AddNamedItem(L"Pig",
    SCRIPTITEM_ISVISIBLE /* | SCRIPTITEM_ISSOURCE*/ | SCRIPTITEM_GLOBALMEMBERS));
  RETURN_FAILED(m_spAs->AddNamedItem(L"Ship",
    SCRIPTITEM_ISVISIBLE /* | SCRIPTITEM_ISSOURCE*/ | SCRIPTITEM_GLOBALMEMBERS));
  RETURN_FAILED(m_spAs->AddNamedItem(L"Host",
    SCRIPTITEM_ISVISIBLE /* | SCRIPTITEM_ISSOURCE*/ | SCRIPTITEM_GLOBALMEMBERS));

  // The BaseBehavior named item MUST be added last. Otherwise, it seems to mask
  // the other named items.
  if (m_pBehaviorBase)
  {
    RETURN_FAILED(m_spAs->AddNamedItem(L"BaseBehavior",
      SCRIPTITEM_ISVISIBLE /* | SCRIPTITEM_ISSOURCE*/ | SCRIPTITEM_GLOBALMEMBERS));
  }

  // Add our TypeLib
  RETURN_FAILED(m_spAs->AddTypeLib(LIBID_PigsLib, 1, 0, 0));

  // Get the scripting engine's IDispatch
  assert(NULL == m_spdScript);
  assert(NULL != m_spAs);
  RETURN_FAILED(m_spAs->GetScriptDispatch(NULL, &m_spdScript));

  // Indicate success
  return S_OK;
}

void CPigBehaviorScript::ShutdownScriptingEngine()
{
  // Shutdown the scripting engine
  if (NULL != m_spdScript)
    m_spdScript = NULL;
  if (NULL != m_spAsp)
    m_spAsp = NULL;
  if (NULL != m_spAs)
  {
    m_spAs->Close();
    m_spAs = NULL;
  }
}

void CPigBehaviorScript::SetCurrentTimer(CPigTimer* pTimer)
{
  XLock lock(this);
  m_pTimerCurrent = pTimer;
}


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigBehaviorScript::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigBehavior,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// IPigBehavior Interface Methods

// Infrastructure Properties
STDMETHODIMP CPigBehaviorScript::get_BehaviorType(IPigBehaviorType** ppType)
{
  // Initialize the [out] parameters
  CLEAROUT(ppType, (IPigBehaviorType*)NULL);

  // Copy the behavior type object reference
  assert(m_pType);
  if (m_pType)
    (*ppType = m_pType)->AddRef();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigBehaviorScript::get_BaseBehavior(IDispatch** ppBaseBehavior)
{
  // Initialize the [out] parameter
  CLEAROUT(ppBaseBehavior, (IDispatch*)NULL);

  // Do nothing if we don't have a base behavior
  if (!m_pBehaviorBase)
    return S_OK;

  // Get the base behavior's script dispatch
  *ppBaseBehavior = m_pBehaviorBase->GetScriptDispatch().Detach();
  return S_OK;
}

STDMETHODIMP CPigBehaviorScript::get_Pig(IPig** ppPig)
{
  CLEAROUT(ppPig, (IPig*)NULL);
  return m_pPig->GetUnknown()->QueryInterface(IID_IPig, (void**)ppPig);
}

STDMETHODIMP CPigBehaviorScript::get_Host(IPigBehaviorHost** ppHost)
{
  // Delegate to the pig
  return m_pPig->get_Host(ppHost);
}

STDMETHODIMP CPigBehaviorScript::get_IsActive(VARIANT_BOOL* pbIsActive)
{
  CLEAROUT(pbIsActive, VARIANT_FALSE);
  XLock lock(this);
  if (NULL != m_pPig)
  {
    *pbIsActive = VARBOOL(m_pPig->GetActiveBehavior() == this);
    if (!*pbIsActive && m_pBehaviorDerived)
      return m_pBehaviorDerived->get_IsActive(pbIsActive);
  }
  return S_OK;
}

STDMETHODIMP CPigBehaviorScript::get_Properties(IDictionary** ppProperties)
{
  CLEAROUT(ppProperties, (IDictionary*)NULL);
  XLock lock(this);
  if (NULL == m_spProperties)
  {
    // Create an instance of the "Scripting.Dictionary" object
    RETURN_FAILED(m_spProperties.CreateInstance("Scripting.Dictionary"));

    // Set dictionary's string comparison mode to TextCompare (ignore case)
    RETURN_FAILED(m_spProperties->put_CompareMode(TextCompare));
  }
  (*ppProperties = m_spProperties)->AddRef();
  return S_OK;
}

STDMETHODIMP CPigBehaviorScript::get_CommandLine(BSTR* pbstrCommandLine)
{
  // Delegate to the derived piglet, if we have one
  XLock lock(this);
  if (m_pBehaviorDerived)
  {
    // Base piglets should never have a command line
    assert(0 == m_bstrCommandLine.Length());

    // Delegate to the derived piglet
    return m_pBehaviorDerived->get_CommandLine(pbstrCommandLine);
  }

  // Initialize the [out] parameter
  CComBSTR bstrTemp(m_bstrCommandLine);
  CLEAROUT(pbstrCommandLine, (BSTR)bstrTemp);
  bstrTemp.Detach();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigBehaviorScript::get_Timer(IPigTimer** ppTimer)
{
  CLEAROUT(ppTimer, (IPigTimer*)NULL);
  XLock lock(this);
  return m_pTimerCurrent ?
    m_pTimerCurrent->QueryInterface(IID_IPigTimer, (void**)ppTimer) :
      GetDerived() ? GetDerived()->get_Timer(ppTimer) : S_OK;
}


// Infrastructure Methods
STDMETHODIMP CPigBehaviorScript::CreateTimer(float fInterval,
  BSTR bstrEventExpression, long nRepetitions, BSTR bstrName,
  IPigTimer** ppTimer)
{
  // Initialize the [out] parameter
  CLEAROUT_ALLOW_NULL(ppTimer, (IPigTimer*)ppTimer);

  // Create a new pig timer object
  CComObject<CPigTimer>* pTimer = NULL;
  RETURN_FAILED(pTimer->CreateInstance(&pTimer));
  IPigTimerPtr spTimer(pTimer);

  // Initialize the new object
  RETURN_FAILED(pTimer->Init(GetMostDerived(), fInterval,
    bstrEventExpression, nRepetitions, bstrName));

  // Copy the new object to the [out] parameter
  if (ppTimer)
    (*ppTimer = spTimer)->AddRef();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IActiveScriptSite Interface Methods

STDMETHODIMP CPigBehaviorScript::GetItemInfo(LPCOLESTR pstrName,
  DWORD dwReturnMask, IUnknown** ppunkItem, ITypeInfo** ppti)
{
  // Initialize the [out] parameters
  CLEAROUT_ALLOW_NULL(ppunkItem,  (IUnknown*)NULL);
  CLEAROUT_ALLOW_NULL(ppti     , (ITypeInfo*)NULL);

  IUnknownPtr spunkItem;
  if (0 == _wcsicmp(L"This", pstrName))
  {
    spunkItem = GetUnknown();
  }
  else if (0 == _wcsicmp(L"Pig", pstrName))
  {
    IPigPtr spPig;
    RETURN_FAILED(get_Pig(&spPig));
    assert(NULL != spPig);
    spunkItem = spPig;
  }
  else if (0 == _wcsicmp(L"Ship", pstrName))
  {
    IPigPtr spPig;
    RETURN_FAILED(get_Pig(&spPig));
    assert(NULL != spPig);
    IPigShipPtr spShip;
    spPig->get_Ship(&spShip);
    spunkItem = spShip;
  }
  else if (0 == _wcsicmp(L"Host", pstrName))
  {
    IPigBehaviorHostPtr spHost;
    RETURN_FAILED(get_Host(&spHost));
    assert(NULL != spHost);
    spunkItem = spHost;
  }
  else if (0 == _wcsicmp(L"BaseBehavior", pstrName))
  {
    IDispatchPtr spDispBaseBehavior;
    RETURN_FAILED(get_BaseBehavior(&spDispBaseBehavior));
    spunkItem = spDispBaseBehavior;
  }

  // Return an error if the specified named item is not recognized
  if (NULL == spunkItem)
    return TYPE_E_ELEMENTNOTFOUND;

  // Get the item's ITypeInfo, if requested
  if (SCRIPTINFO_ITYPEINFO & dwReturnMask)
  {
    IProvideClassInfoPtr sppci(spunkItem);
    if (NULL != sppci)
      RETURN_FAILED(sppci->GetClassInfo(ppti))
    else
      return TYPE_E_ELEMENTNOTFOUND;
  }

  // Get the item's IUnknown, if requested
  if (SCRIPTINFO_IUNKNOWN & dwReturnMask)
    (*ppunkItem = spunkItem)->AddRef();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigBehaviorScript::OnScriptError(IActiveScriptError* pScriptError)
{
	EXCEPINFO info;
	BSTR line; // = SysAllocString(L"kljadslkfalsdhflasfdlahdslhasldfjhalsdjfhlasjfdhlajsdhfljashdfljahsdlfjhasldhjlfas");
	DWORD sourceContext;
	ULONG lineNumber;
	LONG charPosition;
	pScriptError->GetExceptionInfo(&info);
	pScriptError->GetSourceLineText(&line);
	pScriptError->GetSourcePosition(&sourceContext, &lineNumber, &charPosition);

	printf("Script Error!\n");
	printf("Exception Info: %S\n", info.bstrDescription);
	//printf("Source Line Text: %S\n", line);
	printf("Source Position: %ld, Line: %u, charPosition: %ld\n", sourceContext, lineNumber, charPosition);

	// Perform default processing
	IActiveScriptSiteImplBase::OnScriptError(pScriptError);

	// Save error information
	m_spAse = pScriptError;

	// Indicating that the script is invalid
	//  m_bAppearsValid = false;
	// TODO: Notify the CPigEngine (to fire an event to the sessions)

  // Indicate success
  return S_OK;
}

