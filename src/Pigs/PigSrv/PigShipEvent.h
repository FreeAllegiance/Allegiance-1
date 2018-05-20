/////////////////////////////////////////////////////////////////////////////
// PigShipEvent.h : Declaration of the CPigShipEvent class

#ifndef __PigShipEvent_h__
#define __PigShipEvent_h__

#include "resource.h"       // main symbols
#include <..\TCLib\AutoHandle.h>
#include <..\TCLib\LookupTable.h>
#include <..\TCLib\ObjectLock.h>
#include <..\TCAtl\ErrorFormatter.h>
#include "PigEvent.h"
#include "PigSrv.h"
#include <Allegiance.h>


/////////////////////////////////////////////////////////////////////////////
// Forward Declarations

class CPig;
class CPigShip;


/////////////////////////////////////////////////////////////////////////////
// CPigShipEvent
class ATL_NO_VTABLE CPigShipEvent : 
  public IPigEventImpl<CPigShipEvent, IPigShipEvent, &IID_IPigShipEvent>,
  public ISupportErrorInfo,
  public TCErrorFormatter<CPigShipEvent, &IID_IPigShipEvent>,
  public CComObjectRoot,
  public CComCoClass<CPigShipEvent, &CLSID_PigShipEvent>
{
// Declarations
public:
  DECLARE_REGISTRY_RESOURCEID(IDR_PigShipEvent)
  DECLARE_PROTECT_FINAL_CONSTRUCT()

// Category Map
public:
  BEGIN_CATEGORY_MAP(CPigShipEvent)
    IMPLEMENTED_CATEGORY(CATID_PigObjects)
  END_CATEGORY_MAP()

// Interface Map
public:
  BEGIN_COM_MAP(CPigShipEvent)
    COM_INTERFACE_ENTRY(IPigShipEvent)
    COM_INTERFACE_ENTRY(IPigEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
  END_COM_MAP()

// Construction / Destruction
public:
  CPigShipEvent() {}
  HRESULT Init(CPigShip* pPigShip, BSTR bstrTarget);

// Attributes
public:
  HRESULT SetTarget(BSTR bstrTarget);

// Operations
public:
  static ImodelIGC* FindTargetName(CPig* pPig, BSTR bstrTarget, bool bFriendsOnly = false);

// Overrides
public:
  // CPigEvent Overrides
  virtual bool IsExpired() = 0;
  virtual bool Pulse(bool bExpired);
  virtual bool ModelTerminated(ImodelIGC* pModelIGC);

// ISupportErrorInfo Interface Methods
public:
  STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

// IPigShipEvent Interface Methods
public:
  STDMETHODIMP get_Action(BSTR* pbstrAction) = 0;
  STDMETHODIMP get_Target(BSTR* pbstrTarget);

// Types
protected:
  typedef TCObjectLock<CPigShipEvent> XLock;

// Data Members
protected:
  TCPtr<CPigShip*>  m_pPigShip;
  TCPtr<ImodelIGC*> m_pTarget;
  float             m_fTargetRadius;

// Friends
protected:
  friend class IPigEventImpl<CPigShipEvent, IPigShipEvent, &IID_IPigShipEvent>;
};


/////////////////////////////////////////////////////////////////////////////
// CPigShipEventImpl
//
template <class T>
class CPigShipEventImpl :
  public CPigShipEvent
{
// IPigShipEvent Interface Methods
public:
  STDMETHODIMP get_Action(BSTR* pbstrAction)
  {
    CLEAROUT(pbstrAction, (BSTR)NULL);
    *pbstrAction = SysAllocString(static_cast<T*>(this)->GetActionName());
    return S_OK;
  }
};


/////////////////////////////////////////////////////////////////////////////
// CPigShipDummyEvent
//
class ATL_NO_VTABLE CPigShipDummyEvent :
  public IDispatchImpl<IPigShipEvent, &IID_IPigShipEvent, &LIBID_PigsLib>,
  public CComObjectRoot
{
// Interface Map
public:
  BEGIN_COM_MAP(CPigShipEvent)
    COM_INTERFACE_ENTRY(IPigEvent)
    COM_INTERFACE_ENTRY(IPigShipEvent)
    COM_INTERFACE_ENTRY(IDispatch)
  END_COM_MAP()

// Construction / Destruction
public:
  CPigShipDummyEvent() {}

// IPigEvent Interface Methods
public:
  STDMETHODIMP put_Name(BSTR bstrName)                     {return S_OK;}
  STDMETHODIMP get_Name(BSTR* pbstrName)
    {CLEAROUT(pbstrName,  (BSTR)NULL);                      return S_OK;}
  STDMETHODIMP put_ExpirationExpression(BSTR bstrExpr)     {return S_OK;}
  STDMETHODIMP get_ExpirationExpression(BSTR* pbstrExpr)
    {CLEAROUT(pbstrExpr, (BSTR)NULL);                       return S_OK;}
  STDMETHODIMP get_IsExpired(VARIANT_BOOL* pbExpired)
    {CLEAROUT(pbExpired, VARIANT_FALSE);                    return S_OK;}
  STDMETHOD(Kill)()                                        {return S_OK;}
  STDMETHODIMP put_InterruptionExpression(BSTR   bstrExpr) {return S_OK;}
  STDMETHODIMP get_InterruptionExpression(BSTR* pbstrExpr)
    {CLEAROUT(pbstrExpr, (BSTR)NULL);                       return S_OK;}

// IPigShipEvent Interface Methods
public:
  STDMETHODIMP get_Action(BSTR* pbstrAction)
    {CLEAROUT(pbstrAction,(BSTR)NULL);    return S_OK;}
  STDMETHODIMP get_Target(BSTR* pbstrTarget)
    {CLEAROUT(pbstrTarget,(BSTR)NULL);    return S_OK;}
};


/////////////////////////////////////////////////////////////////////////////
// CPigFaceEvent
//
class ATL_NO_VTABLE CPigFaceEvent :
  public CPigShipEventImpl<CPigFaceEvent>
{
// Construction / Destruction
public:
  CPigFaceEvent() :
    m_bMatchUpVector(false),
    m_bVector(false),
    m_vector(Vector::GetZero())
  {
  }
  HRESULT Init(CPigShip* pPigShip, VARIANT* pvarTarget, bool bMatchUpVector);

// Attributes
public:
  static const OLECHAR* GetActionName() {return L"face";}

// Overrides
public:
  // CPigEvent Overrides
  virtual bool IsExpired();
  virtual void DoDefaultExpiration();

// IPigShipEvent Interface Methods
public:
  STDMETHODIMP get_Target(BSTR* pbstrTarget);

// Data Members
protected:
  bool   m_bMatchUpVector:1;
  bool   m_bVector:1;
  Vector m_vector;
};


/////////////////////////////////////////////////////////////////////////////
// CPigDroneEvent
//
class ATL_NO_VTABLE CPigDroneEvent :
  public CPigShipEventImpl<CPigDroneEvent>
{
// Construction / Destruction
public:
  HRESULT Init(CPigShip* pPigShip, const char* szVerb, BSTR bstrTarget);
  HRESULT Init(CPigShip* pPigShip, const char* szVerb, ImodelIGC* pTarget);

// Attributes
public:
  const OLECHAR* GetActionName() {return m_bstrVerb;}

// Overrides
public:
  // CPigEvent Overrides
  virtual bool IsExpired();

// Data Members
protected:
  CComBSTR m_bstrVerb, bstrTarget;
};


/////////////////////////////////////////////////////////////////////////////

#endif //__PigShipEvent_h__
