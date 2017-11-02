#ifndef __PagePlayers_h__
#define __PagePlayers_h__

#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// PagePlayers.h : header file
//

#include "AutoSizer.h"


/////////////////////////////////////////////////////////////////////////////
// Forward Declaration

class CAllSrvUISheet;


/////////////////////////////////////////////////////////////////////////////
// CPagePlayers dialog

class CPagePlayers : public CPropertyPage
{
private: 
	CAllSrvUISheet *m_mpSheet;

// Declarations
public:
  DECLARE_DYNAMIC(CPagePlayers)
  DECLARE_MESSAGE_MAP()

// Construction
public:
  CPagePlayers(CAllSrvUISheet *pSheet);

// Attributes
public:

// Overrides
public:
  void OnEvent(IAGCEvent* pEvent);
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CPagePlayers)
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
  void SavePlayerListColumnOrderToRegistry();
  void SavePlayerListColumnWidthsToRegistry();
  void SaveSendChatMRUToRegistry();
  void UpdateUI(bool bUpdateData = false);
  void SendChat();
  void PopulatePlayersList();
  void AddPlayer(IAGCEvent* pEvent);
  void RemovePlayer(IAGCEvent* pEvent);
  void SectorOrTeamChange(IAGCEvent* pEvent, int iColumn,
    LPCOLESTR pszProperty);
  void TeamInfoChange(IAGCEvent* pEvent);

// Message Handlers
protected:
  // Generated message map functions
  //{{AFX_MSG(CPagePlayers)
  virtual BOOL OnInitDialog();
  afx_msg void OnEndDragPlayerList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnEndTrackPlayerList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnItemChangedPlayerList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnDeleteItemPlayerList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnBootPlayers();
  //}}AFX_MSG

// Data Members
protected:
  //{{AFX_DATA(CPagePlayers)
  enum { IDD = IDD_PAGE_PLAYERS };
  CButton m_btnBootPlayers;
  CListCtrl m_listPlayers;
  CStatic m_staticSendChat;
  CComboBox m_comboSendChat;
  CButton m_btnSendChat;
  CString m_strSendChat;
  //}}AFX_DATA
protected:
  enum
  {
    c_iColumnName,
    c_iColumnTeam,
    c_iColumnSector,
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
    c_dwDelayResize = 200
  };
  ITCStringsPtr m_spStrings;
  CAutoSizer    m_AutoSizer;
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
/////////////////////////////////////////////////////////////////////////////

#endif // !__PagePlayers_h__
