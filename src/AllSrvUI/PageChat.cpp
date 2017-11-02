/////////////////////////////////////////////////////////////////////////////
// PageChat.cpp : implementation file
//

#include "stdafx.h"
#include "AllSrvUI.h"
#include "AllSrvUISheet.h"
#include "DlgChatPreferences.h"
#include "PageChat.h"
#include "AdminPageThreadWnd.h"

#ifdef _DEBUG
  #define new DEBUG_NEW
  #undef THIS_FILE
  static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPageChat property page

IMPLEMENT_DYNAMIC(CPageChat, CPropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Static Initializtion

BEGIN_AUTO_SIZER_MAP(_AutoSizerMap)
  AUTO_SIZER_ENTRY(IDC_CHAT_LIST_CLEAR    , LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_CHAT_LIST_CLEAR    , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_CHAT_PREFERENCES   , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_CAPTION  , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_TEXT     ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_TEXT     , TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_ALL_GAMES, LeftRight, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_SEND_CHAT_ALL_GAMES, TopBottom, 0,  Bottom, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_CHAT_LIST          ,     Right, 0,   Right, NoRefresh)
  AUTO_SIZER_ENTRY(IDC_CHAT_LIST          ,    Bottom, 0,  Bottom, NoRefresh)
END_AUTO_SIZER_MAP()


/////////////////////////////////////////////////////////////////////////////
// Message Map

BEGIN_MESSAGE_MAP(CPageChat, CPropertyPage)
  //{{AFX_MSG_MAP(CPageChat)
  ON_NOTIFY(HDN_ENDDRAG, 0, OnEndDragChatList)
  ON_NOTIFY(HDN_ENDTRACK, 0, OnEndTrackChatList)
  ON_BN_CLICKED(IDC_CHAT_PREFERENCES, OnChatPreferences)
  ON_BN_CLICKED(IDC_CHAT_LIST_CLEAR, OnChatListClear)
  ON_WM_DESTROY()
  ON_WM_TIMER()
	ON_BN_CLICKED(IDC_SEND_CHAT_ALL_GAMES, OnSendChatAllGames)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Construction

CPageChat::CPageChat(CAllSrvUISheet *pSheet) :
  CPropertyPage(CPageChat::IDD),
  m_Prefs(this),
  m_bAdminPageActivated(false),
  m_bChatActivated(false),
  m_pthAdminPageMsg(NULL),
  m_pthAdminPageRun(NULL),
  m_pWndAdminPageMsgThread(NULL),
  m_pWndAdminPageRunThread(NULL),
  m_mpSheet(pSheet)
{
  //{{AFX_DATA_INIT(CPageChat)
  m_strSendChat = _T("");
  m_bSendChatAllGames = FALSE;
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

void CPageChat::OnEvent(IAGCEvent* pEvent)
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
    case EventID_ChatMessage:
    case EventID_AdminPage:
    {
      // Determine if this is an Admin alert
      bool bAdminAlert = EventID_AdminPage == eEvent;

      // Get the GameID parameter of the event
      CComVariant varGameID;
      if (FAILED(pEvent->get_Property(&CComVariant(L"GameID"), &varGameID)))
        return;
      if (VT_BSTR != V_VT(&varGameID) || 0 != _wcsicmp(V_BSTR(&varGameID), L"TEST"))
      {
        varGameID.ChangeType(VT_I4);

        // Get the current GameID
        if (!m_mpSheet->GetGame())
          return;
        AGCGameID idGame = 0;
        if (FAILED(m_mpSheet->GetGame()->get_GameID(&idGame)))
          return;

        // Ignore chats not for this game
        if (idGame != V_I4(&varGameID))
          return;
      }

      // Add the chat to the list control
      if (!bAdminAlert || m_Prefs.m_bAdminChatList)
      {
        // Extract the event parameters
        CComBSTR bstrSender, bstrTarget, bstrMessage;
        if (FAILED(pEvent->get_SubjectName(&bstrSender)))
          return;
        CComVariant varTarget, varMessage;
        if (FAILED(pEvent->get_Property(&CComVariant(L"Target"), &varTarget)))
          return;
        if (FAILED(pEvent->get_Property(&CComVariant(L"Message"), &varMessage)))
          return;
        varTarget.ChangeType(VT_BSTR);
        bstrTarget = V_BSTR(&varTarget);
        varMessage.ChangeType(VT_BSTR);
        bstrMessage = V_BSTR(&varMessage);

        // Get the current item count
        int cItems = m_listChat.GetItemCount();

        // Determine if the last item in the list is visible
        int nLastPlus = m_listChat.GetTopIndex() + m_listChat.GetCountPerPage();
        bool bLastWasVisible = nLastPlus >= cItems;

        // Add the new item to the end
        USES_CONVERSION;
        int iIndex = cItems;
        m_listChat.InsertItem(iIndex, OLE2CT(bstrSender));
        m_listChat.SetItemText(iIndex, c_iColumnTarget, OLE2CT(bstrTarget));
        m_listChat.SetItemText(iIndex, c_iColumnMessage, OLE2CT(bstrMessage));

        // Make last item visible if last was previously
        if (bLastWasVisible)
          m_listChat.EnsureVisible(iIndex, false);
      }

      // Trim the size of the list if it has more than limit
      TrimExcessChats();

      // Was this an admin alert?
      if (bAdminAlert)
      {
        // Check preferences
        if (m_Prefs.m_bAdminMessageBox)
          m_pWndAdminPageMsgThread->PostAdminPageMsg(pEvent);
        if (m_Prefs.m_bAdminRun)
          m_pWndAdminPageRunThread->PostAdminPageRun(pEvent);
      }
      break;
    }
  }
}

BOOL CPageChat::OnSetActive()
{
  // Reload the chat MRU from the registry
  LoadFromRegistry();

  // Update the UI
  UpdateUI();

  // Perform default processing
  return CPropertyPage::OnSetActive();
}

BOOL CPageChat::PreTranslateMessage(MSG* pMsg)
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

void CPageChat::DoDataExchange(CDataExchange* pDX)
{
  // Perform default processing
  CPropertyPage::DoDataExchange(pDX);

  //{{AFX_DATA_MAP(CPageChat)
  DDX_Control(pDX, IDC_SEND_CHAT_ALL_GAMES, m_btnSendChatAllGames);
  DDX_Control(pDX, IDC_CHAT_LIST, m_listChat);
  DDX_Control(pDX, IDC_SEND_CHAT_CAPTION, m_staticSendChat);
  DDX_Control(pDX, IDC_SEND_CHAT_TEXT, m_comboSendChat);
  DDX_CBString(pDX, IDC_SEND_CHAT_TEXT, m_strSendChat);
  DDX_Check(pDX, IDC_SEND_CHAT_ALL_GAMES, m_bSendChatAllGames);
  //}}AFX_DATA_MAP
}

LRESULT CPageChat::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
  LRESULT lr;
  if (m_AutoSizer.ProcessMessage(message, wParam, lParam, &lr))
    return lr;

  // Perform default processing
  return CPropertyPage::WindowProc(message, wParam, lParam);
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

void CPageChat::LoadFromRegistry()
{
  // Allow the Preferences dialog to load its registry first
  m_Prefs.LoadFromRegistry();

  // Open/create the registry key
  CRegKey key;
  key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);

  // ChatListColumnOrder
  CComBSTR bstrColumnOrder;
  LoadRegString(key, TEXT("ChatListColumnOrder"), bstrColumnOrder);
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
    m_listChat.SetColumnOrderArray(c_cColumns, vecOrder);
  }

  // ChatListColumnWidths
  CComBSTR bstrColumnWidths;
  LoadRegString(key, TEXT("ChatListColumnWidths"), bstrColumnWidths);
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
      m_listChat.SetColumnWidth(V_I4(&i), _wtoi(bstrColumnWidths));
    }
  }
  else
  {
    for (int i = 0; i < c_cColumns; ++i)
    {
      TCHAR szColumn[_MAX_PATH];
      LVCOLUMN lvc = {LVCF_TEXT, 0, 0, szColumn, sizeofArray(szColumn)};
      m_listChat.GetColumn(i, &lvc);
      int cx = m_listChat.GetStringWidth(CString(szColumn) + TEXT("    "));
      m_listChat.SetColumnWidth(i, cx);
    }
  }

  // SendChatMRU
  m_comboSendChat.ResetContent();
  CRegKey keyMRU;
  if (ERROR_SUCCESS == keyMRU.Create(key, TEXT("SendChatMRU")))
  {
    // Read the count of strings
    DWORD cStrings = 0;
	//mdvalley: former QueryDWORDValue(TEXT, cStrings)
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

  // SendChatAllGames
  DWORD dwSendChatAllGames = 0;
  // mdvalley: formor QueryDWORDValue(TEXT, dwSend)
  key.QueryValue(dwSendChatAllGames, TEXT("SendChatAllGames"));
  m_bSendChatAllGames = !!dwSendChatAllGames;
}

void CPageChat::SaveChatListColumnOrderToRegistry()
{
  // Open/create the registry key
  CRegKey key;
  key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);

  // Get the current column ordering
  int vecOrder[c_cColumns];
  for (int iCol = 0; iCol < c_cColumns; ++iCol)
    vecOrder[iCol] = iCol;
  m_listChat.GetColumnOrderArray(vecOrder, c_cColumns);

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
  // mdvalley: SetString(text, strColumn)
  key.SetValue(strColumnOrder, TEXT("ChatListColumnOrder"));
}

void CPageChat::SaveChatListColumnWidthsToRegistry()
{
  // Open/create the registry key
  CRegKey key;
  key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI);

  // Get the column header of the chat list
  CHeaderCtrl* pHeader = m_listChat.GetHeaderCtrl();

  // Format a comma-delimited string of the column widths
  CString strColumnWidths;
  for (int i = 0; i < c_cColumns; ++i)
  {
    HDITEM hdi = {HDI_WIDTH};
    pHeader->GetItem(i, &hdi);

    if (i)
      strColumnWidths += ",";

    TCHAR szNum[16];
    strColumnWidths += _itot(hdi.cxy, szNum, 10);
  }

  // Save to the registry
  // mdvalley: SetString, str, TEXT
  key.SetValue(strColumnWidths, TEXT("ChatListColumnWidths"));
}

void CPageChat::SaveSendChatMRUToRegistry()
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
  // mdvalley: SetDWORD(text, cstring)
  keyMRU.SetValue(cStrings, TEXT(".Count"));

  // Write each string
  for (int i = 0; i < cStrings; ++i)
  {
    TCHAR szInt[16];
    CString strMRUItem;
    m_comboSendChat.GetLBText(i, strMRUItem);
	// mdvalley: SetString(_itot, str)
    keyMRU.SetValue(strMRUItem, _itot(i, szInt, 10));
  }
}

void CPageChat::SaveSendChatAllGames()
{
  // Open/create the registry key
  CRegKey key;
  if (ERROR_SUCCESS != key.Create(HKEY_LOCAL_MACHINE, HKLM_AllSrvUI))
    return;

  // SendChatAllGames
  DWORD dwSendChatAllGames = m_bSendChatAllGames;
  // mdvalley: SetDWORD(text, dwsend)
  key.SetValue(dwSendChatAllGames, TEXT("SendChatAllGames"));
}

void CPageChat::TrimExcessChats()
{
  // Only trim if there is a limit
  if (m_Prefs.m_dwChatListLimit)
  {
    int cExcess = m_listChat.GetItemCount() - m_Prefs.m_dwChatListLimit;
    if (cExcess > 0)
    {
      m_listChat.SetRedraw(false);
      while (cExcess--)
        m_listChat.DeleteItem(0);
      m_listChat.SetRedraw(true);
      m_listChat.Invalidate();
    }
  }
}

void CPageChat::UpdateActivateAdminPage()
{
  bool bAdminPageMonitored = m_Prefs.IsAdminPageMonitored();
  if (!m_bAdminPageActivated != !bAdminPageMonitored)
  {
    // Activate/deactivate the event notification
    if (bAdminPageMonitored)
      m_mpSheet->GetSession()->ActivateEvents(EventID_AdminPage, -1);
    else
      m_mpSheet->GetSession()->DeactivateEvents(EventID_AdminPage, -1);
    m_bAdminPageActivated = bAdminPageMonitored;
  }
}

void CPageChat::UpdateActivateChatMessage()
{
  bool bChatMonitored = m_Prefs.IsChatMonitored();
  if (!m_bChatActivated != !bChatMonitored)
  {
    // Activate/deactivate the event notification
    if (bChatMonitored)
      m_mpSheet->GetSession()->ActivateEvents(EventID_ChatMessage, -1);
    else
      m_mpSheet->GetSession()->DeactivateEvents(EventID_ChatMessage, -1);
    m_bChatActivated = bChatMonitored;
  }
}

void CPageChat::UpdateUI(bool bUpdateData)
{
  // Update data members from fields, if specified
  if (bUpdateData)
    UpdateData();

  // Determine if a game is in progress or not
  bool bGameInProgress = NULL != m_mpSheet->GetGame();

  // Enable/disable the controls as needed
  m_staticSendChat.EnableWindow(bGameInProgress);
  m_comboSendChat.EnableWindow(bGameInProgress);
}

void CPageChat::SendChat()
{
  // Update data members from fields
  UpdateData();

  // Send the chat message to the game
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

    // Send the chat to current game or all games, as specified
    CComBSTR bstrText(m_strSendChat);
    if (m_bSendChatAllGames && m_mpSheet->IsServerInMultiMode())
		m_mpSheet->GetServer()->SendMsg(bstrText);
    else
      m_mpSheet->GetGame()->SendChat(bstrText, -1);

    // Clear the string
    m_strSendChat.Empty();
    UpdateData(false);

    // Update the MRU list
    SaveSendChatMRUToRegistry();
  }
}

UINT CPageChat::AdminPageMsgThreadThunk(void* pvParam)
{
  // Enter this thread into a COM STA
  CoInitialize(NULL);

  // Reinterpret the specified parameter as a 'this' pointer
  CPageChat* pThis = reinterpret_cast<CPageChat*>(pvParam);

  // Call through the pointer
  pThis->AdminPageMsgThreadProc();

  // Shutdown the STA
  CoUninitialize();

  // Obligatory return code
  return 0;
}

UINT CPageChat::AdminPageRunThreadThunk(void* pvParam)
{
  // Enter this thread into a COM STA
  CoInitialize(NULL);

  // Reinterpret the specified parameter as a 'this' pointer
  CPageChat* pThis = reinterpret_cast<CPageChat*>(pvParam);

  // Call through the pointer
  pThis->AdminPageRunThreadProc();

  // Shutdown the STA
  CoUninitialize();

  // Obligatory return code
  return 0;
}

void CPageChat::AdminPageMsgThreadProc()
{
  // Get the main window handle
  HWND hwnd = GetSafeHwnd();

  // Create a window on this thread
  CAdminPageThreadWnd wnd;
  wnd.Create();
  m_pWndAdminPageMsgThread = &wnd;

  // Signal that we've initialized
  m_evtAdminPageMsgThreadStartup.SetEvent();

  // Enter a message loop
  MSG msg;
  while (::GetMessage(&msg, NULL, 0, 0))
  {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  // Clear the window pointer
  m_pWndAdminPageMsgThread = NULL;
}

void CPageChat::AdminPageRunThreadProc()
{
  // Get the main window handle
  HWND hwnd = GetSafeHwnd();

  // Create a window on this thread
  CAdminPageThreadWnd wnd;
  wnd.Create(m_Prefs.m_strShortcutPath);
  m_pWndAdminPageRunThread = &wnd;

  // Signal that we've initialized
  m_evtAdminPageRunThreadStartup.SetEvent();

  // Enter a message loop
  MSG msg;
  while (::GetMessage(&msg, NULL, 0, 0))
  {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  // Clear the window pointer
  m_pWndAdminPageRunThread = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// Message Handlers

BOOL CPageChat::OnInitDialog()
{
  // Register for events of interest
  m_mpSheet->GetSession()->ActivateEvents(EventID_GameDestroyed, -1);

  // Perform default processing
  CPropertyPage::OnInitDialog();

  // Initialize the AutoSize object
  m_AutoSizer.SetWindowAndRules(*this, _AutoSizerMap);

  // Show/hide buttons depending on server mode
  bool bMultiMode = m_mpSheet->IsServerInMultiMode();
  m_btnSendChatAllGames.ShowWindow(bMultiMode ? SW_SHOW : SW_HIDE);
  m_btnSendChatAllGames.EnableWindow(bMultiMode);

  // Create worker threads
  m_pthAdminPageMsg = AfxBeginThread(AdminPageMsgThreadThunk, this,
    THREAD_PRIORITY_NORMAL, 4096);
  m_pthAdminPageRun = AfxBeginThread(AdminPageRunThreadThunk, this,
    THREAD_PRIORITY_NORMAL, 4096);
  _ASSERTE(m_pthAdminPageMsg);
  _ASSERTE(m_pthAdminPageRun);

  // Allow threads time to get our window handle and initialize USER32
  HANDLE phEvents[] =
  {
    m_evtAdminPageMsgThreadStartup,
    m_evtAdminPageRunThreadStartup,
  };
  const DWORD c_cEvents = sizeofArray(phEvents);
  WaitForMultipleObjects(c_cEvents, phEvents, true, INFINITE);

  // Set the list view control extended styles
  DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP;
  m_listChat.SetExtendedStyle(dwExStyle);

  // Add columns to the list view control
  m_listChat.InsertColumn(0, CString((LPCSTR)IDS_COLUMN_SENDER ));
  m_listChat.InsertColumn(1, CString((LPCSTR)IDS_COLUMN_TARGET ));
  m_listChat.InsertColumn(2, CString((LPCSTR)IDS_COLUMN_MESSAGE));

  // Load previous values from registry
  LoadFromRegistry();

  // Register for necessary admin/chat events
  UpdateActivateAdminPage();
  UpdateActivateChatMessage();

  // Update fields from data members
  UpdateData(false);

  // Update the UI
  UpdateUI();

  // Set the focus to the SendChat combo box
  m_comboSendChat.SetFocus();
  return false;
}

void CPageChat::OnEndDragChatList(NMHDR* pNMHDR, LRESULT* pResult)
{
  HD_NOTIFY* phdn = (HD_NOTIFY*)pNMHDR;
  if (pNMHDR->hwndFrom != m_listChat.GetHeaderCtrl()->GetSafeHwnd())
    return;

  // Defer saving the new column ordering until controls have updated
  KillTimer(c_idTimerReorder);
  SetTimer(c_idTimerReorder, c_dwDelayReorder, NULL);
  *pResult = 0;
}

void CPageChat::OnEndTrackChatList(NMHDR* pNMHDR, LRESULT* pResult)
{
  HD_NOTIFY* phdn = (HD_NOTIFY*)pNMHDR;
  if (pNMHDR->hwndFrom != m_listChat.GetHeaderCtrl()->GetSafeHwnd())
    return;

  // Defer saving the new column widths until controls have updated
  KillTimer(c_idTimerResize);
  SetTimer(c_idTimerResize, c_dwDelayResize, NULL);
  *pResult = 0;
}

void CPageChat::OnChatPreferences()
{
  // Display the Chat Preferences dialog box
  int nResponse = m_Prefs.DoModal();
  if (IDCANCEL == nResponse)
    return;

  // Trim the size of the list if it has more than new limit
  TrimExcessChats();

  // Register for necessary admin/chat events
  UpdateActivateAdminPage();
  UpdateActivateChatMessage();
}

void CPageChat::OnChatListClear()
{
  m_listChat.SetRedraw(false);
  m_listChat.DeleteAllItems();
  m_listChat.SetRedraw(true);
  m_listChat.Invalidate();
}

void CPageChat::OnDestroy()
{
  // Get the worker thread handles
  HANDLE phObjects[] =
  {
    m_pthAdminPageMsg->m_hThread,
    m_pthAdminPageRun->m_hThread,
  };
  const DWORD c_cObjects = sizeofArray(phObjects);

  // Tell each thread to exit
  m_pthAdminPageMsg->PostThreadMessage(WM_QUIT, 0, 0);
  m_pthAdminPageRun->PostThreadMessage(WM_QUIT, 0, 0);

  // Wait a reasonable amount of time for the threads to exit
  const DWORD c_dwTimeout = 5000;
  WaitForMultipleObjects(c_cObjects, phObjects, true, c_dwTimeout);

  // Perform default processing
  CPropertyPage::OnDestroy();
}

void CPageChat::OnTimer(UINT_PTR nIDEvent)
{
  switch (nIDEvent)
  {
    case c_idTimerReorder:
      KillTimer(nIDEvent);
      SaveChatListColumnOrderToRegistry();
      return;
    case c_idTimerResize:
      KillTimer(nIDEvent);
      SaveChatListColumnWidthsToRegistry();
      return;
  }

  // Perform default processing
  CPropertyPage::OnTimer(nIDEvent);
}

void CPageChat::OnSendChatAllGames()
{
  // Update data members from fields
  UpdateData();

  // Update the registry
  SaveSendChatAllGames();
}

