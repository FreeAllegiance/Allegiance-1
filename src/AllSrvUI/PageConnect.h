#ifndef __PageConnect_h__
#define __PageConnect_h__

#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// PageConnect.h : header file
//

#include "AutoSizer.h"


/////////////////////////////////////////////////////////////////////////////
// Forward Declaration

class CAllSrvUISheet;


/////////////////////////////////////////////////////////////////////////////
// CPageConnect dialog

class CPageConnect : public CPropertyPage
{
private:
	CAllSrvUISheet *m_mpSheet;

// Declarations
public:
  DECLARE_DYNAMIC(CPageConnect)
  DECLARE_MESSAGE_MAP()

// Construction
public:
  CPageConnect(CAllSrvUISheet *pSheet);

// Attributes
public:

// Overrides
public:
  void OnEvent(IAGCEvent* pEvent);
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CPageConnect)
  public:
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
  //}}AFX_VIRTUAL

// Implementation
protected:
  /*CAllSrvUISheet* GetSheet()
  {
    return reinterpret_cast<CAllSrvUISheet*>(GetParent());
  }*/
  static UINT CreateSessionThreadThunk(void* pvParam);
  void CreateSessionThreadProc();

// Message Handlers
protected:
  // Generated message map functions
  //{{AFX_MSG(CPageConnect)
  virtual BOOL OnInitDialog();
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  //}}AFX_MSG
  afx_msg LRESULT OnSessionSucceeded(WPARAM, LPARAM);
  afx_msg LRESULT OnSessionFailed(WPARAM, LPARAM);

// Data Members
protected:
  //{{AFX_DATA(CPageConnect)
  enum { IDD = IDD_PAGE_CONNECT };
  CProgressCtrl m_progress;
  //}}AFX_DATA
protected:
  enum {wm_SessionSucceeded = WM_APP, wm_SessionFailed,};
  CWinThread* m_pthCreateSession;
  CEvent      m_evtCreateSession;
  CAutoSizer  m_AutoSizer;
public:
	afx_msg void OnNMCustomdrawProgressConnecting(NMHDR *pNMHDR, LRESULT *pResult);
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
/////////////////////////////////////////////////////////////////////////////

#endif // !__PageConnect_h__
