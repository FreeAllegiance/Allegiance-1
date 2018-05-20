/////////////////////////////////////////////////////////////////////////////
// PigAccountDispenser.h : Declaration of the CPigAccountDispenser

#ifndef __PigAccountDispenser_h__
#define __PigAccountDispenser_h__

#include "PigAccts.h"
#include <..\TCLib\ObjectLock.h>
#include "resource.h"       // main symbols
#include <objsafe.h>


/////////////////////////////////////////////////////////////////////////////
// CPigAccountDispenser
class ATL_NO_VTABLE CPigAccountDispenser : 
  public IDispatchImpl<IPigAccountDispenser, &IID_IPigAccountDispenser, &LIBID_PigsLib>,
  public ISupportErrorInfo,
  public CComObjectRootEx<CComMultiThreadModel>,
  public CComCoClass<CPigAccountDispenser, &CLSID_PigAccountDispenser>
{
// Declarations
public:
  DECLARE_CLASSFACTORY_SINGLETON(CPigAccountDispenser)
  DECLARE_REGISTRY_RESOURCEID(IDR_PigAccountDispenser)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

// Category Map
public:
  BEGIN_CATEGORY_MAP(CPigAccountDispenser)
    IMPLEMENTED_CATEGORY(CATID_PigObjects)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
  END_CATEGORY_MAP()

// Interface Map
public:
  BEGIN_COM_MAP(CPigAccountDispenser)
    COM_INTERFACE_ENTRY(IPigAccountDispenser)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
  END_COM_MAP()

// Construction / Destruction
public:
  CPigAccountDispenser();
  HRESULT FinalConstruct();
  void FinalRelease();

// Attributes
public:
  void AccountReleased(_bstr_t bstrName);

// Implementation
protected:
  HRESULT LoadAccounts();

// ISupportsErrorInfo Interface Methods
public:
  STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

// IPigAccountDispenser Interface Methods
public:
  STDMETHODIMP get_NextAvailable(IPigAccount** ppAccount);
  STDMETHODIMP get_Names(ITCStrings** ppNames);
  STDMETHODIMP get_NamesAvailable(ITCStrings** ppNames);
  STDMETHODIMP get_NamesInUse(ITCStrings** ppNames);

// Types
protected:
  struct XAccount
  {
    _bstr_t m_bstrName;
    _bstr_t m_bstrPassword;
	_bstr_t m_bstrCDKey;
    bool    m_bAvailable:1;
  };
  typedef TCObjectLock<CPigAccountDispenser> XLock;
  typedef std::vector<XAccount>              XAccounts;
  typedef XAccounts::iterator                XAccountIt;

// Data Members
protected:
  XAccounts  m_Accounts;
  XAccountIt m_itAvailable;
};


/////////////////////////////////////////////////////////////////////////////

#endif //__PigAccountDispenser_h__
