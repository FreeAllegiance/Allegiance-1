/////////////////////////////////////////////////////////////////////////////
// PigAccountDispenser.cpp : Implementation of the CPigAccountDispenser class.

#include "pch.h"
#include <TCLib.h>
#include <..\TCAtl\ObjectMap.h>
#include <..\TCLib\AutoHandle.h>
#include <AGC.h>
#include "PigAccount.h"
#include "PigAccountDispenser.h"


/////////////////////////////////////////////////////////////////////////////
// CPigAccountDispenser

TC_OBJECT_EXTERN_IMPL(CPigAccountDispenser)


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPigAccountDispenser::CPigAccountDispenser() :
  m_itAvailable(m_Accounts.begin())
{
}

HRESULT CPigAccountDispenser::FinalConstruct()
{
  // Read the collection of accounts from the INI file
  RETURN_FAILED(LoadAccounts());

  // Indicate success
  return S_OK;
}

void CPigAccountDispenser::FinalRelease()
{
}


/////////////////////////////////////////////////////////////////////////////
// Attributes

void CPigAccountDispenser::AccountReleased(_bstr_t bstrName)
{
  XLock lock(this);
  for (XAccountIt it = m_Accounts.begin(); it != m_Accounts.end(); ++it)
  {
    if (it->m_bstrName == bstrName)
    {
      assert(!it->m_bAvailable);
      it->m_bAvailable = true;
      if (m_Accounts.end() == m_itAvailable)
        m_itAvailable = it;
      return;
    }
  }
  assert(!"CPigAccountDispenser::AccountRelease: Should never get here!");
}


/////////////////////////////////////////////////////////////////////////////
// Implementation

HRESULT CPigAccountDispenser::LoadAccounts()
{
  XLock lock(this);
  assert(0 == m_Accounts.size());

  // Get the path name of the module
  TCHAR szModule[_MAX_PATH];
  _VERIFYE(GetModuleFileName(_Module.GetModuleInstance(), szModule,
    sizeofArray(szModule)));

  // Create the name of the INI file
  TCHAR szINI[_MAX_PATH];
  TCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szName[_MAX_FNAME];
  _tsplitpath(szModule, szDrive, szDir, szName, NULL);
  _tmakepath(szINI, szDrive, szDir, szName, ".ini");

  // Compute the number of bytes needed to read all the account names
  TCHandle shFile = CreateFile(szINI, GENERIC_READ, FILE_SHARE_READ, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (INVALID_HANDLE_VALUE == shFile)
    return S_FALSE;
  DWORD cbFile = GetFileSize(shFile, NULL);
  shFile = NULL; // Closes the file
  if (!cbFile)
    return S_FALSE;

  // Allocate a buffer
  TCArrayPtr<TCHAR*> spsz = new TCHAR[cbFile];
  if (spsz.IsNull())
    return E_OUTOFMEMORY;

  // Get the number of user accounts to use
  int cNames = GetPrivateProfileInt(_T("general"), _T("NameCount"), -1, szINI);

  // Get the CDKey to use for all the accounts
  TCArrayPtr<TCHAR*> szCDKey = new TCHAR[MAX_PATH];
  if (szCDKey.IsNull())
	  return E_OUTOFMEMORY;


  GetPrivateProfileString(_T("general"), _T("CDKey"), _T("NOTSET"), szCDKey, MAX_PATH, szINI);
  printf("CDKey: %hS\n", (LPCTSTR)szCDKey);
  // Read all of the account names
  GetPrivateProfileString(_T("accounts"), NULL, _T(""), spsz, cbFile, szINI);

  // Break-out each account name
  LPCTSTR psz = spsz;
  while (_T('\0') != *psz && (-1 == cNames || m_Accounts.size() < cNames))
  {
    // Lookup the account name's password
    TCHAR szPassword[_MAX_PATH];
    GetPrivateProfileString(_T("accounts"), psz, _T(""), szPassword,
      sizeofArray(szPassword), szINI);

    // Create an account item and add it to the vector
    XAccount xa;
    xa.m_bstrName     = psz;
    xa.m_bstrPassword = szPassword;
    xa.m_bAvailable   = true;
	xa.m_bstrCDKey = szCDKey;
    m_Accounts.push_back(xa);

    // Advance to the next account name
    psz += xa.m_bstrName.length() + 1;
  }

  // Initialize the next available iterator
  m_itAvailable = m_Accounts.begin();

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigAccountDispenser::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigAccountDispenser,
  };
  for (int i = 0; i < sizeof(arr) / sizeof(arr[0]) ; ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// IPigAccountDispenser Interface Methods

STDMETHODIMP CPigAccountDispenser::get_NextAvailable(IPigAccount** ppAccount)
{
  // Initialize the [out] parameter
  CLEAROUT(ppAccount, (IPigAccount*)NULL);

  // Return an error if no accounts are currently available
  XLock lock(this);
  if (m_Accounts.end() == m_itAvailable)
    return Error(IDS_E_NONE_AVAILABLE, IID_IPigAccountDispenser);

  // Create a new pig account object
  CComObject<CPigAccount>* pAccount = NULL;
  RETURN_FAILED(pAccount->CreateInstance(&pAccount));
  _SVERIFYE(pAccount->Init(this, m_itAvailable->m_bstrName,
    m_itAvailable->m_bstrPassword, m_itAvailable->m_bstrCDKey));

  // QI the new object for the IPigAccount interface
  _SVERIFYE(pAccount->QueryInterface(IID_IPigAccount, (void**)ppAccount));
  
  // Mark the account as in-use
  m_itAvailable->m_bAvailable = false;

  // Find the next available account
  XAccountIt it;
  for (it = m_itAvailable + 1; m_Accounts.end() != it; ++it)
  {
    if (it->m_bAvailable)
    {
      m_itAvailable = it;
      break;
    }
  }

  // Loop from the beginning back to the current one
  if (m_Accounts.end() == it)
  {
    for (it = m_Accounts.begin(); it != m_itAvailable; ++it)
      if (it->m_bAvailable)
        break;
    m_itAvailable = (it == m_itAvailable) ? m_Accounts.end() : it;
  }

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigAccountDispenser::get_Names(ITCStrings** ppNames)
{
  // Initialize the [out] parameter
  CLEAROUT(ppNames, (ITCStrings*)NULL);

  // Create a string collection object
  ITCStringsPtr spNames;
  RETURN_FAILED(spNames.CreateInstance(CLSID_TCStrings));

  // Loop thru the accounts
  XLock lock(this);
  for (XAccountIt it = m_Accounts.begin(); m_Accounts.end() != it; ++it)
    RETURN_FAILED(spNames->Add(it->m_bstrName));

  // Detach the interface pointer to the [out] parameter
  *ppNames = spNames.Detach();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigAccountDispenser::get_NamesAvailable(ITCStrings** ppNames)
{
  // Initialize the [out] parameter
  CLEAROUT(ppNames, (ITCStrings*)NULL);

  // Create a string collection object
  ITCStringsPtr spNames;
  RETURN_FAILED(spNames.CreateInstance(CLSID_TCStrings));

  // Loop thru the accounts
  XLock lock(this);
  for (XAccountIt it = m_Accounts.begin(); m_Accounts.end() != it; ++it)
    if (it->m_bAvailable)
      RETURN_FAILED(spNames->Add(it->m_bstrName));

  // Detach the interface pointer to the [out] parameter
  *ppNames = spNames.Detach();

  // Indicate success
  return S_OK;
}

STDMETHODIMP CPigAccountDispenser::get_NamesInUse(ITCStrings** ppNames)
{
  // Initialize the [out] parameter
  CLEAROUT(ppNames, (ITCStrings*)NULL);

  // Create a string collection object
  ITCStringsPtr spNames;
  RETURN_FAILED(spNames.CreateInstance(CLSID_TCStrings));

  // Loop thru the accounts
  XLock lock(this);
  for (XAccountIt it = m_Accounts.begin(); m_Accounts.end() != it; ++it)
    if (!it->m_bAvailable)
      RETURN_FAILED(spNames->Add(it->m_bstrName));

  // Detach the interface pointer to the [out] parameter
  *ppNames = spNames.Detach();

  // Indicate success
  return S_OK;
}

