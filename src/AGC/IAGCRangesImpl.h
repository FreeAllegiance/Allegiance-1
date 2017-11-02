#ifndef __IAGCRangesImpl_h__
#define __IAGCRangesImpl_h__

/////////////////////////////////////////////////////////////////////////////
// IAGCRangesImpl.h : Declaration of the IAGCRangesImpl class template.
//

#include <AGC.h>
#include <..\TCLib\RangeSet.h>
#include <..\TCLib\ObjectLock.h>

/////////////////////////////////////////////////////////////////////////////
// Interface Map Macro
//
// Classes derived from IAGCRangesImpl should include this macro in their
// interface maps.
//
#define COM_INTERFACE_ENTRIES_IAGCRangesImpl()                              \
	  COM_INTERFACE_ENTRY(IAGCRangesPrivate)                                  \
	  COM_INTERFACE_ENTRY(IDispatch)                                          \
	  COM_INTERFACE_ENTRY(ISupportErrorInfo)                                  \
    COM_INTERFACE_ENTRY(IPersistStreamInit)                                 \
    COM_INTERFACE_ENTRY(IPersistPropertyBag)                                \
    COM_INTERFACE_ENTRY2(IPersistStream, IPersistStreamInit)                \
    COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)                      \
    COM_INTERFACE_ENTRY_AUTOAGGREGATE(IID_IMarshal, m_punkMBV.p,            \
      CLSID_TCMarshalByValue)


/////////////////////////////////////////////////////////////////////////////
// IAGCRangesImpl
//
template <class T, class RT, class ITF, class RTITF, const GUID* plibid>
class ATL_NO_VTABLE IAGCRangesImpl :
  public IDispatchImpl<ITF, &__uuidof(ITF), plibid>,
  public IAGCRangesPrivate,
	public ISupportErrorInfo,
  public IPersistStreamInit,
  public IPersistPropertyBag
{
// Types
public:
  typedef TCObjectLock<T>                            XLock;
  typedef range<RT>                                  XRange;
  typedef rangeset<XRange>			                 XRangeSet;
  typedef typename XRangeSet::iterator            XRangeIt;
  typedef typename XRangeSet::reverse_iterator    XRangeRevIt;
  typedef IDispatchImpl<ITF, &__uuidof(ITF), plibid> IAGCRangesImplBase;

// Construction
public:
  IAGCRangesImpl()
  {
    // Create a range COM object to be reused multiple times
    ZSucceeded(T::CreateRange(RT(), RT(), &m_spRange));
    m_spRange.QueryInterface(&m_spPrivate);  
    assert(NULL != m_spPrivate);
    m_spRange.QueryInterface(&m_spPersist);    
    if (NULL == m_spPersist)
    {
      ZSucceeded(m_spRange->QueryInterface(IID_IPersistStreamInit,
        (void**)&m_spPersist));
    }
  }

// Overrides
public:
  static HRESULT CreateRange(const RT& value1, const RT& value2,
    RTITF** ppRange);

// IAGC<*>Ranges Interface Methods
public:
  STDMETHODIMP get_Count(long* pnCount)
  {
    XLock lock(static_cast<T*>(this));
    CLEAROUT(pnCount, static_cast<long>(m_ranges.size()));    
    return S_OK;
  }
  STDMETHODIMP get__NewEnum(IUnknown** ppunkEnum)
  {
    // Create a new CComEnum enumerator object
    typedef CComObject<CComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT,
      _Copy<VARIANT> > > CEnum;
    CEnum* pEnum = new CEnum;
    assert(NULL != pEnum);
    IUnknownPtr spEnum(pEnum);

    // Get the number of items in the collection
    XLock lock(static_cast<T*>(this));
    long cTotal = m_ranges.size();
    
    // Create a temporary array of variants
    std::vector<CComVariant> vecTemp(cTotal);

    // Populate the temporary array of variants
    long i = 0;
    RT value1, value2;
    for (XRangeIt it = m_ranges.begin(); it != m_ranges.end(); ++it)
    {
      value1 = it->lower();
      value2 = it->upper();

      CComPtr<RTITF> spRange;
      RETURN_FAILED(T::CreateRange(value1, value2, &spRange));
      vecTemp[i++] = (IDispatch*)spRange;
    }

    // Initialize enumerator object with the temporary CComVariant vector
// VS.Net 2003 port

    //Imago 8/5/09 Is this code even used?  If so it may be incorrect...
    // see IAGCCollectionImpl.h(81)
#if _MSC_VER >= 1310
	RETURN_FAILED(pEnum->Init(&(*vecTemp.begin()), &(*vecTemp.end()), NULL, AtlFlagCopy));
#else
	RETURN_FAILED(pEnum->Init(vecTemp.begin(), vecTemp.end(), NULL, AtlFlagCopy));
#endif

    // Copy the new object to the [out] parameter
    RETURN_FAILED(spEnum->QueryInterface(IID_IEnumVARIANT, (void**)ppunkEnum));

    // Indicate success
    return S_OK;
  }
  STDMETHODIMP get_Item(VARIANT* pvIndex, RTITF** ppRange)
  {
    // Initialize the [out] parameter
    CLEAROUT(ppRange, (RTITF*)NULL);

    // Coerce the specified index to a long
    CComVariant varIndex;
    RETURN_FAILED(VariantChangeType(&varIndex, pvIndex, 0, VT_I4));
    long nIndex = V_I4(&varIndex);
    XLock lock(static_cast<T*>(this));

    // Validate the specified index
    if (0 > nIndex || nIndex >= (long)m_ranges.size())
      return E_INVALIDARG;

    // Loop from beginning or end, depending on which is closest
    RT value1, value2;
	// mdvalley: "it" declared earlier.
    if (nIndex < (long)(m_ranges.size() / 2))
    {
		XRangeIt it;
      for (it = m_ranges.begin(); nIndex; ++it)
        --nIndex;
      value1 = it->lower();
      value2 = it->upper();
    }
    else
    {
		XRangeRevIt it;
      for (it = m_ranges.rbegin(); nIndex; ++it)
        --nIndex;
      value1 = it->lower();
      value2 = it->upper();
    }
    lock.Unlock();

    // Allow the most-derived class to create the range COM object
    return T::CreateRange(value1, value2, ppRange);
  }
  STDMETHODIMP put_DisplayString(BSTR bstr)
  {
    // Initialize the object
    RETURN_FAILED(InitNew());

    // Do nothing more if string is empty
    UINT cch = BSTRLen(bstr);
    if (!cch)
      return S_OK;

    // Create a string collection to help with the parsing
    ITCStringsPtr spStrings;
    RETURN_FAILED(spStrings.CreateInstance("TCObj.Strings"));

    // Add the specified string as a list of delimited items
    RETURN_FAILED(spStrings->AddDelimited(CComBSTR(L";"), bstr));

    // Iterate through each string of the set
    long cItems;
    RETURN_FAILED(spStrings->get_Count(&cItems));
    for (CComVariant v(0L); V_I4(&v) < cItems; ++V_I4(&v))
    {
      // Get the next string
      CComBSTR bstrItem;
      RETURN_FAILED(spStrings->get_Item(&v, &bstrItem));

      // Load the string into the helper range object
      RETURN_FAILED(m_spRange->put_DisplayString(bstrItem));

      // Insert the range into the set
      XRangeSet::key_type range;
      RETURN_FAILED(m_spPrivate->CopyRangeTo(&range));
      m_ranges.insert(range);
    }

    // Indicate success
    return S_OK;
  }
  STDMETHODIMP get_DisplayString(BSTR* pbstr)
  {
    // Create a string collection to help with the parsing
    ITCStringsPtr spStrings;
    RETURN_FAILED(spStrings.CreateInstance("TCObj.Strings"));

    // Create a persistent string for each item in the set
    XLock lock(static_cast<T*>(this));
    for (XRangeIt it = m_ranges.begin(); it != m_ranges.end(); ++it)
    {
      RETURN_FAILED(m_spPrivate->InitFromRange(&(*it)));
      CComBSTR bstrItem;
      RETURN_FAILED(m_spRange->get_DisplayString(&bstrItem));
      if (bstrItem.Length())
        spStrings->Add(bstrItem);
    }

    // Create a string delimited by semi-colons (;)
    return spStrings->get_DelimitedItems(CComBSTR(L";"), pbstr);
  }
  STDMETHODIMP AddByValues(RT value1, RT value2)
  {
    XLock lock(static_cast<T*>(this));
    m_ranges.insert(value1, value2);
    return S_OK;
  }
  STDMETHODIMP Add(RTITF* pRange)
  {
    RT value1, value2;
    RETURN_FAILED(pRange->get_Lower(&value1));
    RETURN_FAILED(pRange->get_Upper(&value2));
    return AddByValues(value1, value2);
  }
  STDMETHODIMP RemoveByValues(RT value1, RT value2)
  {
    XLock lock(static_cast<T*>(this));
    m_ranges.erase(value1, value2);
    return S_OK;
  }
  STDMETHODIMP Remove(RTITF* pRange)
  {
    RT value1, value2;
    RETURN_FAILED(pRange->get_Lower(&value1));
    RETURN_FAILED(pRange->get_Upper(&value2));
    return RemoveByValues(value1, value2);
  }
  STDMETHODIMP RemoveAll()
  {
    XLock lock(static_cast<T*>(this));
    m_ranges.clear();
    return S_OK;
  }
  STDMETHODIMP get_IntersectsWithValue(RT value,
    VARIANT_BOOL* pbIntersects)
  {
    XLock lock(static_cast<T*>(this));
    bool bIntr = m_ranges.find(make_range(value, value)) != m_ranges.end();
    CLEAROUT(pbIntersects, VARBOOL(bIntr));
    return S_OK;
  }
  STDMETHODIMP get_IntersectsWithRangeValues(RT value1, RT value2,
    VARIANT_BOOL* pbIntersects)
  {
    XLock lock(static_cast<T*>(this));
    bool bIntr = m_ranges.find(make_range(value1, value2)) != m_ranges.end();
    CLEAROUT(pbIntersects, VARBOOL(bIntr));
    return S_OK;
  }
  STDMETHODIMP get_IntersectsWithRange(RTITF* pRange,
    VARIANT_BOOL* pbIntersects)
  {
    RT value1, value2;
    RETURN_FAILED(pRange->get_Lower(&value1));
    RETURN_FAILED(pRange->get_Upper(&value2));
    return get_IntersectsWithRangeValues(value1, value2, pbIntersects);
  }

// IAGCRangesPrivate Interface Methods
public:
  STDMETHODIMP InitFromRanges(const void* pvRanges)
  {
    const XRangeSet* pRanges = reinterpret_cast<const XRangeSet*>(pvRanges);
    XLock lock(static_cast<T*>(this));
    if (pRanges)
      m_ranges = *pRanges;
    else
      m_ranges.clear();
    return S_OK;
  }
  STDMETHODIMP CopyRangesTo(void* pvRanges)
  {
    XRangeSet* pRanges = reinterpret_cast<XRangeSet*>(pvRanges);
    XLock lock(static_cast<T*>(this));
    *pRanges = m_ranges;
    return S_OK;
  }

// ISupportErrorInfo Interface Methods
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
  {
	  static const IID* arr[] = 
	  {
		  &__uuidof(ITF),
	  };
	  for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	  {
		  if (InlineIsEqualGUID(*arr[i],riid))
			  return S_OK;
	  }
	  return S_FALSE;
  }

// IPersist Interface Methods
public:
  STDMETHODIMP GetClassID(CLSID* pClassID)
  {
    __try
    {
      *pClassID = T::GetObjectCLSID();
    }
    __except(1)
    {
      return E_POINTER;
    }
    return S_OK;
  }

// IPersistStreamInit Interface Methods
public:
  STDMETHODIMP IsDirty()
  {
    // We don't maintain a dirty flag
    return S_OK;
  }
  STDMETHODIMP Load(LPSTREAM pStm)
  {
    // Read the number of range items to follow
    DWORD cItems;
    RETURN_FAILED(pStm->Read(&cItems, sizeof(cItems), NULL));

    // Clear the set
    XLock lock(static_cast<T*>(this));
    m_ranges.clear();

    // Read each range item
    assert(NULL != m_spPrivate);
    assert(NULL != m_spPersist);
    for (DWORD i = 0; i < cItems; ++i)
    {
      RETURN_FAILED(m_spPersist->Load(pStm));
      XRangeSet::key_type range;
      RETURN_FAILED(m_spPrivate->CopyRangeTo(&range));
      m_ranges.insert(range);
    }

    // Indicate success
    return S_OK;
  }
  STDMETHODIMP Save(LPSTREAM pStm, BOOL fClearDirty)
  {
    UNUSED_ALWAYS(fClearDirty);

    // Write out the number of range items to follow
    XLock lock(static_cast<T*>(this));
    DWORD cItems = m_ranges.size();
    RETURN_FAILED(pStm->Write(&cItems, sizeof(cItems), NULL));

    // Write each range to the stream
    assert(NULL != m_spPrivate);
    assert(NULL != m_spPersist);
    for (XRangeIt it = m_ranges.begin(); it != m_ranges.end(); ++it)
    {
      RETURN_FAILED(m_spPrivate->InitFromRange(&(*it)));
      RETURN_FAILED(m_spPersist->Save(pStm, fClearDirty));
    }

    // Indicate success
    return S_OK;
  }
  STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pCbSize)
  {
    XLock lock(static_cast<T*>(this));
    ULARGE_INTEGER uli;
    assert(NULL != m_spPersist);
    RETURN_FAILED(m_spPersist->GetSizeMax(&uli));

    pCbSize->HighPart = 0;
    pCbSize->LowPart  = sizeof(DWORD) + m_ranges.size() * uli.LowPart;
    return S_OK;
  }
  STDMETHODIMP InitNew()
  {
    XLock lock(static_cast<T*>(this));
    m_ranges.clear();
    return S_OK;
  }

// IPersistPropertyBag Interface Methods
public:
  STDMETHODIMP Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
  {
    // Load the persistent string value of the range set
    CComVariant var(BSTR(NULL));
    RETURN_FAILED(pPropBag->Read(L"Value", &var, pErrorLog));

    // Load the string into the object
    return put_DisplayString(V_BSTR(&var));
  }
  STDMETHODIMP Save(IPropertyBag* pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
  {
    UNUSED_ALWAYS(fClearDirty);
    UNUSED_ALWAYS(fSaveAllProperties);

    // Create a display string of the items in the set
    CComBSTR bstr;
    RETURN_FAILED(get_DisplayString(&bstr));

    // Save the persistent string value of the range set
    return pPropBag->Write(L"Value", &CComVariant(bstr));
  }

// Data Members
protected:
  XRangeSet           m_ranges;
  CComPtr<RTITF>      m_spRange;
  IAGCRangePrivatePtr m_spPrivate;
  IPersistStreamPtr   m_spPersist;
// Data Members
public:  
  CComPtr<IUnknown> m_punkMBV;
};


/////////////////////////////////////////////////////////////////////////////

#endif //__IAGCRangesImpl_h__
