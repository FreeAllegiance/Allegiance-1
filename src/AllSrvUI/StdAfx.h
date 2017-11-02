// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__42522DF4_4621_4DC3_8967_8416F636D54C__INCLUDED_)
#define AFX_STDAFX_H__42522DF4_4621_4DC3_8967_8416F636D54C__INCLUDED_

#define _WIN32_WINNT 0x0A00


#if _MSC_VER > 1000
  #pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

// MFC Includes
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
  #include <afxcmn.h>       // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxmt.h>          // MFC support for multi-threading

// Win32 Includes
#include <shlobj.h>
#include <lmcons.h>         // For UNLEN

// ATL Includes
#include <atlbase.h>        // For CRegKey

// ZLib Includes
#define __MODULE__ "ALLSRVUI"
#include <mmsystem.h>
#define ZLIB_SKIP_NEW_REDEF
#include <zlib.h>
#include <zreg.h>
#include <FixDelete.h>

// TCLib Includes
#include <..\TCLib\AutoHandle.h>

// AGC and AllSrv Includes
#include <agc.h>
#include <AllSrvModuleIDL.h>


/////////////////////////////////////////////////////////////////////////////
// Determine if we're running on Windows NT or not.
//
// Return Value: *true* if the operation system is Windows NT or
// Windows 2000. *false* if the operating system is Windows 95 or Windows 98.
//
inline bool IsWinNT()
{
  OSVERSIONINFO osvi = {sizeof(osvi)};
  GetVersionEx(&osvi);
  return !!(VER_PLATFORM_WIN32_NT & osvi.dwPlatformId);
}


/////////////////////////////////////////////////////////////////////////////
// Macros

#ifndef sizeofArray
  ///////////////////////////////////////////////////////////////////////////
  // Counts the number of elements in a fixed-length array.
  // Parameters:  x - The name of the array of which to compute the size.
  #define sizeofArray(x) (sizeof(x) / sizeof(x[0]))
#endif // !sizeofArray


/////////////////////////////////////////////////////////////////////////////
// Allegiance product registry keys
//
#include <regkey.h>


/////////////////////////////////////////////////////////////////////////////
// Constants
const int c_cMaxChatsInRegistry = 30;
//const int LVS_EX_LABELTIP = 0x00004000;




/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_STDAFX_H__42522DF4_4621_4DC3_8967_8416F636D54C__INCLUDED_)
