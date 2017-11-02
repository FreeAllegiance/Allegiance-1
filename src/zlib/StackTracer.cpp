#include "StackTracer.h"

#include <tchar.h>

#include "zassert.h"

/*
// -- https://www.codeproject.com/Articles/41923/Get-the-call-stack-when-an-exception-is-being-caug
// Description: This class is used to get the call stack when there is an exception being caught use SEH
//
// Author: Baiyan Huang
// Date: 8/30/2009
//
*/


#include "StackTracer.h"
#include <DbgHelp.h>
#include <sstream>
#pragma comment(lib, "dbghelp.lib")

StackTracer StackTracer::s_StackTracer;

// Generally, call stack with depth of 6 should be enough for our diagnosing.
const int CALLSTACK_DEPTH = 6;			

// Translate exception code to description
#define CODE_DESCR(code) CodeDescMap::value_type(code, #code)

StackTracer::StackTracer(void)
:m_dwExceptionCode(0)
{
	// Get machine type
	m_dwMachineType = 0;

    char* wszProcessor = ::_tgetenv(_T("PROCESSOR_ARCHITECTURE"));
	if (wszProcessor)
	{
		if ((!strcmp(_T("EM64T"), wszProcessor)) ||!strcmp(_T("AMD64"), wszProcessor))
		{
			m_dwMachineType = IMAGE_FILE_MACHINE_AMD64;
		}
		else if (!strcmp(_T("x86"), wszProcessor))
		{
			m_dwMachineType = IMAGE_FILE_MACHINE_I386;
		}
	}

	// Exception code description
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_ACCESS_VIOLATION));       
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_DATATYPE_MISALIGNMENT));  
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_BREAKPOINT));             
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_SINGLE_STEP));            
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_ARRAY_BOUNDS_EXCEEDED));  
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_FLT_DENORMAL_OPERAND));   
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_FLT_DIVIDE_BY_ZERO));     
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_FLT_INEXACT_RESULT));     
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_FLT_INVALID_OPERATION));  
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_FLT_OVERFLOW));           
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_FLT_STACK_CHECK));        
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_FLT_UNDERFLOW));          
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_INT_DIVIDE_BY_ZERO));     
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_INT_OVERFLOW));           
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_PRIV_INSTRUCTION));       
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_IN_PAGE_ERROR));          
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_ILLEGAL_INSTRUCTION));    
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_NONCONTINUABLE_EXCEPTION));			
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_STACK_OVERFLOW));         
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_INVALID_DISPOSITION));    
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_GUARD_PAGE));             
	m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_INVALID_HANDLE));         
	//m_mapCodeDesc.insert(CODE_DESCR(EXCEPTION_POSSIBLE_DEADLOCK));      
	// Any other exception code???
}

StackTracer::~StackTracer(void)
{
}

LONG StackTracer::ExceptionFilter(LPEXCEPTION_POINTERS e)
{
	return s_StackTracer.HandleException(e);
}

void StackTracer::OutputStackTraceToDebugF()
{
	debugf(StackTracer::GetExceptionMsg().c_str());
}


std::string StackTracer::GetExceptionMsg()
{
	std::ostringstream  m_ostringstream;

	// Exception Code
	CodeDescMap::iterator itc = s_StackTracer.m_mapCodeDesc.find(s_StackTracer.m_dwExceptionCode);
	
	if(itc != s_StackTracer.m_mapCodeDesc.end())
		m_ostringstream << "Exception: " << itc->second << "\r\n";
	else
		m_ostringstream << "Unknown Exception...\r\n";

	m_ostringstream<<"------------------------------------------------------------------\r\n";

	// Call Stack
	std::vector<FunctionCall>::iterator itbegin = s_StackTracer.m_vecCallStack.begin();
	std::vector<FunctionCall>::iterator itend = s_StackTracer.m_vecCallStack.end();
	std::vector<FunctionCall>::iterator it;
	for (it = itbegin; it < itend; it++)
	{
		std::string strFunctionName = it->FunctionName.empty() ? "UnkownFunction" : it->FunctionName;
		std::string strFileName = it->FileName.empty() ? "UnkownFile" : it->FileName;

		m_ostringstream << "Function : " << strFunctionName << "\r\n";
		m_ostringstream << "    [Source File : " << strFileName << "]\r\n"; 
		m_ostringstream << "    [Source Line : " << it->LineNumber << "]\r\n"; 
	}

	return m_ostringstream.str();
}

uint32_t StackTracer::GetExceptionCode()
{
	return s_StackTracer.m_dwExceptionCode;
}

std::vector<FunctionCall> StackTracer::GetExceptionCallStack()
{
	return s_StackTracer.m_vecCallStack;
}

LONG __stdcall StackTracer::HandleException(LPEXCEPTION_POINTERS e)
{
	m_dwExceptionCode = e->ExceptionRecord->ExceptionCode;
	m_vecCallStack.clear();

	HANDLE hProcess = INVALID_HANDLE_VALUE;

	// Initializes the symbol handler
	if(!SymInitialize( GetCurrentProcess(), NULL, TRUE ))
	{
		SymCleanup(hProcess);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	// Work through the call stack upwards.
	TraceCallStack( e->ContextRecord);

	// ...
	SymCleanup(hProcess);
	
	return(EXCEPTION_EXECUTE_HANDLER) ;
}

// Work through the stack to get the entire call stack
void StackTracer::TraceCallStack(CONTEXT* pContext)
{
	// Initialize stack frame
	STACKFRAME64 sf;
	memset(&sf, 0, sizeof(STACKFRAME));

#if defined(_WIN64)
	sf.AddrPC.Offset = pContext->Rip;
	sf.AddrStack.Offset = pContext->Rsp;
	sf.AddrFrame.Offset = pContext->Rbp;
#elif defined(WIN32)
	sf.AddrPC.Offset = pContext->Eip;
	sf.AddrStack.Offset = pContext->Esp;
	sf.AddrFrame.Offset = pContext->Ebp;
#endif
	sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrStack.Mode = AddrModeFlat;
	sf.AddrFrame.Mode = AddrModeFlat;

	if (0 == m_dwMachineType)
		return;

	// Walk through the stack frames.
	HANDLE hProcess = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	while(StackWalk64(m_dwMachineType, hProcess, hThread, &sf, pContext, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0))
	{
		if( sf.AddrFrame.Offset == 0 || m_vecCallStack.size() >= CALLSTACK_DEPTH)
			break;

		// 1. Get function name at the address
		const int nBuffSize = (sizeof(SYMBOL_INFO) + MAX_SYM_NAME*sizeof(TCHAR) + sizeof(ULONG64) - 1) / sizeof(ULONG64);
		ULONG64 symbolBuffer[nBuffSize];
		PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)symbolBuffer;

		pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		pSymbol->MaxNameLen = MAX_SYM_NAME;

		FunctionCall curCall = {"", "", 0};

		DWORD64 dwSymDisplacement = 0;
		if(SymFromAddr( hProcess, sf.AddrPC.Offset, &dwSymDisplacement, pSymbol ))
		{
			curCall.FunctionName = std::string(pSymbol->Name);
		}

		//2. get line and file name at the address
		IMAGEHLP_LINE64 lineInfo = { sizeof(IMAGEHLP_LINE64) };
		uint32_t dwLineDisplacement = 0;

		if(SymGetLineFromAddr64(hProcess, sf.AddrPC.Offset, LPDWORD(&dwLineDisplacement), &lineInfo ))
		{
			curCall.FileName = std::string(lineInfo.FileName);
			curCall.LineNumber = lineInfo.LineNumber;
		}

		// Call stack stored
		m_vecCallStack.push_back(curCall);

	}
}
