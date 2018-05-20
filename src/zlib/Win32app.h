//////////////////////////////////////////////////////////////////////////////
//
// Win32 Application
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Win32App_h_
#define _Win32App_h_

#include <windows.h>

#include "tref.h"

std::string GetExecutablePath();
void GlobalConfigureLoggers(bool bLogToOutput, bool bLogToFile);

class ExceptionData {
public:
    EXCEPTION_RECORD *ExceptionRecord;
    CONTEXT          *ContextRecord;
};
class ZString;
class Win32App : public IObject {
protected:
    __declspec(dllexport) Win32App();

public:
    __declspec(dllexport) virtual ~Win32App();

    static  void    Exit(int value);

    virtual HRESULT Initialize(const ZString& strCommandLine);
    virtual void    Terminate();
    virtual int     OnException(uint32_t code, EXCEPTION_POINTERS* pdata);

    virtual void    DebugOutput(const char *psz);
    virtual bool    OnAssert(const char* psz, const char* pszFile, int line, const char* pszModule);
    virtual void    OnAssertBreak();

	// KGJV - added for DX9 behavior
	virtual bool    IsBuildDX9();

	//imago 6/10
	virtual bool EnforceFilter( bool bEnforce );
	virtual bool WriteMemory( uint8_t* pTarget, const uint8_t* pSource, uint32_t Size );
	static int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers);
	static LONG __stdcall ExceptionHandler( EXCEPTION_POINTERS* pep );
};

#endif
