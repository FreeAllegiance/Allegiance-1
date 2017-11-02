#ifndef __VersionInfo_h__
#define __VersionInfo_h__

/////////////////////////////////////////////////////////////////////////////
// VersionInfo.h : Declaration of the ZVersionInfo class.
//

#include <Windows.h>
#include <winver.h>
#include <tchar.h>

#include "zassert.h"
#include "zstring.h"

#ifndef sizeofArray
  ///////////////////////////////////////////////////////////////////////////
  // Counts the number of elements in a fixed-length array.
  // Parameters:  x - The name of the array of which to compute the size.
  #define sizeofArray(x) (sizeof(x) / sizeof(x[0]))
#endif // !sizeofArray


/////////////////////////////////////////////////////////////////////////////
// ZVersionInfo

class ZVersionInfo
{
// Construction / Destruction
public:
  ZVersionInfo(bool);
  ZVersionInfo(LPCTSTR szModule);
  ZVersionInfo(HINSTANCE hinstance = NULL);
  virtual ~ZVersionInfo();
  bool Load(LPCTSTR szModule);
  bool Load(HINSTANCE hinstance = NULL);
  bool Load(const void* pvVerInfo, UINT cbVerInfo);
  void Unload();

// Attributes
public:
  // File Version
  ZString GetFileVersionString   () const;
  uint16_t    GetFileVersionMSHigh   () const;
  uint16_t    GetFileVersionMSLow    () const;
  uint16_t    GetFileVersionLSHigh   () const;
  uint16_t    GetFileVersionLSLow    () const;
  uint16_t    GetFileBuildNumber     () const;
  // Product Version
  ZString GetProductVersionString() const;
  uint16_t    GetProductVersionMSHigh() const;
  uint16_t    GetProductVersionMSLow () const;
  uint16_t    GetProductVersionLSHigh() const;
  uint16_t    GetProductVersionLSLow () const;
  uint16_t    GetProductBuildNumber  () const;

  // File Flags
  bool IsDebug            () const;
  bool IsInfoInferred     () const;
  bool IsPatched          () const;
  bool IsPreRelease       () const;
  bool IsPrivateBuild     () const;
  bool IsSpecialBuild     () const;

  // Target Operating Systems
  bool IsTargetUnknown    () const;
  bool IsTargetDOS        () const;
  bool IsTargetOS216      () const;
  bool IsTargetOS232      () const;
  bool IsTargetNT         () const;
  bool IsTargetBase       () const;
  bool IsTargetWin16      () const;
  bool IsTargetPM16       () const;
  bool IsTargetPM32       () const;
  bool IsTargetWin32      () const;
  bool IsTargetDOSWin16   () const;
  bool IsTargetDOSWin32   () const;
  bool IsTargetOS216PM16  () const;
  bool IsTargetOS232PM32  () const;
  bool IsTargetNTWin32    () const;

  // File Types
  bool IsApp              () const;
  bool IsDll              () const;
  bool IsDriver           () const;
  bool IsFont             () const;
  bool IsVxd              () const;
  bool IsStaticLib        () const;

  // Driver Types
  bool IsDriverUnknown    () const;
  bool IsDriverPrinter    () const;
  bool IsDriverKeyboard   () const;
  bool IsDriverLanguage   () const;
  bool IsDriverDisplay    () const;
  bool IsDriverMouse      () const;
  bool IsDriverNetwork    () const;
  bool IsDriverSystem     () const;
  bool IsDriverInstallable() const;
  bool IsDriverSound      () const;
  bool IsDriverComm       () const;
  bool IsDriverInputMethod() const;

  // Font Types
  bool IsFontRaster       () const;
  bool IsFontVector       () const;
  bool IsFontTrueType     () const;

  // String Values
  void   SetLanguageID    (LANGID wLangID = -1);
  LANGID GetLanguageID    () const;
  void   SetCodePage      (uint16_t wCodePage = -1);
  uint16_t   GetCodePage      () const;
  ZString GetStringValue(LPCTSTR pszKey, bool* pbExists = NULL) const;

  // Common String Values
  ZString GetCompanyName     (bool* pbExists = NULL) const;
  ZString GetFileDescription (bool* pbExists = NULL) const;
  ZString GetFileVersion     (bool* pbExists = NULL) const;
  ZString GetInternalName    (bool* pbExists = NULL) const;
  ZString GetLegalCopyright  (bool* pbExists = NULL) const;
  ZString GetOriginalFilename(bool* pbExists = NULL) const;
  ZString GetProductName     (bool* pbExists = NULL) const;
  ZString GetProductVersion  (bool* pbExists = NULL) const;
  ZString GetSpecialBuild    (bool* pbExists = NULL) const;
  ZString GetOLESelfRegister (bool* pbExists = NULL) const;

  // Raw Data Access
  const uint8_t*             GetVerInfo    () const;
  UINT                    GetVerInfoSize() const;
  const VS_FIXEDFILEINFO* GetFixed      () const;

// Data Members
protected:
  uint8_t*             m_pVerInfo;
  UINT              m_cbVerInfo;
  VS_FIXEDFILEINFO* m_pFixed;
  LANGID            m_wLangID;
  uint16_t              m_wCodePage;
};


/////////////////////////////////////////////////////////////////////////////
// Attributes


/////////////////////////////////////////////////////////////////////////////
// Versions

inline ZString ZVersionInfo::GetFileVersionString() const
{
  assert(m_pFixed);
  TCHAR szText[32];
  _stprintf_s(szText, 32, TEXT("%hu.%02hu.%02hu.%04hu"),
    GetFileVersionMSHigh(), GetFileVersionMSLow(),
    GetFileVersionLSHigh(), GetFileVersionLSLow());
  return ZString(szText);
}

inline uint16_t ZVersionInfo::GetFileVersionMSHigh() const
{
  assert(m_pFixed);
  return HIWORD(m_pFixed->dwFileVersionMS);
}

inline uint16_t ZVersionInfo::GetFileVersionMSLow() const
{
  assert(m_pFixed);
  return LOWORD(m_pFixed->dwFileVersionMS);
}

inline uint16_t ZVersionInfo::GetFileVersionLSHigh() const
{
  assert(m_pFixed);
  return HIWORD(m_pFixed->dwFileVersionLS);
}

inline uint16_t ZVersionInfo::GetFileVersionLSLow() const
{
  assert(m_pFixed);
  return LOWORD(m_pFixed->dwFileVersionLS);
}

inline uint16_t ZVersionInfo::GetFileBuildNumber() const
{
  return GetFileVersionLSLow();
}

inline ZString ZVersionInfo::GetProductVersionString() const
{
  assert(m_pFixed);
  TCHAR szText[32];
  _stprintf_s(szText, 32, TEXT("%hd.%02hd.%02hd.%04hd"),
    GetProductVersionMSHigh(), GetProductVersionMSLow(),
    GetProductVersionLSHigh(), GetProductVersionLSLow());
  return ZString(szText);
}

inline uint16_t ZVersionInfo::GetProductVersionMSHigh() const
{
  assert(m_pFixed);
  return HIWORD(m_pFixed->dwProductVersionMS);
}

inline uint16_t ZVersionInfo::GetProductVersionMSLow() const
{
  assert(m_pFixed);
  return LOWORD(m_pFixed->dwProductVersionMS);
}

inline uint16_t ZVersionInfo::GetProductVersionLSHigh() const
{
  assert(m_pFixed);
  return HIWORD(m_pFixed->dwProductVersionLS);
}

inline uint16_t ZVersionInfo::GetProductVersionLSLow() const
{
  assert(m_pFixed);
  return LOWORD(m_pFixed->dwProductVersionLS);
}

inline uint16_t ZVersionInfo::GetProductBuildNumber() const
{
  return GetProductVersionLSLow();
}


/////////////////////////////////////////////////////////////////////////////
// File Flags

inline bool ZVersionInfo::IsDebug() const
{
  assert(m_pFixed);
  return !!(VS_FF_DEBUG & m_pFixed->dwFileFlagsMask & m_pFixed->dwFileFlags);
}

inline bool ZVersionInfo::IsInfoInferred() const
{
  assert(m_pFixed);
  return !!(VS_FF_INFOINFERRED & m_pFixed->dwFileFlagsMask & m_pFixed->dwFileFlags);
}

inline bool ZVersionInfo::IsPatched() const
{
  assert(m_pFixed);
  return !!(VS_FF_PATCHED & m_pFixed->dwFileFlagsMask & m_pFixed->dwFileFlags);
}

inline bool ZVersionInfo::IsPreRelease() const
{
  assert(m_pFixed);
  return !!(VS_FF_PRERELEASE & m_pFixed->dwFileFlagsMask & m_pFixed->dwFileFlags);
}

inline bool ZVersionInfo::IsPrivateBuild() const
{
  assert(m_pFixed);
  return !!(VS_FF_PRIVATEBUILD & m_pFixed->dwFileFlagsMask & m_pFixed->dwFileFlags);
}

inline bool ZVersionInfo::IsSpecialBuild() const
{
  assert(m_pFixed);
  return !!(VS_FF_SPECIALBUILD & m_pFixed->dwFileFlagsMask & m_pFixed->dwFileFlags);
}


/////////////////////////////////////////////////////////////////////////////
// Target Operating Systems

inline bool ZVersionInfo::IsTargetUnknown() const
{
  assert(m_pFixed);
  return VOS_UNKNOWN == m_pFixed->dwFileOS;
}

inline bool ZVersionInfo::IsTargetDOS() const
{
  assert(m_pFixed);
  return HIWORD(VOS_DOS) == HIWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetOS216() const
{
  assert(m_pFixed);
  return HIWORD(VOS_OS216) == HIWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetOS232() const
{
  assert(m_pFixed);
  return HIWORD(VOS_OS232) == HIWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetNT() const
{
  assert(m_pFixed);
  return HIWORD(VOS_NT) == HIWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetBase() const
{
  assert(m_pFixed);
  return VOS__BASE == LOWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetWin16() const
{
  assert(m_pFixed);
  return VOS__WINDOWS16 == LOWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetPM16() const
{
  assert(m_pFixed);
  return VOS__PM16 == LOWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetPM32() const
{
  assert(m_pFixed);
  return VOS__PM32 == LOWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetWin32() const
{
  assert(m_pFixed);
  return VOS__WINDOWS32 == LOWORD(m_pFixed->dwFileOS);
}

inline bool ZVersionInfo::IsTargetDOSWin16() const
{
  assert(m_pFixed);
  return VOS_DOS_WINDOWS16 == m_pFixed->dwFileOS;
}

inline bool ZVersionInfo::IsTargetDOSWin32() const
{
  assert(m_pFixed);
  return VOS_DOS_WINDOWS32 == m_pFixed->dwFileOS;
}

inline bool ZVersionInfo::IsTargetOS216PM16() const
{
  assert(m_pFixed);
  return VOS_OS216_PM16 == m_pFixed->dwFileOS;
}

inline bool ZVersionInfo::IsTargetOS232PM32() const
{
  assert(m_pFixed);
  return VOS_OS232_PM32 == m_pFixed->dwFileOS;
}

inline bool ZVersionInfo::IsTargetNTWin32() const
{
  assert(m_pFixed);
  return VOS_NT_WINDOWS32 == m_pFixed->dwFileOS;
}


/////////////////////////////////////////////////////////////////////////////
// File Types

inline bool ZVersionInfo::IsApp() const
{
  assert(m_pFixed);
  return VFT_APP == m_pFixed->dwFileType; 
}

inline bool ZVersionInfo::IsDll() const
{
  assert(m_pFixed);
  return VFT_DLL == m_pFixed->dwFileType; 
}

inline bool ZVersionInfo::IsDriver() const
{
  assert(m_pFixed);
  return VFT_DRV == m_pFixed->dwFileType; 
}

inline bool ZVersionInfo::IsFont() const
{
  assert(m_pFixed);
  return VFT_FONT == m_pFixed->dwFileType;  
}

inline bool ZVersionInfo::IsVxd() const
{
  assert(m_pFixed);
  return VFT_VXD == m_pFixed->dwFileType; 
}

inline bool ZVersionInfo::IsStaticLib() const
{
  assert(m_pFixed);
  return VFT_STATIC_LIB == m_pFixed->dwFileType;  
}


/////////////////////////////////////////////////////////////////////////////
// Driver Types

inline bool ZVersionInfo::IsDriverUnknown() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_UNKNOWN == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverPrinter() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_PRINTER == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverKeyboard() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_KEYBOARD == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverLanguage() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_LANGUAGE == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverDisplay() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_DISPLAY == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverMouse() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_MOUSE == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverNetwork() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_NETWORK == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverSystem() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_SYSTEM == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverInstallable() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_INSTALLABLE == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverSound() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_SOUND == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverComm() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_COMM == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsDriverInputMethod() const
{
  assert(m_pFixed);
  return IsDriver() && VFT2_DRV_INPUTMETHOD == m_pFixed->dwFileSubtype;
}


/////////////////////////////////////////////////////////////////////////////
// Font Types

inline bool ZVersionInfo::IsFontRaster() const
{
  assert(m_pFixed);
  return IsFont() && VFT2_FONT_RASTER == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsFontVector() const
{
  assert(m_pFixed);
  return IsFont() && VFT2_FONT_VECTOR == m_pFixed->dwFileSubtype;
}

inline bool ZVersionInfo::IsFontTrueType() const
{
  assert(m_pFixed);
  return IsFont() && VFT2_FONT_TRUETYPE == m_pFixed->dwFileSubtype;
}


/////////////////////////////////////////////////////////////////////////////
// String Values

inline void ZVersionInfo::SetLanguageID(LANGID wLangID)
{
  m_wLangID = wLangID;
}

inline LANGID ZVersionInfo::GetLanguageID() const
{
  return m_wLangID;
}

inline void ZVersionInfo::SetCodePage(uint16_t wCodePage)
{
  m_wCodePage = wCodePage;
}

inline uint16_t ZVersionInfo::GetCodePage() const
{
  return m_wCodePage;
}


/////////////////////////////////////////////////////////////////////////////
// Common String Values

inline ZString ZVersionInfo::GetCompanyName(bool* pbExists) const
{
  return GetStringValue(TEXT("CompanyName"), pbExists);
}

inline ZString ZVersionInfo::GetFileDescription(bool* pbExists) const
{
  return GetStringValue(TEXT("FileDescription"), pbExists);
}

inline ZString ZVersionInfo::GetFileVersion(bool* pbExists) const
{
  return GetStringValue(TEXT("FileVersion"), pbExists);
}

inline ZString ZVersionInfo::GetInternalName(bool* pbExists) const
{
  return GetStringValue(TEXT("InternalName"), pbExists);
}

inline ZString ZVersionInfo::GetLegalCopyright(bool* pbExists) const
{
  return GetStringValue(TEXT("LegalCopyright"), pbExists);
}

inline ZString ZVersionInfo::GetOriginalFilename(bool* pbExists) const
{
  return GetStringValue(TEXT("OriginalFilename"), pbExists);
}

inline ZString ZVersionInfo::GetProductName(bool* pbExists) const
{
  return GetStringValue(TEXT("ProductName"), pbExists);
}

inline ZString ZVersionInfo::GetProductVersion(bool* pbExists) const
{
  return GetStringValue(TEXT("ProductVersion"), pbExists);
}

inline ZString ZVersionInfo::GetSpecialBuild(bool* pbExists) const
{
  return GetStringValue(TEXT("SpecialBuild"), pbExists);
}

inline ZString ZVersionInfo::GetOLESelfRegister(bool* pbExists) const
{
  return GetStringValue(TEXT("OLESelfRegister"), pbExists);
}


/////////////////////////////////////////////////////////////////////////////
// Raw Data Access

inline const uint8_t* ZVersionInfo::GetVerInfo() const
{
  return m_pVerInfo;
}

inline UINT ZVersionInfo::GetVerInfoSize() const
{
  return m_cbVerInfo;
}

inline const VS_FIXEDFILEINFO* ZVersionInfo::GetFixed() const
{
  return m_pFixed;
}


/////////////////////////////////////////////////////////////////////////////

#endif // !__VersionInfo_h__

