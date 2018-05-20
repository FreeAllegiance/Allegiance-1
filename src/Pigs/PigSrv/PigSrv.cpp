/////////////////////////////////////////////////////////////////////////////
// PigSrv.cpp : Implementation of WinMain
//
// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f PigSrvps.mk in the project directory.

#include "pch.h"
#include "SrcInc.h"
#include <TCLib.h>
#include <..\TCLib\CoInit.h>
#include <..\TCLib\WinApp.h>
#include <AGC.h>
#include "resource.h"
#include "PigSrv.h"
#include "steam_api.h"


/////////////////////////////////////////////////////////////////////////////
// Global Initialization

CAGCModule _AGCModule;
CExeModule _Module;
TCWinApp  g_app;


/////////////////////////////////////////////////////////////////////////////
// External Declarations

TC_OBJECT_EXTERN(CPigSession)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigBehaviorHost)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigBehaviorScript)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigBehaviorScriptType)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPig)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigs)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigTimer)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigShip)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigShipEvent)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigHullTypes)
TC_OBJECT_EXTERN_NON_CREATEABLE(CPigBehaviorStack)
// End: External Declarations


/////////////////////////////////////////////////////////////////////////////
// Object Map

BEGIN_OBJECT_MAP(ObjectMap)
  TC_OBJECT_ENTRY_STD(PigSession)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigBehaviorHost)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigBehaviorScript)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigBehaviorScriptType)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(Pig)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(Pigs)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigTimer)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigShip)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigShipEvent)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigHullTypes)
  TC_OBJECT_ENTRY_STD_NON_CREATEABLE(PigBehaviorStack)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Function Prototypes

HRESULT DoMain(int argc, TCHAR* argv[], TCHAR* envp[]);


/////////////////////////////////////////////////////////////////////////////
// Program Entry Point
//
extern "C" int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	const char * t1 = GetCommandLineA();

  // Initialize the application, if needed
  g_papp->Initialize(GetCommandLine());

  // Just display syntax when the command-line is empty
  if (argc < 2)
    return _Module.Syntax();

  // Enter this thread into a COM apartment
  TCCoInit coInit(COINIT_MULTITHREADED);
  assert(coInit.Succeeded());

  // Initialize AGC
  HRESULT hr = _AGCModule.Init();
  if (FAILED(hr))
    return _Module.ReportError(hr, TEXT("AGC Initialization"));

  // Initialize the ATL _Module object
  _Module.Init(ObjectMap, GetModuleHandle(NULL), &LIBID_PigsLib);

  // Create the set of available AGCEventID ranges
  CComPtr<IAGCEventIDRanges> spRanges;
  hr = spRanges.CoCreateInstance(L"AGC.EventIDRanges");
  if (FAILED(hr))
    return _Module.ReportError(hr, TEXT("AGC Event ID Ranges Creation"));

  // Add our ranges to it
  ZSucceeded(spRanges->AddByValues(EventID_AGC_LowerBound, EventID_AGC_UpperBound));
  ZSucceeded(spRanges->AddByValues(PigEventID_Pigs_LowerBound, PigEventID_Pigs_UpperBound));

  // Set the ranges of available event ID's
  GetAGCGlobal()->SetAvailableEventIDRanges(spRanges);

  // Perform the bulk of processing
  hr = DoMain(argc, argv, envp);

  // Terminate the ATL _Module object
  _Module.Term();

  // Terminate AGC
  _AGCModule.Term();
  
  // Return the last result
  return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
HRESULT DoMain(int argc, TCHAR* argv[], TCHAR* envp[])
{
  // Parse the command line options
  HRESULT hr = _Module.ParseCommandLine(argc, argv);
  if (FAILED(hr))
    return hr;
  if (S_FALSE == hr)
    return S_OK;

  // Initialize security
  if (FAILED(hr = _Module.InitializeSecurity()))
    return _Module.ReportError(hr, TEXT("Security Initialization"));

  // Register our class (factory) objects
  if (FAILED(hr = _Module.RegisterClassObjects()))
  {
    _Module.Term();
    _AGCModule.Term();
    return _Module.ReportError(hr, TEXT("Class Object Registration"));
  }

  // Set the console title
  TCHAR szTitle[_MAX_PATH];
  _stprintf(szTitle, TEXT("Active Pigs - PID=%u"), GetCurrentProcessId());
  SetConsoleTitle(szTitle);

  // Find the console's main window and minimize it
  /*LPCTSTR pszClass = IsWinNT() ? TEXT("ConsoleWindowClass") : TEXT("tty");
  HWND hwnd = FindWindow(pszClass, szTitle);
  if (hwnd)
    CloseWindow(hwnd);*/

  // Change the console title
  SetConsoleTitle(TEXT("Active Pigs"));

  // Display status
  _Module.Echo(TEXT("The server is running. Ctrl+C to exit."));
  _Module.EchoFlush();

  // Handle Ctrl+C and other important exiting events
  SetConsoleCtrlHandler(_Module.ConsoleCtrlHandler, true);

  bool steamInitResult = SteamAPI_Init();
  if (steamInitResult == false)
  {
	  // If you are debugging locally, then you need to put steam_appid.txt with 700480 in it next to the Allegaince EXE that you are debugging.
	  ::MessageBoxA(NULL, "Steam Client is not running. Please launch Steam and try again.", "Error", MB_ICONERROR | MB_OK);
	  ::exit(-1);
  }

  // Enter a message loop (very last Unlock will post WM_QUIT message)
  MSG msg;
  while (GetMessage(&msg, 0, 0, 0))
  {
	  SteamAPI_RunCallbacks();
	  DispatchMessage(&msg);
  }

  // Revoke our class (factory) objects
  _Module.RevokeClassObjects();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Overrides

HRESULT CExeModule::GetAccessSecurity(PSECURITY_DESCRIPTOR* psd, DWORD* pcb)
{
  // Perform default processing under NT
  if (IsWinNT())
    return TCComModuleBase::GetAccessSecurity(psd, pcb);

  // Initialize the [out] parameter
  if (pcb)
    *pcb = 0;

  // HACKHACK: Load the binary data for Win9x access permission
  void* pvData;
  DWORD cbData;
  RETURN_FAILED(LockAndLoadResource(_Module.GetResourceInstance(),
    TEXT("AccessPermission"), MAKEINTRESOURCE(IDR_AccessPermission_9X),
    &pvData, &cbData));

  // Allocate a buffer to be returned
  BYTE* pbData = new BYTE[cbData];
  if (!pbData)
    return E_OUTOFMEMORY;

  // Copy the data into the buffer
  CopyMemory(pbData, pvData, cbData);

  // Copy the values to the [out] parameters
  *psd = pbData;
  if (pcb)
    *pcb = cbData;
  return S_OK;
}

void CExeModule::DisplaySyntax()
{
  Echo(IsWin9x()? IDS_SYNTAX_9X : IDS_SYNTAX_NT);
  EchoFlush();
}


LPCTSTR CExeModule::GetRegInfo(UINT& idResource, bool& bRegTypeLib)
{
  idResource  = IDR_PigSrv;
  bRegTypeLib = false;
  return TEXT("Pigs");
}

_ATL_REGMAP_ENTRY* CExeModule::GetRegMapEntries()
{
  return _AGCModule.GetRegMapEntries();
}

