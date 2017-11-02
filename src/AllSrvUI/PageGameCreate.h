#ifndef __PageGameCreate_h__
#define __PageGameCreate_h__

#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// PageGameCreate.h : header file
//

#include "AutoSizer.h"


/////////////////////////////////////////////////////////////////////////////
// Forward Declaration

class CAllSrvUISheet;


/////////////////////////////////////////////////////////////////////////////
// CPageGameCreate dialog

class CPageGameCreate :
  public CPropertyPage
{
private:
	CAllSrvUISheet *m_mpSheet;

// Declarations
public:
  DECLARE_DYNAMIC(CPageGameCreate)
  DECLARE_MESSAGE_MAP()

// Construction
public:
  CPageGameCreate(CAllSrvUISheet *pSheet);

// Attributes
public:

// Overrides
public:
  void OnEvent(IAGCEvent* pEvent);
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CPageGameCreate)
  public:
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
  void UpdateUI(bool bUpdateData = false);
  void UpdateFromGameParameters();

// Message Handlers
protected:
  // Generated message map functions
  //{{AFX_MSG(CPageGameCreate)
  virtual BOOL OnInitDialog();
  afx_msg void OnPrivateGame();
  afx_msg void OnPublicGame();
  afx_msg void OnChangeGameName();
  afx_msg void OnChangeGameCore(); // KGJV: core
  afx_msg void OnGameSettings();
  afx_msg void OnGameSelect();
  afx_msg void OnGameCreate();
  afx_msg void OnGameDestroy();
  //}}AFX_MSG

// Data Members
protected:
  //{{AFX_DATA(CPageGameCreate)
  enum { IDD = IDD_PAGE_GAME_CREATE };
  CStatic m_staticMaxPlayers;
  CComboBox m_comboMaxPlayers;
  CStatic m_staticGamePassword;
  CStatic m_staticGameName;
  CStatic m_staticGameCore; // KGJV - added core
  CButton m_btnGameDestroy;
  CButton m_btnGameCreate;
  CButton m_btnGameSelect;
  CButton m_btnGameSettings;
  CEdit m_editGamePassword;
  CEdit m_editGameName; 
  CEdit m_editGameCore; // KGJV - added core
  int   m_iGameVisibility;
  CString m_strGameName;
  CString m_strGamePassword;
  CString m_strGameCore; // KGJV - added core
  CButton m_btnPublicGame;
  CButton m_btnPrivateGame;
  int   m_nMaxPlayers;
  CButton m_btnLockGameOpen;
  CButton m_btnAET; // KGJV - added AET
  BOOL  m_bLockGameOpen;
  BOOL  m_bAET; // KGJV - added AET
  //}}AFX_DATA

protected:
  IAGCGameParametersPtr m_spGameParameters;
  CAutoSizer            m_AutoSizer;
  CString               m_strLobbyServer;
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
/////////////////////////////////////////////////////////////////////////////

#endif // !__PageGameCreate_h__
