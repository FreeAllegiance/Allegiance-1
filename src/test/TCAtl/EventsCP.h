#ifndef __EventsCP_h__
#define __EventsCP_h__

/////////////////////////////////////////////////////////////////////////////
// EventsCP.h | Declaration of the TCComEventsCP class.

#pragma warning(disable: 4786)

#include <vector>
#include <algorithm>
#include "EventCall.h"
#include <..\TCLib\WorkerThread.h>
#include <..\TCLib\ObjectLock.h>

/////////////////////////////////////////////////////////////////////////////
// TCComEventsCP is the base class for a connection point that has an
// *IUnknown* derived v-table sink interface.
//
// Consider the following IDL (Interface Definition Language) for a simple
// event interface:
//
//      // ISampleEvents Interface
//      [
//        object, oleautomation, pointer_default(unique),
//        uuid(098A9AF0-1C79-11d2-8519-00400536E6C1),
//        helpstring("ISampleEvents: Sample event interface.")
//      ]
//      interface ISampleEvents : IUnknown
//      {
//        HRESULT OnHello([in] VARIANT_BOOL IsHappy, [in] BSTR Text);
//        …
//        HRESULT OnGoodbye([in] VARIANT_BOOL IsHappy, [in] BSTR Text);
//        …
//      }; // End: interface ISampleEvents : IUnknown
//
// To declare a connection point class for this sample interface, use the
// BEGIN_TCComEventsCP and END_TCComEventsCP macros along with the
// TCComEventCP_Fn macros:
//
//      BEGIN_TCComEventsCP(ISampleEventsCP, ISampleEvents)
//        TCComEventCP_Fn2(OnHello, VARIANT_BOOL, BSTR)
//        TCComEventCP_Fn2(OnGoodbye, VARIANT_BOOL, BSTR)
//      END_TCComEventsCP()
//
// Then, derive your component object from ISampleEventsCP and list the GUID
// of the connection point in your *CONNECTION_POINT_MAP*. To actually fire
// an event to all connected sinks, call the firing function, which is named
// by prefixing *Fire_* to the event name:
//
//        // Fire an 'OnHello' event
//        Fire_OnHello(bHappy, bstrText);
//
// Note: If you have a second event interface, derived from *IDispatch*, with
// the same event methods as the v-table interface, you should use the
// derived class, TCComDualEventsCP, to be able to fire both types of events
// with a single *Fire_* method call.
//
// Parameters:
//   T - The class derived from TCComEventsCP.
//   IV - The v-table interface name.
//   piid - A constant pointer to the IID of the v-table interface, /IV/.
//
// See Also: BEGIN_TCComEventsCP, BEGIN_TCComEventsCP_IID,
// TCComEventCP_Fn macros, TCComDualEventsCP
template <class T, class IV, const IID* piid>
class ATL_NO_VTABLE TCComEventsCP :
  public TCWorkerThread,
  public IConnectionPointImpl<T, piid>
{
// Group=Types
public:
  // Declares a type used as an alias for the /IV/ template parameter.
  typedef IV TIV;
  // Declares a type used as an alias for an /IV/ interface pointer.
  typedef IV* PIV;
  // Declares a vector type of /IV/ interface pointers.
  typedef std::vector<IV*> vector_vtbl;
  // Declares a vector iterator type for vector_vtbl.
  typedef typename vector_vtbl::iterator it_vtbl;

// Overrides
protected:
  virtual IUnknown* OnGetUnknown();
  virtual TC_WorkItemRelProc OnGetWorkItemRelProc();
  virtual void OnMessage(UINT idMsg, int cParams, LPARAM* rgParams);
  static void WINAPI ReleaseArgs(UINT idMsg, int cParams, LPARAM* rgParams);

// Implementation
protected:
  void FireEvents(TCComEventCall<IV>& call);
  void GetEventSinks(vector_vtbl& vec_vtbl);
  void RemoveFailedSink(HRESULT hr, IV* pIfVtbl);

// Enumerations
protected:
  enum {e_idFireEvents};
};


/////////////////////////////////////////////////////////////////////////////
// Group=Overrides

/////////////////////////////////////////////////////////////////////////////
// Returns the *IUnknown* of this connection point.
template <class T, class IV, const IID* piid>
IUnknown* TCComEventsCP<T, IV, piid>::OnGetUnknown()
{
  return static_cast<T*>(this)->GetUnknown();
}

/////////////////////////////////////////////////////////////////////////////
// Returns the address of the static ReleaseArgs method.
template <class T, class IV, const IID* piid>
TC_WorkItemRelProc TCComEventsCP<T, IV, piid>::OnGetWorkItemRelProc()
{
  return ReleaseArgs;
}

/////////////////////////////////////////////////////////////////////////////
// Uses the function call TCComEventCall::operator() of the class derived
// from TCComEventCall.
template <class T, class IV, const IID* piid>
void TCComEventsCP<T, IV, piid>::OnMessage(UINT idMsg, int cParams,
  LPARAM* rgParams)
{
  switch (idMsg)
  {
    case e_idFireEvents:
    {
      // Get the call pointer from the parameter list
      assert(1 == cParams);
      TCComEventCall<IV>* pCall = (TCComEventCall<IV>*)rgParams[0];

      // Call FireEvents in the context of the utility thread
      assert(pCall);
      FireEvents(*pCall);
      break;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Deletes the object derived from TCComEventCall.
template <class T, class IV, const IID* piid>
void TCComEventsCP<T, IV, piid>::ReleaseArgs(UINT idMsg, int cParams,
  LPARAM* rgParams)
{
  switch (idMsg)
  {
    case e_idFireEvents:
    {
      // Get the call pointer from the parameter list
      assert(1 == cParams);
      TCComEventCall<IV>* pCall = (TCComEventCall<IV>*)rgParams[0];

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
// Remarks: This method is used by the TCComEventCP_Fn macros to fire the
// specified event to all connected sinks. The event is specified as a
// reference to an TCComEventCall-derived class instance.
//
// Parameters:
//   call - A reference to an instance of a class derived from
// TCComEventCall. This class is derived automatically by the
// TCComEventCP_Fn macros.
//
// See Also: TCComEventCP_Fn macros, TCComEventCall,
// TCComDualEventsCP::FireEvents
template <class T, class IV, const IID* piid>
void TCComEventsCP<T, IV, piid>::FireEvents(TCComEventCall<IV>& call)
{
  // Copy all of the vtable sinks
  vector_vtbl vec_vtbl;
  GetEventSinks(vec_vtbl);

  // Notify all vtable event sinks
  HRESULT hr;
  for (it_vtbl it = vec_vtbl.begin(); it != vec_vtbl.end(); ++it)
    if (FAILED(hr = call(*it)))
      RemoveFailedSink(hr, *it);
}

/////////////////////////////////////////////////////////////////////////////
// Description: Copies the derived-class's event sinks to the specified
// vector.
//
// Remarks: Copies the derived-class's event sinks to the specified vector.
// The derived-class instance is locked just long enough to copy the event
// sinks. The instance does *not* remain locked for the duration of the event
// calls.
//
// Parameters:
//   vec_vtbl - A reference to the vector where the v-table event sinks are
// to be copied.
//
// See Also: TCComEventsCP::FireEvents, TCComDualEventsCP::GetEventSinks
template <class T, class IV, const IID* piid>
inline void TCComEventsCP<T, IV, piid>::GetEventSinks(
// VS.Net 2003 port: use same type as in declaration (see line 92)
#if _MSC_VER >= 1310
	vector_vtbl& vec_vtbl)
#else
	TCComEventsCP<T, IV, piid>::vector_vtbl& vec_vtbl)
#endif
{
  // Lock the object (just long enough to copy the event sinks)
  TCObjectLock<T> lock(static_cast<T*>(this));

  // Transform the vtable sinks to the specified vec_vtbl
  CComDynamicUnkArray& vec_v =
    static_cast<IConnectionPointImpl<T, piid>*>(this)->m_vec;
  vec_vtbl.resize(vec_v.end() - vec_v.begin(), NULL);
  it_vtbl itDest = vec_vtbl.begin();
  for (IUnknown** itSrc = vec_v.begin(); itSrc != vec_v.end(); ++itSrc, ++itDest)
    *itDest = static_cast<IV*>(*itSrc);
}

/////////////////////////////////////////////////////////////////////////////
// Description: Removes an event sink if an event call failed.
//
// Remarks: This method removes an event sink from the derived-class object
// if an event call failed. This prevents all further event calls to that
// sink. Typically, the event call fails due to an RPC timeout error.
//
// Parameters:
//   hr - The HRESULT of the failed event call.
//   pIfVtbl - The v-table interface pointer of the sink that failed.
//
// See Also: TCComEventsCP::FireEvents, TCComDualEventsCP::RemoveFailedSink
template <class T, class IV, const IID* piid>
void TCComEventsCP<T, IV, piid>::RemoveFailedSink(HRESULT hr, IV* pIfVtbl)
{
  // Cast to the appropriate connection point
  IConnectionPointImpl<T, piid>* pCP =
    static_cast<IConnectionPointImpl<T, piid>*>(this);

  // Use the CP vector to convert the specified interface pointer to a cookie
  DWORD dwCookie = pCP->m_vec.GetCookie((IUnknown**)&pIfVtbl);

  // Unadvise the failed connection point sink
  pCP->Unadvise(dwCookie);
}


/////////////////////////////////////////////////////////////////////////////
// Group=

/////////////////////////////////////////////////////////////////////////////
// Sanity Macros

/////////////////////////////////////////////////////////////////////////////
// Macro Group: BEGIN_TCComEventsCP, BEGIN_TCComEventsCP_IID,
//   END_TCComEventsCP
//
// Declaration:
// #define BEGIN_TCComEventsCP(className, IV)                               \
//   BEGIN_TCComEventsCP_IID(className, IV, IID_##IV)                      
// …
// #define BEGIN_TCComEventsCP_IID(className, IV, IID_vtbl)                 \
//   template <class T>                                                     \
//   class ATL_NO_VTABLE className : public TCComEventsCP<T, IV, &IID_vtbl> \
//   {                                                                      \
//   protected:                                                             \
//     typedef TCComEventsCP<T, IV, &IID_vtbl>::TIV TIV;                    \
//     typedef TCComEventsCP<T, IV, &IID_vtbl>::PIV PIV;
// …
// #define END_TCComEventsCP()                                              \
//   };
//
// Parameters:
//   className - The name of the connection point class that is being
// declared.
//   IV - The name of the *interface* which the connection point represents.
//   IID_vtbl - The IID of the *interface* which the connection point
// represents.
//
// Remarks: These macros declare a class derived from the TCComEventsCP
// template. Their use is not mandatory, but does simplify the entire
// declaration of such a connection point class. Either *BEGIN_* macro should
// be followed by 0, 1, or more instances of the TCComEventCP_Fn macros,
// followed by the END_TCComEventsCP macro. See the
// TCComEventsCP class overview for an example and more details.
//
// See Also: TCComEventsCP, TCComEventCP_Fn macros
#define BEGIN_TCComEventsCP(className, IV)                                  \
  BEGIN_TCComEventsCP_IID(className, IV, IID_##IV)                        

/////////////////////////////////////////////////////////////////////////////
// {partof:BEGIN_TCComEventsCP}
#define BEGIN_TCComEventsCP_IID(className, IV, IID_vtbl)                    \
  template <class T>                                                        \
  class ATL_NO_VTABLE className : public TCComEventsCP<T, IV, &IID_vtbl>    \
  {                                                                         \
  protected:                                                                \
    typedef typename TCComEventsCP<T, IV, &IID_vtbl>::TIV TIV;                       \
    typedef typename TCComEventsCP<T, IV, &IID_vtbl>::PIV PIV;

/////////////////////////////////////////////////////////////////////////////
// {partof:BEGIN_TCComEventsCP}
#define END_TCComEventsCP()                                                 \
  };


#ifdef _DOCJET_ONLY
  ///////////////////////////////////////////////////////////////////////////
  // Macro Group: TCComEventCP_Fn Group of Macros
  //
  // Declaration:
  // #define TCComEventCP_Fn0(fnName)
  // #define TCComEventCP_Fn1(fnName,t1)
  // #define TCComEventCP_Fn2(fnName,t1,t2)
  // #define TCComEventCP_Fn3(fnName,t1,t2,t3)
  // #define TCComEventCP_Fn4(fnName,t1,t2,t3,t4)
  // #define TCComEventCP_Fn5(fnName,t1,t2,t3,t4,t5)
  // #define TCComEventCP_Fn6(fnName,t1,t2,t3,t4,t5,t6)
  // #define TCComEventCP_Fn7(fnName,t1,t2,t3,t4,t5,t6,t7)
  // #define TCComEventCP_Fn8(fnName,t1,t2,t3,t4,t5,t6,t7,t8)
  // #define TCComEventCP_Fn9(fnName,t1,t2,t3,t4,t5,t6,t7,t8,t9)
  // #define TCComEventCP_Fn10(fnName,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10)
  // #define TCComEventCP_Fn11(fnName,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11)
  // #define TCComEventCP_Fn12(fnName,t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12)
  //
  // Parameters:
  //   fnName - The event name. This must match the name of the event in the
  // v-table *interface*.
  //   t1_thru_t12 - The C++ parameter types of the event arguments. These
  // must match the parameter types of the event in the v-table *interface*.
  //
  // Remarks: These macros all perform the same function and are different
  // only in that they specify an event with a different number of arguments.
  //
  // An inline member function, named by prefixing *Fire_* to the
  // /fnName/ parameter, is declared by each of these macros. The member
  // function is prototyped to take the /t/ parameters, if any. The member
  // function will fire the event to all of the connected sinks. Actually,
  // the TCUtilityThread class is used to post the event firing process as an
  // element of work to be performed in the context of the shared utility
  // thread. As such, the *Fire_* methods are prototyped with a void return
  // type. Currently, receiving a result or an [out] parameter from a
  // connected sink is not supported.
  //
  // Internally, the macros derive a class from TCComEventCall, to hold the
  // arguments of the event call, if any. The /t1/ thru /t12/ macro
  // parameters are used to create and initialize this nested class
  // appropriately. The DECLARE_CTOR macros are used to declare appropriate
  // member variables and constructors that initialize those variables.
  #define TCComEventCP_Fn
#endif // _DOCJET_ONLY

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn0(name)                                              \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name();                                               \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name()                                                \
  {                                                                         \
    CCall##name* pCall = new CCall##name;                                   \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn1(name, t1)                                          \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_1(CCall##name, t1)                                         \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1);                                           \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1)                                           \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1);                               \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn2(name, t1, t2)                                      \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_2(CCall##name, t1, t2)                                     \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2);                                     \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2)                                    \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2);                           \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn3(name, t1, t2, t3)                                  \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_3(CCall##name, t1, t2, t3)                                 \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3);                               \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3)                             \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3);                       \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn4(name, t1, t2, t3, t4)                              \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_4(CCall##name, t1, t2, t3, t4)                             \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4);                         \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4)                      \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4);                   \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn5(name, t1, t2, t3, t4, t5)                          \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_5(CCall##name, t1, t2, t3, t4, t5)                         \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5);                   \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)               \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5);               \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn6(name, t1, t2, t3, t4, t5, t6)                      \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_6(CCall##name, t1, t2, t3, t4, t5, t6)                     \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6);             \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)        \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5, a6);           \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn7(name, t1, t2, t3, t4, t5, t6, t7)                  \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_7(CCall##name, t1, t2, t3, t4, t5, t6, t7)                 \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7);       \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6,        \
    t7 a7)                                                                  \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5, a6, a7);       \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn8(name, t1, t2, t3, t4, t5, t6, t7, t8)              \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_8(CCall##name, t1, t2, t3, t4, t5, t6, t7, t8)             \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8); \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6,        \
    t7 a7, t8 a8)                                                           \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5, a6, a7, a8);   \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn9(name, t1, t2, t3, t4, t5, t6, t7, t8, t9)          \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_9(CCall##name, t1, t2, t3, t4, t5, t6, t7, t8, t9)         \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8,  \
        m_a9);                                                              \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6,        \
    t7 a7, t8 a8, t9 a9)                                                    \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5, a6, a7, a8,    \
      a9);                                                                  \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn10(name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)    \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_10(CCall##name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)   \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8,  \
        m_a9, m_a10);                                                       \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6,        \
    t7 a7, t8 a8, t9 a9, t10 a10)                                           \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5, a6, a7, a8,    \
      a9, a10);                                                             \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn11(name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10,    \
  t11)                                                                      \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_11(CCall##name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10,   \
      t11)                                                                  \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8,  \
        m_a9, m_a10, m_a11);                                                \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6,        \
    t7 a7, t8 a8, t9 a9, t10 a10, t11 a11)                                  \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5, a6, a7, a8,    \
      a9, a10, a11);                                                        \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         

/////////////////////////////////////////////////////////////////////////////
// {partof:TCComEventCP_Fn}
#define TCComEventCP_Fn12(name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10,    \
  t11, t12)                                                                 \
  class CCall##name : public TCComEventCall<TIV>                            \
  {                                                                         \
    DECLARE_CTOR_12(CCall##name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10,   \
      t11, t12)                                                             \
    public: virtual HRESULT OnCallEventSink(PIV pvtbl)                      \
    {                                                                       \
      return pvtbl->##name(m_a1, m_a2, m_a3, m_a4, m_a5, m_a6, m_a7, m_a8,  \
        m_a9, m_a10, m_a11, m_a12);                                         \
    }                                                                       \
  };                                                                        \
  public: void Fire_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6,        \
    t7 a7, t8 a8, t9 a9, t10 a10, t11 a11, t12 a12)                         \
  {                                                                         \
    CCall##name* pCall = new CCall##name(a1, a2, a3, a4, a5, a6, a7, a8,    \
      a9, a10, a11, a12);                                                   \
    pCall->SetEventName(#name);                                             \
    PostMessage(e_idFireEvents, 1, LPARAM(pCall));                          \
  }                                                                         


#ifdef _DOCJET_ONLY
  ///////////////////////////////////////////////////////////////////////////
  // Macro Group: DECLARE_CTOR Construction Macros
  //
  // Declaration:
  // #define DECLARE_CTOR_1(className, t1)
  // #define DECLARE_CTOR_2(className, t1, t2)
  // #define DECLARE_CTOR_3(className, t1, t2, t3)
  // #define DECLARE_CTOR_4(className, t1, t2, t3, t4)
  // #define DECLARE_CTOR_5(className, t1, t2, t3, t4, t5)
  // #define DECLARE_CTOR_6(className, t1, t2, t3, t4, t5, t6)
  // #define DECLARE_CTOR_7(className, t1, t2, t3, t4, t5, t6, t7)
  // #define DECLARE_CTOR_8(className, t1, t2, t3, t4, t5, t6, t7, t8)
  // #define DECLARE_CTOR_9(className, t1, t2, t3, t4, t5, t6, t7, t8, t9)
  // #define DECLARE_CTOR_10(className,                                     \
  //   t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)
  // #define DECLARE_CTOR_11(className,                                     \
  //   t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11)
  // #define DECLARE_CTOR_12(className,                                     \
  //   t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12)
  // #define DECLARE_CTOR_13(className,                                     \
  //   t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13)
  // #define DECLARE_CTOR_14(className,                                     \
  //   t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14)
  // #define DECLARE_CTOR_15(className,                                     \
  //   t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15)
  // #define DECLARE_CTOR_16(className,                                     \
  //   t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16)
  //
  // Parameters:
  //   className - The name of the class for which a constructor is being
  // declared.
  //   t1_thru_t16 - The data types of the member variables and constructor
  // arguments to be declared.
  //
  // Remarks: These macros all perform the same function and are different
  // only in that they specify a different number of arguments. For each
  // argument, a *protected* data member is declared of the type specified by
  // each /t/ parameter, and a constructor argument is declared of that same
  // type. The constructor is declared *public* and its initializer list
  // simply initializes the declared member variables to the values specified
  // in the constructor argument list.
  //
  // Note: These macros are currently only used by the TCComEventCP_Fn and
  // TCComDualEventCP_Fn macros. If their use is ever needed outside the
  // scope of connection point sinks, they should be moved into their own
  // header file and included in this one.
  //
  // See Also: TCComEventCP_Fn macros, TCComDualEventCP_Fn macros
  #define DECLARE_CTOR
#endif // _DOCJET_ONLY

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_1(name, t1)                                            \
  protected: t1 m_a1;                                                       \
  public: name(t1 a1)                                                       \
  : m_a1(a1) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_2(name, t1, t2)                                        \
  protected: t1 m_a1; t2 m_a2;                                              \
  public: name(t1 a1, t2 a2)                                                \
  : m_a1(a1), m_a2(a2) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_3(name, t1, t2, t3)                                    \
  protected: t1 m_a1; t2 m_a2; t3 m_a3;                                     \
  public: name(t1 a1, t2 a2, t3 a3)                                         \
  : m_a1(a1), m_a2(a2), m_a3(a3) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_4(name, t1, t2, t3, t4)                                \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4;                            \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4)                                  \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_5(name, t1, t2, t3, t4, t5)                            \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5;                   \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)                           \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_6(name, t1, t2, t3, t4, t5, t6)                        \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6;          \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)                    \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_7(name, t1, t2, t3, t4, t5, t6, t7)                    \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7)             \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_8(name, t1, t2, t3, t4, t5, t6, t7, t8)                \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8;                                                                  \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8)      \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_9(name, t1, t2, t3, t4, t5, t6, t7, t8, t9)            \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9;                                                         \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9)                                                                  \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_10(name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10)      \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9; t10 m_a10;                                              \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9, t10 a10)                                                         \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9), m_a10(a10) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_11(name, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11) \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9; t10 m_a10; t11 m_a11;                                   \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9, t10 a10, t11 a11)                                                \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9), m_a10(a10), m_a11(a11) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_12(name,                                               \
  t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12)                        \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9; t10 m_a10; t11 m_a11; t12 m_a12;                        \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9, t10 a10, t11 a11, t12 a12)                                       \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9), m_a10(a10), m_a11(a11), m_a12(a12) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_13(name,                                               \
  t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13)                   \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9; t10 m_a10; t11 m_a11; t12 m_a12; t13 m_a13;             \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9, t10 a10, t11 a11, t12 a12, t13 a13)                              \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9), m_a10(a10), m_a11(a11), m_a12(a12), m_a13(a13) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_14(name,                                               \
  t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14)              \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9; t10 m_a10; t11 m_a11; t12 m_a12; t13 m_a13; t14 m_a14;  \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9, t10 a10, t11 a11, t12 a12, t13 a13, t14 a14)                     \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9), m_a10(a10), m_a11(a11), m_a12(a12), m_a13(a13),       \
  m_a14(a14) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_15(name,                                               \
  t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15)         \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9; t10 m_a10; t11 m_a11; t12 m_a12; t13 m_a13; t14 m_a14;  \
  t15 m_a15;                                                                \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9, t10 a10, t11 a11, t12 a12, t13 a13, t14 a14, t15 a15)            \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9), m_a10(a10), m_a11(a11), m_a12(a12), m_a13(a13),       \
  m_a14(a14), m_a15(a15) {}

/////////////////////////////////////////////////////////////////////////////
// {partof:DECLARE_CTOR}
#define DECLARE_CTOR_16(name,                                               \
  t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16)    \
  protected: t1 m_a1; t2 m_a2; t3 m_a3; t4 m_a4; t5 m_a5; t6 m_a6; t7 m_a7; \
  t8 m_a8; t9 m_a9; t10 m_a10; t11 m_a11; t12 m_a12; t13 m_a13; t14 m_a14;  \
  t15 m_a15; t16 m_a16;                                                     \
  public: name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6, t7 a7, t8 a8,      \
    t9 a9, t10 a10, t11 a11, t12 a12, t13 a13, t14 a14, t15 a15, t16 a16)   \
  : m_a1(a1), m_a2(a2), m_a3(a3), m_a4(a4), m_a5(a5), m_a6(a6), m_a7(a7),   \
  m_a8(a8), m_a9(a9), m_a10(a10), m_a11(a11), m_a12(a12), m_a13(a13),       \
  m_a14(a14), m_a15(a15), m_a16(a16) {}


#ifdef _DOCJET_ONLY

  ///////////////////////////////////////////////////////////////////////////
  // TCComPropertyNotifySinkCP declares a connection point class for the
  // *IPropertyNotifySink* interface.
  //
  // Please refer to the TCComEventsCP class overview for more information on
  // how to use such a class.
  //
  // See Also: TCComEventsCP, BEGIN_TCComEventsCP, TCComFakeNotifySink,
  // TCComPropertyClass
  template <class T>
  class ATL_NO_VTABLE TCComPropertyNotifySinkCP :
    public TCComEventsCP<T, IPropertyNotifySink, &IID_IPropertyNotifySink>
  {
    void Fire_OnChanged(DISPID dispID);     // {secret}
    void Fire_OnRequestEdit(DISPID dispID); // {secret}
  };

#else // _DOCJET_ONLY

  BEGIN_TCComEventsCP(TCComPropertyNotifySinkCP, IPropertyNotifySink)
    TCComEventCP_Fn1(OnChanged, DISPID);
    TCComEventCP_Fn1(OnRequestEdit, DISPID);
  END_TCComEventsCP()

#endif // _DOCJET_ONLY


/////////////////////////////////////////////////////////////////////////////

#endif // !__EventsCP_h__
