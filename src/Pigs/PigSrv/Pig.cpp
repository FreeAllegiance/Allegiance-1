/////////////////////////////////////////////////////////////////////////////
// Pig.cpp : Implementation of the CPig class.

#include "pch.h"
#include <TCLib.h>
#include <..\TCAtl\ObjectMap.h>
#include <AGC.h>
#include <memory>

#include "PigTimer.h"
#include "PigBehavior.h"
#include "PigBehaviorHost.h"
#include "PigBehaviorScript.h"
#include "PigBehaviorStack.h"
#include "PigBehaviorType.h"
#include "PigHullTypes.h"
#include "PigShip.h"
#include "Pigs.h"
#include "Pig.h"
//#include "PigDirectPlay.h"


/////////////////////////////////////////////////////////////////////////////
// Begin: Allegiance Initialization                                        //
                                                                           //
HKEY g_dwArtHive = HKEY_LOCAL_MACHINE;                                     //
char g_szArtReg[] =                                                        //
  "SOFTWARE\\Classes\\AppID\\{F132B4E3-C6EF-11D2-85C9-00C04F68DEB0}";      //
                                                                           //
// End: Allegiance Initialization                                          //
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CPig

TC_OBJECT_EXTERN_NON_CREATEABLE_IMPL(CPig)


/////////////////////////////////////////////////////////////////////////////
// Types


/////////////////////////////////////////////////////////////////////////////
// Description: Parameters used between the creator thread and the pig thread.
//
class CPig::XThreadParams
{
// Construction
public:
  XThreadParams() :
    m_pth(NULL),
    m_hevt(NULL),
    m_pType(NULL),
    m_bstrCommandLine(NULL),
    m_hr(S_OK),
    m_dwGITCookie(0)
  {
  }

// Attributes
public:
  TCThread*               m_pth;
  HANDLE                  m_hevt;
  CPigBehaviorScriptType* m_pType;
  BSTR                    m_bstrCommandLine;
  HRESULT                 m_hr;
  DWORD                   m_dwGITCookie;
  IStreamPtr              m_spstmError;
};


/////////////////////////////////////////////////////////////////////////////
// Description: Parameters passed to a chat command procedure.
//
struct CPig::XChatCommand
{
  IshipIGC*  m_pshipSender;
  ChatTarget m_ctRecipient;
  ObjectID   m_oidRecipient;
  SoundID    m_voiceOver;
  ZString    m_strText;
  ZString    m_strVerb;
  ZString    m_strMessage;
  CommandID  m_cid;
  ImodelIGC* m_pmodelTarget;
};

struct CPig::XSectorChange
{
  IPigBehaviorPtr m_spBehavior;
  IAGCSectorPtr   m_spSectorOld;
  IAGCSectorPtr   m_spSectorNew;
};

struct CPig::XShipDamaged
{
  IPigBehaviorPtr m_spBehavior;
  float           m_amount;
  float           m_leakage;
  IAGCShipPtr     m_spShip;
  IAGCModelPtr    m_spModel;
  IAGCVectorPtr   m_spVector1;
  IAGCVectorPtr   m_spVector2;
};

struct CPig::XShipKilled
{
  IPigBehaviorPtr m_spBehavior;
  float           m_amount;
	IAGCShipPtr     m_spShip;
  IAGCModelPtr    m_spModel;
  IAGCVectorPtr   m_spVector1;
  IAGCVectorPtr   m_spVector2;
};

struct CPig::XAlephHit
{
  IPigBehaviorPtr m_spBehavior;
  IAGCAlephPtr    m_spAleph;
};


/////////////////////////////////////////////////////////////////////////////
// Static Initialization

TCAutoCriticalSection CPig::s_csCreate, CPig::s_csUpdate, CPig::s_csLogon;


// Macro for table of special chat commands
#define CPig_ChatCommand_ENTRY(szVerb, bToken, cchSig, fnName)              \
  {{szVerb, bToken, cchSig}, fnName},

// Table of special chat commands
TCLookupTable_BEGIN(CPig, ChatCommands)    
  CPig_ChatCommand_ENTRY("ruapig?",  true, 6, &OnChat_AreYouAPig)
	//imago 10/14
	//CPig_ChatCommand_ENTRY("eval"   ,  true, 2, &OnChat_Evaluate)
	//CPig_ChatCommand_ENTRY("? "     , false, 1, &OnChat_Evaluate)
TCLookupTable_END()                      


/////////////////////////////////////////////////////////////////////////////
// Macro for verifying the result of calling script methods
//
#ifdef _DEBUG
  #define VerifyScriptMethod(_hr, _fn)                                      \
    {                                                                       \
      HRESULT _VSM_hr = _hr;                                                \
      if (FAILED(_VSM_hr))                                                  \
      {                                                                     \
        debugf(_fn "() script method failed: %s\n",                         \
          _com_error(_VSM_hr).ErrorMessage());                              \
        if (false) {ZSucceeded(_VSM_hr);}                                   \
      }                                                                     \
    }
#else // _DEBUG
  #define VerifyScriptMethod(_hr, _fn) {_hr;}
#endif // _DEBUG


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPig::CPig() :
  m_timeLastUpdate(Time::Now()),
  m_bDisconnected(false),
  m_bInReceive(false),
  m_eCurrentState(PigState_NonExistant),
  m_bLogonAck(false),
  m_dwGITCookie(0),
  m_nTicksPerSecond(20),
  m_rMaxShipAngleThreshold(0.f),
  m_rMaxShipDistanceThreshold(0.f),
  m_dwMaxShipsUpdateLatencyThreshold(0)
{
  // Calculate the number of milliseconds per tick
  m_msPerTick = 1000 / m_nTicksPerSecond;

  // Ensure that a base class never gets deleted before its time
  for (int i = 6; i; --i)
    IObjectSingle::AddRef();

  // Initialize the performance thresholds to zero (no threshold)
  for (int i = 0; i < sizeofArray(m_rShipAngleThresholds); ++i)
    m_rShipAngleThresholds[i] = 0.f;
  for (int i = 0; i < sizeofArray(m_rShipDistanceThresholds); ++i)
    m_rShipDistanceThresholds[i] = 0.f;
  for (int i = 0; i < sizeofArray(m_dwShipsUpdateLatencyThresholds); ++i)
    m_dwShipsUpdateLatencyThresholds[i] = 0;
}

HRESULT CPig::FinalConstruct()
{
  // Lock static variables during object construction
  XLockStatic lockStatic(&s_csCreate);

  // Register the client event sink for callbacks
  m_ClientEventSink = IClientEventSink::CreateDelegate(this);
  BaseClient::GetClientEventSource()->AddSink(m_ClientEventSink);

  // Create the AGC wrapper for the ship object (IGC ship may be NULL)
  assert(!m_pShip);
  CComObject<CPigShip>* pShip = NULL;
  ZSucceeded(pShip->CreateInstance(&pShip));
  (m_pShip = pShip)->AddRef();
  ZSucceeded(m_pShip->Init(this));

  // Initialize the base class
  BaseClient::Initialize(Time::Now());

  // Indicate success
  return S_OK;
}

void CPig::FinalRelease()
{
  // Lock static variables during object destruction
  XLockStatic lockStatic(&s_csCreate);

  // Remove our game from the IGC/AGC map
  GetAGCGlobal()->RemoveAGCObject(BaseClient::GetCore(), true);

  // Terminate the base class
  BaseClient::Terminate();

  // Release our Ship object
  if (m_pShip)
  {
    CoDisconnectObject(m_pShip->GetUnknown(), 0);
    m_pShip->Release();
    m_pShip = NULL;
  }

  // Unregister our client event sink
  BaseClient::GetClientEventSource()->RemoveSink(m_ClientEventSink);

  // Post the quit message to ourself
  XLock lock(this);
  if (m_pth)
  {
    m_pth->PostThreadMessage(WM_QUIT);
    m_pth = NULL;
  }
}

HRESULT CPig::Create(CPigBehaviorScriptType* pType, BSTR bstrCommandLine,
  DWORD* pdwGITCookie)
{
  // Clear the [out] paremeter
  CLEAROUT(pdwGITCookie, (DWORD)0);

  // Create a thread parameters structure, which is passed to the thread proc
  XThreadParams tp;

  // Create the thread in a suspended state
  TCThread* pth = TCThread::BeginMsgThread(ThreadThunk, &tp,
    THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

  // Create the event used to signal that the thread has created its objects
  TCHandle hevt(CreateEvent(NULL, false, false, NULL));

  // Set the members of the thread parameters structure
  tp.m_pth             = pth;
  tp.m_hevt            = hevt;  
  tp.m_pType           = pType;
  tp.m_bstrCommandLine = bstrCommandLine;

  // Resume the thread and wait for it to exit or the event to be signaled
  HANDLE hObjects[] = {pth->m_hThread, hevt};
  pth->ResumeThread();
  DWORD dwWait = WaitForMultipleObjects(sizeofArray(hObjects), hObjects,
    false, INFINITE);

  // Determine which event satisfied the wait
  if (WAIT_OBJECT_0 == dwWait)
  {
    // The thread exited, a failure must have occurred
    assert(FAILED(tp.m_hr));

    // Get the IErrorInfo, if any
    if (NULL != tp.m_spstmError)
    {
      // Rewind the stream
      LARGE_INTEGER li = {0, 0};
      _SVERIFYE(tp.m_spstmError->Seek(li, STREAM_SEEK_SET, NULL));

      // Unmarshal the error info from the stream
      IErrorInfoPtr spei;
      _SVERIFYE(CoGetInterfaceAndReleaseStream(tp.m_spstmError.Detach(),
        IID_IErrorInfo, (void**)&spei));
      SetErrorInfo(0, spei);
    }

    // Return the failure HRESULT
    return tp.m_hr;
  }
  else if ((WAIT_OBJECT_0 + 1) == dwWait)
  {
    // The thread proc successfully created the object
    ZSucceeded(tp.m_hr);

    // Copy the GIT cookie to the [out] parameter
    *pdwGITCookie = tp.m_dwGITCookie;
    return tp.m_hr;
  }

  // This should never happen
  assert(false);
  return E_UNEXPECTED;
}


/////////////////////////////////////////////////////////////////////////////
// Attributes

#define CPig_OnStateTransition(newState)                                    \
    case PigState_##newState:                                               \
    {                                                                       \
      debugf("CPig::wm_FireStateChange(%hs)\n", #newState);                 \
      _AGCModule.TriggerEvent(NULL, PigEventID_StateChange_##newState,      \
        bstrName, -1, -1, -1, 1,                                            \
        "PrevState", VT_BSTR, bstrPrevState);                               \
      VerifyScriptMethod(pBehavior->OnState##newState(ePrevState),          \
        "OnState" #newState);                                               \
      break;                                                                \
    }


PigState CPig::SetCurrentState(PigState eNewState)
{
  PigState ePrevState;
  {
    XLock lock(this);

    // Change the state and save the previous state
    ePrevState = m_eCurrentState;
    m_eCurrentState = eNewState;
  }

  // Notify the current behavior of the state transition
  m_pth->PostThreadMessage(wm_FireStateChange, WPARAM(eNewState),
    LPARAM(ePrevState));

  // Return the previous state
  return ePrevState;
}

SoundID CPig::GetOink()
{
  // Declare a table of sound ID's
  static const SoundID IDs[] =
    {oink1Sound, oink2Sound, oink3Sound, oink4Sound, oink5Sound,};
  static SoundID idNext = rand() % sizeofArray(IDs);

  // Choose a random sound ID
  return IDs[InterlockedIncrement((long*)&idNext) % sizeofArray(IDs)];
}


/////////////////////////////////////////////////////////////////////////////
// Operations

void CPig::ReceiveSendAndUpdate()
{
  // Check for recursion (a bad thing)
  assert(!m_bInReceive);
  m_bInReceive = true;

  // Get the current time and last update times
  m_lastUpdate = GetLastUpdateTime();
  m_now        = Time::Now();

  // See if we are auto downloading, and if done, allow logon to finish
  BaseClient::HandleAutoDownload(50);

  // Call the base class to receive messages and perform periodic updates
  HRESULT hr = BaseClient::ReceiveMessages(); // OnAppMessage will process all messages

  // Send updates and messages
  if (SUCCEEDED(hr) && BaseClient::GetNetwork()->IsConnected())
  {
    // Update the ship
    {
      XLockStatic lock(&s_csUpdate);
      if (BaseClient::GetShip())
        BaseClient::SendUpdate(m_lastUpdate);
    }

    // Check for server lag
    BaseClient::CheckServerLag(Time::Now());

    // Send messages if still connected
    if (BaseClient::GetNetwork()->IsConnected())
      BaseClient::SendMessages();

    // Update damage
    {
      XLockStatic lock(&s_csUpdate);

      // Update IGC
      BaseClient::GetCore()->Update(m_now);
    }
  }

  // Reset the last update time
  m_timeLastUpdate = m_now;
  m_bInReceive = false;
}

bool CPig::WaitInTimerLoop(HANDLE hObject, DWORD dwMilliseconds)
{
  _TRACE2("CPig::WaitInTimerLoop(): ThreadId = %u (0x%X)\n",
    GetCurrentThreadId(), GetCurrentThreadId());

  // Add the specified handle to the end of the collection
  if (hObject)
    m_Handles.push_back(hObject);
  else
    assert(INFINITE != dwMilliseconds);
  const DWORD cHandles = m_Handles.size();

  // If this is the main timer loop, initialize the behavior
  if (hObject == m_pth->m_hEventExit)
  {
    // Call OnActivate on the active behavior
    CPigBehaviorScript* pBehavior = GetActiveBehavior();
    _SVERIFYE(pBehavior->OnActivate(NULL));

    // Call OnStateNonExistant on the active behavior
    _SVERIFYE(pBehavior->OnStateNonExistant(PigState_Terminated));
  }

  // Compute the remaining time of the wait, for finite waits
  DWORD dwWaitBegin = GetTickCount();
  DWORD dwWaitEnd = dwWaitBegin + dwMilliseconds;

  // Loop indefinitely
  while (true)
  {
    // Compute the amount of time remaining for this time slice
    DWORD dwSliceElapsed = GetTickCount() - m_dwSliceStart;
		DWORD dwSliceRemaining = m_msPerTick - std::min(m_msPerTick, dwSliceElapsed);

    // Compute the timeout value for this wait iteration
    DWORD dwTimeout = (INFINITE == dwMilliseconds) ? dwSliceRemaining
			: std::min(dwSliceRemaining, (DWORD) std::max(0L, long(dwWaitEnd - GetTickCount())));

    // Wait for an object to be signaled or the time slice to expire
    DWORD dwWait = MsgWaitForMultipleObjects(cHandles, &(*m_Handles.begin()),
      false, dwTimeout, QS_ALLINPUT);

    // Determine what awoke the wait operation     //////////////////////////
    if (WAIT_TIMEOUT == dwWait)                    // Time slice elapsed
    {
      // Return if specified timeout has expired
      if (INFINITE != dwMilliseconds &&
        (GetTickCount() - dwWaitBegin) >= dwMilliseconds)
      {
        // Remove the specified handle from the end of the collection
        if (hObject)
        {
          assert(cHandles == m_Handles.size());
          m_Handles.pop_back();
        }
        return true;
      }

      // Save the current time as the beginning of the next time slice
      m_dwSliceStart = GetTickCount();

      // Receive/send messages from/to server and update everything
      ReceiveSendAndUpdate();

      // Evaluate the active behavior's event objects
      CPigBehaviorScript* pBehavior = GetActiveBehavior();
      if (pBehavior)
        pBehavior->PulseEvents();
    }                                              //////////////////////////
    else if ((WAIT_OBJECT_0 + cHandles) == dwWait) // Messages in queue
    {
      // Copy each message in the queue
      std::vector<MSG*> msgs;
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        MSG* pMsg = new MSG;
        CopyMemory(pMsg, &msg, sizeof(*pMsg));
        msgs.push_back(pMsg);
      }

      // Handle each message
      bool bEvent = false;
      std::vector<MSG*>::iterator it;
      for (it = msgs.begin(); it != msgs.end(); ++it)
      {
        if (HandleThreadMessage(*it, hObject))
          bEvent = true;
        delete *it;
      }
      if (bEvent)
      {
        // Remove the specified handle from the end of the collection
        if (hObject)
        {
          assert(cHandles == m_Handles.size());
          m_Handles.pop_back();
          return true;
        }
        else
        {
          return false;
        }
      }
    }                                              //////////////////////////
    else                                           // An object was signaled
    {
      // Remove the specified handle from the end of the collection
      assert(cHandles == m_Handles.size());
      m_Handles.pop_back();

      // Determine if the specified handle was signaled and return
      return (WAIT_OBJECT_0 + cHandles - 1) == dwWait;
    }
  }
}


void CPig::SendChat(ChatTarget chatTarget, const char* psz,
  IshipIGC* pshipRecipient, SoundID voiceOver)
{
  BaseClient::SendChat(GetShip(), chatTarget,
    pshipRecipient ? pshipRecipient->GetObjectID() : NA, voiceOver, psz,
    c_cidNone, NA, NA);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

HRESULT CPig::Stack_get_Count(long* pnCount)
{
  XLock lock(this);
  CLEAROUT(pnCount, (long)m_Behaviors.size());
  return S_OK;
}

HRESULT CPig::Stack_get__NewEnum(IUnknown** ppunkEnum)
{
  // Create a new CComEnum enumerator object
  typedef CComObject<CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT,
    _Copy<VARIANT> > > CEnum;
  CEnum* pEnum = new CEnum;
  assert(NULL != pEnum);

  // Get the number of items in the collection
  XLock lock(this);
  long cTotal = m_Behaviors.size();
  
  // Create a temporary array of variants
  std::vector<CComVariant> vecTemp(cTotal);

  // Get the IDispatch of each item of the collection
  for (CComVariant i(0L); V_I4(&i) < cTotal; ++V_I4(&i))
  {
    IPigBehaviorPtr spItem;
    RETURN_FAILED(Stack_get_Top(V_I4(&i), &spItem));
    vecTemp[V_I4(&i)] = (IDispatch*)spItem;
  }

  // Initialize enumerator object with the temporary CComVariant vector
  // VS.Net 2003 port - accomodate change in iterators under VC.Net 200x (see 'breaking changes' in vsnet doc)
#if _MSC_VER >= 1310
  HRESULT hr = pEnum->Init(&(*vecTemp.begin()), &(*vecTemp.end()), NULL, AtlFlagCopy);
#else
  HRESULT hr = pEnum->Init(vecTemp.begin(), vecTemp.end(), NULL, AtlFlagCopy);
#endif
  if (SUCCEEDED(hr))
    hr = pEnum->QueryInterface(IID_IEnumVARIANT, (void**)ppunkEnum);
  if (FAILED(hr))
    delete pEnum;

  // Return the last result
  return hr;
}

HRESULT CPig::Stack_Push(BSTR bstrType, BSTR bstrCommandLine,
  IPigBehavior** ppBehavior)
{
  // Initialize the [out] parameter
  CLEAROUT(ppBehavior, (IPigBehavior*)NULL);

  // bstrType is optional, so assign a default value if not specified
  CComBSTR bstrTheType(bstrType);
  if (!bstrTheType.Length())
    bstrTheType = L"Default";

  // Find the specified behavior type
  CPigBehaviorScriptType* pType = GetEngine().GetBehaviorType(bstrTheType);
  if (!pType)
    return Error(IDS_E_CREATE_BEHAVIOR_TYPE, IID_IPigBehaviorStack);

  // Create an instance of a behavior object
  CComObject<CPigBehaviorScript>* pBehaviorNew = NULL;
  RETURN_FAILED(pBehaviorNew->CreateInstance(&pBehaviorNew));
  IPigBehaviorPtr spBehaviorNew(pBehaviorNew->GetUnknown());
  assert(NULL != spBehaviorNew);

  // Initialize the behavior object
  RETURN_FAILED(pBehaviorNew->Init(pType, this, bstrCommandLine));

  // Notify the active behavior that it is being deactivated
  XLock lock(this);
  CPigBehaviorScript* pBehavior = GetActiveBehavior();
  if (pBehavior)
    VerifyScriptMethod(pBehavior->OnDeactivate(spBehaviorNew), "OnDeactivate");

  // Push the new behavior onto the top of the stack
  pBehaviorNew->AddRef(); // Reference for the pointer in the stack
  m_Behaviors.push_back(pBehaviorNew);

  // Notify the active behavior that it has been activated
  VerifyScriptMethod(spBehaviorNew->OnActivate(pBehavior), "OnActivate");

  // Detach the new behavior to the [out] parameter
  *ppBehavior = spBehaviorNew.Detach();

  // Indicate success
  return S_OK;
}

HRESULT CPig::Stack_Pop()
{
  // Do nothing if no behaviors are on the stack (or just one left)
  XLock lock(this);
  if (m_Behaviors.size() < 2)
    return S_FALSE;

  // Get the behavior which will become active, if any
  IPigBehaviorPtr spBehaviorNew;
  if (m_Behaviors.size() > 1)
    _SVERIFYE(Stack_get_Top(1, &spBehaviorNew));

  // Notify the active behavior that it is being deactivated
  CPigBehaviorScript* pBehavior = GetActiveBehavior();
  VerifyScriptMethod(pBehavior->OnDeactivate(spBehaviorNew), "OnDeactivate");

  // Pop the active behavior off the top of the stack
  m_Behaviors.pop_back();

  // Notify the active behavior that it has been activated
  if (NULL != spBehaviorNew)
    VerifyScriptMethod(spBehaviorNew->OnActivate(pBehavior), "OnActivate");

  // Schedule the popped behavior to be terminated and released
  _VERIFYE(m_pth->PostThreadMessage(wm_TerminateBehavior, (WPARAM)pBehavior));

  // Indicate success
  return S_OK;
}

HRESULT CPig::Stack_get_Top(long nFromTop, IPigBehavior** ppBehavior)
{
  // Initialize the [out] parameter
  CLEAROUT(ppBehavior, (IPigBehavior*)NULL);

  // Zero is always allowed
  XLock lock(this);
  if (!nFromTop && m_Behaviors.empty())
    return S_OK;

  // Validate the specified index
  if (nFromTop >= m_Behaviors.size())
    return E_INVALIDARG;

  // Access using the reverse iterator
  CPigBehaviorScript* pBehavior = *(m_Behaviors.rbegin() + nFromTop);

  // QI for the IPigBehavior interface
  return pBehavior->QueryInterface(IID_IPigBehavior, (void**)ppBehavior);
}

unsigned CALLBACK CPig::ThreadThunk(void* pvParam)
{
  _TRACE2("CPig::ThreadThunk(): ThreadId = %u (0x%X)\n",
    GetCurrentThreadId(), GetCurrentThreadId());

  // Reinterpret the parameter as an XThreadParams structure
  XThreadParams* ptp = reinterpret_cast<XThreadParams*>(pvParam);

	// Enter this thread into an MTA
	RETURN_FAILED(ptp->m_hr = CoInitializeEx(NULL, COINIT_MULTITHREADED));

  // Create an instance of the pig object
  HRESULT hr;
  CComObject<CPig>* pPig = NULL;
  if (SUCCEEDED(ptp->m_hr = hr = ThreadCreatePig(ptp, pPig)))
  {
    IPigPtr spPig(pPig->GetUnknown());

    // Increment our reference to the engine
    GetEngine().AddRef();

    // Signal the event, now that the object has been created
    _VERIFYE(SetEvent(ptp->m_hevt));

    // Note: ptp is now off limits!
    ptp = NULL;

    // TODO: Put SEH around this and gracefully handle the exceptions
    {
      // Delegate to the non-static method
      pPig->ThreadProc();
    }

    // Remove ourself from the engine's pig collection
    GetEngine().GetPigs().RemovePig(pPig->m_dwGITCookie);

    // Revoke ourself from the GIT
    _SVERIFYE(GetEngine().RevokeInterfaceFromGlobal(pPig->m_dwGITCookie));
    pPig->m_dwGITCookie = 0;

    // Disconnect all external references
    CoDisconnectObject(spPig, 0);

    // Release our reference to ourself
    spPig = NULL;

    // Wait in a message loop until our FinalRelease posts a WM_QUIT message
    // TODO: Hang out for a limited amount of time?
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // Release our reference to the engine
    GetEngine().Release();
  }
  else
  {
    // Get the error info of the thread, if any
    IErrorInfoPtr spei;
    if (S_OK == GetErrorInfo(0, &spei) && NULL != spei)
    {
      // TEST: QI for IPersistStream*
      IPersistStreamPtr spP1(spei);
      IPersistStreamInitPtr spP2(spei);

      // Create a stream on an HGLOBAL
      _SVERIFYE(CreateStreamOnHGlobal(NULL, true, &ptp->m_spstmError));

      // Marshal the IErrorInfo into the stream
      _SVERIFYE(CoMarshalInterThreadInterfaceInStream(IID_IErrorInfo, spei,
        &ptp->m_spstmError));
    }
  }

  // Remove this thread from the STA
  CoUninitialize();

  // Return the result of the pig object creation
  return hr;
}

HRESULT CPig::ThreadCreatePig(CPig::XThreadParams* ptp, CComObject<CPig>*& pPig)
{
  // Get the account dispenser
  IPigAccountDispenserPtr spDispenser;
  RETURN_FAILED(GetEngine().get_AccountDispenser(&spDispenser));

  // Get an available account from the dispenser
  IPigAccountPtr spAccount;
  RETURN_FAILED(spDispenser->get_NextAvailable(&spAccount));

  // Lock static variables during the object creation
  ptp->m_hr = pPig->CreateInstance(&pPig);
  IPigPtr spPig(pPig);
  if (FAILED(ptp->m_hr))
    return ptp->m_hr;
  assert(NULL != spPig);

  // Set the pig's account and cache the account name
  pPig->m_spAccount = spAccount;
  RETURN_FAILED(spAccount->get_Name(&pPig->m_bstrName));

  RETURN_FAILED(spAccount->get_CdKey(&pPig->m_bstrCdKey));

  // Create an instance of a behavior object attached to the pig
  CComObject<CPigBehaviorScript>* pBehavior = NULL;
  RETURN_FAILED(pBehavior->CreateInstance(&pBehavior));
  IPigBehaviorPtr spBehavior(pBehavior->GetUnknown());
  assert(NULL != spBehavior);

  // Initialize the behavior
  RETURN_FAILED(pBehavior->Init(ptp->m_pType, pPig, ptp->m_bstrCommandLine));

  // Add ourself to the Global Interface Table (GIT)
  RETURN_FAILED((GetEngine().RegisterInterfaceInGlobal(spPig, IID_IPig,
    &ptp->m_dwGITCookie)));

  // Get our pig user name
  CComBSTR bstrName;
  RETURN_FAILED(pPig->get_Name(&bstrName));

  // Add our GIT cookie to the engine's pig collection
  GetEngine().GetPigs().AddPig(ptp->m_dwGITCookie, _bstr_t(bstrName));

  // Add the new behavior object to the top of the stack
  pBehavior->AddRef(); // Reference for the pointer in the stack
  pPig->m_Behaviors.push_back(pBehavior);
  pBehavior = NULL;

  // Save several of the XThreadParams values into the object
  pPig->m_pth         = ptp->m_pth;
  pPig->m_dwGITCookie = ptp->m_dwGITCookie;

  // Indicate success
  return S_OK;
}

void CPig::ThreadProc()
{
  m_dwSliceStart = GetTickCount();

  // Enter the main timer loop
  _VERIFYE(WaitInTimerLoop(m_pth->m_hEventExit));

  // Deactivate and terminate the active behavior, if any
  CPigBehaviorScript* pBehavior = GetActiveBehavior();
  if (pBehavior)
  {
    // Call OnStateTerminated and OnDeactivate on only the active behavior
    _SVERIFYE(pBehavior->OnStateTerminated(GetCurrentState()));
    _SVERIFYE(pBehavior->OnDeactivate(NULL));

    // Terminate active and any remaining behaviors on the stack, if any
    do
    {
      // Pop the behavior off the stack
      m_Behaviors.pop_back();

      // Terminate the behavior
      pBehavior->Term();

      // Release the behavior
      pBehavior->Release();

    } while (pBehavior = GetActiveBehavior());
  }

  // Terminate the behavior stack, if any
  if (NULL != m_spBehaviorStack)
  {
    CoDisconnectObject(m_spBehaviorStack, 0);
    m_spBehaviorStack = NULL;
  }
}

bool CPig::HandleThreadMessage(const MSG* pMsg, HANDLE hObject)
{
  TranslateMessage(pMsg);
  DispatchMessage(pMsg);
  switch (pMsg->message)
  {
    case wm_TerminateBehavior:
    {
      CPigBehaviorScript* pBehavior = (CPigBehaviorScript*)(pMsg->wParam);
      assert(pBehavior);
      pBehavior->Term();
      pBehavior->Release();
      break;
    }
    case wm_Shutdown:
    {
      // If the wParam is non-zero, decrement it and defer operation
      if (pMsg->wParam)
      {
        m_pth->PostThreadMessage(pMsg->message, pMsg->wParam - 1);
        break;
      }

      // Logoff, if we are other than non-existant
      if (PigState_NonExistant != GetCurrentState())
        Logoff();

      // Signal the thread to exit
      XLock lock(this);
      HANDLE hth = m_pth->m_hThread;
      ::SetEvent(m_pth->m_hEventExit);

      // Set the state to PigState_Terminated
      SetCurrentState(PigState_Terminated);
      break;
    }
    case wm_FireStateChange:
    {
      // Parse the message parameters
      PigState eNewState  = static_cast<PigState>(pMsg->wParam);
      PigState ePrevState = static_cast<PigState>(pMsg->lParam);

      // Get the pig user name (used by CPig_OnStateTransition macro)
      CComBSTR& bstrName = m_bstrName;

      // Get the names of both states
      CComBSTR bstrNewState, bstrPrevState;
      GetEngine().get_StateName(eNewState, &bstrNewState);
      GetEngine().get_StateName(ePrevState, &bstrPrevState);

      // Notify the current behavior of the state transition
      CPigBehaviorScript* pBehavior = GetActiveBehavior();
      if (pBehavior)
      {
        switch (eNewState)
        {
          CPig_OnStateTransition(NonExistant)
          CPig_OnStateTransition(LoggingOn)
          CPig_OnStateTransition(LoggingOff)
          CPig_OnStateTransition(MissionList)
          CPig_OnStateTransition(CreatingMission)
          CPig_OnStateTransition(JoiningMission)
          CPig_OnStateTransition(QuittingMission)
          CPig_OnStateTransition(TeamList)
          CPig_OnStateTransition(JoiningTeam)
          CPig_OnStateTransition(WaitingForMission)
          CPig_OnStateTransition(Docked)
          CPig_OnStateTransition(Launching)
          CPig_OnStateTransition(Flying)
          CPig_OnStateTransition(Terminated)
          default: assert(false);
        }
      }
      break;
    }
    case wm_SignalEvent:
    {
      HANDLE hevt = reinterpret_cast<HANDLE>(pMsg->wParam);
      if (hevt)
        ::SetEvent(hevt);
      if (hevt == hObject)
        return true;
      break;
    }
    case wm_OnMissionStarted:
    {
      // Parse the message parameters
      IPigBehavior* pBehavior = reinterpret_cast<IPigBehavior*>(pMsg->wParam);
      assert(pBehavior);
			pBehavior->OnMissionStarted(); //imago 10/14
      pBehavior->Release();
      break;
    }
    case wm_OnReceiveChat:
    {
      // Parse the message parameters
      XChatCommand* pChat = reinterpret_cast<XChatCommand*>(pMsg->wParam);
      XChatCommand& chat = *pChat;
      std::auto_ptr<XChatCommand> apChat(pChat);

      // Break out the first token
      ZString strMessage(chat.m_strText);
      ZString strVerb(strMessage.GetToken());

      // Get the ship of the chat's origin, if any
      m_spShipChatOrigin = NULL;
      if (chat.m_pshipSender)
        GetAGCGlobal()->GetAGCObject(chat.m_pshipSender, IID_IAGCShip,
          (void**)&m_spShipChatOrigin);

      // Get the chat target
      m_eChatTarget = (AGCChatTarget)chat.m_ctRecipient;

      // Check for first-crack messages
      for (int i = 0; i < TCLookupTable_SIZE(ChatCommands); ++i)
      {
        ChatCommands_ValueType& entry = ChatCommands[i];
        assert(entry.m_key.m_szVerb && '\0' != *entry.m_key.m_szVerb);
        ZString strVerbCompare(entry.m_key.m_szVerb, true);

        // Compute the minimum number of significant characters
        int cchMin = (entry.m_key.m_cchSignificant <= 0) ?
          strlen(entry.m_key.m_szVerb) : entry.m_key.m_cchSignificant;

        // Compare down to the minimum number of significant characters
        bool bMatched = false;
        ZString strCompare(entry.m_key.m_bToken ?
          strVerb : chat.m_strText.Left(strVerbCompare.GetLength()));
        int cch = strCompare.GetLength();
        while (cch-- >= cchMin &&
          !(bMatched = (!_stricmp(strCompare, strVerbCompare.Left(cch + 1)))))
            strCompare = strCompare.Left(cch);

        // Process the verb, if it was matched
        if (bMatched)
        {
          chat.m_strVerb    = strCompare;
          chat.m_strMessage = entry.m_key.m_bToken ?
            strMessage : chat.m_strText.RightOf(strCompare.GetLength());
          if ((this->*entry.m_value)(chat))
            return false;
        }
      }

      // Allow the ship to interpret the message
      XLock lock(this);
      if (m_pShip && m_pShip->OnNewChatMessage())
        return false;

      // Allow the active behavior to interpret the message
      CPigBehaviorScript* pBehavior = GetActiveBehavior();
      if (pBehavior)
      {
        VARIANT_BOOL bHandled = VARIANT_FALSE;
        HRESULT hr = pBehavior->OnReceiveChat(CComBSTR(chat.m_strText),
          m_spShipChatOrigin, &bHandled);
        VerifyScriptMethod(hr, "OnReceiveChat");
        if (S_OK == hr && bHandled)
        {
          m_spShipChatOrigin = NULL;
          m_eChatTarget = AGCChat_NoSelection;
          return false;
        }
      }

      // Attempt to interpret the chat message as a behavior/piglet invoke command
			/* imago 10/14
      CComBSTR bstrType(strVerb);
      CPigBehaviorScriptType* pType = GetEngine().GetBehaviorType(bstrType);
      if (pType)
      {
        // Create and make active the specified type of piglet
        CComBSTR bstrCommandLine(strMessage);
        IPigBehaviorPtr spBehavior;
        if (SUCCEEDED(Stack_Push(bstrType, bstrCommandLine, &spBehavior)))
        {
          // Get the type of the created behavior
          assert(NULL != spBehavior);
          IPigBehaviorTypePtr spType;
          _SVERIFYE(spBehavior->get_BehaviorType(&spType));

          // Get the name of the behavior type
          CComBSTR bstrName;
          _SVERIFYE(spType->get_Name(&bstrName));

          // Format the response string
          char szFmt[_MAX_PATH], szMsg[_MAX_PATH];
          _VERIFYE(::LoadStringA(_Module.GetResourceInstance(),
            IDS_FMT_PIGLETINVOKED, szFmt, sizeofArray(szFmt)));
          sprintf(szMsg, szFmt, (BSTR)bstrName);

          // Reply to the sender
          SendChat(CHAT_INDIVIDUAL, szMsg, chat.m_pshipSender);
          m_spShipChatOrigin = NULL;
          m_eChatTarget = AGCChat_NoSelection;
          return false;
        }
      }
			*/

      #if 0
        // Attempt to evaluate the chat message as script text
        chat.m_strVerb.SetEmpty();
        chat.m_strMessage = strText;
        OnChat_Evaluate(chat);
      #endif

      // Reset the ship of the chat's origin
      m_spShipChatOrigin = NULL;
      m_eChatTarget = AGCChat_NoSelection;
      break;
    }
    case wm_OnSectorChange:
    {
      // Parse the message parameters
      XSectorChange* pParams = reinterpret_cast<XSectorChange*>(pMsg->wParam);
      std::auto_ptr<XSectorChange> apParams(pParams);

      // Notify the active behavior
      HRESULT hr = pParams->m_spBehavior->OnSectorChange(
        pParams->m_spSectorOld, pParams->m_spSectorNew);
      VerifyScriptMethod(hr, "OnSectorChange");
      break;
    }
    case wm_OnShipDamaged:
    {
      // Parse the message parameters
      XShipDamaged* pParams = reinterpret_cast<XShipDamaged*>(pMsg->wParam);
      std::auto_ptr<XShipDamaged> apParams(pParams);

      // Notify the active behavior
      HRESULT hr = pParams->m_spBehavior->OnShipDamaged(pParams->m_spShip,
        pParams->m_spModel, pParams->m_amount, pParams->m_leakage,
        pParams->m_spVector1, pParams->m_spVector2);
      VerifyScriptMethod(hr, "OnShipDamaged");
      break;
    }
    case wm_OnShipKilled:
    {
      // Parse the message parameters
      XShipKilled* pParams = reinterpret_cast<XShipKilled*>(pMsg->wParam);
      std::auto_ptr<XShipKilled> apParams(pParams);
      
      // Notify the active behavior
			HRESULT hr = pParams->m_spBehavior->OnShipKilled(pParams->m_spShip, 
				pParams->m_spModel, pParams->m_amount, pParams->m_spVector1, pParams->m_spVector2);
      VerifyScriptMethod(hr, "OnShipKilled");
      break;
    }
    case wm_OnAlephHit:
    {
      // Parse the message parameters
      XAlephHit* pParams = reinterpret_cast<XAlephHit*>(pMsg->wParam);
      std::auto_ptr<XAlephHit> apParams(pParams);
      
      // Notify the active behavior
      HRESULT hr = pParams->m_spBehavior->OnAlephHit(pParams->m_spAleph);
      VerifyScriptMethod(hr, "OnAlephHit");
      break;
    }
  }
  return false;
}

HRESULT CPig::ProcessAppMessage(FEDMESSAGE* pfm)
{
  // Handle messages before default processing
  const FEDMSGID fmid = pfm->fmid;

	/*
  if (fmid != FM_CS_PING && 
      fmid != FM_S_LIGHT_SHIPS_UPDATE &&
		
		//imago 10/14
		fmid != FM_S_ASTEROID_MINED &&
		fmid != FM_S_BUCKET_STATUS &&
		fmid != FM_S_EXPORT &&

      fmid != FM_S_HEAVY_SHIPS_UPDATE &&
      fmid != FM_CS_CHATMESSAGE && 
      fmid != FM_S_STATIONS_UPDATE && 
      fmid != FM_S_PROBES_UPDATE)
      debugf("Received %s at time %u\n", g_rgszMsgNames[fmid], Time::Now().clock());
	*/

  switch (pfm->fmid)
  {

#ifdef _DEBUG

      case FM_S_EXPORT:
      {
        // Check for static objects
        CASTPFM(pfmExport, S, EXPORT, pfm);
        ObjectType ot = pfmExport->objecttype;
        if (OT_staticBegin <= ot && ot <= OT_staticEnd)
        {
          ZError("FM_S_EXPORT is exporting static objects (and it shouldn't be)!");
        }
        break;
      }
	 
#endif // _DEBUG

	  case FM_S_POSITIONREQ:
	  {
		  CASTPFM(pfmPositionRequest, S, POSITIONREQ, pfm);
		  // If the are joining our team and I'm the leader
		  if (pfmPositionRequest->iSide == BaseClient::GetSideID() && BaseClient::MyPlayerInfo()->IsTeamLeader()) {
			  // if AutoAccept is off
			  FMD_S_MISSIONDEF myMissionDef = BaseClient::MyMission()->GetMissionDef();
			  if (myMissionDef.rgfAutoAccept[BaseClient::GetSideID()] == false) {
				  // Accept them if my are in our squad
				  // get the squadTag
				  ZString myTag = ZString(BaseClient::MyPlayerInfo()->CharacterName()).RightOf("@");
				  ZString theirTag = ZString(BaseClient::FindPlayer(pfmPositionRequest->shipID)->CharacterName()).RightOf("@");

				  BaseClient::SetMessageType(c_mtGuaranteed);
				  BEGIN_PFM_CREATE(*BaseClient::GetNetwork(), pfmPositionAck, C, POSITIONACK)
					  END_PFM_CREATE
					  pfmPositionAck->iSide = pfmPositionRequest->iSide;
				  pfmPositionAck->shipID = pfmPositionRequest->shipID;

				  // If their tag = my tag, then they can join my team.
				  pfmPositionAck->fAccepted = myTag == theirTag;
				  
				  BaseClient::SendMessages();
			  }
		  }
		  break;
	  }
  }


  // Keep track of each ship's performance details
  XHeavyShips                   HeavyShips;
  if (fmid == FM_S_HEAVY_SHIPS_UPDATE && HasAnyPerformanceThresholds())
  {
    CASTPFM(pfmHeavy, S, HEAVY_SHIPS_UPDATE, pfm);
    ServerHeavyShipUpdate* phsu = (ServerHeavyShipUpdate*)(FM_VAR_REF(pfmHeavy, rgHeavyShipUpdates));
    if (phsu)
    {
      ServerHeavyShipUpdate* phsuMax = (ServerHeavyShipUpdate*)((char*)phsu + pfmHeavy->cbrgHeavyShipUpdates);

      // Resize the array to maintain previous details of each ship
      HeavyShips.resize(phsuMax - phsu);

      // Process the heavy ship updates
      int cShips = 0;
      while (phsu < phsuMax)
      {
        ServerHeavyShipUpdate&  hsu = *(phsu++);

        //Never get updates for yourself
        assert (hsu.shipID != BaseClient::GetShip()->GetObjectID());

        IshipIGC*   pship = BaseClient::GetCore()->GetShip(hsu.shipID);
        if (pship && (pship->GetParentShip() == NULL) && pship->GetCluster())
        {
          HeavyShips[cShips].pShip    = pship;
          HeavyShips[cShips].vPosPrev = pship->GetPosition();
          HeavyShips[cShips].vFwdPrev = pship->GetOrientation().GetForward();
          ++cShips;
        }
      }
      HeavyShips.resize(cShips);
    }
  }

  // Perform default processing
  {
    // Ensure that only one thread is in here at a time
    XLockStatic lock(&s_csUpdate);

    // Allow the base class to handle the message
    HRESULT hr = BaseClient::HandleMsg(pfm, m_lastUpdate, m_now);
    RETURN_FAILED(hr);
  }

  // Handle messages after default processing
  if (HeavyShips.size())
  {
    if (m_dwMaxShipsUpdateLatencyThreshold)
    {
      CASTPFM(pfmHeavy, S, HEAVY_SHIPS_UPDATE, pfm);
      Time delta = ServerTimeFromClientTime(Time::Now()) - pfmHeavy->timeReference;
      int iExceeded =
				findMaxThresholdExceeded<DWORD>(m_dwShipsUpdateLatencyThresholds,
          sizeofArray(m_dwShipsUpdateLatencyThresholds), delta.clock());
      if (-1 != iExceeded)
      {
        _AGCModule.TriggerEvent(NULL, PigEventID_ShipsUpdateLatencyThreshold,
          m_bstrName, -1, -1, -1, 3,
          "ThresholdIndex", VT_I4, iExceeded + 1,
          "ActualValue"   , VT_I4, delta.clock(),
          "ThresholdValue", VT_I4, m_dwShipsUpdateLatencyThresholds[iExceeded]);
      }
    }

    // Loop through each ship in this heavy update
    for (XHeavyShipsIt it = HeavyShips.begin(); it != HeavyShips.end(); ++it)
    {
      // Check for ShipAngle thresholds
      if (m_rMaxShipAngleThreshold)
      {
        // Compute the delta angle
        float product = it->vFwdPrev * it->pShip->GetOrientation().GetForward();
        float delta;
        if (product >= 1.f)
          delta = 0.f;
        else if (product <= -1.f)
          delta = pi;
        else
          delta = acos(product);
        int iExceeded =
          findMaxThresholdExceeded(m_rShipAngleThresholds,
            sizeofArray(m_rShipAngleThresholds), delta);
        if (-1 != iExceeded)
        {
          _AGCModule.TriggerEvent(NULL, PigEventID_ShipAngleThreshold,
            m_bstrName, it->pShip->GetUniqueID(), -1, -1, 5,
            "ThresholdIndex", VT_I4   , iExceeded + 1,
            "ShipName"      , VT_LPSTR, it->pShip->GetName(),
            "ShipID"        , VT_I4   , it->pShip->GetUniqueID(),
            "ActualValue"   , VT_R4   , delta,
            "ThresholdValue", VT_R4   , m_rShipAngleThresholds[iExceeded]);
        }
      }

      // Check for ShipDistance thresholds
      if (m_rMaxShipDistanceThreshold)
      {
        // Compute the delta distance
        float delta = (it->pShip->GetPosition() - it->vPosPrev).LengthSquared();
        int iExceeded =
          findMaxThresholdExceeded(m_rShipDistanceThresholds,
            sizeofArray(m_rShipDistanceThresholds), delta);
        if (-1 != iExceeded)
        {
          _AGCModule.TriggerEvent(NULL, PigEventID_ShipDistanceThreshold,
            m_bstrName, it->pShip->GetUniqueID(), -1, -1, 5,
            "ThresholdIndex", VT_I4   , iExceeded + 1,
            "ShipName"      , VT_LPSTR, it->pShip->GetName(),
            "ShipID"        , VT_I4   , it->pShip->GetUniqueID(),
            "ActualValue"   , VT_R4   , delta,
            "ThresholdValue", VT_R4   , m_rShipDistanceThresholds[iExceeded]);
        }
      }
    }
  }

	//imago 10/14
	switch (pfm->fmid)
	{
		case FM_S_GAME_OVER:
		{
			CASTPFM(pfmGameOver, S, GAME_OVER, pfm);
			GameOver(pfmGameOver->iSideWinner);
		}
	}

  // Indicate success
  return S_OK;
}

bool CPig::OnChat_AreYouAPig(CPig::REFXChatCommand chat)
{
  // Get the computer name
  char szName[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD cchName = sizeofArray(szName);
  GetComputerNameA(szName, &cchName);

  // Do nothing if the computer name does not match that specified, if any
  if (chat.m_strMessage.GetLength() && _stricmp(chat.m_strMessage, szName))
    return true;

  // Format the Oink! reply
  char szFmt[_MAX_PATH], szMsg[_MAX_PATH];
  _VERIFYE(LoadStringA(_Module.GetResourceInstance(), IDS_FMT_OINK, szFmt,
    sizeofArray(szFmt)));
  sprintf(szMsg, szFmt, szName);

  // Reply to the sender
  SendChat(CHAT_INDIVIDUAL, szMsg, chat.m_pshipSender, GetOink());

  // Indicate that we handled the message
  return true;
}

bool CPig::OnChat_Evaluate(CPig::REFXChatCommand chat)
{
  // Can't be script if sender is NULL
  if (!chat.m_pshipSender)
    return false;

  // Evaluate the expression, if specified
  HRESULT hr = E_FAIL;
  CComVariant varResult;
  if (chat.m_strMessage.GetLength())
  {
    if (FAILED(hr = GetActiveBehavior()->Eval(chat.m_strMessage, &varResult)))
    {
      IErrorInfoPtr spei;
      GetErrorInfo(0, &spei);
      CComBSTR bstr;
      spei->GetDescription(&bstr);
      varResult = bstr;
    }
  }
  else if (chat.m_strVerb.GetLength())
  {
    CComBSTR bstr;
    _SVERIFYE(bstr.LoadString(IDS_E_EVAL_SYNTAX));
    varResult = bstr;
  }

  // Ensure that there is a string
  if (VT_BSTR != V_VT(&varResult))
  {
    VariantChangeTypeEx(&varResult, &varResult, GetThreadLocale(),
      VARIANT_LOCALBOOL | VARIANT_ALPHABOOL, VT_BSTR);
    if (!BSTRLen(V_BSTR(&varResult)))
      varResult = SUCCEEDED(hr) ? L"{no result}" : L"{empty}";
  }
  else if (!BSTRLen(V_BSTR(&varResult)))
    varResult = L"{empty}";

  // Ignore error if verb is empty and recipient is not CHAT_INDIVIDUAL
  if (FAILED(hr) && chat.m_strVerb.IsEmpty() && CHAT_INDIVIDUAL != chat.m_ctRecipient)
    return false;

  // Reply to the sender
  USES_CONVERSION;
  SendChat(CHAT_INDIVIDUAL, OLE2CA(V_BSTR(&varResult)), chat.m_pshipSender);

  // Indicate that we handled the message
  return true;
}

bool CPig::FindMissionName(char* pszMissionName)
{
  // Return an error if there are no missions
  if (!m_mapMissions.GetCount())
    return false;

  // Define an iterator type for (BaseClient) mission map
  typedef TMapListWrapper<DWORD, MissionInfo*>::Iterator MissionIt;

  // Iterate through the mission map
  for (MissionIt it(BaseClient::m_mapMissions); !it.End(); it.Next())
    if (0 == _stricmp(pszMissionName, it.Value()->Name()))
      return true;

  // Mission name was not found
  return false;  
}

void CPig::CacheMaxShipAngleThresholds()
{
  XLock lock(this);
  m_rMaxShipAngleThreshold = m_rShipAngleThresholds[0];
  for (int i = 1; i < sizeofArray(m_rShipAngleThresholds); ++i)
    if (m_rShipAngleThresholds[i] > m_rMaxShipAngleThreshold)
      m_rMaxShipAngleThreshold = m_rShipAngleThresholds[i];
}

void CPig::CacheMaxShipDistanceThresholds()
{
  XLock lock(this);
  m_rMaxShipDistanceThreshold = m_rShipDistanceThresholds[0];
  for (int i = 1; i < sizeofArray(m_rShipDistanceThresholds); ++i)
    if (m_rShipDistanceThresholds[i] > m_rMaxShipDistanceThreshold)
      m_rMaxShipDistanceThreshold = m_rShipDistanceThresholds[i];
}

void CPig::CacheMaxShipsUpdateLatencyThresholds()
{
  XLock lock(this);
  m_dwMaxShipsUpdateLatencyThreshold = m_dwShipsUpdateLatencyThresholds[0];
  for (int i = 1; i < sizeofArray(m_dwShipsUpdateLatencyThresholds); ++i)
    if (m_dwShipsUpdateLatencyThresholds[i] > m_dwMaxShipsUpdateLatencyThreshold)
      m_dwMaxShipsUpdateLatencyThreshold = m_dwShipsUpdateLatencyThresholds[i];
}

HRESULT CPig::SendBytes(FedMessaging* pfm, VARIANT* pvBytes, bool bGuaranteed)
{
  // Currently, BSTR is the only supported VARIANT type, must be a filename
  CComVariant vBytes(*pvBytes);
  if (VT_BSTR != V_VT(&vBytes))
    if (FAILED(vBytes.ChangeType(VT_BSTR)))
      return E_INVALIDARG;

  // String cannot be empty
  if (!BSTRLen(V_BSTR(&vBytes)))
    return E_INVALIDARG;

  // Convert the BSTR to an LPCTSTR
  USES_CONVERSION;
	LPCSTR pszFileName = OLE2CA(V_BSTR(&vBytes));

  // Open the file
  ZFile file(pszFileName);
  if (!file.IsValid())
    return E_FAIL;

  // Send the bytes of the file to the specified dplay wrapper
  return pfm->GenericSend(pfm->GetServerConnection(),
    file.GetPointer(), file.GetLength(),
    bGuaranteed ? FM_GUARANTEED : FM_NOT_GUARANTEED);

  // TODO: Fire an event
}


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPig::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPig,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// BaseClient Overrides
HRESULT CPig::OnSessionLost(char* szReason, FedMessaging * pthis)
{
  XLock lock(this);
  if (!m_bDisconnected)
  {
    // Only do this once
    m_bDisconnected = true;

    // Perform default processing
    BaseClient::OnSessionLost(szReason, pthis);

    // Shutdown the pig
    Shutdown();
  }
  return S_OK;
}

void CPig::ModifyShipData(DataShipIGC* pds)
{
  // Set the pig pilot type
  pds->pilotType = c_ptCheatPlayer;
}

void CPig::OnQuitMission(QuitSideReason reason, const char* szMessageParam)
{
  // Perform default processing
  BaseClient::OnQuitMission(reason);

  // Set the event
  if (PigState_QuittingMission == GetCurrentState())
    ::SetEvent(m_evtQuitMission);
  else
    SetCurrentState(PigState_MissionList);
}

const char* CPig::GetArtPath()  
{ 
  CComBSTR bstrPath;
  GetEngine().get_ArtPath(&bstrPath);

  USES_CONVERSION;
  return OLE2CA(bstrPath);
} 

IAutoUpdateSink* CPig::OnBeginAutoUpdate()
{
  return this;
}

bool CPig::ResetStaticData(char* szIGCStaticFile, ImissionIGC** ppStaticIGC,
  Time tNow, bool bEncrypt)
{
  // Lock static variables during BaseClient re-initialization
  XLockStatic lockStatic(&s_csCreate);

  // Delegate to the base class
  return BaseClient::ResetStaticData(szIGCStaticFile, ppStaticIGC, tNow,
    GetIsZoneClub());
}

void CPig::StartLockDown(const ZString& strReason, LockdownCriteria criteria)
{
  // Do nothing to keep the BaseClient implementation from asserting
}

void CPig::EndLockDown(LockdownCriteria criteria)
{
  // Do nothing to keep the BaseClient implementation from asserting
}

void CPig::CreateDummyShip()
{
  // Perform default processing
  BaseClient::CreateDummyShip();

  // Set the IGC pointer of the AGC ship wrapper object
  assert(m_pShip);
  m_pShip->SetIGC(BaseClient::GetShip());
}

void CPig::DestroyDummyShip()
{
  // Remove our previous ship from the IGC/AGC map
  assert(BaseClient::GetShip());
  assert(m_pShip);
  m_pShip->SetIGC(NULL);

  // Perform default processing
  BaseClient::DestroyDummyShip();
}


/////////////////////////////////////////////////////////////////////////////
// IFedMessagingSite Overrides

HRESULT CPig::OnAppMessage(FedMessaging* pthis, CFMConnection & cnxnFrom, FEDMESSAGE* pfm)
{
  // Process the message
  return ProcessAppMessage(pfm);
}

int CPig::OnMessageBox(FedMessaging * pthis, const char* strText, const char* strCaption,
  UINT nType)
{
  UNUSED_ALWAYS(nType);
  _TRACE2("%hs: %hs\n", strCaption, strText);
  return 0;
}

void CPig::OnPreCreate(FedMessaging * pthis)
{

  // Lock a static critcal section while creating DPLAY object
  //CPigDirectPlay::Lock();
}

//void CPig::OnPostCreate(FedMessaging * pthis, IDirectPlayX* pdpIn, IDirectPlayX** pdpOut)
//{
//
//  // Hook the DPLAY object with a delegator, if creation succeeded
//  *pdpOut = pdpIn ? new CPigDirectPlay(pdpIn) : NULL;
//
//  // Unlock a static critical section after creating DPLAY object
//  CPigDirectPlay::Unlock();
//}


/////////////////////////////////////////////////////////////////////////////
// IClientEventSink Overrides

void CPig::OnLogonAck(bool fValidated, bool bRetry, LPCSTR szFailureReason)
{
  XLock lock(this);
  assert(PigState_CreatingMission == GetCurrentState()
		|| PigState_JoiningMission == GetCurrentState()
		|| PigState_WaitingForMission == GetCurrentState()); //imago 10/14
  if (!fValidated)
  {
    m_bLogonAck = fValidated;
    m_evtLogonAck.Set(szFailureReason);
  }
}

void CPig::OnLogonLobbyAck(bool fValidated, bool bRetry, LPCSTR szFailureReason)
{
  XLock lock(this);
  assert(PigState_LoggingOn == GetCurrentState());
  m_bLogonAck = fValidated;
  m_evtLogonLobbyAck.Set(szFailureReason);
}

void CPig::OnAddMission(MissionInfo* pMissionDef)
{
}

void CPig::OnMissionStarted(MissionInfo* pMissionDef)
{
  if (pMissionDef == BaseClient::MyMission())
  {
    // Notify the active behavior, if any
    IPigBehavior* pBehavior = GetActiveBehavior();
    if (pBehavior)
    {
      pBehavior->AddRef();
      m_pth->PostThreadMessage(wm_OnMissionStarted, (WPARAM)pBehavior);
    }
  }
}

void CPig::OnAddPlayer(MissionInfo* pMissionDef, SideID sideID,
  PlayerInfo* pPlayerInfo)
{
  if (pPlayerInfo == BaseClient::MyPlayerInfo())
  {
    PigState eState = GetCurrentState();
    if (PigState_JoiningTeam == eState || PigState_JoiningMission == eState
      || PigState_CreatingMission == eState)
    {
      XLock lock(this);
      m_bTeamAccepted = true;

      XEvent* pEvt = NULL;
      switch (eState)
      {
        case PigState_JoiningTeam    : pEvt = &m_evtJoiningTeam    ; break;
        case PigState_JoiningMission :
          pEvt = (SIDE_TEAMLOBBY == sideID) ? &m_evtJoiningMission : &m_evtJoiningMission2;
          break;
        case PigState_CreatingMission: pEvt = &m_evtCreatingMission; break;
      }
      assert(pEvt);
      pEvt->Set();
    }
  }
}

void CPig::OnDelRequest(MissionInfo* pMissionInfo, SideID sideID,
  PlayerInfo* pPlayerInfo, DelPositionReqReason reason)
{
  if (pPlayerInfo == BaseClient::MyPlayerInfo())
  {
    PigState eState = GetCurrentState();
    if (PigState_JoiningTeam == eState) // || PigState_QuittingMission == eState)
    {
      XLock lock(this);
      m_bTeamAccepted = false;

	  printf("join team rejected: %ld\n", reason);

      m_evtJoiningTeam.Set();
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// IIgcSite Overrides

TRef<ThingSite> CPig::CreateThingSite(ImodelIGC* pModel)
{
  return new CPigsThingSite();
}

TRef<ClusterSite> CPig::CreateClusterSite(IclusterIGC* pCluster)
{
  return new CPigsClusterSite(pCluster);
}

void CPig::ReceiveChat(IshipIGC* pshipSender, ChatTarget ctRecipient,
  ObjectID oidRecipient, SoundID voiceOver, const char* szText,
  CommandID cid, ObjectType otTarget, ObjectID oidTarget,
  ImodelIGC* pmodelTarget, bool bObjectModel)
{
  // If we sent this message, then ignore it
	if (pshipSender && BaseClient::GetShip()->GetObjectID() == pshipSender->GetObjectID()) //imago 10/14 was BaseClient::GetShip() == pshipSender
		return;

	 //imago ignore attack commands in a lifepod 10/14
	if (cid == c_cidAttack && BaseClient::GetShip()->GetHullType()->HasCapability(c_habmLifepod))
		return;

	//imago ignore commands to other stuff 10/14
	if (BaseClient::GetShip()->GetObjectID() != oidRecipient && cid > 0)
    return;

  // Perform default processing
  BaseClient::ReceiveChat(pshipSender, ctRecipient, oidRecipient,voiceOver,
    szText, cid, otTarget, oidTarget);

  // Package the parameters into a chat command structure
  XChatCommand* pChat = new XChatCommand;
  pChat->m_pshipSender  = pshipSender;
  pChat->m_ctRecipient  = ctRecipient;
  pChat->m_oidRecipient = oidRecipient;
  pChat->m_voiceOver    = voiceOver;
  pChat->m_strText      = szText ? szText : "";
  pChat->m_cid          = cid;
  pChat->m_pmodelTarget = GetCore()->GetModel(otTarget, oidTarget);

  // If the message specifies a legal command, queue it
  if (BaseClient::GetShip()->LegalCommand(pChat->m_cid, pChat->m_pmodelTarget))
    BaseClient::GetShip()->SetCommand(c_cmdQueued, pChat->m_pmodelTarget, pChat->m_cid);

  // Defer processing
  m_pth->PostThreadMessage(wm_OnReceiveChat, (WPARAM)pChat);
}

void CPig::SendChat(IshipIGC* pshipSender, ChatTarget chatTarget,
  ObjectID oidRecipient, SoundID soVoiceOver, const char* szText,
  CommandID cid, ObjectType otTarget, ObjectID oidTarget,
  ImodelIGC* pmodelTarget)
{
  // Ensure that pshipSender is not NULL
  if (!pshipSender)
    pshipSender = BaseClient::GetShip();

  // Perform default processing
  BaseClient::SendChat(pshipSender, chatTarget, oidRecipient, soVoiceOver,
    szText, cid, otTarget, oidTarget, pmodelTarget);
}


void CPig::PlayNotificationSound(SoundID soundID, ImodelIGC* model)
{
  if (BaseClient::GetShip() == model)
  {
    switch (soundID)
    {
      case salShipCantRipcordSound:
        m_bstrTryingToRipcord.LoadString(IDS_E_CANT_RIPCORD);
        break;
      case salNoRipcordSound:
        m_bstrTryingToRipcord.LoadString(IDS_E_NO_RIPCORD);
        break;
      case salRipcordAbortedSound:
        m_bstrTryingToRipcord.LoadString(IDS_E_RIPCORD_ABORTED);
        break;
    }
  }
}


int CPig::MessageBox(const ZString& strText, const ZString& strCaption,
  UINT nType)
{
//  LogMessageAndTime(LOG_TYPE_ERROR, (char*)(LPCSTR)strText);
  return 0;
}

void CPig::ChangeStation(IshipIGC* pship, IstationIGC* pstationOld,
  IstationIGC* pstationNew)
{
  if (pship == BaseClient::GetShip())
  {
    PigState eState = GetCurrentState();
    if (pstationNew)
    {
      // Indicate that we've docked
      m_evtDocked.Set();

      // We've docked
      if (PigState_Flying == eState || PigState_WaitingForMission == eState)
        SetCurrentState(PigState_Docked);
    }
    else if (pstationOld)
    {
      // Reset the AutoPilot flag
      BaseClient::SetAutoPilot(false);

      // Get the source ship and base hull type
      assert (pstationOld);
      IshipIGC* pshipSource = BaseClient::GetShip()->GetSourceShip();
      assert (pshipSource);
      const IhullTypeIGC* pht = pshipSource->GetBaseHullType();
      assert (pht);

      //If no weapon is selected, try to select a weapon
			if (pshipSource && pshipSource->GetObjectID() == BaseClient::GetShip()->GetObjectID()) //imago 10/14
      {
        if (BaseClient::m_selectedWeapon >= pht->GetMaxFixedWeapons()
          || !BaseClient::GetWeapon())
            BaseClient::NextWeapon();
      }

      // Reset the Vector Lock bit
      GetShip()->SetStateBits(coastButtonIGC, 0);

      // Reset the controls
      ControlData cd;
      cd.Reset();
      BaseClient::GetShip()->SetControls(cd);

      // We've launched
      if (PigState_Launching == eState)
        m_evtLaunching.Set();
    }
  }

  // Perform default processing
  BaseClient::ChangeStation(pship, pstationOld, pstationNew);
}

void CPig::ChangeCluster(IshipIGC*  pship, IclusterIGC* pclusterOld,
  IclusterIGC* pclusterNew)
{
	if (pship && pship->GetObjectID() == BaseClient::GetShip()->GetObjectID())
  {
    // Only notify the active behavior if both clusters are non-null
    if (pclusterOld && pclusterNew)
    {
      // Determine if there is an active behavior
      CPigBehaviorScript* pBehavior = GetActiveBehavior();
      if (pBehavior)
      {
        // Create the parameter structure
        XSectorChange* pParams = new XSectorChange;
        pParams->m_spBehavior = pBehavior;

        // Get the AGCSector objects
        GetAGCGlobal()->GetAGCObject(pclusterOld, IID_IAGCSector,
          (void**)&pParams->m_spSectorOld);
        GetAGCGlobal()->GetAGCObject(pclusterNew, IID_IAGCSector,
          (void**)&pParams->m_spSectorNew);

        // Notify the active behavior
        m_pth->PostThreadMessage(wm_OnSectorChange, (WPARAM)pParams);
      }
    }

    if (pclusterOld && BaseClient::GetNetwork()->IsConnected())
      pclusterOld->SetActive(false);

    // Perform default processing
    BaseClient::ChangeCluster(pship, pclusterOld, pclusterNew);

    if (pclusterNew && BaseClient::GetNetwork()->IsConnected())
      pclusterNew->SetActive(true);
  }
}

void CPig::TerminateModelEvent(ImodelIGC * pModel)
{
  // Perform default processing
  BaseClient::TerminateModelEvent(pModel);

  // Notify the action objects of each behavior, active or not
  for (XBehaviorIt it = m_Behaviors.begin(); m_Behaviors.end() != it; ++it)
    (*it)->ModelTerminated(pModel);
}

void CPig::DamageShipEvent(Time now, IshipIGC* ship, ImodelIGC* launcher,
  DamageTypeID type, float amount, float leakage, const Vector& p1, const Vector& p2)
{
  // Determine if there is an active behavior
  CPigBehaviorScript* pBehavior = GetActiveBehavior();
  if (pBehavior)
  {
    // Create the parameter structure
    XShipDamaged* pParams = new XShipDamaged;
    pParams->m_spBehavior = pBehavior;
    pParams->m_amount     = amount;
    pParams->m_leakage    = leakage;

    // Get the AGCShip object that was damaged
    if (ship)
      GetAGCGlobal()->GetAGCObject(ship, IID_IAGCShip, (void**)&pParams->m_spShip);

    // Get the AGCModel object that damaged the ship
    if (launcher)
      GetAGCGlobal()->GetAGCObject(launcher, IID_IAGCModel, (void**)&pParams->m_spModel);

    // Create AGCVectors
    _SVERIFYE(GetAGCGlobal()->MakeAGCVector(&p1, &pParams->m_spVector1));
    _SVERIFYE(GetAGCGlobal()->MakeAGCVector(&p2, &pParams->m_spVector2));

    // Notify the active behavior
    m_pth->PostThreadMessage(wm_OnShipDamaged, (WPARAM)pParams);
  }

  // Perform default processing
  BaseClient::DamageShipEvent(now, ship, launcher, type, amount, leakage, p1, p2);
}

//imago changed to fire for any ship 10/14
void CPig::KillShipEvent(Time timeCollision, IshipIGC* ship,
  ImodelIGC* launcher, float amount, const Vector& p1, const Vector& p2)
{

    // Determine if there is an active behavior
    CPigBehaviorScript* pBehavior = GetActiveBehavior();
    if (pBehavior)
    {
      // Create the parameter structure
      XShipKilled* pParams = new XShipKilled;
      pParams->m_spBehavior = pBehavior;
      pParams->m_amount     = amount;


		// Get the AGCShip object that was damaged
		if (ship)
			GetAGCGlobal()->GetAGCObject(ship, IID_IAGCShip, (void**)&pParams->m_spShip);

      // Get the AGCModel object that damaged the ship
      if (launcher)
			GetAGCGlobal()->GetAGCObject(launcher, IID_IAGCModel, (void**)&pParams->m_spModel);

      // Create AGCVectors
      _SVERIFYE(GetAGCGlobal()->MakeAGCVector(&p1, &pParams->m_spVector1));
      _SVERIFYE(GetAGCGlobal()->MakeAGCVector(&p2, &pParams->m_spVector2));

      // Notify the active behavior
      m_pth->PostThreadMessage(wm_OnShipKilled, (WPARAM)pParams);
    }

  // Perform default processing
  BaseClient::KillShipEvent(timeCollision, ship, launcher, amount, p1, p2);
}

void CPig::HitWarpEvent(IshipIGC* ship, IwarpIGC* warp)
{
  _TRACE_BEGIN
    _TRACE_PART2("\nCPig::HitWarpEvent(): ship=%08X (id=%04hX), ", ship,
      ship->GetObjectID());
    _TRACE_PART2("GetShip()=%08X (id=%04hX)\n", BaseClient::GetShip(),
      BaseClient::GetShip()->GetObjectID());
  _TRACE_END

  // Only notify the behavior if the ship was us
		if (ship && BaseClient::GetShip()->GetObjectID() == ship->GetObjectID())
  {
    // Determine if there is an active behavior
    CPigBehaviorScript* pBehavior = GetActiveBehavior();
    if (pBehavior)
    {
      // Create the parameter structure
      XAlephHit* pParams = new XAlephHit;
      pParams->m_spBehavior = pBehavior;

      // Get the AGCAleph object that we hit
      if (warp)
        GetAGCGlobal()->GetAGCObject(warp, IID_IAGCAleph,
          (void**)&pParams->m_spAleph);

      // Notify the active behavior
      m_pth->PostThreadMessage(wm_OnAlephHit, (WPARAM)pParams);
    }
  }

  // Perform default processing
  BaseClient::HitWarpEvent(ship, warp);
}

bool CPig::HitTreasureEvent(Time now, IshipIGC* ship, ItreasureIGC* treasure)
{
  // Perform default processing
  return BaseClient::HitTreasureEvent(now, ship, treasure);
}

//imago 10/14
void CPig::GameOver(SideID iWinner)
{
	SetCurrentState(PigState_WaitingForMission);
}


/*
void CPig::ActivateRipcord(IshipIGC* ship, bool activeF)
{
  // Reset the ship's "keyboard"
  if (!activeF && GetShip() == ship)
    GetShip()->SetStateBits(ripcordActiveIGC, 0);
}
*/

/////////////////////////////////////////////////////////////////////////////
// IPig Interface Methods

void CPig::OnBeginRetrievingFileList()
{
  // Set the current directory to the EXE directory
  TCHAR szFileName[_MAX_PATH];
  GetModuleFileName(NULL, szFileName, sizeofArray(szFileName));
  TCHAR szDrive[_MAX_DRIVE], szPath[_MAX_PATH];
  _tsplitpath(szFileName, szDrive, szPath, NULL, NULL);
  _tmakepath(szFileName, szDrive, szPath, NULL, NULL);
  SetCurrentDirectory(szFileName);
}

bool CPig::ShouldFilterFile(const char* szFileName) // if returns true, then file is not downloaded
{
  char szExt[_MAX_EXT];
  _splitpath(szFileName, NULL, NULL, NULL, szExt);
  if (0 == _stricmp(szExt, ".cvh") || 0 == _stricmp(szExt, ".igc"))
    return false; // do not filter-out these files
  return true; // filter-out the rest
}

void CPig::OnAutoUpdateSystemTermination(bool bErrorOccurred, bool bRestarting)
{
  // Reset the auto-download pointer
  XLock lock(this);
  BaseClient::m_pAutoDownload = NULL;

  // Re-logon to the lobby if update succeeded
  if (!bErrorOccurred)
  {
    if (!bRestarting)
    {
      // Succeed the logon attempt
      m_bLogonAck = true;

      // Trigger an event
      _AGCModule.TriggerEvent(NULL, PigEventID_AutoUpdateSucceeded,
        m_bstrName, -1, -1, -1, 0);

      // Re-logon
      BaseClient::ConnectToLobby(NULL); // NULL == relog in with same ci
    }
    else
    {
      // Fail the logon attempt
      m_bLogonAck = false;

      // Trigger an event since we don't support Restarting
      _AGCModule.TriggerEvent(NULL, PigEventID_AutoUpdateRestart,
        m_bstrName, -1, -1, -1, 0);
    }
  }
  else
  {
    // This should have been set to false in OnError
    assert(!m_bLogonAck);
  }

  // Signal the event
  m_evtLogonLobbyAck.Set(m_evtLogonLobbyAck.m_bstr);
}

void CPig::OnError(char* szErrorMessage)
{
  XLock lock(this);
  m_bLogonAck = false;
  m_evtLogonLobbyAck.m_bstr = szErrorMessage;

  // Trigger an event
  _AGCModule.TriggerEvent(NULL, PigEventID_AutoUpdateError,
    m_bstrName, -1, -1, -1, 1,
    "ErrorMessage", VT_LPSTR, szErrorMessage);
}


/////////////////////////////////////////////////////////////////////////////
// IPig Interface Methods

STDMETHODIMP CPig::get_PigState(PigState* pePigState)
{
  CLEAROUT(pePigState, GetCurrentState());
  return S_OK;
}

STDMETHODIMP CPig::get_PigStateName(BSTR* pbstrPigState)
{
  // Delegate to the engine
  return GetEngine().get_StateName(GetCurrentState(), pbstrPigState);
}

STDMETHODIMP CPig::get_Stack(IPigBehaviorStack** ppStack)
{
  // Create the object, if not already created
  if (NULL == m_spBehaviorStack)
  {
    CComObject<CPigBehaviorStack>* pStack = NULL;
    RETURN_FAILED(pStack->CreateInstance(&pStack));
    IPigBehaviorStackPtr spStack(pStack);
    RETURN_FAILED(pStack->Init(this));
    m_spBehaviorStack = spStack;
  }

  // QI the stack object for the requested interface
  return m_spBehaviorStack->QueryInterface(IID_IPigBehaviorStack,
    (void**)ppStack);
}

STDMETHODIMP CPig::get_Name(BSTR* pbstrName)
{
  CLEAROUT(pbstrName, (BSTR)NULL);
  *pbstrName = m_bstrName.Copy();
  return S_OK;
}

STDMETHODIMP CPig::get_Ship(IPigShip** ppShip)
{
  IPigShipPtr spShip(m_pShip->GetUnknown());
  CLEAROUT(ppShip, (IPigShip*)spShip);
  spShip.Detach();
  return S_OK;
}

STDMETHODIMP CPig::put_UpdatesPerSecond(long nPerSecond)
{
  // Validate the specified parameter
  if (1 > nPerSecond || nPerSecond > 100)
    return Error(IDS_E_INVALID_PERSEC, IID_IPig);

  // Save the new values
  XLock lock(this);
  m_nTicksPerSecond = nPerSecond;
  m_msPerTick = 1000 / m_nTicksPerSecond;

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPig::get_UpdatesPerSecond(long* pnPerSecond)
{
  // Initialize the [out] parameters
  CLEAROUT(pnPerSecond, m_nTicksPerSecond);
  return S_OK;
}

STDMETHODIMP CPig::get_HullTypes(IPigHullTypes** ppHullTypes)
{
  // Initialize the [out] parameters
  CLEAROUT(ppHullTypes, (IPigHullTypes*)NULL);

  // Create a new instance of the collection object
  CComObject<CPigHullTypes>* pHullTypes = NULL;
  RETURN_FAILED(pHullTypes->CreateInstance(&pHullTypes));
  IPigHullTypesPtr spHullTypes(pHullTypes);
  RETURN_FAILED(pHullTypes->Init(BaseClient::GetCore()->GetHullTypes(),
    BaseClient::GetShip()->GetStation()));

  // Detach the new instance to the [out] parameter
  *ppHullTypes = spHullTypes.Detach();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPig::get_Money(AGCMoney* pnMoney)
{
  CLEAROUT(pnMoney, (AGCMoney)BaseClient::GetMoney());
  return S_OK;
}

STDMETHODIMP CPig::Logon()
{
  _TRACE1("CPig::Logon(): GetCurrentThreadId = %u\n", GetCurrentThreadId());
  assert(NULL != m_spAccount);

  // Validate the current state
  if (PigState_NonExistant != GetCurrentState())
    return Error(IDS_E_LOGON_NONEXISTANT, IID_IPig);

  // Get the current lobby mode
  PigLobbyMode eMode;
  ZSucceeded(GetEngine().get_LobbyMode(&eMode));
  BaseClient::SetIsZoneClub(PigLobbyMode_Club == eMode);

  // Initialize the logon acknowledgement status
  XLock lock(this);
  m_evtLogonLobbyAck.Reset();
  m_bLogonAck = false;

  // Set the state to PigState_LoggingOn
  SetCurrentState(PigState_LoggingOn);

  // Copy the lobby server to the connection parameters
  USES_CONVERSION;
  BaseClient::ConnectInfo ci;
  ci.strServer = OLE2CT(GetEngine().GetMissionServer());

  // Big HACK! Due to the Connect method trashing ci.strServer with
  // the CfgInfo value.
  BaseClient::GetCfgInfo().strClubLobby   = ci.strServer;
  BaseClient::GetCfgInfo().strPublicLobby = ci.strServer;

  printf("%S logging in, CDKey is {%S}.\n", m_bstrName, m_bstrCdKey);
  // Send the CDKey when we log in.
  char cCdKey[MAX_PATH];
  StrCpy(cCdKey, OLE2CA(m_bstrCdKey));
  BaseClient::SetCDKey(cCdKey);
  // Copy the pig account name and password to the connection parameters
	Strcpy(ci.szName, OLE2CA(m_bstrName));



	//imago 10/14
	/*CComBSTR bstrPW;
	RETURN_FAILED(m_spAccount->get_Password(&bstrPW));
	LPSTR pszPW = bstrPW.Length() ? OLE2A(bstrPW) : "";
	Strcpy(ci.szPW,pszPW);*/
  
#ifdef USEAUTH
  // Authenticate the account on the Zone authentication server, if any
  TRef<IZoneAuthClient> pzac;
  if (BSTRLen(GetEngine().GetZoneAuthServer()))
  {
    // Get the account's password
    CComBSTR bstrPW;
    RETURN_FAILED(m_spAccount->get_Password(&bstrPW));
    LPSTR pszPW = bstrPW.Length() ? OLE2A(bstrPW) : "";

    // Get the authentication client object
    //pzac = BaseClient::CreateZoneAuthClient();

    // Set the authentication server of the client object
    LPCSTR pszZoneAuthServer = OLE2CT(GetEngine().GetZoneAuthServer());
    pzac->SetAuthServer(pszZoneAuthServer);

    // Authenticate the user name and password
    HRESULT hr = pzac->Authenticate(ci.szName, pszPW, true, false,
      GetEngine().GetZoneAuthTimeout());
    if (FAILED(hr))
    {
      // Format the HRESULT
      char szHRESULT[16];
      sprintf(szHRESULT, "0x%08X", hr);

      // Trigger an event
      _AGCModule.TriggerEvent(NULL, PigEventID_ZoneAuthFailed, m_bstrName,
        -1, -1, -1, 3,
        "ZoneAuthServer", VT_LPSTR, pszZoneAuthServer,
        "HRESULT"       , VT_I4   , hr,
        "0xHRESULT"     , VT_LPSTR, szHRESULT);

      // Return an error
      return Error(IDS_E_ZONEAUTH_FAILED, IID_IPig);
    }

    // Get the authentication ticket
    DWORD cbZoneTicket;
    DWORD cbName = sizeof(ci.szName);
    ZSucceeded(pzac->GetTicket(&ci.pZoneTicket, &cbZoneTicket, ci.szName, &cbName));
    assert(cbName <= sizeof(ci.szName));
    ci.cbZoneTicket = cbZoneTicket;
  }
#endif

  // Logon to the lobby server
  HRESULT hr;
  {
    XLockStatic lock(&s_csLogon);
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ci.ftLastArtUpdate);
    hr = BaseClient::ConnectToLobby(&ci);
  }

#ifdef USEAUTH
  // Free the authentication client object, if any
  if (pzac != NULL)
  {
    //BaseClient::FreeZoneAuthClient();
    pzac = NULL;
  }
#endif

  // Check for success of the lobby connection
  if (FAILED(hr))
  {
    // Trigger an event
    _AGCModule.TriggerEvent(NULL, PigEventID_ConnectFailed, m_bstrName,
      -1, -1, -1, 1,
      "LobbyServer", VT_LPSTR, (LPCSTR)ci.strServer);

    // Return an error
    return Error(IDS_E_CONNECT_FAILED, IID_IPig);
  }

  // Wait for the logon acknowledgement event
  if (!WaitInTimerLoop(m_evtLogonLobbyAck))
    return S_FALSE;

  // Interpret the results of the logon acknowledgement
  if (!m_bLogonAck)
  {
    // Trigger an event
    _AGCModule.TriggerEvent(NULL, PigEventID_LogonDenied, m_bstrName,
      -1, -1, -1, 2,
      "LobbyServer", VT_LPSTR, (LPCSTR)ci.strServer,
      "Reason"     , VT_BSTR , (BSTR)m_evtLogonLobbyAck);

    // Set the state to PigState_NonExistant
    SetCurrentState(PigState_NonExistant);

    // Report the error
    return Error(m_evtLogonLobbyAck, IID_IPig, E_FAIL);
  }

  // Generate a random delay
  srand(GetTickCount());
  int nDelay = 2000 + (rand() % 4000);
  if (!WaitInTimerLoop(NULL, nDelay))
    return S_FALSE;

  // Set the state to PigState_MissionList
  SetCurrentState(PigState_MissionList);
  return S_OK;
}

STDMETHODIMP CPig::Logoff()
{
  // Ignore if state is PigState_NonExistant or PigState_Terminated
  PigState eState = GetCurrentState();
  if (PigState_NonExistant == eState || PigState_Terminated == eState)
    return S_FALSE;

  // Set the state to PigState_LoggingOff
  SetCurrentState(PigState_LoggingOff);

  // Disconnect the ship object
  if (NULL != m_spShip)
  {
    CoDisconnectObject(m_spShip, 0);
    m_spShip = NULL;
  }

  // Only Disconnect if this is a normal termination, otherwise DPLAY can
  //   hang for 30 sec or more on Close if the link really is dead.
  if (!m_bDisconnected)
  {
    // Logoff from the server
    BaseClient::Disconnect();

    // Logoff from the lobby
    BaseClient::DisconnectLobby();
  }

  // Set the state to PigState_NonExistant
  SetCurrentState(PigState_NonExistant);
  return S_OK;
}

STDMETHODIMP CPig::CreateMission(BSTR bstrServer, BSTR bstrAddr, IPigMissionParams* pMissionParams)
{
	/* Imago 10/14
  // Only allowed when the current lobby mode is Zone
  PigLobbyMode eMode;
  ZSucceeded(GetEngine().get_LobbyMode(&eMode));
  if (PigLobbyMode_Club != eMode)
    return Error(IDS_E_LOBBYMODE_OP_UNSUPPORTED, IID_IPig);
	*/

  // Validate the current state
  if (PigState_MissionList != GetCurrentState())
    return Error(IDS_E_CREATEMISSION_MISSIONLIST, IID_IPig);

  // Validate the specified mission parameters
  if (pMissionParams)
  {
    RETURN_FAILED(pMissionParams->Validate());
  }

  // Get the stream of the specified mission parameters data
  IPigMissionParamsPrivatePtr spPrivate(pMissionParams);
  if (NULL == spPrivate)
    return E_UNEXPECTED;
  IStreamPtr spstm;
  RETURN_FAILED(spPrivate->GetData(&spstm));

  // Seek to the start of the stream
  LARGE_INTEGER li = {0, 0};
  RETURN_FAILED(spstm->Seek(li, STREAM_SEEK_SET, NULL));

  // Read the byte count
  UINT cb = 0;
  RETURN_FAILED(spstm->Read(&cb, sizeof(cb), NULL));
  if (sizeof(MissionParams) != cb)
    return E_UNEXPECTED;

  // Read the MissionParams structure from the stream
  MissionParams mp;
  RETURN_FAILED(spstm->Read(&mp, sizeof(mp), NULL));

  // Close the stream and private interface
  spstm = NULL;
  spPrivate = NULL;

  // Initialize the acknowledgement event status
  m_evtCreatingMission.Reset();

  // Set the state to PigState_CreatingMission
  SetCurrentState(PigState_CreatingMission);
	USES_CONVERSION;
	CreateMissionReq(OLE2CA(bstrServer),OLE2CA(bstrAddr),mp.szIGCStaticFile,mp.strGameName); //Imago 10/14

  // Wait for the acknowledgement event
  if (!WaitInTimerLoop(m_evtCreatingMission))
    return S_FALSE;

  // Return to Mission List, if mission could not be created
  if (!m_bTeamAccepted)
  {
    SetCurrentState(PigState_MissionList);
    return Error(IDS_E_CREATEMISSION_NOSERVERS, IID_IPig);
  }

  // Disconnect from the lobby
  BaseClient::DisconnectLobby();

  // Set the state (NOTE: creating a mission automatically joins a team)
  SetCurrentState(PigState_WaitingForMission);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPig::JoinMission(BSTR bstrMissionOrPlayer)
{
  // Validate the current state
  if (PigState_MissionList != GetCurrentState())
    return Error(IDS_E_JOINMISSION_MISSIONLIST, IID_IPig);

  // Return an error if there are no missions to join
  if (!m_mapMissions.GetCount())
    return Error(IDS_E_JOINMISSION_NONE, IID_IPig);

  // Define an iterator type for (BaseClient) mission map
  typedef TMapListWrapper<DWORD, MissionInfo*>::Iterator MissionIt;

  // Find a mission using the specified string, if any
  MissionInfo * pMissionInfo = NULL;
  if (BSTRLen(bstrMissionOrPlayer))
  {
    // Convert the specified string to ANSI
    USES_CONVERSION;
    LPCSTR pszMissionOrPlayer = OLE2CA(bstrMissionOrPlayer);
    int    cchMissionOrPlayer = strlen(pszMissionOrPlayer);

    // Create a local array of the missions with positions available (with matching names)
    std::vector<MissionInfo*> vecAvailableMissions;
    for (MissionIt it(BaseClient::m_mapMissions); !it.End(); it.Next())
    {
      if (it.Value()->GetAnySlotsAreAvailable())
      {
        int cch = strlen(it.Value()->Name());
        if (cch >= cchMissionOrPlayer && 0 == _strnicmp(pszMissionOrPlayer,
          it.Value()->Name(), cchMissionOrPlayer))
            vecAvailableMissions.push_back(it.Value());
      }
    }

    // Choose a random mission of those available that match
    if (vecAvailableMissions.size())
    {
      int nIndex = rand() % vecAvailableMissions.size();
      pMissionInfo = vecAvailableMissions[nIndex];
    }

    // TODO: Find a mission with the specified player, if any
    if (!pMissionInfo)
    {
      // TODO: This is more complex, since we have to join each mission to look for the player
    }

    // Error
    if (!pMissionInfo)
    {
      return Error("Specified game not found or has no positions available", IID_IPig);
    }
  }
  else
  {
    // Create a local array of the missions with positions available
    std::vector<MissionInfo*> vecAvailableMissions;
    for (MissionIt it(BaseClient::m_mapMissions); !it.End(); it.Next())
      if (it.Value()->GetAnySlotsAreAvailable())
        vecAvailableMissions.push_back(it.Value());

    // Fail if no missions have positions available
    if (!vecAvailableMissions.size())
      return Error("No games exist with positions available", IID_IPig);

    // Choose a random mission
    int nIndex = rand() % vecAvailableMissions.size();
    pMissionInfo = vecAvailableMissions[nIndex];
  }
  assert(pMissionInfo);

  // Initialize the acknowledgement event status
  m_evtJoiningMission.Reset();
  m_bTeamAccepted = false;

  // Set the state to PigState_JoiningMission
  SetCurrentState(PigState_JoiningMission);

  BaseClient::JoinMission(pMissionInfo, "");

  // Wait for the acknowledgement event
  if (!WaitInTimerLoop(m_evtJoiningMission))
    return S_FALSE;

  // Note: First acknowledement represents having joined the team lobby side.
  //       So, joining the lobby team of a game does not ever fail.
  assert(m_bTeamAccepted);

  // Initialize the acknowledgement event status
  m_evtJoiningMission2.Reset();

  // Disconnect from the lobby
  BaseClient::DisconnectLobby();

  // Wait for the acknowledgement event FOR A MAXIMUM OF 15 SECONDS
	if (!WaitInTimerLoop(m_evtJoiningMission2, 5000)) //imago 10/14
    return S_FALSE;

  // If the acknowledgement event did not get set, we are on the team lobby
  if (WAIT_TIMEOUT == WaitForSingleObject(m_evtJoiningMission2, 0))
  {
    // Set the state
    SetCurrentState(PigState_TeamList);
  }
  else
  {
    // Set the state
    SetCurrentState(PigState_WaitingForMission);
  }
  
  // Indicate success
  return S_OK;
}

STDMETHODIMP CPig::JoinTeam(BSTR bstrCivName, BSTR bstrTeamOrPlayer)
{
	//imago 10/14
  // Validate the current state
	//if (PigState_TeamList != GetCurrentState())
	//return Error(IDS_E_JOINTEAM_TEAMLIST, IID_IPig);
	USES_CONVERSION;
  // Find a team using the specified string, if any
  SideID idSide = NA;
  if (BSTRLen(bstrTeamOrPlayer))
  {
    // Find a team with the specified name, if any
    for (SideID i = 0; i < BaseClient::MyMission()->NumSides(); ++i)
    {
	  if (!_stricmp(OLE2CA(bstrTeamOrPlayer), BaseClient::MyMission()->SideName(i)))
      {
        if (0 < BaseClient::MyMission()->SideAvailablePositions(i))
        {
          idSide = i;
          break;
        }
      }
      else
      {
        // TODO: Find a player on this team with the specified name, if any
        // ShipList& shl = BaseClient::MyMission()->GetSideInfo()->GetMembers();
      }
    }

	//printf("Found target side: %ld\n", idSide);

    // Error
    if (NA == idSide)
    {
		// try an empty team. Or a team where the leader is the same squad as me
		for (SideID i = 0; i < BaseClient::MyMission()->NumSides(); ++i)
		{
			// if Empty team
			if (BaseClient::MyMission()->SideLeaderShipID(i) == -1) // should be -1 if no leader
			{
				idSide = i;
				break;
			}
			else { // check if the leader is the same squad as me
				ShipID leaderShipId = BaseClient::MyMission()->SideLeaderShipID(i);
				PlayerInfo* leaderPlayerInfo = BaseClient::FindPlayer(leaderShipId);
				// get the squadTag
				ZString leaderName = ZString(leaderPlayerInfo->CharacterName());
				ZString leaderTag = leaderName.RightOf("@");
				ZString myTag = ZString(OLE2CA(m_bstrName)).RightOf("@");
				if (myTag == leaderTag) {
					idSide = i;
					break;
				}
			}
		}
		if (NA == idSide)
		{
			return Error("Specified team not found or has no positions available", IID_IPig);
		}
    }
  }
  else
  {
    // Setting idSide to NA will tell the server to choose the emptiest side
    idSide = NA;
  }

  //printf("Empty team target side: %ld\n", idSide);

  // Create and queue the message to the server
  BaseClient::SetMessageType(c_mtGuaranteed);
  BEGIN_PFM_CREATE(*BaseClient::GetNetwork(), pfm, C, POSITIONREQ)
  END_PFM_CREATE
  pfm->iSide     = idSide;

  // Initialize the acknowledgement event status
  m_evtDocked.Reset();
  m_evtJoiningTeam.Reset();
  m_bTeamAccepted = false;

  // Set the state to PigState_JoiningTeam
  SetCurrentState(PigState_JoiningTeam);

  // Send the message
  BaseClient::SendMessages();

  // Wait for the acknowledgement event
  if (!WaitInTimerLoop(m_evtJoiningTeam))
	return S_FALSE;
 
  // Set the state
  if (m_bTeamAccepted)
  {
		PigState eStateNew = (WAIT_TIMEOUT != WaitForSingleObject(m_evtDocked, 0)) ? PigState_Docked : PigState_WaitingForMission;
    SetCurrentState(eStateNew);
		//imago 10/14
		if (eStateNew == PigState_WaitingForMission && BSTRLen(bstrCivName) && BaseClient::MyPlayerInfo()->IsTeamLeader()) {
			char * civNames[c_cSidesMax] = {'\0'};
			CivID civSelection = NA;
			SideID civSide = BaseClient::GetSideID();
			char * token;
			int i = 0;
			token = strtok((char *)OLE2CA(bstrCivName), ",");
			while(token)
			{
				civNames[i] = token;
				token = strtok(NULL, ",");
				i++;
			}
			for (CivilizationLinkIGC* linkCiv = BaseClient::GetCore()->GetCivilizations()->first();  linkCiv != NULL; linkCiv = linkCiv->next())
            {
				if(!_stricmp(linkCiv->data()->GetName(),civNames[civSide])) {
					civSelection = linkCiv->data()->GetObjectID();
					break;
				}
			}

			FedMessaging &fm = *BaseClient::GetNetwork();

			if (BSTRLen(bstrTeamOrPlayer)) {
				// set the team name
				BaseClient::SetMessageType(c_mtGuaranteed);
				BEGIN_PFM_CREATE(fm, pfmSetTeamInfo, CS, SET_TEAM_INFO)
					END_PFM_CREATE
					pfmSetTeamInfo->sideID = civSide;
				pfmSetTeamInfo->squadID = NA;
				strcpy(pfmSetTeamInfo->SideName, OLE2CA(bstrTeamOrPlayer));

				//BaseClient::SendMessages();
			}

			if (civSelection != NA) {
				BaseClient::SetMessageType(c_mtGuaranteed);
				BEGIN_PFM_CREATE(fm, pfmChangeCiv, CS, CHANGE_TEAM_CIV)
				END_PFM_CREATE
				pfmChangeCiv->iSide = civSide; 
				pfmChangeCiv->civID = civSelection;
				pfmChangeCiv->random = false;
				//BaseClient::SendMessages();
			}
			
			// Turn off AutoAccept
			{
				//AUTO_ACCEPT
				BaseClient::SetMessageType(c_mtGuaranteed);
				BEGIN_PFM_CREATE(*BaseClient::GetNetwork(), pfmSetAutoAccept, CS, AUTO_ACCEPT)
					END_PFM_CREATE
				pfmSetAutoAccept->iSide = civSide;
				pfmSetAutoAccept->fAutoAccept = false;
				//BaseClient::SendMessages();
			}
			
		}

		//if (BaseClient::MyPlayerInfo()->IsTeamLeader())  
		
		
		//if (BaseClient::MyPlayerInfo()->IsMissionOwner()) 
		//{
		//	// Make the mission configuration what the Pigs Like.
		//	
		//	MissionParams currentMissionParams = BaseClient::MyMission()->GetMissionParams();
		//	BaseClient::SetMessageType(c_mtGuaranteed);

		//	BEGIN_PFM_CREATE_ALLOC(*BaseClient::GetNetwork(), pfmMissionParams, CS, MISSIONPARAMS)
		//	END_PFM_CREATE
		//	pfmMissionParams->missionparams.Reset();
		//	
		//	//strcpy(pfmMissionParams->missionparams.strGameName, BaseClient::MyMission()->Name());
		//	//strcpy(pfmMissionParams->missionparams.strGameName, "Pig Pen");
		//	strcpy(pfmMissionParams->missionparams.strGameName, currentMissionParams.strGameName);
		//	strcpy(pfmMissionParams->missionparams.szIGCStaticFile, currentMissionParams.szIGCStaticFile);
		//	strcpy(pfmMissionParams->missionparams.szCustomMapFile, currentMissionParams.szCustomMapFile);
		//	pfmMissionParams->missionparams.nTotalMaxPlayersPerGame = currentMissionParams.nTotalMaxPlayersPerGame;
		//	pfmMissionParams->missionparams.bEjectPods = false;
		//	pfmMissionParams->missionparams.bAllowDevelopments = false;
		//	pfmMissionParams->missionparams.fStartCountdown = 9.0f;
		//	pfmMissionParams->missionparams.fRestartCountdown = 9.0f;
		//	pfmMissionParams->missionparams.mmMapType = c_mmBrawl;
		//	pfmMissionParams->missionparams.bAllowEmptyTeams = true;
		//	pfmMissionParams->missionparams.dtGameLength = 60.0f * 60.0f; // 1 hour
		//	pfmMissionParams->missionparams.iMaxImbalance = 32767; // 32767 = N/A
		//	printf("Attempting to change the mission paramerters\n");
		//	ZString errorMsg = ZString(pfmMissionParams->missionparams.Invalid(true));
		//	if (errorMsg.IsEmpty()) {
		//		printf("No error message from InValid()\n");
		//	}
		//	else {
		//		printf(errorMsg);
		//	}
		//	BaseClient::GetNetwork()->QueueExistingMsg((FEDMESSAGE *)pfmMissionParams);
		//	PFM_DEALLOC(pfmMissionParams);
		//}

		BaseClient::SendMessages();
  }
  else
  {
    SetCurrentState(PigState_TeamList);
    return Error("The team did not accept you", IID_IPig);
  }

  return S_OK;
}

STDMETHODIMP CPig::QuitGame()
{
  // Validate the current state
  if (PigState_TeamList != GetCurrentState())
    return Error(IDS_E_QUITMISSION_TEAMLIST, IID_IPig);

  // Initialize the acknowledgement event status
  m_evtQuitMission.Reset();

  // Create and queue the message to the server
  BaseClient::QuitMission();

  // Set the state to PigState_QuittingMission
  SetCurrentState(PigState_QuittingMission);

  // Send the message
  BaseClient::SendMessages();

  // Wait for the acknowledgement event
  if (!WaitInTimerLoop(m_evtQuitMission))
    return S_FALSE;

  // Set the state
  SetCurrentState(PigState_MissionList);
  return S_OK;
}

STDMETHODIMP CPig::Launch()
{

  // Validate the current state
  if (PigState_Docked != GetCurrentState())
    return Error(IDS_E_LAUNCH_DOCKED, IID_IPig);

  // Wait up to 30 seconds until pings are computed
  int cWaits = 15;
  while (!m_serverOffsetValidF && cWaits--)
  {
    _TRACE0("CPig::Launch: m_serverOffsetValidF is false!\n");
    WaitInTimerLoop(NULL, 2000);
  }
  if (!m_serverOffsetValidF)
  {
    assert(!"m_serverOffsetValidF never got set!");
    return E_UNEXPECTED;
  }

  // Create and queue the message to the server
  BaseClient::SetMessageType(c_mtGuaranteed);
  BEGIN_PFM_CREATE(*BaseClient::GetNetwork(), pfm, C, DOCKED)
  END_PFM_CREATE
  pfm->stationID = NA;

  // Initialize the acknowledgement event status
  m_evtLaunching.Reset();

  // Set the state to PigState_Launching
  SetCurrentState(PigState_Launching);

  // Send the message
  BaseClient::SendMessages();

  // Wait for the acknowledgement event
  if (!WaitInTimerLoop(m_evtLaunching))
    return S_FALSE;

  // Note: Launching should not ever fail.

  // Set the state
  SetCurrentState(PigState_Flying);

  // Indicate success
  return S_OK;
}

//imago 10/14
STDMETHODIMP CPig::IsMissionOwner(BOOL* bOwner)
{
	XLock lock(this);
	CLEAROUT(bOwner, (BOOL)BaseClient::MyPlayerInfo()->IsMissionOwner());
	return S_OK;
}

//imago 10/14
STDMETHODIMP CPig::IsTeamLeader(BOOL* bLeader)
{
	XLock lock(this);
	CLEAROUT(bLeader, (BOOL)BaseClient::MyPlayerInfo()->IsTeamLeader());
	return S_OK;
}

//imago 10/14
STDMETHODIMP CPig::SetSkills(float fShoot, float fTurn, float fGoto)
{
	XLock lock(this);
	BaseClient::MyPlayerInfo()->GetShip()->SetSkills(fShoot, fTurn, fGoto);
	return S_OK;
}

STDMETHODIMP CPig::Shutdown()
{
  // Shut down during the next message loop iteration
  XLock lock(this);
  m_pth->PostThreadMessage(wm_Shutdown, 1);

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPig::get_Game(IAGCGame** ppGame)
{
  return GetAGCGlobal()->GetAGCObject(BaseClient::GetCore(), IID_IAGCGame,
    (void**)ppGame);
}

STDMETHODIMP CPig::get_Me(IAGCShip** ppShip)
{
  CLEAROUT(ppShip, (IAGCShip*)NULL);
  if (NULL != m_spShipChatOrigin)
    (*ppShip = m_spShipChatOrigin)->AddRef();
  return S_OK;
}

STDMETHODIMP CPig::get_ChatTarget(AGCChatTarget* peChatTarget)
{
  CLEAROUT(peChatTarget, m_eChatTarget);
  return S_OK;
}

STDMETHODIMP CPig::get_Host(IPigBehaviorHost** ppHost)
{
  // Create the host object, if it's NULL
  if (NULL == m_spHost)
  {
    CComObject<CPigBehaviorHost>* pHost = NULL;
    RETURN_FAILED(pHost->CreateInstance(&pHost));
    m_spHost = pHost;
    _SVERIFYE(pHost->Init(this));
  }

  // Copy and AddRef the Host reference to the [out] parameter
  CLEAROUT(ppHost, (IPigBehaviorHost*)m_spHost);
  (*ppHost)->AddRef();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPig::StartCustomGame(IPigMissionParams* pMissionParams)
{
  // Validate the current state
  if (PigState_WaitingForMission != GetCurrentState())
    return Error(IDS_E_STARTGAME_MISSIONLIST, IID_IPig);

  // Validate that the minimum number of players exist on each team
  for (SideID i = 0; i < BaseClient::MyMission()->NumSides(); ++i)
    if (BaseClient::MyMission()->SideNumPlayers(i)
      < BaseClient::MyMission()->MinPlayersPerTeam())
        return Error(IDS_E_STARTGAME_MINPLAYERS, IID_IPig);


	// Validate the specified mission parameters
	if (pMissionParams)
	{
		RETURN_FAILED(pMissionParams->Validate());
		// Get the stream of the specified mission parameters data
		IPigMissionParamsPrivatePtr spPrivate(pMissionParams);
		if (NULL == spPrivate)
			return E_UNEXPECTED;
		IStreamPtr spstm;
		RETURN_FAILED(spPrivate->GetData(&spstm));

		// Seek to the start of the stream
		LARGE_INTEGER li = {0, 0};
		RETURN_FAILED(spstm->Seek(li, STREAM_SEEK_SET, NULL));

		// Read the byte count
		UINT cb = 0;
		RETURN_FAILED(spstm->Read(&cb, sizeof(cb), NULL));
		if (sizeof(MissionParams) != cb)
			return E_UNEXPECTED;

		// Read the MissionParams structure from the stream
		MissionParams mp;
		RETURN_FAILED(spstm->Read(&mp, sizeof(mp), NULL));

		// BT - Hack fix to ensure that create custom game message matches current fedsrv check. 
		// TODO: Move this to read registry
		mp.nTotalMaxPlayersPerGame = 200;

		// Close the stream and private interface
		spstm = NULL;
		spPrivate = NULL;

		// Create and queue the message to the server
		BaseClient::SetMessageType(c_mtGuaranteed);
		BEGIN_PFM_CREATE(*BaseClient::GetNetwork(), pfmParams, CS, MISSIONPARAMS)
		END_PFM_CREATE
		CopyMemory(&pfmParams->missionparams, &mp, sizeof(mp));

		// Send the message
		BaseClient::SendMessages();
	}

	// Create and queue the message to the server
	BaseClient::SetMessageType(c_mtGuaranteed);
	BEGIN_PFM_CREATE(*BaseClient::GetNetwork(), pfm, C, START_GAME)
	END_PFM_CREATE

	// Send the message
	BaseClient::SendMessages();

	// Wait (in intervals) for the game to start
	while (PigState_WaitingForMission == GetCurrentState())
		if (!WaitInTimerLoop(NULL, 500))
			return S_FALSE;

	// Indicate success
	return S_OK;
}

STDMETHODIMP CPig::StartGame()
{
	// Validate the current state
	//if (PigState_WaitingForMission != GetCurrentState())
		//return Error(IDS_E_STARTGAME_MISSIONLIST, IID_IPig);

	// Make which ever pig calls start game the side leader. Sometimes pigs join out of order so the mission owner is not the side leader.
	//BaseClient::MyMission()->SetSideLeader(BaseClient::MyPlayerInfo());

	// Validate that the minimum number of players exist on each team
	for (SideID i = 0; i < BaseClient::MyMission()->NumSides(); ++i)
	{
		// Ensure that all sides are ready to launch, even if the humans didn't get a chance to ready up. Pigs wait for no one.
		//BaseClient::MyMission()->SideForceReady(i);

		//if (BaseClient::MyMission()->SideReady(i) == false)
		//	return Error(IDS_E_STARTGAME_SIDE_NOT_READY, IID_IPig);

		if (BaseClient::MyMission()->SideNumPlayers(i)
			< BaseClient::MyMission()->MinPlayersPerTeam())
			return Error(IDS_E_STARTGAME_MINPLAYERS, IID_IPig);
	}

  // Create and queue the message to the server
  BaseClient::SetMessageType(c_mtGuaranteed);
  BEGIN_PFM_CREATE(*BaseClient::GetNetwork(), pfm, C, START_GAME)
  END_PFM_CREATE

  // Send the message
  BaseClient::SendMessages();

  // Wait (in intervals) for the game to start
  while (PigState_WaitingForMission == GetCurrentState())
    if (!WaitInTimerLoop(NULL, 500))
      return S_FALSE;

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPig::put_ShipAngleThreshold1(float rRadians)
{
  XLock lock(this);
  m_rShipAngleThresholds[0] = fabs(rRadians);
  CacheMaxShipAngleThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipAngleThreshold1(float* prRadians)
{
  XLock lock(this);
  CLEAROUT(prRadians, m_rShipAngleThresholds[0]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipAngleThreshold2(float rRadians)
{
  XLock lock(this);
  m_rShipAngleThresholds[1] = fabs(rRadians);
  CacheMaxShipAngleThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipAngleThreshold2(float* prRadians)
{
  XLock lock(this);
  CLEAROUT(prRadians, m_rShipAngleThresholds[1]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipAngleThreshold3(float rRadians)
{
  XLock lock(this);
  m_rShipAngleThresholds[2] = fabs(rRadians);
  CacheMaxShipAngleThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipAngleThreshold3(float* prRadians)
{
  XLock lock(this);
  CLEAROUT(prRadians, m_rShipAngleThresholds[2]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipDistanceThreshold1(float rDistance)
{
  XLock lock(this);
  m_rShipDistanceThresholds[0] = fabs(rDistance);
  CacheMaxShipDistanceThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipDistanceThreshold1(float* prDistance)
{
  XLock lock(this);
  CLEAROUT(prDistance, m_rShipDistanceThresholds[0]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipDistanceThreshold2(float rDistance)
{
  XLock lock(this);
  m_rShipDistanceThresholds[1] = fabs(rDistance);
  CacheMaxShipDistanceThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipDistanceThreshold2(float* prDistance)
{
  XLock lock(this);
  CLEAROUT(prDistance, m_rShipDistanceThresholds[1]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipDistanceThreshold3(float rDistance)
{
  XLock lock(this);
  m_rShipDistanceThresholds[2] = fabs(rDistance);
  CacheMaxShipDistanceThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipDistanceThreshold3(float* prDistance)
{
  XLock lock(this);
  CLEAROUT(prDistance, m_rShipDistanceThresholds[2]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipsUpdateLatencyThreshold1(long nMilliseconds)
{
  XLock lock(this);
  m_dwShipsUpdateLatencyThresholds[0] = (DWORD)nMilliseconds;
  CacheMaxShipsUpdateLatencyThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipsUpdateLatencyThreshold1(long* pnMilliseconds)
{
  XLock lock(this);
  CLEAROUT(pnMilliseconds, (long)m_dwShipsUpdateLatencyThresholds[0]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipsUpdateLatencyThreshold2(long nMilliseconds)
{
  XLock lock(this);
  m_dwShipsUpdateLatencyThresholds[1] = (DWORD)nMilliseconds;
  CacheMaxShipsUpdateLatencyThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipsUpdateLatencyThreshold2(long* pnMilliseconds)
{
  XLock lock(this);
  CLEAROUT(pnMilliseconds, (long)m_dwShipsUpdateLatencyThresholds[1]);
  return S_OK;
}

STDMETHODIMP CPig::put_ShipsUpdateLatencyThreshold3(long nMilliseconds)
{
  XLock lock(this);
  m_dwShipsUpdateLatencyThresholds[2] = (DWORD)nMilliseconds;
  CacheMaxShipsUpdateLatencyThresholds();
  return S_OK;
}

STDMETHODIMP CPig::get_ShipsUpdateLatencyThreshold3(long* pnMilliseconds)
{
  XLock lock(this);
  CLEAROUT(pnMilliseconds, (long)m_dwShipsUpdateLatencyThresholds[2]);
  return S_OK;
}

STDMETHODIMP CPig::SendBytesToLobby(VARIANT* pvBytes, VARIANT_BOOL bGuaranteed)
{
  if (!BaseClient::GetFMLobby()->IsConnected())
    return Error(IDS_E_SENDBYTES_NOLOBBY, IID_IPig);

  // Delegate to common method
  return SendBytes(BaseClient::GetFMLobby(), pvBytes, !!bGuaranteed);
}

STDMETHODIMP CPig::SendBytesToGame(VARIANT* pvBytes, VARIANT_BOOL bGuaranteed)
{
  if (!BaseClient::GetNetwork()->IsConnected())
    return Error(IDS_E_SENDBYTES_NOGAME, IID_IPig);

  // Delegate to common method
  return SendBytes(BaseClient::GetNetwork(), pvBytes, !!bGuaranteed);
}

