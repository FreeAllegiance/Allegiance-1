/////////////////////////////////////////////////////////////////////////////
// PigAccount.h : Declaration of the CPigAccount class

#ifndef __PigAccount_h__
#define __PigAccount_h__

#include "resource.h"       // main symbols
#include <..\TCLib\ObjectLock.h>
#include "PigAccts.h"


/////////////////////////////////////////////////////////////////////////////
// Forward Declarations

class CPigAccountDispenser;


/////////////////////////////////////////////////////////////////////////////
// CPigAccount
class ATL_NO_VTABLE CPigAccount : 
  public IDispatchImpl<IPigAccount, &IID_IPigAccount, &LIBID_PigsLib>,
  public ISupportErrorInfo,
  public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPigAccount, &CLSID_PigAccount>
{
// Declarations
public:
  DECLARE_REGISTRY_RESOURCEID(IDR_PigAccount)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

// Category Map
public:
  BEGIN_CATEGORY_MAP(CPigAccount)
    IMPLEMENTED_CATEGORY(CATID_PigObjects)
  END_CATEGORY_MAP()

// Interface Map
public:
  BEGIN_COM_MAP(CPigAccount)
    COM_INTERFACE_ENTRY(IPigAccount)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
  END_COM_MAP()

// Construction / Destruction
public:
  CPigAccount();
  HRESULT FinalConstruct();
  void FinalRelease();
  HRESULT Init(CPigAccountDispenser* pDispenser, _bstr_t bstrName,
    _bstr_t bstrPassword, _bstr_t bstrCdKey);

// ISupportErrorInfo Interface Methods
public:
  STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

// IPigAccount Interface Methods
public:
  STDMETHODIMP get_Name(BSTR* pbstrName);
  STDMETHODIMP get_Password(BSTR* pbstrPassword);
  STDMETHODIMP get_CdKey(BSTR* pbstrCdKey);

// Types
protected:
  typedef TCObjectLock<CPigAccount> XLock;

// Data Members
protected:
  CPigAccountDispenser* m_pDispenser;
  _bstr_t               m_bstrName, m_bstrPassword, m_bstrCdKey;
};


/////////////////////////////////////////////////////////////////////////////

#endif //__PigAccount_h__
