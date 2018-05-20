// pch.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_pch_H__1F7E7E81_C6F0_11D2_85C9_00C04F68DEB0__INCLUDED_)
#define AFX_pch_H__1F7E7E81_C6F0_11D2_85C9_00C04F68DEB0__INCLUDED_

#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0A00
#endif
#define __MODULE__ "PigSrv"                                                //

//imago 10/14
#undef _ATL_DEBUG_QI
#undef _ATL_DEBUG_INTERFACES

#pragma warning(disable: 4786)

#include <atlbase.h>
#include <..\TCAtl\ComModule.h>
class CExeModule : public TCComModule<CExeModule>
{
// Command-line Option Map
public:
  BEGIN_CMDLINE_OPTION_MAP()
    CMDLINE_OPTION_Automation()
    CMDLINE_OPTION_RegServer9x(CExeModule)
    CMDLINE_OPTION_RegServerNT(CExeModule)
    CMDLINE_OPTION_UnregServer(CExeModule)
    CMDLINE_OPTION_NoExit(CExeModule)
    CMDLINE_OPTION_Exit(CExeModule)
    CMDLINE_OPTION_Help(CExeModule)
  END_CMDLINE_OPTION_MAP()

// Overrides
public:
  HRESULT GetAccessSecurity(PSECURITY_DESCRIPTOR* psd, DWORD* pcb);
  void    DisplaySyntax();
  bool    UseMessageBox() {return false;}
  LPCTSTR GetRegInfo(UINT& idResource, bool& bRegTypeLib);
  _ATL_REGMAP_ENTRY* GetRegMapEntries();
};
extern CExeModule _Module;
#include <atlcom.h>

#include <algorithm>
#include <map>
#include <set>
#include <list>
#include <vector>
#include <stack>
#include <queue>
#include <comdef.h>
#include <activscp.h>
#include <msxml.h>
//#include <comdef.h>

// BT - STEAM
#include "steam_api.h"

#pragma warning(disable:4192)

/////////////////////////////////////////////////////////////////////////////
// Begin: Allegiance Header Files                                          //
//                                                                         //
#include <dplay8.h>                                                         //
//#include <dplobby.h>                                                       //
#include <ddraw.h>                                                         //
//#include <d3d.h>                                                           //
//#include <dsound.h>                                                        //
#include <wininet.h>                                                       //
#include <shlwapi.h>                                                       //
#include <guids.h>                                                         //
#include <..\zlib\zlib.h>                                                  //
#include <FixDelete.h>                                                     //
//                                                                         //
// End: Allegiance Header Files                                            //
/////////////////////////////////////////////////////////////////////////////



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_pch_H__1F7E7E81_C6F0_11D2_85C9_00C04F68DEB0__INCLUDED)
