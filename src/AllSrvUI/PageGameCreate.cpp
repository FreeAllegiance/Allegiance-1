/////////////////////////////////////////////////////////////////////////////
// PageGameCreate.cpp : implementation file
//

#include "stdafx.h"
#include "AllSrvUI.h"
#include "PageGameCreate.h"
#include "AllSrvUISheet.h"
#include "GameParamsSheet.h"

#ifdef _DEBUG
  #define new DEBUG_NEW
  #undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPageGameCreate property page

IMPLEMENT_DYNAMIC(CPageGameCreate, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Static Initializtion

BEGIN_AUTO_SIZER_MAP(_AutoSizerMap)
  AUTO_SIZER_ENTRY(IDC_GAME_VISIBILITY      ,     Right, 0,   Right, BkRefresh)
//AUTO_SIZER_ENTRY(IDC_PRIVATE_GAME         ,     Right, 0,   Right, NoRefresh)
//AUTO_SIZER_ENTRY(IDC_PUBLIC_GAME          ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_SETTINGS_CAPTION,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_SETTINGS_CAPTION,    Bottom, 0,  Bottom, BkRefresh)
//AUTO_SIZER_ENTRY(IDC_GAME_NAME_CAPTION    ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_NAME            ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_CORE            ,     Right, 0,   Right, NoRefresh)
//AUTO_SIZER_ENTRY(IDC_GAME_PASSWORD_CAPTION,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_PASSWORD        ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_MAX_PLAYERS_CAPTION  , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_MAX_PLAYERS_CAPTION  , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_MAX_PLAYERS          , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_MAX_PLAYERS          , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_LOCK_GAME_OPEN       , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_LOCK_GAME_OPEN       , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_AET                  , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_AET                  , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_SELECT          , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_SELECT          , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_SETTINGS        , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_CREATE          , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_CREATE          , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_DESTROY         , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_GAME_DESTROY         , TopBottom, 0,  Bottom, NoRefresh)
END_AUTO_SIZER_MAP()


/////////////////////////////////////////////////////////////////////////////
// Message Map

BEGIN_MESSAGE_MAP(CPageGameCreate, CPropertyPage)
  //{{AFX_MSG_MAP(CPageGameCreate)
  ON_BN_CLICKED(IDC_PRIVATE_GAME, OnPrivateGame)
  ON_BN_CLICKED(IDC_PUBLIC_GAME, OnPublicGame)
  ON_EN_CHANGE(IDC_GAME_NAME, OnChangeGameName)
  ON_EN_CHANGE(IDC_GAME_CORE, OnChangeGameCore)
  ON_BN_CLICKED(IDC_GAME_SELECT, OnGameSelect)
  ON_BN_CLICKED(IDC_GAME_SETTINGS, OnGameSettings)
  ON_BN_CLICKED(IDC_GAME_CREATE, OnGameCreate)
  ON_BN_CLICKED(IDC_GAME_DESTROY, OnGameDestroy)
  ON_WM_TIMER()
  //}}AFX_MSG_MAP
  
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Construction

CPageGameCreate::CPageGameCreate(CAllSrvUISheet *pSheet) :
  CPropertyPage(CPageGameCreate::IDD),
	m_mpSheet(pSheet)
{
  //{{AFX_DATA_INIT(CPageGameCreate)
  m_iGameVisibility = -1;
  m_strGameName = _T("");
  m_strGamePassword = _T("");
  m_strGameCore = _T(""); // KGJV: core
  m_nMaxPlayers = -1;
  m_bLockGameOpen = FALSE;
  m_bAET = FALSE; // KGJV: AET
  //}}AFX_DATA_INIT
}



/////////////////////////////////////////////////////////////////////////////
// Attributes

/////////////////////////////////////////////////////////////////////////////
// Overrides

void CPageGameCreate::OnEvent(IAGCEvent* pEvent)
{
  AGCEventID eEvent;
  if (FAILED(pEvent->get_ID(&eEvent)))
    return;

  switch (eEvent)
  {
    case EventID_GameDestroyed:
    {
      UpdateUI();
      break;
    }
    case EventID_GameStateChange:
    {
      // Get the current Game Parameters
      HRESULT hr = m_mpSheet->GetGame()->get_GameParameters(&m_spGameParameters);
      if (FAILED(hr))
        m_mpSheet->HandleError(hr, "retrieving Game Parameters object", true);

      // Update the data members from the game parameters
      UpdateFromGameParameters();

      // Update fields from data members
      UpdateData(false);

      // Update the UI
      UpdateUI();
      break;
    }
  }
}

void CPageGameCreate::DoDataExchange(CDataExchange* pDX)
{
  // Perform default processing
  CPropertyPage::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(CPageGameCreate)
  DDX_Control(pDX, IDC_LOCK_GAME_OPEN, m_btnLockGameOpen);
  DDX_Control(pDX, IDC_AET, m_btnAET); // KGJV added AET
  DDX_Control(pDX, IDC_MAX_PLAYERS_CAPTION, m_staticMaxPlayers);
  DDX_Control(pDX, IDC_MAX_PLAYERS, m_comboMaxPlayers);
  DDX_Control(pDX, IDC_GAME_PASSWORD_CAPTION, m_staticGamePassword);
  DDX_Control(pDX, IDC_GAME_NAME_CAPTION, m_staticGameName);
  DDX_Control(pDX, IDC_GAME_CORE_CAPTION, m_staticGameCore); // KGJV added core
  DDX_Control(pDX, IDC_GAME_DESTROY, m_btnGameDestroy);
  DDX_Control(pDX, IDC_GAME_CREATE, m_btnGameCreate);
  DDX_Control(pDX, IDC_GAME_SELECT, m_btnGameSelect);
  DDX_Control(pDX, IDC_GAME_SETTINGS, m_btnGameSettings);
  DDX_Control(pDX, IDC_GAME_PASSWORD, m_editGamePassword);
  DDX_Control(pDX, IDC_GAME_NAME, m_editGameName);
  DDX_Control(pDX, IDC_GAME_CORE, m_editGameCore); // KGJV added core
  DDX_Radio(pDX, IDC_PRIVATE_GAME, m_iGameVisibility);
  DDX_Text(pDX, IDC_GAME_NAME, m_strGameName);
  DDX_Text(pDX, IDC_GAME_PASSWORD, m_strGamePassword);
  DDX_Text(pDX, IDC_GAME_CORE, m_strGameCore); // KGJV added core
  DDX_Control(pDX, IDC_PUBLIC_GAME, m_btnPublicGame);
  DDX_Control(pDX, IDC_PRIVATE_GAME, m_btnPrivateGame);
  DDX_CBIndex(pDX, IDC_MAX_PLAYERS, m_nMaxPlayers);
  DDX_Check(pDX, IDC_LOCK_GAME_OPEN, m_bLockGameOpen);
  DDX_Check(pDX, IDC_AET, m_bAET); // KGJV added AET
  //}}AFX_DATA_MAP
}

LRESULT CPageGameCreate::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  LRESULT lr;
  if (m_AutoSizer.ProcessMessage(message, wParam, lParam, &lr))
    return lr;

  // Perform default processing
  return CPropertyPage::WindowProc(message, wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

void CPageGameCreate::UpdateUI(bool bUpdateData)
{
  // Update data members from fields, if specified
  if (bUpdateData)
    UpdateData();

  // Determine if a game is in progress or not
  bool bGameInProgress = NULL != m_mpSheet->GetGame();

  // Enable/disable the controls as needed
  m_btnPrivateGame.EnableWindow(!bGameInProgress);
  m_btnPublicGame.EnableWindow(!bGameInProgress);
  m_staticGameName.EnableWindow(!bGameInProgress);
  m_staticGameCore.EnableWindow(!bGameInProgress); // KGJV- added core
  m_editGameName.EnableWindow(!bGameInProgress);
  m_editGameCore.EnableWindow(!bGameInProgress); // KGJV- added core
  m_staticGamePassword.EnableWindow(!bGameInProgress);
  m_editGamePassword.EnableWindow(!bGameInProgress);
  m_staticMaxPlayers.EnableWindow(!bGameInProgress);
  m_comboMaxPlayers.EnableWindow(!bGameInProgress);
  m_btnGameCreate.EnableWindow(!bGameInProgress && !m_strGameName.IsEmpty());
  m_btnGameDestroy.EnableWindow(bGameInProgress);
  m_btnLockGameOpen.EnableWindow(!bGameInProgress);
  m_btnAET.EnableWindow(!bGameInProgress); // KGJV - added AET
}

void CPageGameCreate::UpdateFromGameParameters()
{
  CComBSTR bstr;
  LPCSTR pszContext = "reading Game Parameters object";

  // GameName
  HRESULT hr;
  if (FAILED(hr = m_spGameParameters->get_GameName(&bstr)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  m_strGameName = bstr;

  // GamePassword
  if (FAILED(hr = m_spGameParameters->get_GamePassword(&bstr)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  m_strGamePassword = bstr;

  // KGJV- GameCore
  if (FAILED(hr = m_spGameParameters->get_IGCStaticFile(&bstr)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  m_strGameCore = bstr;


  // TotalMaxPlayers
  short cMaxPlayers;
  if (FAILED(hr = m_spGameParameters->get_TotalMaxPlayers(&cMaxPlayers)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  CString strMaxPlayers;
  strMaxPlayers.Format(TEXT("%d"), cMaxPlayers);
  m_nMaxPlayers = m_comboMaxPlayers.FindStringExact(-1, strMaxPlayers);
  if (m_nMaxPlayers < 0)
    m_nMaxPlayers = m_comboMaxPlayers.FindStringExact(-1, TEXT("8"));

  // LockGameOpen
  VARIANT_BOOL bLockGameOpen;
  if (FAILED(hr = m_spGameParameters->get_LockGameOpen(&bLockGameOpen)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  m_bLockGameOpen = !!bLockGameOpen;

  // AET - KGJV
  VARIANT_BOOL bAET;
  if (FAILED(hr = m_spGameParameters->get_AllowEmptyTeams(&bAET)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  m_bAET = !!bAET;

}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

BOOL CPageGameCreate::OnInitDialog() 
{
  HRESULT hr;

  // Register for events of interest
  m_mpSheet->GetSession()->ActivateEvents(EventID_GameStateChange, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_GameDestroyed, -1);

  // Get the Game Parameters of the current game, if any
  if (m_mpSheet->GetGame())
  {
    hr = m_mpSheet->GetGame()->get_GameParameters(&m_spGameParameters);
    if (FAILED(hr))
      return S_OK == m_mpSheet->HandleError(hr, "retrieving Game Parameters object", true);
  }
  else
  {
    // Create an instance of the AGC Game Parameters
    hr = m_spGameParameters.CreateInstance(CLSID_AGCGameParameters);
    if (FAILED(hr))
      return S_OK == m_mpSheet->HandleError(hr, "creating Game Parameters object", true);

    // Set the default game name
    TCHAR szUserName[UNLEN + 1];
    DWORD cchUserName = sizeofArray(szUserName);
    GetUserName(szUserName, &cchUserName);
    WCHAR szDefaultGameName[_MAX_PATH];
	// mdvalley: _s for the hell of it
#if _MSC_VER >= 1400
    swprintf_s(szDefaultGameName, _MAX_PATH, L"%hs's Game", szUserName);
#else
	swprintf(szDefaultGameName, L"%hs's Game", szUserName);
#endif
    hr = m_spGameParameters->put_GameName(CComBSTR(szDefaultGameName));
    if (FAILED(hr))
      return S_OK == m_mpSheet->HandleError(hr, "setting Game Parameters property", true);
  }

  // Perform default processing
  CPropertyPage::OnInitDialog();

  // Initialize the AutoSize object
  m_AutoSizer.SetWindowAndRules(*this, _AutoSizerMap);

  // Show/hide buttons depending on server mode
  bool bMultiMode = m_mpSheet->IsServerInMultiMode();
  m_btnGameSelect.ShowWindow (bMultiMode ? SW_SHOW : SW_HIDE);
  m_btnGameCreate.ShowWindow (bMultiMode ? SW_HIDE : SW_SHOW);
  m_btnGameDestroy.ShowWindow(bMultiMode ? SW_HIDE : SW_SHOW);
  m_btnGameSelect.EnableWindow ( bMultiMode);
  m_btnGameCreate.EnableWindow (!bMultiMode);
  m_btnGameDestroy.EnableWindow(!bMultiMode);

  // Read previous values from registry
  CRegKey key;
  key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);

  if (m_mpSheet->GetGame())
  {
    // Get the LobbyServer of the current game
    CComBSTR bstrLobbyServer;
    hr = m_mpSheet->GetServer()->get_LobbyServer(&bstrLobbyServer);
    if (FAILED(hr))
      return S_OK == m_mpSheet->HandleError(hr, "retriving LobbyServer property", true);

    // Set the data members associated with the fields
    if (!bstrLobbyServer.Length())
    {
      // Select the 'private' radio button
      m_iGameVisibility = 0;
    }
    else
    {
      // Select the 'public' radio button
      m_iGameVisibility = 1;
    }
  }
  else
  {
    // Always select the 'private' radio button
    m_iGameVisibility = 0;

    // Read the previous game parameters into a stream
    IStreamPtr spStm;
    hr = LoadRegStream(key, TEXT("GameParameters"), &spStm);
    if (SUCCEEDED(hr))
    {
      // Persist the game parameters from the stream
      IPersistStreamInitPtr spPersist(m_spGameParameters);
      if (NULL != spPersist && FAILED(hr = spPersist->Load(spStm)))
        spPersist->InitNew();

      // If binary representation has changed, just re-initialize the object
      if (FAILED(m_spGameParameters->Validate()))
        spPersist->InitNew();
    }
  }

  // Update the data members from the game parameters
  UpdateFromGameParameters();

  // Update fields from data members
  UpdateData(false);

  // Update the UI
  UpdateUI();

  // Return true to set the focus to the first tabstop control
  return true;
}

void CPageGameCreate::OnPrivateGame() 
{
  UpdateUI(true);
}

void CPageGameCreate::OnPublicGame() 
{
  UpdateUI(true);
}

void CPageGameCreate::OnChangeGameName() 
{
  UpdateUI(true);
}

// KGJV: added core
void CPageGameCreate::OnChangeGameCore()
{
  UpdateUI(true);
}

void CPageGameCreate::OnGameSettings() 
{
  CGameParamsSheet dlg;
  dlg.DoModal();
}

void CPageGameCreate::OnGameSelect() 
{
  m_mpSheet->SelectGame();
}

void CPageGameCreate::OnGameCreate() 
{
  CWaitCursor wait;

  // Update data members from fields
  UpdateData();

  // Populate the IAGCGameParameters with the current name and password
  _ASSERTE(!m_strGameName.IsEmpty());
  LPCSTR pszContext = "setting Game parameters";
  HRESULT hr = m_spGameParameters->put_GameName(CComBSTR(m_strGameName));
  if (SUCCEEDED(hr))
    hr = m_spGameParameters->put_GamePassword(CComBSTR(m_strGamePassword));
  if (FAILED(hr))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  // KGJV- core
  if (!m_strGameCore.IsEmpty())
  {
	hr = m_spGameParameters->put_IGCStaticFile(CComBSTR(m_strGameCore));
	if (FAILED(hr))
	{
		m_mpSheet->HandleError(hr, pszContext, true);
		return;
	}
  }

  // Set the TotalMaxPlayers property of the IAGCGameParameters
  CString strMaxPlayers;
  m_comboMaxPlayers.GetLBText(m_nMaxPlayers, strMaxPlayers);
  short cMaxPlayers = static_cast<short>(_ttoi(strMaxPlayers));
  if (SUCCEEDED(hr = m_spGameParameters->put_TotalMaxPlayers(cMaxPlayers)))
  {
    // Ensure that min/team is 1 and max/team is TotalMax
    if (SUCCEEDED(hr = m_spGameParameters->put_MinPlayers(1)))
      hr = m_spGameParameters->put_MaxPlayers(cMaxPlayers);
  }
  if (FAILED(hr))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }

  // Set the LockGameOpen property of the IAGCGameParameters
  VARIANT_BOOL bLockGameOpen = m_bLockGameOpen ? VARIANT_TRUE : VARIANT_FALSE;
  if (FAILED(hr = m_spGameParameters->put_LockGameOpen(bLockGameOpen)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }

  // KGJV Set the AET property of the IAGCGameParameters
  VARIANT_BOOL bAET = m_bAET ? VARIANT_TRUE : VARIANT_FALSE;
  if (FAILED(hr = m_spGameParameters->put_AllowEmptyTeams(bAET)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }


  // Determine whether or not to connect to a lobby server
  bool bLobbyServer = (1 == m_iGameVisibility);

  // Create the game
  hr = m_mpSheet->CreateGame(bLobbyServer, m_spGameParameters);
  if (SUCCEEDED(hr))
  {
    SetDefID(m_btnGameSettings.GetDlgCtrlID());
    m_btnGameSettings.SetFocus();

    // Get the game parameters of the just-created game
    hr = m_mpSheet->GetGame()->get_GameParameters(&m_spGameParameters);
    if (FAILED(hr))
      m_mpSheet->HandleError(hr, "retrieving Game Parameters object", true);

    // Update the data members from the game parameters
    UpdateFromGameParameters();

    // Update fields from data members
    UpdateData(false);
  }
  else
  { 
    // return now because it is possible that the sheet was destroyed when we 
    // tried to create a game in m_mpSheet->CreateGame().  This can happen
    // if an auto-update is needed.  If we call UpdateUI() in such cases,
    // this app will crash.
    return;
  }

  // Update the UI
  UpdateUI();

  // Open/create the registry key to save the page settings
  pszContext = "saving preferences to the registry";
  CRegKey key;
  LONG lr = key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);
  if (ERROR_SUCCESS != lr)
  {
    m_mpSheet->HandleError(HRESULT_FROM_WIN32(lr), pszContext, false);
    return;
  }

  // Save the Game Parameters to the registry
  IStreamPtr spStm;
  hr = CreateStreamOnHGlobal(NULL, true, &spStm);
  if (SUCCEEDED(hr))
  {
    IPersistStreamInitPtr spPersist(m_spGameParameters);
    if (NULL != spPersist)
    {
      hr = spPersist->Save(spStm, true);
      if (SUCCEEDED(hr))
        hr = SaveRegStream(key, "GameParameters", spStm);
    }
    else
    {
      hr = E_NOINTERFACE;
    }
  }
  if (FAILED(hr))
    m_mpSheet->HandleError(hr, pszContext, false);
}


void CPageGameCreate::OnGameDestroy() 
{
  CWaitCursor wait;

  // Destroy the game
  HRESULT hr = m_mpSheet->DestroyGame();

  // Update the UI
  UpdateUI();
}
