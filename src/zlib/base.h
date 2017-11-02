//////////////////////////////////////////////////////////////////////////////
//
// Base
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _base_h_
#define _base_h_

#include <Windows.h>

#include "tref.h"
#include "zstring.h"

////////////////////////////////////////////////////////////////////////////////
//
// Helper Functions
//
////////////////////////////////////////////////////////////////////////////////

bool IsWindows9x();


////////////////////////////////////////////////////////////////////////////////
//
// Waitable Object
//
////////////////////////////////////////////////////////////////////////////////

class WaitableObject : public IObject {
protected:
    HANDLE m_handle;

public:
    WaitableObject(HANDLE handle = NULL) :
        m_handle(handle)
    {}

    virtual ~WaitableObject()
    {
        CloseHandle(m_handle);
    }

    uint32_t Wait(uint32_t dwTimeout = -1)
    {
        return WaitForSingleObject(m_handle, dwTimeout);
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// File
//
////////////////////////////////////////////////////////////////////////////////

class ZFile : public WaitableObject {
private:
	PathString m_pathString; // BT - STEAM

protected:

    uint8_t* m_p;
    HANDLE m_hfileMapping;

public:
	ZFile(); // BUILD_DX9: added for DX9 but can stay for DX7 as well

    ZFile(const PathString& strPath, uint32_t how = OF_READ | OF_SHARE_DENY_WRITE);
    virtual ~ZFile();

    virtual bool  IsValid();
    virtual int   GetLength();
    virtual uint8_t * GetPointer(bool bWrite = false, bool bCopyOnWrite = false);

    virtual uint32_t Read(void* p, uint32_t length);
    uint32_t Write(void* p, uint32_t length);

    bool  Write(uint32_t value);
    bool  Write(int   value);
    bool  Write(float value);
    bool  Write(const ZString& str);
    bool  WriteString(const ZString& str);
    bool  WriteAlignedString(const ZString& str);
    bool  WritePad(int length);
//KGJV 32B - added Tell and Seek
    long  Tell();
    int   Seek(long offset, int origin);

    template<class Type>
    bool WriteStructure(const Type& value) 
    {
        return Write(&value, sizeof(Type));
    }

	// BT - STEAM
	ZString GetSha1Hash();
	static FILETIME GetMostRecentFileModificationTime(ZString &searchPath);
};

class ZWriteFile : public ZFile {
public:
    ZWriteFile(const PathString& strPath);
};

////////////////////////////////////////////////////////////////////////////////
//
// Event
//
////////////////////////////////////////////////////////////////////////////////

class Event : public WaitableObject {
public:
    Event() : WaitableObject(CreateEvent(NULL, FALSE, FALSE, NULL))
    {}

    void Set()   { SetEvent(m_handle); }
    void Reset() { ResetEvent(m_handle); }
    void Pulse() { PulseEvent(m_handle); }
};

#endif
