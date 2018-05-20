/////////////////////////////////////////////////////////////////////////////
// PigHullTypes.cpp : Implementation of the CPigHullTypes class.

#include "pch.h"
#include <TCLib.h>
#include <..\TCAtl\ObjectMap.h>
#include "PigHullTypes.h"


/////////////////////////////////////////////////////////////////////////////
// CPigHullTypes

TC_OBJECT_EXTERN_NON_CREATEABLE_IMPL(CPigHullTypes)


/////////////////////////////////////////////////////////////////////////////
// Construction / Destruction

CPigHullTypes::CPigHullTypes()
{
}

HRESULT CPigHullTypes::Init(const HullTypeListIGC* pslistIGC, IstationIGC* pStation)
{
  assert(NULL != pslistIGC);

  // Copy each hull type that the station can buy
  for (HullTypeLinkIGC* it = pslistIGC->first(); it; it = it->next())
  {
    IhullTypeIGC* pHullTypeIGC = it->data();
    if (!pStation || pStation->CanBuy(pHullTypeIGC))
      m_HullTypesIGC.push_back(pHullTypeIGC);
  }

  // Indicate success
  return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfo Interface Methods

STDMETHODIMP CPigHullTypes::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* arr[] = 
  {
    &IID_IPigHullTypes,
  };
  for (int i = 0; i < sizeofArray(arr); ++i)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// ITCCollection Interface Methods

STDMETHODIMP CPigHullTypes::get_Count(long* pnCount)
{
  XLock lock(this);
  CLEAROUT(pnCount, (long)m_HullTypesIGC.size());
  return S_OK;
}

STDMETHODIMP CPigHullTypes::get__NewEnum(IUnknown** ppunkEnum)
{
  // Clear the [out] parameter
  CLEAROUT(ppunkEnum, (IUnknown*)NULL);

  // Lock the object
  XLock lock(this);

  // Copy the items into to a temporary CComVariant vector
  std::vector<CComVariant> vecTemp(m_HullTypesIGC.size(), CComVariant());
  for (CComVariant i(0L); V_I4(&i) < m_HullTypesIGC.size(); ++V_I4(&i))
  {
    IAGCHullTypePtr spHullType;
    RETURN_FAILED(GetAGCGlobal()->GetAGCObject(m_HullTypesIGC[V_I4(&i)],
      IID_IAGCHullType, (void**)&spHullType));
    vecTemp[V_I4(&i)] = (IDispatch*)spHullType;
  }

  // Create a new CComEnum enumerator object
  typedef CComObject<CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT,
    _Copy<VARIANT> > > CEnum;
  CEnum* pEnum = new CEnum;
  assert(NULL != pEnum);

  CComVariant* pargTemp = new CComVariant [m_HullTypesIGC.size()];

  int i = 0;
  for (std::vector<CComVariant>::iterator iter = vecTemp.begin(); iter != vecTemp.end(); iter++)
  {
	  pargTemp[i] = *iter;

	  //pargTemp[i].Copy((VARIANT *) &iter);
	  //pargTemp[i] = &((CComVariant) &iter);
	  i++;
  }


  // Initialize enumerator object with the temporary CComVariant vector
  // VS.Net 2003 port - accomodate change in iterators under VC.Net 200x (see 'breaking changes' in vsnet doc)
#if _MSC_VER >= 1310
  //HRESULT hr = pEnum->Init(&(*vecTemp.begin()), &(*vecTemp.end()), NULL, AtlFlagCopy);
  HRESULT hr = pEnum->Init(&pargTemp[0], &pargTemp[m_HullTypesIGC.size() - 1], NULL, AtlFlagCopy);
#else
  HRESULT hr = pEnum->Init(vecTemp.begin(), vecTemp.end(), NULL, AtlFlagCopy);
#endif
  if (SUCCEEDED(hr))
    hr = pEnum->QueryInterface(IID_IEnumVARIANT, (void**)ppunkEnum);
  if (FAILED(hr))
    delete pEnum;

  // Return the last result
  return hr;
}


/////////////////////////////////////////////////////////////////////////////
// IPigHullTypes Interface Methods

STDMETHODIMP CPigHullTypes::get_Item(VARIANT* pvIndex, IAGCHullType** ppHullType)
{
  // Initialize the [out] parameter
  CLEAROUT(ppHullType, (IAGCHullType*)NULL);

  // Convert the specified variant to an index
  CComVariant var;
  RETURN_FAILED(VariantChangeType(&var, pvIndex, 0, VT_I4));
  long iIndex = V_I4(&var);

  // Validate the index
  if (0 > iIndex || iIndex >= m_HullTypesIGC.size())
    return E_INVALIDARG;

  // Get or create the AGC object associated with the IGC hull type object
  return GetAGCGlobal()->GetAGCObject(m_HullTypesIGC[iIndex],
    IID_IAGCHullType, (void**)ppHullType);
}

