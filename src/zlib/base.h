//////////////////////////////////////////////////////////////////////////////
//
// Base
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _base_h_
#define _base_h_

#include <memory>

#include "tref.h"
#include "zstring.h"

////////////////////////////////////////////////////////////////////////////////
//
// File
//
////////////////////////////////////////////////////////////////////////////////
class ZFilePrivate;
class ZFile : public IObject {
protected:
	std::unique_ptr<ZFilePrivate> d;

public:
	ZFile(); // BUILD_DX9: added for DX9 but can stay for DX7 as well

    ZFile(const PathString& strPath, uint32_t how = OF_READ | OF_SHARE_DENY_WRITE);
    virtual ~ZFile();

    virtual bool  IsValid();
    virtual int   GetLength();
    virtual uint8_t * GetPointer(bool bWrite = false, bool bCopyOnWrite = false);
	virtual void SetPointer(uint8_t * ptr);
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

#endif
