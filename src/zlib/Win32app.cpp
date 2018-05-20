#include "Win32app.h"
#include "regkey.h"
#include "SlmVer.h"

//Imago 7/10
#include <dbghelp.h>
#include <crtdbg.h>

#include "zstring.h"
#include "VersionInfo.h"
#include <ctime>
#include "zmath.h"
#include "window.h"

#include "Logger.h"

#ifndef NO_STEAM
	#include "steam_api.h"	
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Some assertion functions
//
//////////////////////////////////////////////////////////////////////////////

Win32App *g_papp;

	// Patch for SetUnhandledExceptionFilter 
const uint8_t PatchBytes[5] = { 0x33, 0xC0, 0xC2, 0x04, 0x00 };

	// Original bytes at the beginning of SetUnhandledExceptionFilter 
uint8_t OriginalBytes[5] = {0};

int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    BOOL bMiniDumpSuccessful;
    char szPathName[MAX_PATH] = "";
    GetModuleFileNameA(nullptr, szPathName, MAX_PATH);
    const char* p1 = strrchr(szPathName, '\\');
    char* p = strrchr(szPathName, '\\');
    if (!p)
        p = szPathName;
    else
        p++;
    if (!p1)
        p1 = "mini";
    else
        p1++;
    ZString zApp = p1;
    uint32_t dwBufferSize = MAX_PATH;
    HANDLE hDumpFile;
    SYSTEMTIME stLocalTime;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;

    GetLocalTime(&stLocalTime);

    snprintf(p, szPathName + sizeof(szPathName) - p, "%s", (PCC)zApp);
    ZVersionInfo vi; ZString zInfo = (LPCSTR)vi.GetFileVersionString();

    char* offsetString = p + zApp.GetLength();
    snprintf(offsetString, szPathName + sizeof(szPathName) - offsetString, "-%s-%04d%02d%02d%02d%02d%02d-%ld-%ld.dmp", (PCC)zInfo,
        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
        GetCurrentProcessId(), GetCurrentThreadId());

    hDumpFile = CreateFileA(szPathName, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, CREATE_ALWAYS, 0, nullptr);

    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;

    MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)
        (MiniDumpWithDataSegs |
            MiniDumpWithHandleData |
            MiniDumpWithThreadInfo |
            MiniDumpWithUnloadedModules |
            MiniDumpWithProcessThreadData);

    bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
        hDumpFile, mdt, &ExpParam, nullptr, nullptr);
#ifndef NO_STEAM
    // BT - STEAM
    SteamAPI_SetMiniDumpComment(p);

    // The 0 here is a build ID, we don't set it
    SteamAPI_WriteMiniDump(0, pExceptionPointers, int(rup)); // Now including build and release number in steam errors.
#endif
    return EXCEPTION_EXECUTE_HANDLER;
}

//Imago 6/10
int Win32App::GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    return ::GenerateDump(pExceptionPointers);
}


//////////////////////////////////////////////////////////////////////////////
//
// Some assertion functions
//
//////////////////////////////////////////////////////////////////////////////
void ZAssertImpl(bool bSucceeded, const char* psz, const char* pszFile, int line, const char* pszModule)
{
    if (!bSucceeded) {
        //
        // Just in case this was a Win32 error get the last error
        //

        uint32_t dwError = GetLastError();
#ifdef _MSC_VER
        if (!g_papp) {
			// Imago removed asm (x64) on ?/?, integrated with mini dump on 6/10
			__try {
				(*(int*)nullptr) = 0;
			}
			__except(GenerateDump(GetExceptionInformation())) {}
        } else if (g_papp->OnAssert(psz, pszFile, line, pszModule)) {
            g_papp->OnAssertBreak();
        }
#else
        ::abort();
#endif
    }
}

std::string GetExecutablePath() {
    char    pCharModulePath[MAX_PATH];
    GetModuleFileName(nullptr, pCharModulePath, MAX_PATH);

    std::string pathFull = std::string(pCharModulePath);

    std::string pathDirectory = pathFull.substr(0, pathFull.find_last_of("\\", pathFull.size()));

    return pathDirectory;
}

std::shared_ptr<SettableLogger> g_pDebugFileLogger = std::make_shared<SettableLogger>(std::make_shared<FileLogger>(GetExecutablePath() + "/debug.log", false));
std::shared_ptr<SettableLogger> g_pDebugOutputLogger = std::make_shared<SettableLogger>(std::make_shared<OutputLogger>());

ILogger* g_pDebugLogger = new MultiLogger(std::vector<std::shared_ptr<ILogger>>({
    g_pDebugOutputLogger,
    g_pDebugFileLogger
}));

void ZDebugOutputImpl(const char *psz)
{
    g_pDebugLogger->Log(std::string(psz));
}

void retailf(const char* format, ...)
{
    const size_t size = 2048; //Avalance: Changed to log longer messages. (From 512)
    char         bfr[size];

    va_list vl;
    va_start(vl, format);
    _vsnprintf_s(bfr, size, (size - 1), format, vl); //Avalanche: Fix off by one error. 
    va_end(vl);

    ZDebugOutputImpl(bfr);
}

// mmf log to file on SRVLOG define as well as _DEBUG
#ifdef _DEBUG
#define SRVLOG
#endif

#ifdef SRVLOG // mmf changed this from _DEBUG

    void ZWarningImpl(bool bSucceeded, const char* psz, const char* pszFile, int line, const char* pszModule)
    {
        if (!bSucceeded) {
            debugf("%s(%d) : ShouldBe failed: '%s'\n", pszFile, line, psz);
        }
    }

    bool ZFailedImpl(HRESULT hr, const char* pszFile, int line, const char* pszModule)
    {
        bool bFailed = FAILED(hr);
        ZAssertImpl(!bFailed, "Function Failed", pszFile, line, pszModule);
        return bFailed;
    }

    bool ZSucceededImpl(HRESULT hr, const char* pszFile, int line, const char* pszModule)
    {
        bool bSucceeded = SUCCEEDED(hr);
        ZAssertImpl(bSucceeded, "Function Failed", pszFile, line, pszModule);
        return bSucceeded;
    }


    #ifdef _TRACE
        bool    g_bEnableTrace = false;
        ZString g_strSpaces;
        int g_indent = 0;
        int g_line   = 0;

        void SetStrSpaces()
        {
            g_strSpaces = 
                  " "
                + ZString((float)g_indent, 2, 0) 
                + ZString(' ', g_indent * 2 + 1);
        }

        void ZTraceImpl(const char* pcc)
        {
            if (g_bEnableTrace) {
                ZDebugOutput(ZString((float)g_line, 4, 0) + g_strSpaces + ZString(pcc) + "\n");
            }
            g_line++;
        }

        void ZEnterImpl(const char* pcc) 
        {
            ZTraceImpl("enter " + ZString(pcc));
            g_indent += 1;
            SetStrSpaces();
        }

        void ZExitImpl(const char* pcc)
        {
            g_indent -= 1;
            SetStrSpaces();
            ZTraceImpl("exit   " + ZString(pcc));
        }

        void ZStartTraceImpl(const char* pcc)
        {
            g_indent = 0;
            g_line   = 0;
            SetStrSpaces();
            ZTraceImpl(pcc);
        }
    #endif

    void debugf(const char* format, ...)
    {
        const size_t size = 2048; //Avalanche: Changed to handle longer messages (from 512)
        char         bfr[size];

        va_list vl;
        va_start(vl, format);
        _vsnprintf_s(bfr, size, (size - 1), format, vl); //Avalanche: Fix off by one error. 
        va_end(vl);

        ZDebugOutputImpl(bfr);
    }

    void GlobalConfigureLoggers(bool bLogToOutput, bool bLogToFile)
    {
        g_pDebugLogger->Log("Changing logging method based on configuration");

        //on startup this is logging to a generic logfile
        if (bLogToFile) {
            g_pDebugFileLogger->Log("Stopping file log, logging continued in timestamped log file");
            g_pDebugFileLogger->SetLogger(
                CreateTimestampedFileLogger(GetExecutablePath() + "/debug_")
            );
        }
        else {
            g_pDebugFileLogger->Log("Stopping file log.");
            g_pDebugFileLogger->SetLogger(
                NullLogger::GetInstance()
            );
        }

        //this is enabled on startup
        if (bLogToOutput == false) {
            g_pDebugFileLogger->Log("Stopping output log");
            g_pDebugOutputLogger->SetLogger(NullLogger::GetInstance());
        }

        g_pDebugLogger->Log("Logging configuration completed");
    }
#endif  // SRVLOG or _DEBUG

//////////////////////////////////////////////////////////////////////////////
//
// Win32 Application
//
//////////////////////////////////////////////////////////////////////////////

Win32App::Win32App()
{
    g_papp = this;
}

Win32App::~Win32App() 
{
}

HRESULT Win32App::Initialize(const ZString& strCommandLine)
{
    return S_OK;
}

void Win32App::Terminate()
{
}

void Win32App::Exit(int value)
{
    _CrtSetDbgFlag(0);
    _exit(value);
}

int Win32App::OnException(uint32_t code, EXCEPTION_POINTERS* pdata)
{
			GenerateDump(pdata);
	return EXCEPTION_CONTINUE_SEARCH;
}
//Imago 6/10
LONG __stdcall Win32App::ExceptionHandler( EXCEPTION_POINTERS* pep ) 
{
			GenerateDump(pep);
	
	return EXCEPTION_EXECUTE_HANDLER; 
}



#ifdef MemoryOutput
    TList<ZString> g_listOutput;
#endif

void Win32App::DebugOutput(const char *psz)
{
    #ifdef MemoryOutput
        g_listOutput.PushFront(ZString(psz));

        if (g_listOutput.GetCount() > 100) {
            g_listOutput.PopEnd();
        }
    #else
        g_pDebugLogger->Log(psz);
    #endif
}

bool Win32App::OnAssert(const char* psz, const char* pszFile, int line, const char* pszModule)
{
    ZDebugOutput(
          ZString("assertion failed: '")
        + psz
        + "' ("
        + pszFile
        + ":"
        + ZString(line)
        + ")\n"
    );

    //return _CrtDbgReport(_CRT_ASSERT, pszFile, line, pszModule, psz) == 1;
    return true;
}

void Win32App::OnAssertBreak()
{
    #ifdef MemoryOutput
        ZString str;

        TList<ZString>::Iterator iter(g_listOutput);

        while (!iter.End()) {
            str = iter.Value() + str;
            iter.Next();
        }
    #endif
    
    //
    // Cause an exception
    //
	// Imago integrated with mini dump on 6/10
#ifdef _MSC_VER
	__try {
        (*(int*)nullptr) = 0;
    }
	__except(GenerateDump(GetExceptionInformation())) {}
#else
    ::abort();
#endif
}

// KGJV - added for DX9 behavior - default is false. override in parent to change this
bool Win32App::IsBuildDX9()
{
	return false;
}


bool Win32App::WriteMemory( uint8_t* pTarget, const uint8_t* pSource, uint32_t Size )
{
	uint32_t ErrCode = 0;


	// Check parameters 

	if( pTarget == nullptr )
	{
		_ASSERTE( !_T("Target address is null.") );
		return false;
	}

	if( pSource == nullptr )
	{
		_ASSERTE( !_T("Source address is null.") );
		return false;
	}

	if( Size == 0 )
	{
		_ASSERTE( !_T("Source size is null.") );
		return false;
	}

	if( IsBadReadPtr( pSource, Size ) )
	{
		_ASSERTE( !_T("Source is unreadable.") );
		return false;
	}


	// Modify protection attributes of the target memory page 

	uint32_t OldProtect = 0;

	if( !VirtualProtect( pTarget, Size, PAGE_EXECUTE_READWRITE, LPDWORD(&OldProtect) ) )
	{
		ErrCode = GetLastError();
		_ASSERTE( !_T("VirtualProtect() failed.") );
		return false;
	}


	// Write memory 

	memcpy( pTarget, pSource, Size );


	// Restore memory protection attributes of the target memory page 

	uint32_t Temp = 0;

	if( !VirtualProtect( pTarget, Size, OldProtect, LPDWORD(&Temp) ) )
	{
		ErrCode = GetLastError();
		_ASSERTE( !_T("VirtualProtect() failed.") );
		return false;
	}


	// Success 

	return true;

}




bool Win32App::EnforceFilter( bool bEnforce )
{
	uint32_t ErrCode = 0;

	
	// Obtain the address of SetUnhandledExceptionFilter 

	HMODULE hLib = GetModuleHandle( _T("kernel32.dll") );

	if( hLib == nullptr )
	{
		ErrCode = GetLastError();
		_ASSERTE( !_T("GetModuleHandle(kernel32.dll) failed.") );
		return false;
	}

	uint8_t* pTarget = (uint8_t*)GetProcAddress( hLib, "SetUnhandledExceptionFilter" );

	if( pTarget == nullptr )
	{
		ErrCode = GetLastError();
		_ASSERTE( !_T("GetProcAddress(SetUnhandledExceptionFilter) failed.") );
		return false;
	}

	if( IsBadReadPtr( pTarget, sizeof(OriginalBytes) ) )
	{
		_ASSERTE( !_T("Target is unreadable.") );
		return false;
	}


	if( bEnforce )
	{
		// Save the original contents of SetUnhandledExceptionFilter 

		memcpy( OriginalBytes, pTarget, sizeof(OriginalBytes) );


		// Patch SetUnhandledExceptionFilter 

		if( !WriteMemory( pTarget, PatchBytes, sizeof(PatchBytes) ) )
			return false;

	}
	else
	{
		// Restore the original behavior of SetUnhandledExceptionFilter 

		if( !WriteMemory( pTarget, OriginalBytes, sizeof(OriginalBytes) ) )
			return false;

	}


	// Success 

	return true;

}

//////////////////////////////////////////////////////////////////////////////
//
// Win Main
//
//////////////////////////////////////////////////////////////////////////////

__declspec(dllexport) int WINAPI Win32Main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr;

    // seed the random number generator with the current time
    // (GetTickCount may be semi-predictable on server startup, so we add the 
    // clock time to shake things up a bit)
    srand(GetTickCount() + (int)time(nullptr));

	// mmf why is this done?
    // shift the stack locals and the heap by a random amount.            
    char* pzSpacer = new char[4 * (int)random(21, 256)];
    pzSpacer[0] = *(char*)_alloca(4 * (int)random(1, 256));

	
	//Imago 6/10 // BT - STEAM - Replacing this with steam logging.
	/*SetUnhandledExceptionFilter(Win32App::ExceptionHandler); 
	g_papp->EnforceFilter( true );

    __try { */

        do {
            BreakOnError(hr = Window::StaticInitialize());

// BUILD_DX9 - KGJV use runtime dynamic instead at preprocessor level
			if (g_papp->IsBuildDX9())
			{
				BreakOnError(hr = g_papp->Initialize(lpszCmdLine));
			}
			else
			{
				// Don't throw an error, if the user selects cancel it can return E_FAIL.
				hr = g_papp->Initialize(lpszCmdLine);
			}
// BUILD_DX9

            //
            // Win32App::Initialize() return S_FALSE if this is a command line app and
            // we shouldn't run the message loop
            //

            if (SUCCEEDED(hr) && S_FALSE != hr) {
                Window::MessageLoop();
            }

            g_papp->Terminate();
            Window::StaticTerminate();

			
     } while (false);

	 // BT - STEAM - Replacing this with steam logging.
	 /*   }  __except (g_papp->OnException(_exception_code(), (EXCEPTION_POINTERS*)_exception_info())){
    }  
    delete pzSpacer;
	if( !g_papp->EnforceFilter( false ) )
	{
		debugf("EnforceFilter(false) failed.\n");
		return 0;
	}*/
    return 0;
}
