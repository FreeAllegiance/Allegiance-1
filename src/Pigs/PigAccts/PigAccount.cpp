/////////////////////////////////////////////////////////////////////////////
// PigAccount.cpp : Implementation of the CPigAccount class.

#include "pch.h"
#include <TCLib.h>
#include <..\TCAtl\ObjectMap.h>

#include "PigAccountDispenser.h"
#include "PigAccount.h"


/////////////////////////////////////////////////////////////////////////////
// CPigAccount

TC_OBJECT_EXTERN_NON_CREATEABLE_IMPL(CPigAccount)


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPigAccount::CPigAccount() :
  m_pDispenser(NULL)
{
}

HRESULT CPigAccount::FinalConstruct()
{
  // Indicate success
  return S_OK;
}

void CPigAccount::FinalRelease()
{
  // Release ourself within the parent object
  XLock lock(this);
  m_pDispenser->AccountReleased(m_bstrName);

  // Release the parent object
  m_pDispenser->Release();
  m_pDispenser = NULL;
}

HRESULT CPigAccount::Init(CPigAccountDispenser* pDispenser, _bstr_t bstrName,
  _bstr_t bstrPassword, _bstr_t bstrCdKey)
{
  // Save the specified parameters
  XLock lock(this);
  m_pDispenser   = pDispenser;
  m_bstrName     = bstrName;
  m_bstrPassword = bstrPassword;
  m_bstrCdKey    = bstrCdKey;
  // AddRef the parent object
  m_pDispenser->AddRef();

  // Indicate success
  return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigAccount::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigAccount,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// IPigAccount Interface Methods

STDMETHODIMP CPigAccount::get_Name(BSTR* pbstrName)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrName, (BSTR)NULL);

  // Copy the string to the [out] parameter
  XLock lock(this);
  *pbstrName = m_bstrName.copy();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigAccount::get_Password(BSTR* pbstrPassword)
{
  // Initialize the [out] parameter
  CLEAROUT(pbstrPassword, (BSTR)NULL);

  // Copy the string to the [out] parameter
  XLock lock(this);
  *pbstrPassword = m_bstrPassword.copy();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigAccount::get_CdKey(BSTR* pbstrCdKey)
{
	// Initialize the [out] parameter
	CLEAROUT(pbstrCdKey, (BSTR)NULL);

	// Copy the string to the [out] parameter
	XLock lock(this);
	*pbstrCdKey = m_bstrCdKey.copy();

	// Indicate success
	return S_OK;
}

