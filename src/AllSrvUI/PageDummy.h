#ifndef __PageDummy_h__
#define __PageDummy_h__

#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// PageDummy.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// Forward Declaration

class CAllSrvUISheet;


/////////////////////////////////////////////////////////////////////////////
// CPageDummy dialog

class CPageDummy : public CPropertyPage
{
// Declarations
public:
  DECLARE_DYNAMIC(CPageDummy)
  DECLARE_MESSAGE_MAP()

// Construction
public:
  CPageDummy();

// Attributes
public:

// Overrides
public:
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CPageDummy)
  public:
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  /*CAllSrvUISheet* GetSheet()
  {
    return reinterpret_cast<CAllSrvUISheet*>(GetParent());
  }*/

// Message Handlers
protected:
  // Generated message map functions
  //{{AFX_MSG(CPageDummy)
  //}}AFX_MSG

// Data Members
protected:
  //{{AFX_DATA(CPageDummy)
  enum { IDD = IDD_PAGE_DUMMY };
  //}}AFX_DATA
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
/////////////////////////////////////////////////////////////////////////////

#endif // !__PageDummy_h__
