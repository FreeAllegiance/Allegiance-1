/////////////////////////////////////////////////////////////////////////////
// CAllSrvUISheet.cpp : implementation file
//

#include "stdafx.h"
#include "AllSrvUI.h"
#include "AllSrvUISheet.h"
#include "DlgAbout.h"
#include "DlgGameSelect.h"

#ifdef _DEBUG
  #define new DEBUG_NEW
  #undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAllSrvUISheet dialog


/////////////////////////////////////////////////////////////////////////////
// Message Map

BEGIN_MESSAGE_MAP(CAllSrvUISheet, CPropertySheet)
  //{{AFX_MSG_MAP(CAllSrvUISheet)
  ON_WM_SYSCOMMAND()
  ON_WM_SIZE()
  ON_WM_MOVE()
  ON_WM_TIMER()
  ON_WM_DESTROY()
  ON_WM_CLOSE()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CAllSrvUISheet::CAllSrvUISheet(CWnd* pParentWnd, UINT iSelectPage) :
  CPropertySheet(AFX_IDS_APP_TITLE, pParentWnd, iSelectPage),
  m_dwConnectionCookie(0),
  m_bInitDone(false),
  m_bMultiMode(false),
m_PageConnect(this),
m_PagePlayers(this),
m_PageChat(this),
m_PageGameCreate(this)
{
  // Add a single page
  AddPage(&m_PageConnect);
}

CAllSrvUISheet::~CAllSrvUISheet()
{
  // Disconnect connection point, if connected
  if (m_dwConnectionCookie && NULL != m_spcp)
  {
    m_spcp->Unadvise(m_dwConnectionCookie);
    m_dwConnectionCookie = 0;
  }
}

BOOL CAllSrvUISheet::Create()
{
  // Declare the style of window to use
  static const DWORD dwStyle = WS_VISIBLE | WS_SYSMENU | WS_CAPTION |
    WS_THICKFRAME | WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | DS_MODALFRAME;

  // Perform default processing
  if (!CPropertySheet::Create(NULL, dwStyle))
    return false;

  // Change this flag
  m_nFlags &= WF_CONTINUEMODAL;

  // Indicate success
  return true;
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

void CAllSrvUISheet::InitSysMenu()
{
  // IDM_ABOUTBOX must be in the system command range.
  ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
  ASSERT(IDM_ABOUTBOX < 0xF000);
  CMenu* pSysMenu = GetSystemMenu(FALSE);
  if (pSysMenu != NULL)
  {
    CString strAboutMenu;
    strAboutMenu.LoadString(IDS_ABOUTBOX);
    if (!strAboutMenu.IsEmpty())
    {
      pSysMenu->AppendMenu(MF_SEPARATOR);
      pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
    }
  }
}

void CAllSrvUISheet::InitIcon()
{
  // Load and set the large icon
  LPCTSTR pszRes = MAKEINTRESOURCE(IDR_MAINFRAME);
  HICON hIcon = (HICON)::LoadImage(AfxGetResourceHandle(), pszRes,
    IMAGE_ICON, ::GetSystemMetrics(SM_CXICON),
    ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
  SetIcon(hIcon, true);

  // Load and set the small icon
  HICON hIconSmall = (HICON)::LoadImage(AfxGetResourceHandle(), pszRes, 
    IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON),
    ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
  SetIcon(hIconSmall, false);
}

void CAllSrvUISheet::AddChildrenToAutoSizer()
{
  // Add some AutoSizer rules for the tab control
  CTabCtrl* pwndTab = GetTabControl();
  m_AutoSizer.AddRule(*pwndTab, AutoSizer_Follow_Right,
    NULL, AutoSizer_Lead_Right, AutoSizer_Refresh_NoRefresh);
  m_AutoSizer.AddRule(*pwndTab, AutoSizer_Follow_Bottom,
    NULL, AutoSizer_Lead_Bottom, AutoSizer_Refresh_NoRefresh);

  // Add some AutoSizer rules for the status bar control
  m_AutoSizer.AddRule(m_wndStatusBar, AutoSizer_Follow_Right,
    NULL, AutoSizer_Lead_Right, AutoSizer_Refresh_NoRefresh);
  m_AutoSizer.AddRule(m_wndStatusBar, AutoSizer_Follow_TopBottom,
    NULL, AutoSizer_Lead_Bottom, AutoSizer_Refresh_NoRefresh);

  // Add some AutoSizer rules for the buttons
  CWnd* pwnd = GetWindow(GW_CHILD);
  while (pwnd)
  {
    // Get tne class name of the child window
    TCHAR szClass[_MAX_PATH];
    GetClassName(*pwnd, szClass, sizeofArray(szClass));
    if (0 == _tcsicmp(szClass, "button"))
    {
      // Add some AutoSizer rules for this child window
      m_AutoSizer.AddRule(*pwnd, AutoSizer_Follow_LeftRight,
        NULL, AutoSizer_Lead_Right, AutoSizer_Refresh_NoRefresh);
      m_AutoSizer.AddRule(*pwnd, AutoSizer_Follow_TopBottom,
        NULL, AutoSizer_Lead_Bottom, AutoSizer_Refresh_NoRefresh);
    }

    // Get the next child window
    pwnd = pwnd->GetWindow(GW_HWNDNEXT);
  }
}

void CAllSrvUISheet::UpdateStatus()
{
  // Do nothing if not connected to the remote session
  if (!GetSession())
    return;

  // Determine the string resource ID for the game stage
  UINT idStage = IDS_STAGE_NONE;
  if (GetGame())
  {
    // Check the stage of the game
    AGCGameStage stage = static_cast<AGCGameStage>(-1);
    HRESULT hr = GetGame()->get_GameStage(&stage);
    if (FAILED(hr))
    {
      // Exit the application, with no notification
      if (RPC_E_CANTCALLOUT_ININPUTSYNCCALL != hr)  // This error is acceptable
        DestroyWindow();
      return;
    }

    // Determine the string resource ID for the game stage
    switch (stage)
    {
      case AGCGameStage_NotStarted: idStage = IDS_STAGE_NOTSTARTED; break;
      case AGCGameStage_Starting  : idStage = IDS_STAGE_STARTING  ; break;
      case AGCGameStage_Started   : idStage = IDS_STAGE_STARTED   ; break;
      case AGCGameStage_Over      : idStage = IDS_STAGE_OVER      ; break;
      case AGCGameStage_Terminate : idStage = IDS_STAGE_TERMINATE ; break;
    }
  }
  else
  {
    // Ensure that the remote session is still connected
    long nProcessID;
    HRESULT hr = GetSession()->get_ProcessID(&nProcessID);
    if (FAILED(hr))
    {
      // Exit the application, with no notification
      if (RPC_E_CANTCALLOUT_ININPUTSYNCCALL != hr)  // This error is acceptable
        DestroyWindow();
      return;
    }
  }

  // Load the Game stage string
  CString strStage(MAKEINTRESOURCE(idStage));

  // Set the status bar text
  m_wndStatusBar.SetText(strStage, 0, 0);
}

HRESULT CAllSrvUISheet::HandleError(HRESULT hr, LPCSTR pszContext, bool bExit)
{
  // Determine the best error description available
  IErrorInfoPtr spei;
  ::GetErrorInfo(0, &spei);
  _com_error e(hr, spei, true);
  _bstr_t bstrError(e.Description());
  if (!bstrError.length())
    bstrError = e.ErrorMessage();

  // Format the error message
  const static TCHAR szFmt[] = TEXT("An error occurred while %hs:\n\n%ls");
  int cch = sizeofArray(szFmt) + strlen(pszContext) + bstrError.length();
  LPTSTR pszMsg = (LPTSTR)_alloca(cch * sizeof(TCHAR));
  _stprintf(pszMsg, szFmt, pszContext, (BSTR)bstrError);

  // Display the message box
  MessageBox(pszMsg, TEXT("Error: Allegiance Game Server"));

  // Destroy the window (exiting the app), if specified
  if (bExit)
    DestroyWindow();

  // Return the specified HRESULT
  return hr;
}

bool CAllSrvUISheet::SelectGame()
{
  CDlgGameSelect dlg(this);
  if (IDCANCEL == dlg.DoModal())
    return false;
  if (dlg.GetSelectedGame() == m_spGame)
    return false;

  // Save the selected game interface
  m_spGame = dlg.GetSelectedGame();

  // Turn off redraw for the next twitch
  SetRedraw(false);

  // Add a dummy property page
  AddPage(&m_PageDummy);

  // Remove our current property pages
  RemovePage(&m_PagePlayers);
  RemovePage(&m_PageChat);
  RemovePage(&m_PageGameCreate);

  // Add our property pages
  AddPage(&m_PageGameCreate);
  AddPage(&m_PageChat);
  AddPage(&m_PagePlayers);

  // Remove the dummy page
  RemovePage(&m_PageDummy);

  // Pre-activate each property page (so that each has a window)
  int iPageSelected = GetActiveIndex();
  for (int iPage = 0; iPage < GetPageCount(); ++iPage)
    SetActivePage(iPage);
  SetActivePage(iPageSelected);

  // Update the status bar
  UpdateStatus();

  // Restore redraw after the twitch
  SetRedraw();
  Invalidate();
  UpdateWindow();

  // Indicate that we changed games
  return true;
}

HRESULT CAllSrvUISheet::CreateGame(bool bLobby,
  IAGCGameParameters* pGameParameters)
{
  _ASSERTE(NULL != m_spServer);
  _ASSERTE(NULL == m_spGame);

  // Set the LobbyServer property
  CComBSTR bstrLobby;
  HRESULT hr = m_spServer->put_PublicLobby(bLobby);
  if (FAILED(hr))
  {
    if (HRESULT_FROM_WIN32(ERROR_CRC) == hr)
      return hr;
    return HandleError(hr, "setting the LobbyServer property", false);
  }

  // Get the Games property
  IAdminGamesPtr spGames;
  hr = m_spServer->get_Games(&spGames);
  if (FAILED(hr))
    return HandleError(hr, "retrieving the AdminGames property", false);

  // Create the game
  hr = spGames->Add(pGameParameters);
  if (FAILED(hr))
    return HandleError(hr, "creating a new game", false);

  // ------------------------------------------------------------------------
  // NOTE: Since the Add method does not return a pointer to the created
  // game, getting the first game must suffice for now. However, if we ever
  // support creating multiple games (from the UI), we must provide a better
  // way to get the created game interface. One suggestion is to add a new
  // method, AddGame, to the games collection, so that it will not break
  // other applications that call the Add method.
  // ------------------------------------------------------------------------
  // Get the first Game, if any
  hr = spGames->get_Item(&CComVariant(0L), &m_spGame);

  // Update the status bar
  UpdateStatus();

  // Return the last result
  return hr;
}

HRESULT CAllSrvUISheet::DestroyGame()
{
  _ASSERTE(NULL != m_spGame);

  // Destroy the game
  IAdminGamePtr spGame(m_spGame);
  m_spGame = NULL;
  HRESULT hr = spGame->Kill();
  if (FAILED(hr))
    HandleError(hr, "destroying game", false);

  // Update the status bar
  UpdateStatus();

  // Return the last result
  return hr;
}

HRESULT CAllSrvUISheet::PostConnect(DWORD dwCookie)
{
  // Create an instance of the Global Interface Table
  IGlobalInterfaceTablePtr spGIT;
  HRESULT hr = spGIT.CreateInstance(CLSID_StdGlobalInterfaceTable);
  if (FAILED(hr))
    return HandleError(hr, "creating Global Interface Table", true);

  // From the GIT get the interface pointer obtained by the worker thread
  if (FAILED(hr = spGIT->GetInterfaceFromGlobal(dwCookie,
    __uuidof(IAdminSession), (void**)&m_spSession)))
      return HandleError(hr, "retrieving interface from GIT", true);

  // Revoke the interface pointer from the GIT
  hr = spGIT->RevokeInterfaceFromGlobal(dwCookie);

  // Set a timer that both pings the server and gets the game stage
  SetTimer(c_StageTimer, c_StageTimeout, NULL);

  // Get the Server property
  if (FAILED(hr = m_spSession->get_Server(&m_spServer)))
    return HandleError(hr, "retrieving AdminServer property", true);

  // Get the Games property
  IAdminGamesPtr spGames;
  if (FAILED(hr = m_spServer->get_Games(&spGames)))
    return HandleError(hr, "retrieving AdminGames property", true);

  // Set the MultiMode flag if more than one game exists
  long cGames = 0;
  if (FAILED(hr = spGames->get_Count(&cGames)))
    return HandleError(hr, "retrieving the AdminGames.Count property", true);
  m_bMultiMode = cGames > 1;

  // Get the first Game, if any
  hr = spGames->get_Item(&CComVariant(0L), &m_spGame);
  // Intentionally ignoring the result, since Game0 may not exist

  // Connect our event sink to the session object
  LPCSTR pszContext = "setting-up the connection point";
  IConnectionPointContainerPtr spcpc(m_spSession);
  if (NULL == spcpc)
    return HandleError(E_NOINTERFACE, pszContext, true);
  hr = spcpc->FindConnectionPoint(__uuidof(IAdminSessionEvents), &m_spcp);
  if (SUCCEEDED(hr))
  {
    // Callback interface gets created with a refcount of one
    XEvents* pxEvents = new XEvents(this);
    hr = m_spcp->Advise(pxEvents, &m_dwConnectionCookie);
    pxEvents->Release(); // Release the creation refcount
  }
  if (FAILED(hr))
    return HandleError(hr, pszContext, true);

  // Enable some events
  m_spSession->ActivateEvents(EventID_GameDestroyed, -1);

  // Turn off redraw for the next twitch
  SetRedraw(false);

  // Add our property pages
  AddPage(&m_PageGameCreate);
  AddPage(&m_PageChat);
  AddPage(&m_PagePlayers);

  // Remove the connection page
  m_AutoSizer.RemoveRules(m_PageConnect);
  RemovePage(&m_PageConnect);

  // Pre-activate each property page (so that each has a window)
  int iPageSelected = GetActiveIndex();
  for (int iPage = 0; iPage < GetPageCount(); ++iPage)
    SetActivePage(iPage);
  SetActivePage(iPageSelected);

  // Update the status bar
  UpdateStatus();

  // Restore redraw after the twitch
  SetRedraw();
  Invalidate();
  UpdateWindow();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Overrides

void CAllSrvUISheet::OnEvent(IAGCEvent* pEvent)
{
  AGCEventID eEvent;
  if (FAILED(pEvent->get_ID(&eEvent)))
    return;

  switch (eEvent)
  {
    case EventID_GameDestroyed:
    {
      m_spGame = NULL;

      // Update the status bar
      UpdateStatus();
      break;
    }
  }
}

void CAllSrvUISheet::PostNcDestroy()
{
  // Delete the object
  delete this;
}

LRESULT CAllSrvUISheet::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  LRESULT lr;
  if (m_AutoSizer.ProcessMessage(message, wParam, lParam, &lr))
    return lr;

  // Perform default processing
  return CPropertySheet::WindowProc(message, wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

BOOL CAllSrvUISheet::OnInitDialog()
{
  // Perform default processing
  CPropertySheet::OnInitDialog();

  // Add "About..." menu item to system menu.
  InitSysMenu();

  // Set the icon for this dialog.
  InitIcon();

  // Create a status bar control
  DWORD dwStyle = WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP;
  m_wndStatusBar.Create(dwStyle, CRect(0, 0, 0, 0), this, -1);

  // Adjust our window size for the status bar control
  CRect rect, rectStatusBar;
  GetWindowRect(rect);
  m_wndStatusBar.GetWindowRect(rectStatusBar);
  rect.bottom += rectStatusBar.Height() + 3;
  MoveWindow(rect);
  m_wndStatusBar.SendMessage(WM_SIZE);

  // Initialize the AutoSize object
  m_AutoSizer.SetWindowAndRules(*this, NULL);

  // Add child windows to AutoSizer object
  AddChildrenToAutoSizer();

  // Restore previous window placement
  CRegKey key;
  if (SUCCEEDED(key.Open(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI, KEY_READ)))
  {
    WINDOWPLACEMENT wp;
    DWORD dwType;
    DWORD cbData = sizeof(wp);
    if (ERROR_SUCCESS == ::RegQueryValueEx(key, TEXT("WindowPlacement"), NULL,
      &dwType, reinterpret_cast<BYTE*>(&wp), &cbData))
    {
      if (REG_BINARY == dwType)
      {
        ::SetWindowPlacement(*this, &wp);
      }
    }
  }

  m_bInitDone = true;

  // Return true to set the focus to the first tabstop control
  return true;
}

void CAllSrvUISheet::OnSysCommand(UINT nID, LPARAM lParam)
{
  // Handle the About... menu item
  if ((nID & 0xFFF0) == IDM_ABOUTBOX)
  {
    CDlgAbout dlgAbout(this);
    dlgAbout.DoModal();
    return;
  }

  // Perform default processing
  CPropertySheet::OnSysCommand(nID, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// Events Sub-Object

// Construction / Destruction
CAllSrvUISheet::XEvents::XEvents(CAllSrvUISheet* pThis) :
  m_pThis(pThis),
  m_nRefs(1)
{
}

CAllSrvUISheet::XEvents::~XEvents()
{
}


// IUnknown Interface Methods
STDMETHODIMP CAllSrvUISheet::XEvents::QueryInterface(REFIID riid, void** ppUnk)
{
  if (IID_IUnknown == riid || __uuidof(IAdminSessionEvents) == riid)
  {
    *ppUnk = this;
    AddRef();
    return S_OK;
  }
  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAllSrvUISheet::XEvents::AddRef(void)
{
  return ++m_nRefs;
}

STDMETHODIMP_(ULONG) CAllSrvUISheet::XEvents::Release(void)
{
  ULONG nRefs = --m_nRefs;
  if (0 == nRefs)
    delete this;
  return nRefs;
}

// IAdminSessionEvents
STDMETHODIMP CAllSrvUISheet::XEvents::OnEvent(IAGCEvent* pEvent)
{
  // Forward to parent class
  m_pThis->OnEvent(pEvent);

  // Notify each property page
  m_pThis->m_PageGameCreate.OnEvent(pEvent);
  m_pThis->m_PageChat.OnEvent(pEvent);
  m_pThis->m_PagePlayers.OnEvent(pEvent);

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

void CAllSrvUISheet::OnSize(UINT nType, int cx, int cy)
{
  // Perform default processing
  CPropertySheet::OnSize(nType, cx, cy);

  // Re-active the active page, if any. (Causes page to resize)
  CPropertyPage* pPage = GetActivePage();
  if (pPage)
    SetActivePage(pPage);
  
  // Defer saving the new size to the registry
  if (m_bInitDone)
    SetTimer(c_SizeTimer, c_SizeTimeout, NULL);
}

void CAllSrvUISheet::OnMove(int x, int y)
{
  // Perform default processing
  CPropertySheet::OnMove(x, y);
  
  // Defer saving the new position to the registry
  if (m_bInitDone)
    SetTimer(c_SizeTimer, c_SizeTimeout, NULL);
}

void CAllSrvUISheet::OnTimer(UINT_PTR nIDEvent)
{
  switch (nIDEvent)
  {
    case c_SizeTimer:
    {
      // Save the current window position to the registry
      WINDOWPLACEMENT wp = {sizeof(wp)};
      ::GetWindowPlacement(*this, &wp);
      CRegKey key;
      key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);
      RegSetValueEx(key, TEXT("WindowPlacement"), 0, REG_BINARY,
        reinterpret_cast<const BYTE*>(&wp), sizeof(wp));

      // Kill the timer
      KillTimer(nIDEvent);
      break;
    }

    case c_StageTimer:
    {
      // Update the status text to reflect the current game stage
      UpdateStatus();
      break;
    }

    default:
    {
      // Perform default processing
      CPropertySheet::OnTimer(nIDEvent);
    }
  }
}

void CAllSrvUISheet::OnDestroy()
{
  // Kill all timers
  for (int i = c_cTimersMin + 1; i < c_cTimersMax; ++i)
    KillTimer(i);

  // Perform default processing
  CPropertySheet::OnDestroy();
}

void CAllSrvUISheet::OnClose() 
{
  // Ask to destroy the game, if not in multi-game mode
  if (!IsServerInMultiMode() && GetGame())
  {
    // Display the message box
    int nType = MB_ICONQUESTION | MB_YESNOCANCEL;
    CString strText(MAKEINTRESOURCE(IDS_DESTROY_ON_EXIT));
    switch (MessageBox(strText, NULL, nType))
    {
      case IDCANCEL:
        // Continue running
        return;

      case IDYES:
        // Destroy the game
        this->DestroyGame();
        break;

      case IDNO:
        break;
    }
  }

  // Perform default processing
  CPropertySheet::OnClose();
}
