/////////////////////////////////////////////////////////////////////////////
// PagePlayers.cpp : implementation file
//

#include "stdafx.h"
#include "AllSrvUI.h"
#include "AllSrvUISheet.h"
#include "PagePlayers.h"

#ifdef _DEBUG
  #define new DEBUG_NEW
  #undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPagePlayers property page

IMPLEMENT_DYNAMIC(CPagePlayers, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Static Initializtion

BEGIN_AUTO_SIZER_MAP(_AutoSizerMap)
  AUTO_SIZER_ENTRY(IDC_BOOT_PLAYERS     , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_BOOT_PLAYERS     , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_CAPTION, TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_TEXT   ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_TEXT   , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_PLAYER_LIST      ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_PLAYER_LIST      ,    Bottom, 0,  Bottom, NoRefresh)
END_AUTO_SIZER_MAP()


/////////////////////////////////////////////////////////////////////////////
// Message Map

BEGIN_MESSAGE_MAP(CPagePlayers, CPropertyPage)
  //{{AFX_MSG_MAP(CPagePlayers)
  ON_NOTIFY(HDN_ENDDRAG, 0, OnEndDragPlayerList)
  ON_NOTIFY(HDN_ENDTRACK, 0, OnEndTrackPlayerList)
  ON_WM_TIMER()
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_PLAYER_LIST, OnItemChangedPlayerList)
  ON_NOTIFY(LVN_DELETEITEM, IDC_PLAYER_LIST, OnDeleteItemPlayerList)
  ON_BN_CLICKED(IDC_BOOT_PLAYERS, OnBootPlayers)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Construction

CPagePlayers::CPagePlayers(CAllSrvUISheet *pSheet) :
  CPropertyPage(CPagePlayers::IDD),
	m_mpSheet(pSheet)
{
  //{{AFX_DATA_INIT(CPagePlayers)
  m_strSendChat = _T("");
  //}}AFX_DATA_INIT

  // Create a TCObj.Strings object
  HRESULT hr = m_spStrings.CreateInstance("TCObj.Strings");
  if (FAILED(hr))
    m_mpSheet->HandleError(hr, "creating TCObj.Strings helper object", true);
}


/////////////////////////////////////////////////////////////////////////////
// Attributes


/////////////////////////////////////////////////////////////////////////////
// Overrides

void CPagePlayers::OnEvent(IAGCEvent* pEvent)
{
  AGCEventID eEvent;
  if (FAILED(pEvent->get_ID(&eEvent)))
    return;

  switch (eEvent)
  {
    case EventID_GameCreated:
    case EventID_GameDestroyed:
    {
      UpdateUI();
      break;
    }

    case EventID_LoginGame:
    {
      AddPlayer(pEvent);
      break;
    }
    case EventID_LogoutGame:
    {
      RemovePlayer(pEvent);
      break;
    }

    case EventID_ShipChangesSectors:
    {
      SectorOrTeamChange(pEvent, 2, L"New Sector Name");
      break;
    }

    case EventID_LeaveTeam:
    case EventID_JoinTeam:
    {
      SectorOrTeamChange(pEvent, 1, L"TeamName");
      break;
    }

    case EventID_TeamInfoChange:
    {
      TeamInfoChange(pEvent);
      break;
    }
  }
}

BOOL CPagePlayers::OnSetActive() 
{
  // Reload the chat MRU from the registry
  LoadFromRegistry();

  // Update the UI
  UpdateUI();

  // Perform default processing
  return CPropertyPage::OnSetActive();
}

BOOL CPagePlayers::PreTranslateMessage(MSG* pMsg) 
{
  if (WM_KEYDOWN == pMsg->message)
  {
    if (VK_RETURN == pMsg->wParam)
    {
      if (&m_comboSendChat == GetFocus() ||
        &m_comboSendChat == GetFocus()->GetParent())
      {
        SendChat();
        return true;
      }
    }
  }

  // Perform default processing
  return CPropertyPage::PreTranslateMessage(pMsg);
}

void CPagePlayers::DoDataExchange(CDataExchange* pDX)
{
  // Perform default processing
  CPropertyPage::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(CPagePlayers)
  DDX_Control(pDX, IDC_BOOT_PLAYERS, m_btnBootPlayers);
  DDX_Control(pDX, IDC_SEND_CHAT_CAPTION, m_staticSendChat);
  DDX_Control(pDX, IDC_SEND_CHAT_TEXT, m_comboSendChat);
  DDX_CBString(pDX, IDC_SEND_CHAT_TEXT, m_strSendChat);
  DDX_Control(pDX, IDC_PLAYER_LIST, m_listPlayers);
  //}}AFX_DATA_MAP
}

LRESULT CPagePlayers::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
  LRESULT lr;
  if (m_AutoSizer.ProcessMessage(message, wParam, lParam, &lr))
    return lr;

  // Perform default processing
  return CPropertyPage::WindowProc(message, wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

void CPagePlayers::LoadFromRegistry()
{
  // Open/create the registry key
  CRegKey key;
  key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);

  // PlayerListColumnOrder
  CComBSTR bstrColumnOrder;
  LoadRegString(key, TEXT("PlayerListColumnOrder"), bstrColumnOrder);
  m_spStrings->RemoveAll();
  m_spStrings->AddDelimited(CComBSTR(L","), bstrColumnOrder);
  long cOrderedColumns = 0;
  m_spStrings->get_Count(&cOrderedColumns);
  if (c_cColumns == cOrderedColumns)
  {
    int vecOrder[c_cColumns];
    for (CComVariant i(0L); V_I4(&i) < c_cColumns; ++V_I4(&i))
    { 
      bstrColumnOrder.Empty();
      m_spStrings->get_Item(&i, &bstrColumnOrder);
      vecOrder[V_I4(&i)] = _wtoi(bstrColumnOrder);
    }
    m_listPlayers.SetColumnOrderArray(c_cColumns, vecOrder);
  }

  // PlayerListColumnWidths
  CComBSTR bstrColumnWidths;
  LoadRegString(key, TEXT("PlayerListColumnWidths"), bstrColumnWidths);
  m_spStrings->RemoveAll();
  m_spStrings->AddDelimited(CComBSTR(L","), bstrColumnWidths);
  long cWidthColumns = 0;
  m_spStrings->get_Count(&cWidthColumns);
  if (c_cColumns == cWidthColumns)
  {
    for (CComVariant i(0L); V_I4(&i) < c_cColumns; ++V_I4(&i))
    { 
      bstrColumnWidths.Empty();
      m_spStrings->get_Item(&i, &bstrColumnWidths);
      m_listPlayers.SetColumnWidth(V_I4(&i), _wtoi(bstrColumnWidths));
    }
  }
  else
  {
    for (int i = 0; i < c_cColumns; ++i)
    {
      TCHAR szColumn[_MAX_PATH];
      LVCOLUMN lvc = {LVCF_TEXT, 0, 0, szColumn, sizeofArray(szColumn)};
      m_listPlayers.GetColumn(i, &lvc);
      int cx = m_listPlayers.GetStringWidth(CString(szColumn) + TEXT("    "));
      m_listPlayers.SetColumnWidth(i, cx);
    }
  }

  // SendChatMRU
  m_comboSendChat.ResetContent();
  CRegKey keyMRU;
  if (ERROR_SUCCESS == keyMRU.Create(key, TEXT("SendChatMRU")))
  {
    // Read the count of strings
    DWORD cStrings = 0;
	// mdvalley: Former QueryDWORDValue(cStr, TEXT);
    keyMRU.QueryValue(cStrings, TEXT(".Count"));

    // Read each string and add it to the combo box
    for (DWORD i = 0; i < cStrings; ++i)
    {
      TCHAR szInt[16];
      CString strMRUItem;
      LoadRegString(keyMRU, _itot(i, szInt, 10), strMRUItem);
      strMRUItem.TrimLeft();
      strMRUItem.TrimRight();
      if (!strMRUItem.IsEmpty())
        m_comboSendChat.InsertString(i, strMRUItem);
    }
  }
}

void CPagePlayers::SavePlayerListColumnOrderToRegistry()
{
  // Open/create the registry key
  CRegKey key;
  key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);

  // Get the current column ordering
  int vecOrder[c_cColumns];
  for (int iCol = 0; iCol < c_cColumns; ++iCol)
    vecOrder[iCol] = iCol;
  m_listPlayers.GetColumnOrderArray(vecOrder, c_cColumns);

  // Format a comma-delimited string of the column ordering
  CString strColumnOrder;
  for (int i = 0; i < c_cColumns; ++i)
  {
    if (i)
      strColumnOrder += ",";
    TCHAR szNum[16];
    strColumnOrder += _itot(vecOrder[i], szNum, 10);
  }

  // Save to the registry
  // mdvalley: SetStringValue (swap)
  key.SetValue(strColumnOrder, TEXT("PlayerListColumnOrder"));
}

void CPagePlayers::SavePlayerListColumnWidthsToRegistry()
{
  // Open/create the registry key
  CRegKey key;
  key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);

  // Format a comma-delimited string of the column widths
  CString strColumnWidths;
  for (int i = 0; i < c_cColumns; ++i)
  {
    if (i)
      strColumnWidths += ",";
    TCHAR szNum[16];
    strColumnWidths += _itot(m_listPlayers.GetColumnWidth(i), szNum, 10);
  }

  // Save to the registry
  // mdvalley: SetStringValue(swap)
  key.SetValue(strColumnWidths, TEXT("PlayerListColumnWidths"));
}

void CPagePlayers::SaveSendChatMRUToRegistry()
{
  // Open/create the registry key
  CRegKey key;
  if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI))
    return;

  // SendChatMRU
  CRegKey keyMRU;
  if (ERROR_SUCCESS != keyMRU.Create(key, TEXT("SendChatMRU")))
    return;

  // Write the count of strings
  int cStrings = min(m_comboSendChat.GetCount(), c_cMaxChatsInRegistry);
  // mdvalley: SetDWORDValue (swap)
  keyMRU.SetValue(cStrings, TEXT(".Count"));

  // Write each string
  for (int i = 0; i < cStrings; ++i)
  {
    TCHAR szInt[16];
    CString strMRUItem;
    m_comboSendChat.GetLBText(i, strMRUItem);
	// mdvalley: SetStringValue(swap)
    keyMRU.SetValue(strMRUItem, _itot(i, szInt, 10));
  }
}

void CPagePlayers::UpdateUI(bool bUpdateData)
{
  // Update data members from fields, if specified
  if (bUpdateData)
    UpdateData();

  // Determine if a game is in progress or not
  bool bGameInProgress = NULL != m_mpSheet->GetGame();

  // Get the number of players that are selected
  UINT cSelectedItems = m_listPlayers.GetSelectedCount();

  // Enable/disable the controls as needed
  m_staticSendChat.EnableWindow(bGameInProgress && cSelectedItems);
  m_comboSendChat.EnableWindow(bGameInProgress && cSelectedItems);
  m_btnBootPlayers.EnableWindow(bGameInProgress && cSelectedItems);
}

void CPagePlayers::SendChat() 
{
  CWaitCursor wait;

  // Update data members from fields
  UpdateData();

  // Get the number of players that are selected
  UINT cSelectedItems = m_listPlayers.GetSelectedCount();
  if (!cSelectedItems)
    return;

  // Send the chat message to the selected users
  m_strSendChat.TrimLeft();
  m_strSendChat.TrimRight();
  if (!m_strSendChat.IsEmpty())
  {
    // Look for the text in the MRU list
    int iFound = m_comboSendChat.FindStringExact(-1, m_strSendChat);
    if (iFound >= 0)
      m_comboSendChat.DeleteString(iFound);

    // Insert the string at the top of the combo box list
    m_comboSendChat.InsertString(0, m_strSendChat);

    // Limit the number of chats in the MRU combo
    int cStrings;
    do
    {
      cStrings = m_comboSendChat.GetCount();
      if (cStrings > c_cMaxChatsInRegistry)
      {
        m_comboSendChat.DeleteString(cStrings - 1);        
      }

    } while (cStrings > c_cMaxChatsInRegistry);

    // Send to the whole game, if all players are selected
    CComBSTR bstrSendChat(m_strSendChat);
    if (m_listPlayers.GetItemCount() == static_cast<int>(cSelectedItems))
    {
      m_mpSheet->GetGame()->SendChat(bstrSendChat, -1);
    }
    else
    {
      // Loop through each selected user
      POSITION it = m_listPlayers.GetFirstSelectedItemPosition();
      while (it)
      {
        // Get the next item
        int iIndex = m_listPlayers.GetNextSelectedItem(it);
        _ASSERTE(-1 != iIndex);
        DWORD dwItemData = m_listPlayers.GetItemData(iIndex);

        // Interpret the item's lParam
        IAdminShip* pShip = reinterpret_cast<IAdminShip*>(dwItemData);
        _ASSERTE(pShip);

        // Send the chat to the ship
        pShip->SendChat(bstrSendChat, -1);
      }
    }

    // Clear the string
    m_strSendChat.Empty();
    UpdateData(false);

    // Update the MRU list
    SaveSendChatMRUToRegistry();
  }
}

void CPagePlayers::PopulatePlayersList()
{
  CWaitCursor wait;
  LPCSTR pszContext = "retrieving collection of players in game";

  // Clear the contents of the list
  m_listPlayers.DeleteAllItems();

  // Do nothing else if there is no game in progress
  if (NULL == m_mpSheet->GetGame())
    return;

  // Get the collection of users in the game
  IAdminUsersPtr spUsers;
  HRESULT hr = m_mpSheet->GetGame()->get_Users(&spUsers);
  if (FAILED(hr))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }

  // Get the enumerator object from the collection
  IUnknownPtr spEnumUnk;
  if (FAILED(hr = spUsers->get__NewEnum(&spEnumUnk)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }
  IEnumVARIANTPtr spEnum(spEnumUnk);
  if (NULL == spEnum)
  {
    m_mpSheet->HandleError(E_NOINTERFACE, pszContext, true);
    return;
  }

  // Iterate through each player from the enumerator
  int iIndex = 0;
  CComVariant players[32];
  do
  {
    // Fetch the next block of players from the enumerator
    ULONG cFetched;
    if (FAILED(hr = spEnum->Next(sizeofArray(players), players, &cFetched)))
    {
      m_mpSheet->HandleError(hr, pszContext, true);
      return;
    }

    // Add each player to the list
    for (ULONG i = 0; i < cFetched; ++i)
    {
      // Convert the VARIANT to IDispatch, if not already
      VARTYPE vt = V_VT(&players[i]);
      if (VT_DISPATCH != vt && VT_UNKNOWN != vt)
      {
        m_mpSheet->HandleError(DISP_E_TYPEMISMATCH, pszContext, true);
        return;
      }

      // Ensure that the IDispatch supports IAdminUser
      IAdminUserPtr spUser(V_DISPATCH(&players[i]));
      if (NULL == spUser)
      {
        m_mpSheet->HandleError(E_NOINTERFACE, pszContext, true);
        return;
      }

      // Clear the VARIANT in the array
      players[i].Clear();

      // Get the ship object of the user
      IAdminShipPtr spShip;
      if (FAILED(hr = spUser->get_Ship(&spShip)))
      {
        m_mpSheet->HandleError(hr, pszContext, true);
        return;
      }

      // Get the interesting properties of the User
      CComBSTR bstrName, bstrTeam, bstrSector;
      // Model Name
      spShip->get_Name(&bstrName);
      // Team
      IAGCTeamPtr spTeam;
      spShip->get_Team(&spTeam);
      if (NULL != spTeam)
        spTeam->get_Name(&bstrTeam);
      // Sector
      IAGCSectorPtr spSector;
      spShip->get_Sector(&spSector);
      if (NULL != spSector)
        spSector->get_Name(&bstrSector);

      // Add the ship to the list
      USES_CONVERSION;
      m_listPlayers.InsertItem(iIndex, OLE2CT(bstrName));
      m_listPlayers.SetItemText(iIndex, c_iColumnTeam, OLE2CT(bstrTeam));
      m_listPlayers.SetItemText(iIndex, c_iColumnSector, OLE2CT(bstrSector));
      m_listPlayers.SetItemData(iIndex, reinterpret_cast<DWORD>(spShip.Detach()));
      ++iIndex;
    }

  } while (S_FALSE != hr);

}

void CPagePlayers::AddPlayer(IAGCEvent* pEvent)
{
  CWaitCursor wait;
  LPCSTR pszContext = "adding new player to the list";
  HRESULT hr;

  // Get the interesting properties of the event
  CComBSTR bstrName;
  pEvent->get_SubjectName(&bstrName);
  long idPlayer = -1;
  pEvent->get_SubjectID(&idPlayer);

  // Name must not be empty
  _ASSERTE(bstrName.Length());

  // Name must not already be in list
  USES_CONVERSION;
  LV_FINDINFO lvfi = {LVFI_STRING, OLE2CT(bstrName)};
  int iIndex = m_listPlayers.FindItem(&lvfi);
  _ASSERTE(-1 == iIndex);

  // Get the IAdminUser for the specified user
  IAdminUserPtr spUser;
//  if (FAILED(hr = m_mpSheet->GetServer()->get_LookupUser(
//    static_cast<AGCUniqueID>(idPlayer), &spUser)))

  //CAllSrvUISheet* sheet = m_mpSheet;
  //IAdminServer* server = sheet->GetServer();
  hr = m_mpSheet->GetServer()->get_FindUser(bstrName, &spUser);

  if (FAILED(hr))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }

  // Get the ship object of the user
  IAdminShipPtr spShip;
  if (FAILED(hr = spUser->get_Ship(&spShip)))
  {
    m_mpSheet->HandleError(hr, pszContext, true);
    return;
  }

  // Add the user to the list
  iIndex = m_listPlayers.GetItemCount();
  m_listPlayers.InsertItem(iIndex, OLE2CT(bstrName));
  m_listPlayers.SetItemText(iIndex, c_iColumnTeam, TEXT(""));
  m_listPlayers.SetItemText(iIndex, c_iColumnSector, TEXT(""));
  m_listPlayers.SetItemData(iIndex, reinterpret_cast<DWORD>(spShip.Detach()));
}

void CPagePlayers::RemovePlayer(IAGCEvent* pEvent)
{
  // Get the interesting properties of the event
  CComBSTR bstrName;
  pEvent->get_SubjectName(&bstrName);

  // Name must not be empty
  _ASSERTE(bstrName.Length());

  // Name must already be in list
  USES_CONVERSION;
  LV_FINDINFO lvfi = {LVFI_STRING, OLE2CT(bstrName)};
  int iIndex = m_listPlayers.FindItem(&lvfi);
  _ASSERTE(-1 != iIndex);

  // Remove the item from the list
  m_listPlayers.DeleteItem(iIndex);
}

void CPagePlayers::SectorOrTeamChange(IAGCEvent* pEvent, int iColumn,
  LPCOLESTR pszProperty)
{
  CWaitCursor wait;

  // Get the interesting properties of the event
  CComBSTR bstrName, bstrProperty;
  pEvent->get_SubjectName(&bstrName);
  CComVariant varNewPropertyName;
  pEvent->get_Property(&CComVariant(pszProperty), &varNewPropertyName);
  varNewPropertyName.ChangeType(VT_BSTR);
  bstrProperty = V_BSTR(&varNewPropertyName);

  // Name must not be empty
  _ASSERTE(bstrName.Length());

  // Find the list item with the specified name
  USES_CONVERSION;
  LV_FINDINFO lvfi = {LVFI_STRING, OLE2CT(bstrName)};
  int iIndex = m_listPlayers.FindItem(&lvfi);
  _ASSERTE(iIndex >= 0);

  // Change the sub-item of the list item
  m_listPlayers.SetItemText(iIndex, iColumn,
    bstrProperty.Length() ? OLE2CT(bstrProperty) : TEXT(""));
}

void CPagePlayers::TeamInfoChange(IAGCEvent* pEvent)
{
  CWaitCursor wait;

  // Get the interesting properties of the event
  CComBSTR bstrTeam, bstrTeamNew;
  pEvent->get_SubjectName(&bstrTeam);
  CComVariant varTeamNew;
  pEvent->get_Property(&CComVariant(L"NewTeamName"), &varTeamNew);
  varTeamNew.ChangeType(VT_BSTR);
  CString strTeam(bstrTeam), strTeamNew(V_BSTR(&varTeamNew));

  // Name must not be empty
  _ASSERTE(!strTeam.IsEmpty());

  // Loop through each item in the list
  int cPlayers = m_listPlayers.GetItemCount();
  for (int i = 0; i < cPlayers; ++i)
  {
    // Get the list item's team name
    CString str(m_listPlayers.GetItemText(i, c_iColumnTeam));

    // Change the team name of this item, if it has the old team name
    if (str == strTeam)
      m_listPlayers.SetItemText(i, c_iColumnTeam, strTeamNew);
  }
}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

BOOL CPagePlayers::OnInitDialog() 
{
  // Register for events of interest
  m_mpSheet->GetSession()->ActivateEvents(EventID_GameCreated, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_GameDestroyed, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_LoginGame, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_LogoutGame, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_ShipChangesSectors, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_LeaveTeam, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_JoinTeam, -1);
  m_mpSheet->GetSession()->ActivateEvents(EventID_TeamInfoChange, -1);

  // Perform default processing
  CPropertyPage::OnInitDialog();

  // Initialize the AutoSize object
  m_AutoSizer.SetWindowAndRules(*this, _AutoSizerMap);

  // Set the list view control extended styles
  DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP;
  m_listPlayers.SetExtendedStyle(dwExStyle);

  // Add columns to the list view control
  m_listPlayers.InsertColumn(c_iColumnName  , CString((LPCSTR)IDS_COLUMN_PLAYER));
  m_listPlayers.InsertColumn(c_iColumnTeam  , CString((LPCSTR)IDS_COLUMN_TEAM  ));
  m_listPlayers.InsertColumn(c_iColumnSector, CString((LPCSTR)IDS_COLUMN_SECTOR));

  // Load previous values from registry
  LoadFromRegistry();

  // Update fields from data members
  UpdateData(false);

  // Update the UI
  UpdateUI();

  // Populate the list of players
  PopulatePlayersList();

  // Set the focus to the first tabstop control
  return true;
}

void CPagePlayers::OnEndDragPlayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  HD_NOTIFY* phdn = (HD_NOTIFY*)pNMHDR;
  if (pNMHDR->hwndFrom != m_listPlayers.GetHeaderCtrl()->GetSafeHwnd())
    return;

  // Defer saving the new column ordering for a few seconds
  KillTimer(c_idTimerReorder);
  SetTimer(c_idTimerReorder, c_dwDelayReorder, NULL);
  *pResult = 0;
}

void CPagePlayers::OnEndTrackPlayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  HD_NOTIFY* phdn = (HD_NOTIFY*)pNMHDR;
  if (pNMHDR->hwndFrom != m_listPlayers.GetHeaderCtrl()->GetSafeHwnd())
    return;

  // Defer saving the new column widths until controls have updated
  KillTimer(c_idTimerResize);
  SetTimer(c_idTimerResize, c_dwDelayResize, NULL);
  *pResult = 0;
}

void CPagePlayers::OnTimer(UINT_PTR nIDEvent) 
{
  switch (nIDEvent)
  {
    case c_idTimerReorder:
      KillTimer(nIDEvent);
      SavePlayerListColumnOrderToRegistry();
      return;
    case c_idTimerResize:
      KillTimer(nIDEvent);
      SavePlayerListColumnWidthsToRegistry();
      return;
  }
  
  // Perform default processing
  CPropertyPage::OnTimer(nIDEvent);
}

void CPagePlayers::OnItemChangedPlayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Interpret the message parameters
  NM_LISTVIEW* pNMLV = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);

  // Update the UI if an item selection has changed
  if (pNMLV->uChanged & LVIF_STATE)
    UpdateUI();

  // Obligatory result code
  *pResult = 0;
}

void CPagePlayers::OnDeleteItemPlayerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
  // Interpret the message parameters
  NM_LISTVIEW* pNMLV = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);

  // Interpret the item's lParam
  IAdminShip* pShip = reinterpret_cast<IAdminShip*>(pNMLV->lParam);
  _ASSERTE(pShip);

  // Release the interface
  pShip->Release();  
  
  // Obligatory result code
  *pResult = 0;
}

void CPagePlayers::OnBootPlayers() 
{
  HRESULT hr;
  LPCSTR pszContext = "booting player from the game";

  // Get the count of selected players
  int cSelected = m_listPlayers.GetSelectedCount();
  _ASSERTE(cSelected);

  // Display a confirmation dialog
  CString strText;
  if (1 == cSelected)
    strText.LoadString(IDS_BOOT_SELECTED_ONE);
  else
    strText.Format(IDS_BOOT_SELECTED, cSelected);

  // Display the message box
  UINT nFlags = MB_ICONQUESTION | MB_YESNO;
  int nResponse = MessageBox(strText, NULL, nFlags);
  if (IDYES != nResponse)
    return;

  // Loop through each selected user
  POSITION it = m_listPlayers.GetFirstSelectedItemPosition();
  while (it)
  {
    // Get the next item
    int iIndex = m_listPlayers.GetNextSelectedItem(it);
    _ASSERTE(-1 != iIndex);
    DWORD dwItemData = m_listPlayers.GetItemData(iIndex);

    // Interpret the item's lParam
    IAdminShip* pShip = reinterpret_cast<IAdminShip*>(dwItemData);
    _ASSERTE(pShip);

    // Get the User property of the Ship
    IAdminUserPtr spUser;
    if (FAILED(hr = pShip->get_User(&spUser)))
    {
      m_mpSheet->HandleError(hr, pszContext, false);
      return;
    }

    // Boot the player from the game
    if (FAILED(hr = spUser->Boot()))
    {
      m_mpSheet->HandleError(hr, pszContext, false);
      return;
    }
  }

  // Give the focus to the list control
  m_listPlayers.SetFocus();

  // Remove the "Default Button" style
  m_btnBootPlayers.SetButtonStyle(
    m_btnBootPlayers.GetButtonStyle() & ~BS_DEFPUSHBUTTON);
}
