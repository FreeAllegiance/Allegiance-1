/////////////////////////////////////////////////////////////////////////////
// PigEngine.cpp: Implementation of the CPigEngine class.
//

#include "pch.h"
#include <TCLib.h>
#include "PigEngine.h"
#include "PigBehaviorType.h"
#include "PigBehaviorScriptType.h"
#include "Pig.h"
#include "Pigs.h"
#include <zreg.h>


/////////////////////////////////////////////////////////////////////////////
// Static Initialization

#define CPigEngine_StateName(stateName)                                     \
  TCLookupTable_ENTRY(PigState_##stateName, OLESTR(#stateName))

TCLookupTable_BEGIN(CPigEngine, StateNames)
  CPigEngine_StateName(NonExistant)
  CPigEngine_StateName(LoggingOn)
  CPigEngine_StateName(LoggingOff)
  CPigEngine_StateName(MissionList)
  CPigEngine_StateName(CreatingMission)
  CPigEngine_StateName(JoiningMission)
  CPigEngine_StateName(QuittingMission)
  CPigEngine_StateName(TeamList)
  CPigEngine_StateName(JoiningTeam)
  CPigEngine_StateName(WaitingForMission)
  CPigEngine_StateName(Docked)
  CPigEngine_StateName(Launching)
  CPigEngine_StateName(Flying)
  CPigEngine_StateName(Terminated)
TCLookupTable_END()

CPigEngine* CPigEngine::m_pInstance = NULL;


/////////////////////////////////////////////////////////////////////////////
// Construciton / Destruction

CPigEngine::CPigEngine() :
  m_dwRefs(0),
  m_pth(NULL),
  m_pPigs(NULL),
  m_nZoneAuthTimeout(6000),
  m_eLobbyMode(PigLobbyMode_Free)//PigLobbyMode_Club
{
}

CPigEngine::~CPigEngine()
{
  XLock lock(this);

  // Release the Pigs collection object
  if (m_pPigs)
    m_pPigs->Release();

  // Signal the thread to exit and wait for it
  if (m_pth)
    m_pth->SignalExitAndWait();

  // Unload all of the scripts
  for (XScriptMapIt it = m_mapScripts.begin(); it != m_mapScripts.end(); ++it)
    UnloadScriptFile(it, false);
  m_mapScripts.clear();

  // Close the event logger, if any
  IAGCEventLoggerPtr spEventLogger(m_spEventLogger);
  if (NULL != spEventLogger)
  {
    // Release the event logger
    m_spEventLogger = NULL;

    // Terminate the event logger
    lock.Unlock();
    ZSucceeded(IAGCEventLoggerPrivatePtr(spEventLogger)->Terminate());
  }
}

HRESULT CPigEngine::Construct()
{
  // Get the path name of the module
  TCHAR szModule[_MAX_PATH];
  _VERIFYE(GetModuleFileName(_Module.GetModuleInstance(), szModule,
    sizeofArray(szModule)));

  // Break the path name of the module into pieces
  TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szName[_MAX_FNAME];
  _tsplitpath(szModule, szDrive, szDir, szName, NULL);

  // Create the event logger object
  RETURN_FAILED(m_spEventLogger.CreateInstance("AGC.EventLogger"));

  // Initialize the event logger object
  CComBSTR bstrEventSource(L"PigSrv");
  CComBSTR bstrRegKey(L"HKCR\\AppID\\{F132B4E3-C6EF-11D2-85C9-00C04F68DEB0}");
  IAGCEventLoggerPrivatePtr spPrivate(m_spEventLogger);
  RETURN_FAILED(spPrivate->Initialize(bstrEventSource, bstrRegKey));

  // Open registry
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ));


  // Load the MissionServer registry value - This is the lobby server's IP address.
  LoadRegString(key, TEXT("MissionServer"), m_bstrMissionServer);

  // Load the AccountServer registry value
  LoadRegString(key, TEXT("AccountServer"), m_bstrAccountServer);

  // Load the ZoneAuthServer registry value
  LoadRegString(key, TEXT("ZoneAuthServer"), m_bstrZoneAuthServer);

  // Load the ZoneAuthTimeout registry value
  DWORD dwZoneAuthTimeout;
  if (ERROR_SUCCESS == key.QueryValue(dwZoneAuthTimeout, TEXT("ZoneAuthTimeout")))
	  m_nZoneAuthTimeout = static_cast<long>(dwZoneAuthTimeout);
  else
	  m_nZoneAuthTimeout = 99999999;

  // Load the LobbyMode registry value
  DWORD dwLobbyMode;
  if (ERROR_SUCCESS == key.QueryValue(dwLobbyMode, TEXT("LobbyMode")))
    m_eLobbyMode =
      (PigLobbyMode_Club <= dwLobbyMode && dwLobbyMode <= PigLobbyMode_Free) ?
        static_cast<PigLobbyMode>(dwLobbyMode) : PigLobbyMode_Club;

  // Attempt to read the ScriptDir value from the registry
  ZString strScriptDir;
  LoadRegString(key, TEXT("ScriptDir"), strScriptDir);

  // Create a directory name from the root directory, by default
  if (strScriptDir.IsEmpty())
  {
    TCHAR szScriptDir[_MAX_PATH + 1];
    _tmakepath(szScriptDir, szDrive, szDir, TEXT("Scripts"), NULL);
    strScriptDir = szScriptDir;
  }

  // Ensure that m_bstrScriptDir ends with a whack
  int nLastChar = strScriptDir.GetLength() - 1;
  if (TEXT('\\') != strScriptDir[nLastChar])
    strScriptDir += ZString("\\");

  // Save the directory name
  m_bstrScriptDir = strScriptDir;

  // Create the pigs collection object with a ref count
  assert(!m_pPigs);
  CComObject<CPigs>* pPigs = NULL;
  RETURN_FAILED(pPigs->CreateInstance(&pPigs));
  (m_pPigs = pPigs)->AddRef();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Attributes

CPigBehaviorScriptType* CPigEngine::GetBehaviorType(BSTR bstrType)
{
  // Find a behavior type that goes by the specified name
  XLock lock(this);
  USES_CONVERSION;
  XBehaviorMapIt it = m_mapBehaviors.find(tstring(OLE2CT(bstrType)));
  if (m_mapBehaviors.end() == it)
    return NULL;

  // Return the located behavior type
  return it->second;
}


/////////////////////////////////////////////////////////////////////////////
// Operations

HRESULT CPigEngine::Initialize(CPigSession* pSession)
{
  XLock lock(this);
  #ifdef _ATL_DEBUG_INTERFACES
    _TRACE1("%d> CPigEngine::Initialize()\n", m_dwRefs + 1);
  #endif
  HRESULT hr = m_dwRefs++ ? m_dwRefs : Construct();
  if (SUCCEEDED(hr) && pSession)
    m_Sessions.insert(pSession);
  return hr;
}

DWORD CPigEngine::Uninitialize(CPigSession* pSession)
{
  XLock lock(this);
  DWORD dwRefs = --m_dwRefs;
  if (pSession)
  {
    XSessionIt it = m_Sessions.find(pSession);
    if (m_Sessions.end() != it)
      m_Sessions.erase(it);
  }
  lock.Unlock();
  #ifdef _ATL_DEBUG_INTERFACES
    _TRACE1("%d< CPigEngine::Uninitialize()\n", dwRefs);
  #endif
  if (0 == dwRefs)
  {
    delete this;
    InterlockedExchange((long*)&m_pInstance, 0);
    GetGIT().Release();
  }
  return dwRefs;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

unsigned CALLBACK CPigEngine::ScriptDirThunk(void* pv)
{
  // Typecast the parameter as the 'this' pointer
  CPigEngine* pThis = reinterpret_cast<CPigEngine*>(pv);

  // Delegate to the non-static method
  pThis->ScriptDirMonitor(pThis->m_pth->m_hEventExit);
  return 0;
}

void CPigEngine::ScriptDirMonitor(HANDLE hevtExit)
{
  assert(m_hDirChange && INVALID_HANDLE_VALUE != m_hDirChange);

  // Enter this thread into the MTA
  _SVERIFYE(CoInitializeEx(NULL, COINIT_MULTITHREADED));

  // Declare an enum and an array of objects on which to wait
  enum {e_DirChange = WAIT_OBJECT_0, e_Exit, e_Msg};
  HANDLE hObjs[] = {m_hDirChange, hevtExit};
  const int cObjs = sizeofArray(hObjs);

  // Process change notification until exit
  UINT idTimer = 0;
  DWORD dwWait = MsgWaitForMultipleObjects(cObjs, hObjs, false, INFINITE,
    QS_ALLINPUT);
  while (e_Exit != dwWait)
  {
    if (e_DirChange == dwWait)
    {
      // Reset the timer
      if (idTimer)
        KillTimer(NULL, idTimer);
      idTimer = SetTimer(NULL, 0, 2000, NULL);

      // Continue waiting for change notifications
      if (!FindNextChangeNotification(m_hDirChange))
      {
        #ifdef _DEBUG
          DWORD dwLastError = ::GetLastError();
          debugf("\nCPigEngine::ScriptDirMonitor(): "
            "FindNextChangeNotification Failed : "
            "GetLastError() = 0x%08X, m_hDirChange = 0x%08X\n",
            dwLastError, m_hDirChange);
        #endif // _DEBUG
        Sleep(1000);
      }
    }
    else // (e_Msg == dwWait)
    {
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        if (WM_TIMER == msg.message)
        {
          assert(msg.wParam == idTimer);

          // Kill the timer
          KillTimer(NULL, idTimer);
          idTimer = 0;

          // Process the files in the directory
          ProcessScriptDirChanges();
        }
        else if (WM_QUIT == msg.message)
        {
          dwWait = e_Exit;
          break;
        }
      }
    }

    // Continue waiting
    dwWait = MsgWaitForMultipleObjects(cObjs, hObjs, false, INFINITE,
      QS_ALLINPUT);
  }

  // Remove this thread from the MTA
  CoUninitialize();

  // Clear the thread pointer
  InterlockedExchange((long*)&m_pth, 0);
}

HRESULT CPigEngine::EnsureScriptsAreLoaded()
{
  // Return immediately if this initialization has already completed
  XLock lock(this);
  if (!m_hDirChange.IsNull() && m_pth)
    return S_OK;


  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ));

  TCHAR szArtPath[_MAX_PATH];
  DWORD cch = sizeofArray(szArtPath);
  long lr = key.QueryValue(szArtPath, TEXT("ArtPath"), &cch);
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  UTL::SetArtPath(szArtPath);

  // Convert the directory name to ANSI
  USES_CONVERSION;
  LPSTR pszScriptDir = OLE2A(m_bstrScriptDir);

  // Remove the whack at the end of the string
  cch = strlen(pszScriptDir);
  assert('\\' == pszScriptDir[cch - 1]);
  pszScriptDir[cch - 1] = '\0';

  // Ensure that the directory exists
  WIN32_FIND_DATA ffd;
  TCFileFindHandle hff = FindFirstFile(pszScriptDir, &ffd);
  if(hff.IsNull() || INVALID_HANDLE_VALUE == hff)
    return HRESULT_FROM_WIN32(GetLastError());

  // Initialize our record of the directory contents
  ProcessScriptDirChanges();

  // Create a directory change notification object
  const DWORD dwFilter = FILE_NOTIFY_CHANGE_FILE_NAME |
    FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_LAST_WRITE;
  m_hDirChange = FindFirstChangeNotification(pszScriptDir, false, dwFilter);
  if (INVALID_HANDLE_VALUE == m_hDirChange)
    return HRESULT_FROM_WIN32(GetLastError());

  // Create the thread that monitors the ProfileScriptDir folder
  m_pth = TCThread::BeginMsgThread(ScriptDirThunk, this,
    THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
  if (!m_pth)
    return E_FAIL;
  m_pth->ResumeThread();

  // TODO: Create the thread that monitors the CLSID registry key

  // Indicate success
  return S_OK;
}

void CPigEngine::ProcessScriptDirChanges()
{
  XLock lock(this);

  // Copy the map of scripts
  XScriptMap mapScripts(m_mapScripts);

  // Loop thru the "*.pig" files in the script directory
  WIN32_FIND_DATA ffd;
  USES_CONVERSION;
  LPSTR pszScriptDirPatt = OLE2A(m_bstrScriptDir + "*.pig");
  TCFileFindHandle hff = FindFirstFile(pszScriptDirPatt, &ffd);
  bool bContinue = !hff.IsNull() && INVALID_HANDLE_VALUE != hff;
  while (bContinue)
  {
    // Do not process directories, temporary or hidden files
    const DWORD dwIgnoreAttributes = FILE_ATTRIBUTE_DIRECTORY |
      FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_HIDDEN;
    if (!(ffd.dwFileAttributes & dwIgnoreAttributes))
    {
      // Attempt to find the file name in the map of scripts
      tstring strFile(ffd.cFileName);
      XScriptMapIt it = mapScripts.find(strFile);
      if (mapScripts.end() == it)
        AddScriptFile(&ffd, strFile);
      else
      {
        if (it->second->IsModified(&ffd))
          ReloadScriptFile(&ffd, it);
        mapScripts.erase(it);
      }
    }

    // Get the next file in the directory
    bContinue = !!FindNextFile(hff, &ffd);
  }

  // Unload all files remaining in the local copy of the map of Scripts
  for (XScriptMapIt it = mapScripts.begin(); it != mapScripts.end(); ++it)
    UnloadScriptFile(it->first);

  ///////////////////////////////////////////////////////////////////////////
  // File Reconcilation is complete, now validate the files
  ///////////////////////////////////////////////////////////////////////////

  // Create a map of all behavior names, invalid or not
  XBehaviorMap mapGoodBadUgly;
  for (XScriptMapIt it = m_mapScripts.begin(); it != m_mapScripts.end(); ++it)
  {
    CPigBehaviorScriptType* pType = it->second;

    // Get the collection of invoke commands for the new object
    XBehaviorMap mapCommands;
    ZSucceeded(GetInvokeCommands(pType, mapCommands));

    // Add each invoke command to the map
    for (XBehaviorMapIt itCmd = mapCommands.begin(); itCmd != mapCommands.end(); ++itCmd)
    {
      mapGoodBadUgly.insert(*itCmd);
      debugf("inv command: %s\n", itCmd->first.c_str());
    }
  }

  // Check for base behaviors that don't exist
  for (XScriptMapIt it = m_mapScripts.begin(); it != m_mapScripts.end(); ++it)
  {
    CPigBehaviorScriptType* pType = it->second;
    if (pType->IsValid())
    {
      tstring strBaseBehavior(pType->GetBaseBehaviorName());
      if (strBaseBehavior.size() && _tcslen(strBaseBehavior.c_str()))
      {
        XBehaviorMapIt itFind = mapGoodBadUgly.find(strBaseBehavior);
        if (mapGoodBadUgly.end() == itFind)
        {
          RemoveInvokeCommands(pType);
          pType->SetBaseNonExistant();
		  printf("Script %S has a base behavior that doesn't exist.", pType->GetScriptFileName());
        }
      }
    }
  }

  // Check for recursive behavior derivations
  for (XScriptMapIt it = m_mapScripts.begin(); it != m_mapScripts.end(); ++it)
  {
    CPigBehaviorScriptType* pType = it->second;
    if (pType->IsValid())
    {
      std::set<tstring, ci_less> setNames;
      CPigBehaviorScriptType* pTypeBase = pType;
      while (!pTypeBase->GetBaseBehaviorName().empty())
      {
        setNames.insert(pTypeBase->GetName());
        if (setNames.end() == setNames.find(pTypeBase->GetBaseBehaviorName()))
        {
          XScriptMapIt itBase = mapGoodBadUgly.find(pTypeBase->GetBaseBehaviorName());
          if (mapGoodBadUgly.end() != itBase) // KG- fix debug assert - was : m_mapBehaviors.end() != itBase
          {
            pTypeBase = itBase->second;
            continue;
          }
        }

        RemoveInvokeCommands(pType);
        pType->SetRecursionError();
		printf("Script %S has recursive base behavior which is an error.", pType->GetScriptFileName());
        break;
      }
    }
  }

  // Check for behaviors that have base behaviors with errors
  for (XScriptMapIt it = m_mapScripts.begin(); it != m_mapScripts.end(); ++it)
  {
    CPigBehaviorScriptType* pType = it->second;
    if (pType->IsValid())
    {
      std::vector<CPigBehaviorScriptType*> vecBaseTypes;
      vecBaseTypes.push_back(pType);
      tstring strBaseBehavior(pType->GetBaseBehaviorName());
      while (strBaseBehavior.size() && _tcslen(strBaseBehavior.c_str()))
      {
        XBehaviorMapIt itBase = mapGoodBadUgly.find(strBaseBehavior);
        assert(mapGoodBadUgly.end() != itBase);
        pType = itBase->second;
        vecBaseTypes.push_back(pType);
        strBaseBehavior = pType->GetBaseBehaviorName();
      }

      std::vector<CPigBehaviorScriptType*>::reverse_iterator rit1, rit2;
      rit1 = rit2 = vecBaseTypes.rbegin();
      for (++rit2; rit2 != vecBaseTypes.rend(); ++rit1, ++rit2)
      {
        CPigBehaviorScriptType* pTypeBase = *rit1;
        CPigBehaviorScriptType* pType     = *rit2;
        if (!pTypeBase->IsValid() && pType->IsValid())
        {
          RemoveInvokeCommands(pType);
          pType->SetBaseError();
		  printf("Script %S has an error in its base behavior.", pType->GetScriptFileName());
        }
      }
    }
  }

  // Compute the stats on how many good and bad scripts are available
  int nValid = 0;
  for (XScriptMapIt it = m_mapScripts.begin(); it != m_mapScripts.end(); ++it)
    if (it->second->IsValid())
      ++nValid;

  // Display the stats on how many good and bad scripts are available
  #ifdef _DEBUG
    _TRACE_BEGIN
      _TRACE_PART1("CPigEngine::ProcessScriptDirChanges(): %d scripts loaded, ",
        m_mapScripts.size());
      _TRACE_PART1("%d appear to be valid.\n", nValid);
    _TRACE_END
  #endif // _DEBUG

  // Trigger an event
  _AGCModule.TriggerEvent(NULL, PigEventID_ScriptsLoaded,
    L"", -1, -1, -1, 2,
    "TotalCount", VT_I4, m_mapScripts.size(),
    "ValidCount", VT_I4, nValid);
}

void CPigEngine::AddScriptFile(const WIN32_FIND_DATA* pffd,
  tstring strFileName)
{
  // Create a new CPigBehaviorScriptType object
  CComObject<CPigBehaviorScriptType>* pScript = NULL;
  HRESULT hr = pScript->CreateInstance(&pScript);
  ZSucceeded(hr);
  if (SUCCEEDED(hr))
  {
    // AddRef the new object
    pScript->AddRef();

    // Initialize the new object
	printf("Loading script %s\n", strFileName.c_str());
	LoadScriptFile(pScript, pffd, strFileName);

    // Add the new object to the map of scripts
    XLock lock(this);
    m_mapScripts.insert(std::make_pair(strFileName, pScript));

  }
}

void CPigEngine::LoadScriptFile(CPigBehaviorScriptType* pScript,
  const WIN32_FIND_DATA* pffd, tstring strFileName)
{
  // Initialize the specified object
  HRESULT hr = pScript->Load(pffd, strFileName);
  if (SUCCEEDED(hr) && SUCCEEDED(hr = AddInvokeCommands(pScript)))
    return;

  // Display the error information
  IErrorInfo* pei;
  HRESULT hrEI = GetErrorInfo(0, &pei);
  _com_error err(hr, pei);
  _bstr_t bstrDesc(err.Description());
  printf("CPigEngine::LoadScriptFile(%s): %s\n", strFileName.c_str(),
    (bstrDesc.length()) ? LPCTSTR(bstrDesc) : err.ErrorMessage());
}

void CPigEngine::ReloadScriptFile(const WIN32_FIND_DATA* pffd,
  CPigEngine::XScriptMapIt& it)
{
  // Get the name and object pointer
  tstring strFileName = it->first;
  CPigBehaviorScriptType* pScript = it->second;  

  // Remove the script's invoke commands
  RemoveInvokeCommands(pScript);
  printf("Reloading %S ", strFileName);
  // Reload the script file
  LoadScriptFile(pScript, pffd, strFileName);
}

void CPigEngine::UnloadScriptFile(tstring strFileName)
{
  // Find the named item in the map
  XScriptMapIt it = m_mapScripts.find(strFileName);
  assert(m_mapScripts.end() != it);

  // Unload the item
  UnloadScriptFile(it);
}

void CPigEngine::UnloadScriptFile(CPigEngine::XScriptMapIt& it, bool bErase)
{
  // Remove the script's invoke commands
  RemoveInvokeCommands(it->second);

  // Shutdown the object
  it->second->Unload();

  // Release the CPigBehaviorScriptType*
  it->second->Release();

  // Remove the entry from the map
  if (bErase)
    m_mapScripts.erase(it);
}

HRESULT CPigEngine::GetInvokeCommands(CPigBehaviorScriptType* pType,
  CPigEngine::XBehaviorMap& mapCommands)
{
  // Get the collection of invoke commands for the new object
  ITCStringsPtr spInvokeCommands;
  _SVERIFYE(pType->get_InvokeCommands(&spInvokeCommands));
  assert(NULL != spInvokeCommands);

  // Iterate through each one and add it to a local map, ignoring duplicates
  long cInvokeCommands = 0;
  _SVERIFYE(spInvokeCommands->get_Count(&cInvokeCommands));
  for (_variant_t i(0L); V_I4(&i) < cInvokeCommands; ++V_I4(&i))
  {
    CComBSTR bstrCommand;
    _SVERIFYE(spInvokeCommands->get_Item(&i, &bstrCommand));
    USES_CONVERSION;
    tstring strCommand(OLE2CT(bstrCommand));

    XBehaviorMapIt it = mapCommands.find(strCommand);
    if (mapCommands.end() == it)
      mapCommands.insert(std::make_pair(strCommand, pType));
  }
  return S_OK;
}

HRESULT CPigEngine::AddInvokeCommands(CPigBehaviorScriptType* pType)
{
  // Get the collection of invoke commands for the new object
  XBehaviorMap mapCommands;
  ZSucceeded(GetInvokeCommands(pType, mapCommands));

  // Iterate through each one and determine if it's already in the map
  XLock lock(this);
  for (XBehaviorMapIt it = mapCommands.begin(); it != mapCommands.end(); ++it)
  {
    XBehaviorMapIt itFind = m_mapBehaviors.find(it->first);
    if (m_mapBehaviors.end() != itFind)
    {
      pType->put_AppearsValid(VARIANT_FALSE);
      return AtlReportError(CLSID_PigSession, IDS_E_BEHAVIOR_INVCMD_REUSED,
        IID_IPigSession, 0, _Module.GetResourceInstance());
    }
  }

  // Add each one from the local map to the main map
  for (XScriptMapIt it = mapCommands.begin(); it != mapCommands.end(); ++it)
  {
    m_mapBehaviors.insert(*it);
    it->second->AddRef();
  }

  // Indicate success
  return S_OK;
}

void CPigEngine::RemoveInvokeCommands(CPigBehaviorScriptType* pType)
{
  // Get the collection of invoke commands for the object
  ITCStringsPtr spInvokeCommands;
  _SVERIFYE(pType->get_InvokeCommands(&spInvokeCommands));
  assert(NULL != spInvokeCommands);

  // Iterate through each one and erase it from the map
  long cInvokeCommands = 0;
  _SVERIFYE(spInvokeCommands->get_Count(&cInvokeCommands));
  XLock lock(this);
  for (_variant_t i(0L); V_I4(&i) < cInvokeCommands; ++V_I4(&i))
  {
    CComBSTR bstrCommand;
    _SVERIFYE(spInvokeCommands->get_Item(&i, &bstrCommand));
    USES_CONVERSION;
    tstring strCommand(OLE2CT(bstrCommand));
    XBehaviorMapIt it = m_mapBehaviors.find(strCommand);
    if (m_mapBehaviors.end() != it)
    {
      it->second->Release();
      m_mapBehaviors.erase(it);
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// IPigSession Interface Methods (delegated from CPigSession)
  
HRESULT CPigEngine::get_SessionInfos(ITCSessionInfos** ppSessionInfos)
{
  return E_NOTIMPL;
}

HRESULT CPigEngine::put_ScriptDir(BSTR bstrScriptDir)
{
  // Get the number of pigs that exist
  XLock lock(this);
  long cPigs = 0;
  if (m_pPigs)
    RETURN_FAILED(m_pPigs->get_Count(&cPigs));

  // Fail if there are any pigs in existance
  if (cPigs)
    return AtlReportError(CLSID_PigSession, IDS_E_PROPUT_PIGSEXIST,
      IID_IPigSession, 0, _Module.GetResourceInstance());

  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified value
  USES_CONVERSION;
  _bstr_t bstrValue(bstrScriptDir);
  long lr = key.SetValue(OLE2CT(bstrValue), TEXT("ScriptDir"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_ScriptDir(BSTR* pbstrScriptDir)
{
  CLEAROUT(pbstrScriptDir, (BSTR)NULL);
  *pbstrScriptDir = m_bstrScriptDir.copy();
  return S_OK;
}


HRESULT CPigEngine::put_ArtPath(BSTR bstrArtPath)
{
  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified value
  USES_CONVERSION;
  _bstr_t bstrValue(bstrArtPath);
  long lr = key.SetValue(OLE2CT(bstrValue), TEXT("ArtPath"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Set the art path in the Allegiance stuff
  //USES_CONVERSION;
  UTL::SetArtPath(OLE2CA(bstrValue));

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_ArtPath(BSTR* pbstrArtPath)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrArtPath, (BSTR)NULL);

  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ));

  // Read the value
  TCHAR szArtPath[_MAX_PATH];
  DWORD cch = sizeofArray(szArtPath);
  long lr = key.QueryValue(szArtPath, TEXT("ArtPath"), &cch);
  if (ERROR_SUCCESS != lr)
    szArtPath[0] = TEXT('\0');

  // Create a BSTR
  USES_CONVERSION;
  *pbstrArtPath = SysAllocString(T2COLE(szArtPath));

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_BehaviorTypes(IPigBehaviorTypes** ppBehaviorTypes)
{
	// Ensure that the scripts have been loaded
	{
		XLock lock(this);
		RETURN_FAILED(EnsureScriptsAreLoaded());
	}
  return E_NOTIMPL;
}

HRESULT CPigEngine::get_Lobbies(IPigLobbies** ppLobbies)
{
  return E_NOTIMPL;
}

HRESULT CPigEngine::get_Pigs(IPigs** ppPigs)
{
  assert(m_pPigs);

  // Initialize the [out] parameter
  CLEAROUT(ppPigs, (IPigs*)NULL);

  // Query the pigs collection object for the IPigs interface
  return m_pPigs->QueryInterface(IID_IPigs, (void**)ppPigs);
}

HRESULT CPigEngine::CreatePig(BSTR bstrType, BSTR bstrCommandLine, IPig** ppPig)
{
  // Initialize the [out] parameter
  CLEAROUT(ppPig, (IPig*)NULL);

  // Lock the object for the following validation checks
  {
    XLock lock(this);

    // Ensure that the scripts have been loaded
    RETURN_FAILED(EnsureScriptsAreLoaded());

    // Ensure that the MissionServer property is not empty
    if (!m_bstrMissionServer.Length())
      return AtlReportError(CLSID_PigSession, IDS_E_NO_MISSIONSRV,
        IID_IPigSession, 0, _Module.GetResourceInstance());

    // Get the number of pigs that exist
    long cPigs = 0;
    if (m_pPigs)
      _SVERIFYE(m_pPigs->get_Count(&cPigs));

    // Get the maximum number of pigs allowed
    long nMaxPigs = 0;
    _SVERIFYE(get_MaxPigs(&nMaxPigs));

    // Check for maximum pigs
    if (nMaxPigs >= 0 && cPigs >= nMaxPigs)
      return AtlReportError(CLSID_PigSession, IDS_E_TOOMANYPIGS,
        IID_IPigSession, 0, _Module.GetResourceInstance());
  }

  // bstrType is optional, so assign a default value if not specified
  CComBSTR bstrTheType(bstrType);
  if (!bstrTheType.Length())
    bstrTheType = L"Default";

  // Find the specified behavior type
  CPigBehaviorScriptType* pBehaviorType = GetBehaviorType(bstrTheType);
  if (!pBehaviorType)
    return AtlReportError(CLSID_PigSession, IDS_E_CREATE_BEHAVIOR_TYPE,
      IID_IPigSession, 0, _Module.GetResourceInstance());

  // Create a new pig thread object and it's associated pig player object
  DWORD dwGITCookie;
  RETURN_FAILED(CPig::Create(pBehaviorType, bstrCommandLine, &dwGITCookie));

  // Get an apartment-safe pointer to the pig object
  RETURN_FAILED(GetInterfaceFromGlobal(dwGITCookie, IID_IPig, (void**)ppPig));
  assert(*ppPig);

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::put_MissionServer(BSTR bstrServer)
{
  // Get the number of pigs that exist
  XLock lock(this);
  long cPigs = 0;
  if (m_pPigs)
    RETURN_FAILED(m_pPigs->get_Count(&cPigs));

  // Fail if there are any pigs in existance
  if (cPigs)
    return AtlReportError(CLSID_PigSession, IDS_E_PROPUT_PIGSEXIST,
      IID_IPigSession, 0, _Module.GetResourceInstance());

  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified value
  USES_CONVERSION;
  long lr = key.SetValue(BSTRLen(bstrServer) ? OLE2CT(bstrServer) : TEXT(""),
    TEXT("MissionServer"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Save the string
  m_bstrMissionServer = bstrServer;

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_MissionServer(BSTR* pbstrServer)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrServer, (BSTR)NULL);

  // Copy the requested value to the [out] parameters
  XLock lock(this);
  *pbstrServer = m_bstrMissionServer.Copy();

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::put_MaxPigs(long nMaxPigs)
{
  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified value
  long lr = key.SetValue((DWORD)nMaxPigs, TEXT("MaxPigs"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_MaxPigs(long* pnMaxPigs)
{
  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ));

  // Get the requested value
  DWORD nMaxPigs = DWORD(-1);
  key.QueryValue((DWORD&)nMaxPigs, TEXT("MaxPigs"));

  // Copy the requested value to the [out] parameter
  CLEAROUT(pnMaxPigs, (long)nMaxPigs);

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_AccountDispenser(IPigAccountDispenser** ppDispenser)
{
  // Get the computer name of the account server
  CComBSTR bstrServer;
  RETURN_FAILED(get_AccountServer(&bstrServer));
  if (!bstrServer.Length())
  {
    // Get the computer name of the mission server
    RETURN_FAILED(get_MissionServer(&bstrServer));
  }

  // Check for '.' to indicate this computer
  if (1 == bstrServer.Length() && OLESTR('.') == bstrServer[0])
    bstrServer.Empty();

  // Create the Pig Account Dispenser on the mission server
  COSERVERINFO si  = {0, bstrServer, NULL, 0};
  MULTI_QI     mqi = {&IID_IPigAccountDispenser, NULL, S_OK};
  RETURN_FAILED(CoCreateInstanceEx(CLSID_PigAccountDispenser, NULL,
    CLSCTX_SERVER, &si, 1, &mqi));

  // Copy the interface pointer to the [out] parameter
  ZSucceeded(mqi.hr);
  *ppDispenser = (IPigAccountDispenser*)mqi.pItf;

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_EventLog(IAGCEventLogger** ppEventLogger)
{
  CLEAROUT(ppEventLogger, (IAGCEventLogger*)NULL);
  (*ppEventLogger = m_spEventLogger)->AddRef();
  return S_OK;
}

HRESULT CPigEngine::put_AccountServer(BSTR bstrServer)
{
  // Get the number of pigs that exist
  XLock lock(this);
  long cPigs = 0;
  if (m_pPigs)
    RETURN_FAILED(m_pPigs->get_Count(&cPigs));

  // Fail if there are any pigs in existance
  if (cPigs)
    return AtlReportError(CLSID_PigSession, IDS_E_PROPUT_PIGSEXIST,
      IID_IPigSession, 0, _Module.GetResourceInstance());

  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified value
  USES_CONVERSION;
  long lr = key.SetValue(BSTRLen(bstrServer) ? OLE2CT(bstrServer) : TEXT(""),
    TEXT("AccountServer"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Save the string
  m_bstrAccountServer = bstrServer;

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_AccountServer(BSTR* pbstrServer)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrServer, (BSTR)NULL);

  // Copy the requested value to the [out] parameters
  XLock lock(this);
  *pbstrServer = m_bstrAccountServer.Copy();

  // Indicate success
  return S_OK;
}


HRESULT CPigEngine::put_ZoneAuthServer(BSTR bstrServer)
{
  // Get the number of pigs that exist
  XLock lock(this);
  long cPigs = 0;
  if (m_pPigs)
    RETURN_FAILED(m_pPigs->get_Count(&cPigs));

  // Fail if there are any pigs in existance
  if (cPigs)
    return AtlReportError(CLSID_PigSession, IDS_E_PROPUT_PIGSEXIST,
      IID_IPigSession, 0, _Module.GetResourceInstance());

  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified value
  USES_CONVERSION;
  long lr = key.SetValue(BSTRLen(bstrServer) ? OLE2CT(bstrServer) : TEXT(""),
    TEXT("ZoneAuthServer"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Save the string
  m_bstrZoneAuthServer = bstrServer;

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_ZoneAuthServer(BSTR* pbstrServer)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrServer, (BSTR)NULL);

  // Copy the requested value to the [out] parameters
  XLock lock(this);
  *pbstrServer = m_bstrZoneAuthServer.Copy();

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::put_ZoneAuthTimeout(long nMilliseconds)
{
  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified timeout to the registry
  long lr = key.SetValue(static_cast<DWORD>(nMilliseconds), TEXT("ZoneAuthTimeout"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Save the specified timeout
  XLock lock(this);
  m_nZoneAuthTimeout = nMilliseconds;

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_ZoneAuthTimeout(long* pnMilliseconds)
{
  // Initialize the [out] parameter
  XLock lock(this);
  CLEAROUT(pnMilliseconds, m_nZoneAuthTimeout);
  return S_OK;
}

HRESULT CPigEngine::put_LobbyMode(PigLobbyMode eMode)
{
  // Get the number of pigs that exist
  XLock lock(this);
  long cPigs = 0;
  if (m_pPigs)
    RETURN_FAILED(m_pPigs->get_Count(&cPigs));

  // Fail if there are any pigs in existance
  if (cPigs)
    return AtlReportError(CLSID_PigSession, IDS_E_PROPUT_PIGSEXIST,
      IID_IPigSession, 0, _Module.GetResourceInstance());

  // Fail if an invalid mode is specified
  if (PigLobbyMode_Club > eMode || eMode > PigLobbyMode_Free)
    return AtlReportError(CLSID_PigSession, IDS_E_LOBBYMODE_UNSUPPORTED,
      IID_IPigSession, 0, _Module.GetResourceInstance());

  // Open the registry key of the AppID
  CRegKey key;
  RETURN_FAILED(key.Open(HKEY_LOCAL_MACHINE, HKLM_PigSrv, KEY_READ | KEY_WRITE));

  // Save the specified lobby mode to the registry
  long lr = key.SetValue(static_cast<DWORD>(eMode), TEXT("LobbyMode"));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Save the specified lobby mode
  m_eLobbyMode = eMode;

  // Indicate success
  return S_OK;
}

HRESULT CPigEngine::get_LobbyMode(PigLobbyMode* peMode)
{
  // Initialize the [out] parameter
  XLock lock(this);
  CLEAROUT(peMode, m_eLobbyMode);
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IPigHost Interface Methods (delegated from CPigBehaviorHost)

HRESULT CPigEngine::get_StateName(PigState ePigState, BSTR* pbstrPigState)
{
  CLEAROUT(pbstrPigState, (BSTR)NULL);
  LPCOLESTR pszPigState = NULL;
  if (!TCLookupTable_FIND(StateNames, ePigState, pszPigState))
    return E_INVALIDARG;
  *pbstrPigState = SysAllocString(pszPigState);
  return S_OK;
}



