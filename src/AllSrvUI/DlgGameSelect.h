#ifndef __DlgGameSelect_h__
#define __DlgGameSelect_h__

#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// DlgGameSelect.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// Forward Declaration

class CAllSrvUISheet;


/////////////////////////////////////////////////////////////////////////////
// CDlgGameSelect dialog

class CDlgGameSelect : public CDialog
{
private:
	CAllSrvUISheet *m_mpSheet;

// Declarations
public:
  DECLARE_MESSAGE_MAP()

// Construction
public:
  CDlgGameSelect(CAllSrvUISheet* pSheet);

// Attributes:
public:
  IAdminGame* GetSelectedGame() {return m_spGame;}

// Overrides
public:
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CDlgGameSelect)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  /*CAllSrvUISheet* GetSheet()
  {
    return reinterpret_cast<CAllSrvUISheet*>(GetParent());
  }*/
  void PopulateGamesList();

// Message Handlers
protected:
  // Generated message map functions
  //{{AFX_MSG(CDlgGameSelect)
  virtual BOOL OnInitDialog();
  afx_msg void OnDeleteItemGameList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnItemChangedGameList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnDblClickGameList(NMHDR* pNMHDR, LRESULT* pResult);
  //}}AFX_MSG

// Data Members
protected:
  //{{AFX_DATA(CDlgGameSelect)
  enum { IDD = IDD_GAME_SELECT };
  CListCtrl  m_listGames;
  //}}AFX_DATA
protected:
  IAdminGamePtr m_spGame;
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
/////////////////////////////////////////////////////////////////////////////

#endif // !__DlgGameSelect_h__
