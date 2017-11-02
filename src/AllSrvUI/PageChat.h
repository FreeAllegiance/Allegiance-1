#ifndef __PageChat_h__
#define __PageChat_h__

#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// PageChat.h : header file
//

#include "DlgChatPreferences.h"
#include "AutoSizer.h"


/////////////////////////////////////////////////////////////////////////////
// Forward Declaration

class CAllSrvUISheet;
class CAdminPageThreadWnd;


/////////////////////////////////////////////////////////////////////////////
// CPageChat dialog

class CPageChat : public CPropertyPage
{
private:
	CAllSrvUISheet *m_mpSheet;

// Declarations
public:
  DECLARE_DYNAMIC(CPageChat)
  DECLARE_MESSAGE_MAP()

// Construction
public:
  CPageChat(CAllSrvUISheet *pSheet);

// Attributes
public:

// Overrides
public:
  void OnEvent(IAGCEvent* pEvent);
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CPageChat)
  public:
  virtual BOOL OnSetActive();
  virtual BOOL PreTranslateMessage(MSG* pMsg);


  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
  //}}AFX_VIRTUAL

// Implementation
protected:
 /* CAllSrvUISheet* GetSheet()
  {
    return reinterpret_cast<CAllSrvUISheet*>(GetParent());
  }*/
  void LoadFromRegistry();
  void SaveToRegistry();
  void SaveChatListColumnOrderToRegistry();
  void SaveChatListColumnWidthsToRegistry();
  void SaveSendChatMRUToRegistry();
  void SaveSendChatAllGames();
  void TrimExcessChats();
  void UpdateActivateAdminPage();
  void UpdateActivateChatMessage();
  void UpdateUI(bool bUpdateData = false);
  void SendChat();
  static UINT AdminPageMsgThreadThunk(void* pvParam);
  static UINT AdminPageRunThreadThunk(void* pvParam);
  void AdminPageMsgThreadProc();
  void AdminPageRunThreadProc();

// Message Handlers
protected:
  // Generated message map functions
  //{{AFX_MSG(CPageChat)
  virtual BOOL OnInitDialog();
  afx_msg void OnEndDragChatList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnEndTrackChatList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnChatPreferences();
  afx_msg void OnChatListClear();
  afx_msg void OnDestroy();
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnSendChatAllGames();
  //}}AFX_MSG

// Data Members
protected:
  //{{AFX_DATA(CPageChat)
  enum { IDD = IDD_PAGE_CHAT };
  CButton m_btnSendChatAllGames;
  CListCtrl m_listChat;
  CStatic m_staticSendChat;
  CComboBox m_comboSendChat;
  CButton m_btnSendChat;
  CString m_strSendChat;
  BOOL  m_bSendChatAllGames;
  //}}AFX_DATA
protected:
  enum
  {
    c_iColumnSender,
    c_iColumnTarget,
    c_iColumnMessage,
    c_cColumns
  };
  enum
  {
    c_idTimerReorder = 1,
    c_idTimerResize,
  };
  enum
  {
    c_dwDelayReorder = 200,
    c_dwDelayResize = 200,
  };
  ITCStringsPtr        m_spStrings;
  CDlgChatPreferences  m_Prefs;
  bool                 m_bAdminPageActivated;
  bool                 m_bChatActivated;
  CWinThread*          m_pthAdminPageMsg;
  CWinThread*          m_pthAdminPageRun;
  CEvent               m_evtAdminPageMsgThreadStartup;
  CEvent               m_evtAdminPageRunThreadStartup;
  CAdminPageThreadWnd* m_pWndAdminPageMsgThread;
  CAdminPageThreadWnd* m_pWndAdminPageRunThread;
  CAutoSizer           m_AutoSizer;

// Friends
protected:
  friend class CInvisibleWnd;
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
/////////////////////////////////////////////////////////////////////////////

#endif // !__PageChat_h__
