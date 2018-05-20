//////////////////////////////////////////////////////////////////////////////
//
// Asserts
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _assert_h_
#define _assert_h_

#include <memory>
#include <windows.h>

#include "Logger.h"

//////////////////////////////////////////////////////////////////////////////
//
// Make sure we didn't include some other assert mechanism
//
//////////////////////////////////////////////////////////////////////////////

#ifdef assert
#error "Don't use assert.h, zlib provides assert functionality."
#endif

//////////////////////////////////////////////////////////////////////////////
//
// Asserts
//
//////////////////////////////////////////////////////////////////////////////
void ZAssertImpl(bool bSucceeded, const char* psz, const char* pszFile, int line, const char* pszModule);
#define ZRetailAssert(bCond)    ZAssertImpl((bCond) ? true : false, #bCond, __FILE__, __LINE__, __MODULE__)

void retailf(const char* , ...);

extern ILogger* g_pDebugLogger;

#ifdef _DEBUG
    //
    // These are implemented in Win32app.cpp
    //

    inline void ZDebugBreak() { DebugBreak(); }


    // void debugf(const char* , ...);
    void ZWarningImpl(bool bSucceeded, const char* psz, const char* pszFile, int line, const char* pszModule);
    bool ZFailedImpl(HRESULT hr, const char* pszFile, int line, const char* pszModule);
    bool ZSucceededImpl(HRESULT hr, const char* pszFile, int line, const char* pszModule);
    // void ZDebugOutputImpl(const char* psz);

    #define ZAssert(bCond)    ZAssertImpl((bCond) ? true : false, #bCond, __FILE__, __LINE__, __MODULE__)
    #define ZVerify(bCond)    ZAssert(bCond)
    #define ZWarning(bCond)   ZWarningImpl((bCond) ? true : false, #bCond, __FILE__, __LINE__, __MODULE__)
    #define ZError(psz)       ZAssertImpl(false, psz, __FILE__, __LINE__, __MODULE__)
    #define ZBadCase()        ZError("Bad case in switch statement"); break;
    #define ZFailed(hr)       ZFailedImpl(hr, __FILE__, __LINE__, __MODULE__)
    #define ZSucceeded(hr)    ZSucceededImpl(hr, __FILE__, __LINE__, __MODULE__)
    // #define ZDebugOutput(str) ZDebugOutputImpl(str)
#else
    // inline void debugf(...) {}

    #define ZDebugBreak()

    #define ZAssert(bCond)
    #define ZWarning(str)
    #define ZVerify(bCond)    (bCond)
    #define ZError(str)
    #define ZBadCase()        break;
    #define ZFailed(hr)       FAILED(hr)
    #define ZSucceeded(hr)    SUCCEEDED(hr)
    // #define ZDebugOutput(str)

#endif

// mmf pulled debugf, ZDebugOutput, and ZDebugOutputImpl out of the above if else to support logging on SRVLOG
// mmf log to file on SRVLOG define as well as _DEBUG
#ifdef _DEBUG
#define SRVLOG
#endif

#ifdef SRVLOG // mmf changed this from _DEBUG
   void debugf(const char* , ...);
   void ZDebugOutputImpl(const char* psz);
   #define ZDebugOutput(str) ZDebugOutputImpl(str)
#else
   inline void debugf(...) {}
   #define ZDebugOutput(str)
#endif

#define ZUnimplemented()  ZError("Unimplemented member called")

#define assert(exp)   ZAssert(exp)
#define ShouldBe(exp) ZWarning(exp)
#define VerifyHR(exp) ZSucceeded(exp)

#define BreakOnError(expr) if (ZFailed(expr)) break;

//////////////////////////////////////////////////////////////////////////////
//
// Tracing
//
//////////////////////////////////////////////////////////////////////////////

#if (defined(_TRACE) && defined(_DEBUG))
    void ZEnterImpl(const char* pcc);
    void ZExitImpl(const char* pcc);
    void ZTraceImpl(const char* pcc);
    void ZStartTraceImpl(const char* pcc);

    #define ZEnter(str)      ZEnterImpl(str)
    #define ZExit(str)       ZExitImpl(str)
    #define ZTrace(str)      ZTraceImpl(str)
    #define ZStartTrace(str) ZStartTraceImpl(str)
#else
    #define ZEnter(str)       
    #define ZExit(str) 
    #define ZTrace(str)       
    #define ZStartTrace(str)    
#endif

#endif

