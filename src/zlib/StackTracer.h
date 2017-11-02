/*
// -- https://www.codeproject.com/Articles/41923/Get-the-call-stack-when-an-exception-is-being-caug
// Description: This class is used to get the call stack when there is an exception being caught use SEH
//
// Author: Baiyan Huang
// Date: 8/30/2009
//
// Usage:
//		__try
//		{
//			// main functions...
//		}
//		__except(StackTracer::ExceptionFilter(GetExceptionInformation()))
//		{
//			// Your code to show or log the exception msg...
//		}
*/

#pragma once

#include <map>
#include <vector>
#include <Windows.h>

struct FunctionCall 
{
	std::string FunctionName;
	std::string FileName;
	int			LineNumber;
};

class StackTracer
{
public:
	// Always return EXCEPTION_EXECUTE_HANDLER after getting the call stack
	static LONG ExceptionFilter(LPEXCEPTION_POINTERS e);

	static void OutputStackTraceToDebugF();

	// return the exception message along with call stacks
	static std::string GetExceptionMsg();

	// Return exception code and call stack data structure so that 
	// user could customize their own message format
	static uint32_t GetExceptionCode();
	static std::vector<FunctionCall> GetExceptionCallStack();

private:
	static StackTracer s_StackTracer;

private:
	StackTracer(void);
	~StackTracer(void);

	// The main function to handle exception
	LONG __stdcall HandleException(LPEXCEPTION_POINTERS e);

	// Work through the stack upwards to get the entire call stack
	void TraceCallStack(CONTEXT* pContext);

private:
	uint32_t m_dwExceptionCode;

	std::vector<FunctionCall> m_vecCallStack;

	typedef std::map<uint32_t,const char*> CodeDescMap;
	CodeDescMap m_mapCodeDesc;

	uint32_t m_dwMachineType; // Machine type matters when trace the call stack (StackWalk64)

};
