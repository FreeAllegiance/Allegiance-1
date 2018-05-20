/////////////////////////////////////////////////////////////////////////////
// PigBehaviorScriptMethods.cpp : Implementation of the
// CPigBehaviorScriptMethods class.

#include "pch.h"
#include "PigBehaviorScriptType.h"
#include "PigBehaviorScriptMethods.h"
#include "resource.h"


/////////////////////////////////////////////////////////////////////////////
// CPigBehaviorScriptMethods


/////////////////////////////////////////////////////////////////////////////
// Static Initialization

#define SCRIPT_METHOD(methodName, cParams)                                  \
  TCLookupTable_ENTRY2(CPigBehaviorScriptMethods::EMethod_##methodName,     \
    OLESTR(#methodName), cParams)

TCLookupTable_BEGIN(CPigBehaviorScriptMethods, Methods)
  SCRIPT_METHOD(OnActivate              , 1)
  SCRIPT_METHOD(OnDeactivate            , 1)
  SCRIPT_METHOD(OnStateNonExistant      , 1)
  SCRIPT_METHOD(OnStateLoggingOn        , 1)
  SCRIPT_METHOD(OnStateLoggingOff       , 1)
  SCRIPT_METHOD(OnStateMissionList      , 1)
  SCRIPT_METHOD(OnStateCreatingMission  , 1)
  SCRIPT_METHOD(OnStateJoiningMission   , 1)
  SCRIPT_METHOD(OnStateQuittingMission  , 1)
  SCRIPT_METHOD(OnStateTeamList         , 1)
  SCRIPT_METHOD(OnStateJoiningTeam      , 1)
  SCRIPT_METHOD(OnStateWaitingForMission, 1)
  SCRIPT_METHOD(OnStateDocked           , 1)
  SCRIPT_METHOD(OnStateLaunching        , 1)
  SCRIPT_METHOD(OnStateFlying           , 1)
  SCRIPT_METHOD(OnStateTerminated       , 1)
  SCRIPT_METHOD(OnMissionStarted        , 0)
  SCRIPT_METHOD(OnReceiveChat           , 2)  // We don't count the [retval]
  SCRIPT_METHOD(OnShipDamaged           , 6)
  SCRIPT_METHOD(OnShipKilled            , 5)
  SCRIPT_METHOD(OnSectorChange          , 2)
  SCRIPT_METHOD(OnAlephHit              , 1)
TCLookupTable_END()


/////////////////////////////////////////////////////////////////////////////
// Operations

HRESULT CPigBehaviorScriptMethods::ResolveMethod(REFCLSID clsid,
  ITypeComp* ptc, LPCOLESTR pszMethodName, short cParams, DISPID* pid,
  CPigBehaviorScriptType* pType)
{
  // Initialize the [out] parameter
  assert(pid);
  *pid = DISPID_UNKNOWN;

  // Lookup the specified method name
  BINDPTR      bp;
  DESCKIND     dk;
  ITypeInfoPtr spti;
  ULONG        hash = LHashValOfName(GetThreadLocale(), pszMethodName);
  if (FAILED(ptc->Bind(const_cast<OLECHAR*>(pszMethodName), hash,
    DISPATCH_METHOD, &spti, &dk, &bp)))
      return S_FALSE;

  // Determine how to process and/or release the returned BINDPTR
  switch (dk)
  {
    case DESCKIND_FUNCDESC:
      // Ensure that the method takes the specified number of parameters
      if (bp.lpfuncdesc->cParams != cParams)
      {
        // Load the error formatting string
        CComBSTR bstrFmt;
        const UINT ids = (bp.lpfuncdesc->cParams < cParams) ?
          IDS_E_FMT_TOOFEWPARAMS : IDS_E_FMT_TOOMANYPARAMS;
        _VERIFYE(bstrFmt.LoadString(ids));

        // Get the script file path name
        CComBSTR bstrFile;
        _SVERIFYE(pType->get_FilePathName(&bstrFile));

        // Format the error message
        OLECHAR szMsg[_MAX_PATH];
        swprintf(szMsg, bstrFmt, bstrFile, pszMethodName,
          bp.lpfuncdesc->cParams, cParams);
        return AtlReportError(clsid, szMsg, IID_IPigBehavior,
          MAKE_HRESULT(1, FACILITY_ITF, ids));
      }

      // Copy the DISPID of the method to the [out] parameter
      *pid = bp.lpfuncdesc->memid;

      // Release the returned FUNCDESC
      assert(NULL != spti);
      spti->ReleaseFuncDesc(bp.lpfuncdesc);
      return S_OK;

    case DESCKIND_VARDESC:
      // Release the returned VARDESC
      assert(NULL != spti);
      spti->ReleaseVarDesc(bp.lpvardesc);
      break;

    case DESCKIND_TYPECOMP:
      // Release the returned ITypeComp
      bp.lptcomp->Release();
      break;
  }

  // The specified method name could not be bound to
  return S_FALSE;
}


HRESULT CPigBehaviorScriptMethods::ResolveMethod(REFCLSID clsid,
  IDispatch* pd, LPCOLESTR pszMethodName, short cParams, DISPID* pid,
  CPigBehaviorScriptType* pType)
{
  assert(pd);  

  // Get the ITypeInfo of the specified IDispatch
  LCID lcid = GetThreadLocale();
  ITypeInfoPtr spti;
  RETURN_FAILED(pd->GetTypeInfo(0, lcid, &spti));

  // Get the ITypeComp of the ITypeInfo
  ITypeCompPtr sptc;
  RETURN_FAILED(spti->GetTypeComp(&sptc));

  // Delegate to the method that takes the ITypeComp*
  return ResolveMethod(clsid, sptc, const_cast<OLECHAR*>(pszMethodName),
    cParams, pid, pType);
}


HRESULT CPigBehaviorScriptMethods::ResolveMethods(REFCLSID clsid,
  IDispatch* pd, CPigBehaviorScriptType* pType)
{
  // Get the ITypeInfo of the specified IDispatch
  LCID lcid = GetThreadLocale();
  ITypeInfoPtr spti;
  RETURN_FAILED(pd->GetTypeInfo(0, lcid, &spti));

  // Get the ITypeComp of the ITypeInfo
  ITypeCompPtr sptc;
  RETURN_FAILED(spti->GetTypeComp(&sptc));

  // Resolve each method name in the table
  for (Methods_ValueType* it = Methods;
    it < Methods + TCLookupTable_SIZE(Methods); ++it)
  {
    EMethod   eMethod   = it->m_key;
    LPCOLESTR pszMethod = it->m_value.m_pszName;
    short     cParams   = it->m_value.m_cParams;

    // Resolve the next method name in the table
    DISPID dispid;
    RETURN_FAILED(ResolveMethod(clsid, sptc, pszMethod, cParams, &dispid,
      pType));
    m_dispids[eMethod] = dispid;
  }

  // Indicate success
  return S_OK;
}

