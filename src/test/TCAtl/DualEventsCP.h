#ifndef __DualEventsCP_h__
#define __DualEventsCP_h__

/////////////////////////////////////////////////////////////////////////////
// DualEventsCP.h | Declaration of the TCComDualEventsCP class.

#include "EventsCP.h"
#include "DualEventCall.h"
#include <..\TCLib\ObjectLock.h>


/////////////////////////////////////////////////////////////////////////////
// TCComDualEventsCP is the base class for a connection point that has both
// *IDispatch* and v-table sink interfaces.
//
// Actually, two connection points exist in this case - one for the
// *dispinterface* and one for the (v-table) *interface* implemented by the
// *dispinterface*. The result is a sort of /dual/ connection point so that
// sinks can easily be written in multiple environments. For example,
// scripting and VB clients would connect to the *dispinterface* connection
// point. However, a C or C++ client would probably want to connect to the
// (v-table) *interface* connection point, since that would not require
// unnecessary implementation of the *IDispatch* methods.
//
// The IDL (Interface Definition Language) for such a pair of corresponding
// interfaces should be similar to the following:
//
//      // ISampleEvents Interface
//      [
//        object, oleautomation, pointer_default(unique),
//        uuid(098A9AF0-1C79-11d2-8519-00400536E6C1),
//        helpstring("ISampleEvents: Sample event interface.")
//      ]
//      interface ISampleEvents : IUnknown
//      {
//        [id(DISPID_OnHello), helpstring("Indicates 'Hello'.")]
//        HRESULT OnHello([in] VARIANT_BOOL IsHappy, [in] BSTR Text);
//        …
//        [id(DISPID_OnGoodbye), helpstring("Indicates 'Goodbye'.")]
//        HRESULT OnGoodbye([in] VARIANT_BOOL IsHappy, [in] BSTR Text);
//        …
//      }; // End: interface ISampleEvents : IUnknown
//      …
//      …
//      // DISampleEvents Interface
//      [
//        oleautomation, uuid(098A9AF1-1C79-11d2-8519-00400536E6C1),
//        helpstring("DISampleEvents Interface")
//      ]
//      dispinterface DISampleEvents
//      {
//        interface ISampleEvents;
//        …
//      }; // End: dispinterface DISampleEvents
//
// Notice that the *interface* and the *dispinterface* must each have a
// unique *GUID*. Also notice that the (v-table) *interface* is derived from
// *IUnknown* and is *not* marked [dual]. The /dual/ nature is implemented by
// the *dispinterface* declaring the (v-table) *interface*. This form of the
// *dispinterface* indicates that all of the methods and properties of the
// specified *interface* will be methods and properties of the
// *dispinterface*, without having to code them twice.
//
// To declare a connection point class for the previous sample interfaces,
// use the BEGIN_TCComDualEventsCP and END_TCComDualEventsCP macros along
// with the TCComDualEventCP_Fn macros:
//
//      BEGIN_TCComDualEventsCP(ISampleEventsCP, ISampleEvents, DIID_DISampleEvents)
//        TCComDualEventCP_Fn2(OnHello, DISPID_OnHello,
//          VARIANT_BOOL, VT_BOOL, BSTR, VT_BSTR)
//        TCComDualEventCP_Fn2(OnGoodbye, DISPID_OnGoodbye,
//          VARIANT_BOOL, VT_BOOL, BSTR, VT_BSTR)
//      END_TCComDualEventsCP()
//
// Then, derive your component object from ISampleEventsCP and list the GUIDs
// of both connection points in your *CONNECTION_POINT_MAP*. To actually fire
// an event to all connected sinks, call the firing function, which is named
// by prefixing *Fire_* to the event name:
//
//        // Fire an 'OnHello' event
//        Fire_OnHello(bHappy, bstrText);
//
// This class is derived from TCComEventsCP, which provides the functionality
// for the (v-table) *interface* connection point. The additional
// considerations for the *dispinterface* are included here and in the
// TCComDualEventCall class. However, when firing an event using this class,
// it will be fired to all of the *dispinterface* sinks /and/ to all of the
// *interface* sinks.
//
// Parameters:
//   T - The class derived from TCComDualEventsCP.
//   IV - The v-table interface name.
//   piid - A constant pointer to the IID of the v-table interface, /IV/.
//   CPD - The dispatch connection point class which serves as a base class.
//
// See Also: BEGIN_TCComDualEventsCP, BEGIN_TCComDualEventsCP_IID,
// END_TCComDualEventsCP, TCComDualEventCP_Fn macros, TCComEventsCP
template <class T, class IV, const IID* piid, class CPD>
class ATL_NO_VTABLE TCComDualEventsCP :
  public TCComEventsCP<T, IV, piid>, public CPD
{
// Group=Types
protected:
  // Declares a vector type of *IDispatch* pointers.
  typedef std::vector<IDispatch*> vector_disp;
  // Declares a vector iterator type for vector_disp.
  typedef vector_disp::iterator it_disp;

// Overrides
protected:
  virtual TC_WorkItemRelProc OnGetWorkItemRelProc();
  virtual void OnMessage(UINT idMsg, int cParams, LPARAM* rgParams);
  static void WINAPI ReleaseArgs(UINT idMsg, int cParams, LPARAM* rgParams);

// Implementation
protected:
  void FireEvents(TCComDualEventCall<IV>& call);
  void GetEventSinks(vector_vtbl& vec_vtbl, vector_disp& vec_disp);
  void RemoveFailedSink(HRESULT hr, IDispatch* pdisp);
};


/////////////////////////////////////////////////////////////////////////////
// Group=Overrides

/////////////////////////////////////////////////////////////////////////////
// Returns the address of the static ReleaseArgs method.
template <class T, class IV, const IID* piid, class CPD>
TC_WorkItemRelProc TCComDualEventsCP<T, IV, piid, CPD>::OnGetWorkItemRelProc()
{
  return ReleaseArgs;
}

/////////////////////////////////////////////////////////////////////////////
// Uses the function call TCComDualEventCall::operator() of the class
// derived from TCComDualEventCall.
template <class T, class IV, const IID* piid, class CPD>
void TCComDualEventsCP<T, IV, piid, CPD>::OnMessage(UINT idMsg, int cParams,
  LPARAM* rgParams)
{
  switch (idMsg)
  {
    case e_idFireEvents:
    {
      // Get the call pointer from the parameter list
      assert(1 == cParams);
      TCComDualEventCall<IV>* pCall = (TCComDualEventCall<IV>*)rgParams[0];

      // Call FireEvents in the context of the utility thread
      assert(pCall);
      FireEvents(*pCall);
      break;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Deletes the object derived from TCComDualEventCall.
template <class T, class IV, const IID* piid, class CPD>
void TCComDualEventsCP<T, IV, piid, CPD>::ReleaseArgs(UINT idMsg,
  int cParams, LPARAM* rgParams)
{
  switch (idMsg)
  {
    case e_idFireEvents:
    {
      // Get the call pointer from the parameter list
      assert(1 == cParams);
      TCComDualEventCall<IV>* pCall = (TCComDualEventCall<IV>*)rgParams[0];

      // Delete the call pointer
      assert(pCall);
      delete pCall;
      break;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
// Group=Implementation

/////////////////////////////////////////////////////////////////////////////
// Description: Fires the event to all connected sinks.
//
// Remarks: This method is used by the TCComDualEventCP_Fn macros to fire
// the specified event to all connected sinks. The event is specified as a
// reference to an TCComDualEventCall-derived class instance.
//
// Parameters:
//   call - A reference to an instance of a class derived from
// TCComDualEventCall. This class is derived automatically by the
// TCComDualEventCP_Fn macros.
//
// See Also: TCComDualEventCP_Fn macros, TCComDualEventCall,
// TCComEventsCP::FireEvents
template <class T, class IV, const IID* piid, class CPD>
void TCComDualEventsCP<T, IV, piid, CPD>::FireEvents(
  TCComDualEventCall<IV>& call)
{
  // Copy all of the vtable and IDispatch sinks
  vector_vtbl vec_vtbl;
  vector_disp vec_disp;
  GetEventSinks(vec_vtbl, vec_disp);

  // Typedef the call object base classes
  typedef TCComEventCall<IV> EC;
  typedef TCComDualEventCall<IV> DEC;

  // Notify all vtable event sinks
  HRESULT hr;
  for (it_vtbl it_v = vec_vtbl.begin(); it_v != vec_vtbl.end(); ++it_v)
    if (FAILED(hr = static_cast<EC&>(call)(*it_v)))
      TCComEventsCP<T, IV, piid>::RemoveFailedSink(hr, *it_v);

  // Notify all IDispatch event sinks
  for (it_disp it_d = vec_disp.begin(); it_d != vec_disp.end(); ++it_d)
    if (FAILED(hr = static_cast<DEC&>(call)(*it_d)))
      RemoveFailedSink(hr, *it_d);
}

/////////////////////////////////////////////////////////////////////////////
// Description: Copies the derived-class's event sinks to the specified
// vectors.
//
// Parameters:
//   vec_vtbl - A reference to the vector where the v-table event sinks are
// to be copied.
//   vec_disp - A reference to the vector where the *IDispatch* event sinks
// are to be copied.
//
// See Also: TCComDualEventsCP::FireEvents, TCComEventsCP::GetEventSinks,
// TCComEventsCP::GetEventSinks
template <class T, class IV, const IID* piid, class CPD>
inline void TCComDualEventsCP<T, IV, piid, CPD>::GetEventSinks(
// VS.Net 2003 port: use same type as in declaration (see line 118)
#if _MSC_VER >= 1310
  vector_vtbl& vec_vtbl,
  vector_disp& vec_disp)
#else
  TCComDualEventsCP<T, IV, piid, CPD>::vector_vtbl& vec_vtbl,
  TCComDualEventsCP<T, IV, piid, CPD>::vector_disp& vec_disp)
#endif
{
  // Lock the object (just long enough to copy the event sinks)
  TCObjectLock<T> lock(static_cast<T*>(this));

  // Copy the vtable sinks to the specified vec_vtbl
  TCComEventsCP<T, IV, piid>::GetEventSinks(vec_vtbl);
      
  // Transform the IDispatch sinks to the specified vec_disp
  CComDynamicUnkArray& vec_d = static_cast<CPD*>(this)->m_vec;
  vec_disp.resize(vec_d.end() - vec_d.begin(), NULL);
  it_disp itDest = vec_disp.begin();
  for (IUnknown** itSrc = vec_d.begin(); itSrc != vec_d.end(); ++itSrc, ++itDest)
    *itDest = static_cast<IDispatch*>(*itSrc);
}

/////////////////////////////////////////////////////////////////////////////
// Description: Removes an event sink if an event call failed.
//
// Parameters:
//   hr - The HRESULT of the failed event call.
//   pdisp - The *IDispatch* pointer of the sink that failed.
//
// See Also: TCComDualEventsCP::FireEvents, TCComEventsCP::RemoveFailedSink,
// TCComEventsCP::RemoveFailedSink
template <class T, class IV, const IID* piid, class CPD>
void TCComDualEventsCP<T, IV, piid, CPD>::RemoveFailedSink(HRESULT hr,
  IDispatch* pdisp)
{
  // Cast to the appropriate connection point
  CPD* pCP = static_cast<CPD*>(this);

  // Use the CP vector to convert the interface pointer to a cookie
  DWORD dwCookie = pCP->m_vec.GetCookie((IUnknown**)&pdisp);

  // Unadvise the failed connection point sink
  pCP->Unadvise(dwCookie);
}


/////////////////////////////////////////////////////////////////////////////
// Group=

/////////////////////////////////////////////////////////////////////////////
// Sanity Macros

/////////////////////////////////////////////////////////////////////////////
// Macro Group: BEGIN_TCComDualEventsCP, BEGIN_TCComDualEventsCP_IID,
//   END_TCComDualEventsCP
//
// Declaration:
// #define BEGIN_TCComDualEventsCP(className, IV, IID_disp)                 \
//   BEGIN_TCComDualEventsCP_IID(className, IV, IID_##IV, IID_disp)        
// …
// #define BEGIN_TCComDualEventsCP_IID(className, IV, IID_vtbl, IID_disp)   \
//   template <class T> class ATL_NO_VTABLE className :                     \
//     public TCComDualEventsCP<T, IV, &IID_vtbl,                           \
//       IConnectionPointImpl<T, &IID_disp> >                               \
//   {                                                                      \
//   protected:                                                             \
//     typedef TCComDualEventsCP<T, IV, &IID_vtbl,                          \
//       IConnectionPointImpl<T, &IID_disp> >::TIV TIV;                     \
//     typedef TCComDualEventsCP<T, IV, &IID_vtbl,                          \
//       IConnectionPointImpl<T, &IID_disp> >::PIV PIV;
//  …
// #define END_TCComDualEventsCP()                                          \
//   };
//
// Parameters:
//   className - The name of the connection point class that is being
// declared.
//   IV - The name of the v-table *interface* which the connection point
// represents.
//   IID_vtbl - The IID of the v-table *interface* which the connection point
// represents.
//   IID_disp - The IID of the *dispinterface* which the connection point
// represents.
//
// Remarks: These macros declare a class derived from the TCComDualEventsCP
// template. Their use is not mandatory, but does simplify the entire
// declaration of such a connection point class. Either macro should be
// followed by 0, 1, or more instances of the TCComDualEventCP_Fn macros,
// followed by an ending brace (}) and semi-colon (;). See the
// TCComDualEventsCP class overview for an example and more details.
//
// See Also: TCComDualEventsCP, TCComDualEventCP_Fn macros
#define BEGIN_TCComDualEventsCP(className, IV, IID_disp)                    \
  BEGIN_TCComDualEventsCP_IID(className, IV, IID_##IV, IID_disp)          

/////////////////////////////////////////////////////////////////////////////
// {partof:BEGIN_TCComDualEventsCP}
#define BEGIN_TCComDualEventsCP_IID(className, IV, IID_vtbl, IID_disp)      \
  template <class T> class ATL_NO_VTABLE className :                        \
    public TCComDualEventsCP<T, IV, &IID_vtbl,                              \
      IConnectionPointImpl<T, &IID_disp> >                                  \
  {                                                                         \
  protected:                                                                \
    typedef typename TCComDualEventsCP<T, IV, &IID_vtbl,                             \
      IConnectionPointImpl<T, &IID_disp> >::TIV TIV;                        \
    typedef typename TCComDualEventsCP<T, IV, &IID_vtbl,                             \
      IConnectionPointImpl<T, &IID_disp> >::PIV PIV;

/////////////////////////////////////////////////////////////////////////////
// {partof:BEGIN_TCComDualEventsCP}
#define END_TCComDualEventsCP()                                             \
  };


#ifdef _DOCJET_ONLY
  ///////////////////////////////////////////////////////////////////////////
  // Macro Group: TCComDualEventCP_Fn Group of Macros
  //
  // Declaration:
  //   #define TCComDualEventCP_Fn0(fnName, dispid)
  //   #define TCComDualEventCP_Fn1(fnName, dispid, t1,vt1)
  //   #define TCComDualEventCP_Fn2(fnName, dispid, t1,vt1, t2,vt2)
  //   #define TCComDualEventCP_Fn3(fnName, dispid, t1,vt1, t2,vt2, t3,vt3)
  //   #define TCComDualEventCP_Fn4(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4)
  //   #define TCComDualEventCP_Fn5(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5)
  //   #define TCComDualEventCP_Fn6(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5, t6,vt6)
  //   #define TCComDualEventCP_Fn7(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5, t6,vt6, t7,vt7)
  //   #define TCComDualEventCP_Fn8(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5, t6,vt6, t7,vt7, t8,vt8)
  //   #define TCComDualEventCP_Fn9(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5, t6,vt6, t7,vt7, t8,vt8, t9,vt9)
  //   #define TCComDualEventCP_Fn10(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5, t6,vt6, t7,vt7, t8,vt8, t9,vt9, t10,vt10)
  //   #define TCComDualEventCP_Fn11(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5, t6,vt6, t7,vt7, t8,vt8, t9,vt9, t10,vt10,
  //     t11,vt11)
  //   #define TCComDualEventCP_Fn12(fnName, dispid, t1,vt1, t2,vt2, t3,vt3,
  //     t4,vt4, t5,vt5, t6,vt6, t7,vt7, t8,vt8, t9,vt9, t10,vt10,
  //     t11,vt11, t12,vt12)
  //
  // Parameters:
  //   fnName - The event name. This must match the name of the event in the
  // v-table *interface*.
  //   dispid - The dispatch ID of the event in the *dispinterface*.
  //   t1_thru_t12 - The C++ parameter types of the event arguments. These
  // must match the parameter types of the event in the v-table *interface*.
  // These must be the C++ equivalents of [oleautomation] compatible types.
  //   vt1_thru_vt12 - The *VT_* variant types of the event arguments. These
  // must be the variant type of the associated /t1/ thru /t12/ argument.
  // These must be [oleautomation] compatible variant types.
  //
  // Remarks: These macros all perform the same function and are different
  // only in that they specify an event with a different number of arguments.
  // For each event argument, both the C++ type and the *VT_* variant type
  // must be specified, in that order.
  //
  // An inline member function, named by prefixing *Fire_* to the
  // /fnName/ parameter, is declared by each of these macros. The member
  // function is prototyped to take the /t/ parameters, if any. The member
  // function will fire the event to all of the connected sinks of both the
  // *dispinterface* and the v-table *interface* connection points. Actually,
  // the TCUtilityThread class is used to post the event firing process as an
  // element of work to be performed in the context of the shared utility
  // thread. As such, the *Fire_* methods are prototyped with a void return
  // type. Currently, receiving a result or an [out] parameter from a
  // connected sink is not supported.
  //
  // Internally, the macros derive a class from TCComDualEventCall, to hold
  // the arguments of the event call, if any. The /t1/ thru /t12/ and
  // /p1/ thru /p12/ macro parameters are used to create and initialize this
  // nested class appropriately. The DECLARE_CTOR macros are used to declare
  // appropriate member variables and constructors that initialize those
  // variables.
  #define TCComDualEventCP_Fn
#endif // _DOCJET_ONLY

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn0(fnName, dispid)                                \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName();                                             \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 0);                                             \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName()                                       \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName;                               \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn1(fnName, dispid, t1, vt1)                       \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_1(CCall##fnName, t1)                                       \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1);                                         \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 1, vt1, m_a1);                                  \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1)                                  \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1);                           \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn2(fnName, dispid, t1, vt1, t2, vt2)              \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_2(CCall##fnName, t1, t2)                                   \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2);                                   \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 2, vt1, m_a1, vt2, m_a2);                       \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2)                           \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2);                       \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn3(fnName, dispid, t1, vt1, t2, vt2, t3, vt3)     \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_3(CCall##fnName, t1, t2, t3)                               \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3);                             \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 3, vt1, m_a1, vt2, m_a2, vt3, m_a3);            \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3)                    \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3);                   \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn4(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,     \
  t4, vt4)                                                                  \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_4(CCall##fnName, t1, t2, t3, t4)                           \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4);                       \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 4, vt1, m_a1, vt2, m_a2, vt3, m_a3,             \
        vt4, m_a4);                                                         \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4)             \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4);               \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn5(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,     \
  t4, vt4, t5, vt5)                                                         \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_5(CCall##fnName, t1, t2, t3, t4, t5)                       \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5);                 \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 5, vt1, m_a1, vt2, m_a2, vt3, m_a3,             \
        vt4, m_a4, vt5, m_a5);                                              \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)      \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5);           \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn6(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,     \
  t4, vt4, t5, vt5, t6, vt6)                                                \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_6(CCall##fnName, t1, t2, t3, t4, t5, t6)                   \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6);           \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 6, vt1, m_a1, vt2, m_a2, vt3, m_a3,             \
        vt4, m_a4, vt5, m_a5, vt6, m_a6);                                   \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5,      \
    t6 a6)                                                                  \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5, a6);       \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn7(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,     \
  t4, vt4, t5, vt5, t6, vt6, t7, vt7)                                       \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_7(CCall##fnName, t1, t2, t3, t4, t5, t6, t7)               \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7);     \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 7, vt1, m_a1, vt2, m_a2, vt3, m_a3,             \
        vt4, m_a4, vt5, m_a5, vt6, m_a6, vt7, m_a7);                        \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5,      \
    t6 a6, t7 a7)                                                           \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5, a6, a7);   \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn8(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,     \
  t4, vt4, t5, vt5, t6, vt6, t7, vt7, t8, vt8)                              \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_8(CCall##fnName, t1, t2, t3, t4, t5, t6, t7, t8)           \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7,      \
         m_a8);                                                             \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 8, vt1, m_a1, vt2, m_a2, vt3, m_a3,             \
        vt4, m_a4, vt5, m_a5, vt6, m_a6, vt7, m_a7, vt8, m_a8);             \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5,      \
    t6 a6, t7 a7, t8 a8)                                                    \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5, a6, a7,    \
       a8);                                                                 \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn9(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,     \
  t4, vt4, t5, vt5, t6, vt6, t7, vt7, t8, vt8, t9, vt9)                     \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_9(CCall##fnName, t1, t2, t3, t4, t5, t6, t7, t8, t9)       \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7,      \
        m_a8, m_a9);                                                        \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 9, vt1, m_a1, vt2, m_a2, vt3, m_a3,             \
        vt4, m_a4, vt5, m_a5, vt6, m_a6, vt7, m_a7, vt8, m_a8, vt9, m_a9);  \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5,      \
    t6 a6, t7 a7, t8 a8, t9 a9)                                             \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5, a6, a7,    \
      a8, a9);                                                              \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn10(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,    \
  t4, vt4, t5, vt5, t6, vt6, t7, vt7, t8, vt8, t9, vt9, t10, vt10)          \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_10(CCall##fnName, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10) \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7,      \
        m_a8, m_a9, m_a10);                                                 \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 10, vt1, m_a1, vt2, m_a2, vt3, m_a3,            \
        vt4, m_a4, vt5, m_a5, vt6, m_a6, vt7, m_a7, vt8, m_a8, vt9, m_a9,   \
        vt10, m_a10);                                                       \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5,      \
    t6 a6, t7 a7, t8 a8, t9 a9, t10 a10)                                    \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5, a6, a7,    \
      a8, a9, a10);                                                         \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn11(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,    \
  t4, vt4, t5, vt5, t6, vt6, t7, vt7, t8, vt8, t9, vt9, t10, vt10,          \
  t11, vt11)                                                                \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_11(CCall##fnName, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, \
      t11)                                                                  \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7,      \
        m_a8, m_a9, m_a10, m_a11);                                          \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 11, vt1, m_a1, vt2, m_a2, vt3, m_a3,            \
        vt4, m_a4, vt5, m_a5, vt6, m_a6, vt7, m_a7, vt8, m_a8, vt9, m_a9,   \
        vt10, m_a10, vt11, m_a11);                                          \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5,      \
    t6 a6, t7 a7, t8 a8, t9 a9, t10 a10, t11 a11)                           \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5, a6, a7,    \
      a8, a9, a10, a11);                                                    \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComDualEventCP_Fn}
#define TCComDualEventCP_Fn12(fnName, dispid, t1, vt1, t2, vt2, t3, vt3,    \
  t4, vt4, t5, vt5, t6, vt6, t7, vt7, t8, vt8, t9, vt9, t10, vt10,          \
  t11, vt11, t12, vt12)                                                     \
  class CCall##fnName : public TCComDualEventCall<TIV>                      \
  {                                                                         \
    DECLARE_CTOR_12(CCall##fnName, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, \
      t11, t12)                                                             \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##fnName(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7,      \
      m_a8, m_a9, m_a10, m_a11, m_a12);                                     \
    }                                                                       \
    public: virtual void OnCreateDispParams()                               \
    {                                                                       \
      SetDispParams(dispid, 12, vt1, m_a1, vt2, m_a2, vt3, m_a3,            \
        vt4, m_a4, vt5, m_a5, vt6, m_a6, vt7, m_a7, vt8, m_a8, vt9, m_a9,   \
        vt10, m_a10, vt11, m_a11, vt12, m_a12);                             \
    }                                                                       \
  };                                                                        \
  public: inline void Fire_##fnName(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5,      \
  t6 a6, t7 a7, t8 a8, t9 a9, t10 a10, t11 a11, t12 a12)                    \
  {                                                                         \
    CCall##fnName* pCall = new CCall##fnName(a1, a2, a3, a4, a5, a6, a7,    \
      a8, a9, a10, a11, a12);                                               \
    pCall->SetEventName(#fnName);                                           \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         


/////////////////////////////////////////////////////////////////////////////

#endif // !__DualEventsCP_h__

