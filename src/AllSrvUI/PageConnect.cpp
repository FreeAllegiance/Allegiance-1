/////////////////////////////////////////////////////////////////////////////
// PageConnect.cpp : implementation file
//

#include "stdafx.h"
#include "AllSrvUI.h"
#include "AllSrvUISheet.h"
#include "PageConnect.h"
#include "..\FedSrv\AdminSessionSecure.h"
#include "..\FedSrv\AdminSessionSecureHost.h"
#include ".\pageconnect.h"

#ifdef _DEBUG
  #define new DEBUG_NEW
  #undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif

//mdvalley: defines good
#define _WIN32_DCOM //Imago removed


/////////////////////////////////////////////////////////////////////////////
// CPageConnect property page

IMPLEMENT_DYNAMIC(CPageConnect, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Static Initializtion

BEGIN_AUTO_SIZER_MAP(_AutoSizerMap)
  AUTO_SIZER_ENTRY(IDC_PROGRESS_CONNECTING_GROUP  ,     Right, 0,   Right, BkRefresh)
  AUTO_SIZER_ENTRY(IDC_PROGRESS_CONNECTING_CAPTION,      Left, 0,    Left, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_PROGRESS_CONNECTING        ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDCANCEL                       ,   HCenter, 0, HCenter, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_LEGAL                      ,     Right, 0,   Right, BkRefresh)
  AUTO_SIZER_ENTRY(IDC_LEGAL                      , TopBottom, 0,  Bottom, BkRefresh)
END_AUTO_SIZER_MAP()


/////////////////////////////////////////////////////////////////////////////
// Message Map

BEGIN_MESSAGE_MAP(CPageConnect, CPropertyPage)
  //{{AFX_MSG_MAP(CPageConnect)
  ON_WM_TIMER()
  //}}AFX_MSG_MAP
  ON_MESSAGE(wm_SessionFailed, OnSessionFailed)
  ON_MESSAGE(wm_SessionSucceeded, OnSessionSucceeded)
  ON_NOTIFY(NM_CUSTOMDRAW, IDC_PROGRESS_CONNECTING, OnNMCustomdrawProgressConnecting)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Construction

CPageConnect::CPageConnect(CAllSrvUISheet *pSheet) :
  CPropertyPage(CPageConnect::IDD),
	m_mpSheet(pSheet)
{
  //{{AFX_DATA_INIT(CPageConnect)
  //}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// Attributes


/////////////////////////////////////////////////////////////////////////////
// Overrides

void CPageConnect::DoDataExchange(CDataExchange* pDX)
{
  // Perform default processing
  CPropertyPage::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(CPageConnect)
  DDX_Control(pDX, IDC_PROGRESS_CONNECTING, m_progress);
  //}}AFX_DATA_MAP
}

LRESULT CPageConnect::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  LRESULT lr;
  if (m_AutoSizer.ProcessMessage(message, wParam, lParam, &lr))
    return lr;

  // Perform default processing
  return CPropertyPage::WindowProc(message, wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

UINT CPageConnect::CreateSessionThreadThunk(void* pvParam)
{
  // Enter this thread into a COM MTA
	//mdvalley: Use older CoInit
 // CoInitialize(NULL); //IMago put back
  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  // Reinterpret the specified parameter as a 'this' pointer
  CPageConnect* pThis = reinterpret_cast<CPageConnect*>(pvParam);

  // Call through the pointer
  pThis->CreateSessionThreadProc();

  // Remove this thread from the COM MTA
  CoUninitialize();

  // Obligatory return code
  return 0;
}

void CPageConnect::CreateSessionThreadProc()
{
  // Create the AdminSession class object
  IAdminSessionClassPtr spClass;

  LPOLESTR lpolestr;
  StringFromCLSID(__uuidof(AdminSession), &lpolestr);
  debugf("one: %s\n", lpolestr);

  StringFromCLSID(__uuidof(spClass), &lpolestr);
  debugf("two: %s\n", lpolestr);

  CoTaskMemFree(lpolestr);

  HRESULT hr = CoGetClassObject(__uuidof(AdminSession), CLSCTX_LOCAL_SERVER,
    NULL, __uuidof(spClass), (void**)&spClass);
  if (FAILED(hr))
  {
    PostMessage(wm_SessionFailed, hr);
    return;
  }

  // Create an instance of the host object for an IAdminSessionHost interface
  AdminSessionSecureHost    xHost;

  // Create an AdminSession object
  IAdminSessionPtr spSession;
  hr = spClass->CreateSession(&xHost, &spSession);
  ::CoDisconnectObject(&xHost, 0);
  if (FAILED(hr))
  {
    PostMessage(wm_SessionFailed, hr);
    return;
  }

  // Get a reference to the GIT
  IGlobalInterfaceTablePtr spGIT;
  hr = spGIT.CreateInstance(CLSID_StdGlobalInterfaceTable);
  if (FAILED(hr))
  {
    PostMessage(wm_SessionFailed, hr);
    return;
  }

  // Add the Session pointer to the GIT
  DWORD dwCookie = 0;
  hr = spGIT->RegisterInterfaceInGlobal(spSession,
    __uuidof(IAdminSession), &dwCookie);
  if (FAILED(hr))
  {
    PostMessage(wm_SessionFailed, hr);
    return;
  }

  TCHandle shEvent = CreateEvent(NULL, false, false, NULL);
  PostMessage(wm_SessionSucceeded, dwCookie, (LPARAM)shEvent.GetHandle());
  WaitForSingleObject(shEvent, INFINITE);
}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

BOOL CPageConnect::OnInitDialog() 
{
  // Perform default processing
  CPropertyPage::OnInitDialog();

  // Initialize the AutoSizer object
  m_AutoSizer.SetWindowAndRules(*this, _AutoSizerMap);

  // Create a worker thread
  m_pthCreateSession = AfxBeginThread(CreateSessionThreadThunk, this,
    THREAD_PRIORITY_NORMAL, 4096);
  _ASSERTE(m_pthCreateSession);

  // Set the range of the progress control
  m_progress.SetRange(0, 60);

  // Set a timer for every half-second
  SetTimer(1, 500, NULL);

  // Set the focus to the first tabstop control
  return false;
}

void CPageConnect::OnTimer(UINT_PTR nIDEvent) 
{
  int nPos = m_progress.GetPos();
  m_progress.SetPos(nPos += 5);
  if (nPos >= 60)
    KillTimer(nIDEvent);

  CPropertyPage::OnTimer(nIDEvent);
}

LRESULT CPageConnect::OnSessionSucceeded(WPARAM wParam, LPARAM lParam)
{
  KillTimer(1);
  m_mpSheet->PostConnect((DWORD)wParam);
  SetEvent((HANDLE)lParam);
  return 0;
}

LRESULT CPageConnect::OnSessionFailed(WPARAM wParam, LPARAM)
{
  KillTimer(1);
  m_mpSheet->HandleError(HRESULT(wParam), "connecting to Game Server", true);
  return 0;
}

// TODO: OnCancel


void CPageConnect::OnNMCustomdrawProgressConnecting(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
