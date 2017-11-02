#pragma once

/////////////////////////////////////////////////////////////////////////////
// zreg.h : Declaration of things to make registry access easier.

#include <malloc.h>  // Needed for _alloca


/////////////////////////////////////////////////////////////////////////////
//
// LoadRegString<T>
//
template <class T>
HRESULT LoadRegString(HKEY hkey, LPCTSTR pszValueName, T& strOut)
{
  // Initialize the [out] parameter
  strOut = TEXT("");

  // Get the size of data from the specified value
  uint32_t dwType;
  uint32_t cbData = 0;
  long lr = RegQueryValueEx(hkey, pszValueName, NULL, LPDWORD(&dwType), NULL, LPDWORD(&cbData));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Fail if the data type is not REG_SZ
  if (REG_SZ != dwType)
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);

  // Do nothing else if data size is zero
  if (!cbData)
    return S_OK;

  // Allocate an automatic block of memory
  LPTSTR pszValue = (LPTSTR)_alloca(cbData + sizeof(TCHAR));

  // Attempt to read the specified value
  lr = RegQueryValueEx(hkey, pszValueName, NULL, NULL, (uint8_t*)pszValue, LPDWORD(&cbData));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Save the value string to the [out] parameter
  pszValue[cbData / sizeof(TCHAR)] = TEXT('\0');
  strOut = pszValue;

  // Indicate success
  return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// LoadRegStream
//
inline HRESULT LoadRegStream(HKEY hkey, LPCTSTR pszValueName, IStream** ppStm)
{
  // Initialize the [out] parameter
  *ppStm = NULL;

  // Get the size of data from the specified value
  uint32_t dwType;
  uint32_t cbData = 0;
  long lr = RegQueryValueEx(hkey, pszValueName, NULL, LPDWORD(&dwType), NULL, LPDWORD(&cbData));
  if (ERROR_SUCCESS != lr)
    return HRESULT_FROM_WIN32(lr);

  // Fail if the data type is not REG_BINARY
  if (REG_BINARY != dwType)
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);

  // Do nothing else if data size is zero
  if (!cbData)
    return S_OK;

  // Allocate a global block of memory
  HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, cbData);
  void* pvValue = GlobalLock(hGlobal);

  // Attempt to read the specified value
  lr = RegQueryValueEx(hkey, pszValueName, NULL, NULL, (uint8_t*)pvValue, LPDWORD(&cbData));

  // Unlock the global block of memory
  GlobalUnlock(hGlobal);

  HRESULT hr = (ERROR_SUCCESS != lr) ? HRESULT_FROM_WIN32(lr) : S_OK;

  // Create a stream on the HGLOBAL
  if (SUCCEEDED(hr))
    hr = CreateStreamOnHGlobal(hGlobal, true, ppStm);

  // If an error occurred, release the global handle
  if (SUCCEEDED(hr))
  {
    // Set the size of the stream
    ULARGE_INTEGER uli;
    uli.HighPart = 0;
    uli.LowPart = cbData;
    (*ppStm)->SetSize(uli);

    // Rewind the stream
    LARGE_INTEGER li;
    li.HighPart = 0;
    li.LowPart = 0;
    (*ppStm)->Seek(li, STREAM_SEEK_SET, NULL);
  }
  else
  {
    GlobalFree(hGlobal);
  }

  // Return the last HRESULT
  return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// SaveRegStream
//
inline HRESULT SaveRegStream(HKEY hkey, LPCTSTR pszValueName, IStream* pStm)
{
  // Get the size of the specified stream
  LARGE_INTEGER li = {0};
  ULARGE_INTEGER uli;
  HRESULT hr = pStm->Seek(li, STREAM_SEEK_END, &uli);
  if (FAILED(hr))
    return hr;
  if (0 != uli.HighPart)
    return E_OUTOFMEMORY;
  uint32_t cbData = uli.LowPart;

  // Get a handle to the stream data
  HGLOBAL hGlobal, hGlobalTemp = NULL;
  if (FAILED(GetHGlobalFromStream(pStm, &hGlobal)))
  {
    // Allocate a block large enough for the contents of the stream
    hGlobal = hGlobalTemp = GlobalAlloc(GMEM_MOVEABLE, cbData);
    if (!hGlobalTemp)
      return E_OUTOFMEMORY;

    // Lock the global block    
    void* pvTemp = GlobalLock(hGlobalTemp);
    if (!pvTemp)
    {
      hr = HRESULT_FROM_WIN32(GetLastError());
      GlobalFree(hGlobalTemp);
      return hr;
    }

    // Read the stream into the global block
    hr = pStm->Read(pvTemp, cbData, NULL);

    // Unlock the global block
    GlobalUnlock(hGlobalTemp);
    if (FAILED(hr))
      return hr;    
  }

  // Lock the handle
  void* pvData = GlobalLock(hGlobal);
  if (pvData)
  {
    // Write the binary data to the registry value
    LONG lr = ::RegSetValueEx(hkey, pszValueName, 0, REG_BINARY,
      (uint8_t*)pvData, cbData);
    if (ERROR_SUCCESS != lr)
      hr = HRESULT_FROM_WIN32(lr);

    // Unlock the handle
    GlobalUnlock(hGlobal);
  }
  else
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
  }

  // Free the HGLOBAL, if not from the stream
  if (hGlobalTemp)
    GlobalFree(hGlobalTemp);

  // Return the last HRESULT
  return hr;
}

