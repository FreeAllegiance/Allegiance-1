#include "base.h"
#include "zassert.h"

////////////////////////////////////////////////////////////////////////////////
//
// Helper Functions
//
////////////////////////////////////////////////////////////////////////////////

bool IsWindows9x()
{
    static bool bChecked = false;
    static bool bIs9x;

    if (!bChecked)
    {
        OSVERSIONINFO osversioninfo;

        osversioninfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        ZVerify(GetVersionEx(&osversioninfo));

        bIs9x = osversioninfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;
        bChecked = true;
    }

    return bIs9x;
}


////////////////////////////////////////////////////////////////////////////////
//
// ZFile
//
////////////////////////////////////////////////////////////////////////////////

// BUILD_DX9: added for DX9 but can stay for DX7 as well
ZFile::ZFile( )
{
	m_p = NULL;
	m_handle = INVALID_HANDLE_VALUE;
}


ZFile::ZFile(const PathString& strPath, uint32_t how) : 
    m_p(NULL),
	m_pathString(strPath) // BT - STEAM
{
	// BT - CSS - 12/8/2011 - Fixing 128 character path limit.
	uint32_t dwDesiredAccess = GENERIC_READ;
	uint32_t dwShareMode = FILE_SHARE_WRITE;
	uint32_t dwCreationDisposition = OPEN_EXISTING;

	if((how & OF_WRITE) == OF_WRITE)
		dwDesiredAccess = GENERIC_WRITE;

	if((how & OF_SHARE_DENY_WRITE) == OF_SHARE_DENY_WRITE)
		dwShareMode = FILE_SHARE_READ;

	if((how & OF_CREATE) == OF_CREATE)
		dwCreationDisposition = CREATE_ALWAYS;

	// Unicode markers / wide format enables up to 32K path length. 
	PathString unicodePath("\\\\?\\");

	// If the path is relative, don't use unicode marker.
	if(strPath.Left(1) == ZString(".") || strPath.FindAny("//") == -1)
		unicodePath = strPath;
	else
		unicodePath += strPath;

	WCHAR* pszw = new WCHAR[unicodePath.GetLength() + 1];
    int result = MultiByteToWideChar(CP_ACP, 0, unicodePath, unicodePath.GetLength(), pszw, unicodePath.GetLength());
	pszw[result] = NULL;

	m_handle = CreateFileW(pszw, dwDesiredAccess,  dwShareMode, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);

	delete pszw;
	// BT - End fix.
}

ZFile::~ZFile()
{
    if (m_p) {
        UnmapViewOfFile(m_p);
        CloseHandle(m_hfileMapping);
    }
}

bool ZFile::IsValid()
{
     return m_handle != (HANDLE)HFILE_ERROR;
}

uint32_t ZFile::Read(void* p, uint32_t length)
{
    uint32_t cbActual;

    ReadFile(m_handle, p, length, (LPDWORD)&cbActual, NULL);
    return cbActual;
}

uint32_t ZFile::Write(void* p, uint32_t length)
{
    uint32_t cbActual;

    WriteFile(m_handle, p, length, (LPDWORD)&cbActual, NULL);

    ZAssert(cbActual == length);

    return cbActual;
}

bool ZFile::Write(const ZString& str)
{
    return (Write((void*)(PCC)str, str.GetLength()) != 0);
}

bool  ZFile::WriteString(const ZString& str)
{
    return (Write((void*)(PCC)str, str.GetLength() + 1) != 0);
}

bool  ZFile::WriteAlignedString(const ZString& str)
{
    int length = str.GetLength() + 1;

    if (Write((void*)(PCC)str, str.GetLength() + 1) != 0) {
        return WritePad(length);
    }

    return false;
}

bool ZFile::WritePad(int length)
{
    static uint8_t zeros[] = { 0, 0, 0 };
    int pad = 4 - (length & 3);

    if (pad != 4) {
        return (Write(zeros, pad) != 0);
    }

    return true;
}

bool ZFile::Write(uint32_t value)
{
    return (Write(&value, 4) != 0);
}

bool ZFile::Write(int value)
{
    return (Write(&value, 4) != 0);
}

bool ZFile::Write(float value)
{
    return (Write(&value, 4) != 0);
}

int ZFile::GetLength()
{
    return GetFileSize(m_handle, NULL);
}

uint8_t* ZFile::GetPointer(bool bWrite, bool bCopyOnWrite)
{
    ZAssert(!bWrite || !bCopyOnWrite);

    if (m_p == NULL) {
        m_hfileMapping = 
            CreateFileMapping(
                m_handle,
                0,
                bWrite
                    ? PAGE_READWRITE
                    : PAGE_READONLY,
                0,
                0,
                NULL
            );

        ZAssert(m_hfileMapping != NULL);

        m_p = 
            (uint8_t*)MapViewOfFile(
                m_hfileMapping, 
                bWrite
                    ? FILE_MAP_WRITE
                    : (bCopyOnWrite
                        ? FILE_MAP_COPY
                        : FILE_MAP_READ), 
                0, 
                0, 
                0
            );

        ZAssert(m_p != NULL);
    }

    return m_p;
}

ZWriteFile::ZWriteFile(const PathString& strPath) :
    ZFile(strPath, OF_CREATE | OF_WRITE | OF_SHARE_EXCLUSIVE)
{
}

// KGJV 32B - added Tell and Seek
long   ZFile::Tell()
{
    uint32_t dwPtr = SetFilePointer(m_handle,0,NULL,FILE_CURRENT);
    if (dwPtr != INVALID_SET_FILE_POINTER)
        return (long) dwPtr;
    else
        return -1;
}
int   ZFile::Seek(long offset, int origin)
{
    uint32_t dwPtr = SetFilePointer(m_handle,offset,NULL,origin);
    return (dwPtr != INVALID_SET_FILE_POINTER);
}

// BT - Steam
// https://social.msdn.microsoft.com/Forums/vstudio/en-US/c43c3b36-c698-45da-89f4-ecd0e825c059/implementing-sha1-hash-using-windows-cryptography-api-and-c?forum=vcgeneral
ZString ZFile::GetSha1Hash()
{
	const int bufsize = 1024;
	const int sha1len = 20;

	uint32_t dwStatus = 0;
	BOOL bResult = FALSE;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	uint8_t rgbFile[bufsize];
	uint32_t cbRead = 0;
	uint8_t rgbHash[sha1len];
	uint32_t cbHash = 0;
	CHAR rgbDigits[] = "0123456789abcdef";

	char hexBuffer[3];
	ZString returnValue = "";

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv,
		NULL,
		NULL,
		PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT))
	{
		dwStatus = GetLastError();
		ZDebugOutput(ZString("CryptAcquireContext failed: ") + ZString((int)dwStatus));

		return "0";
	}

	if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
	{
		dwStatus = GetLastError();

		ZDebugOutput(ZString("CryptCreateHash failed: ") + ZString((int)dwStatus));
		CryptReleaseContext(hProv, 0);
		return "0";
	}

	for (cbRead = Read(rgbFile, bufsize); cbRead > 0; cbRead = Read(rgbFile, bufsize))
	{
		if (!CryptHashData(hHash, rgbFile, cbRead, 0))
		{
			dwStatus = GetLastError();

			ZDebugOutput(ZString("CryptHashData failed: ") + ZString((int)dwStatus));

			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			return "0";
		}
	}



	cbHash = sha1len;
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, LPDWORD(&cbHash), 0))
	{
		//ZDebugOutput(ZString("SHA1 hash of file ") + m_pathString.GetFilename() + " is: ");

		for (uint32_t i = 0; i < cbHash; i++)
		{
			sprintf(hexBuffer, "%c%c", rgbDigits[rgbHash[i] >> 4],
				rgbDigits[rgbHash[i] & 0xf]);

			returnValue += hexBuffer;

			//ZDebugOutput(hexBuffer);
		}
		//ZDebugOutput("\n");
	}
	else
	{
		dwStatus = GetLastError();
		ZDebugOutput(ZString("CryptGetHashParam failed: ") + ZString((int)dwStatus));
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	Seek(0, 0);

	return returnValue;
}

// BT - STEAM
// https://stackoverflow.com/questions/27217063/how-to-read-filename-of-the-last-modified-file-in-a-directory
FILETIME ZFile::GetMostRecentFileModificationTime(ZString &searchPath)
{
	WIN32_FIND_DATAW ffd;
	wchar_t currentFile[MAX_PATH], lastModifiedFilename[MAX_PATH];
	FILETIME currentModifiedTime, lastModified;
	HANDLE hFile;
	bool first_file = true;

	char szLocalDate[255], szLocalTime[255];

	currentModifiedTime.dwHighDateTime = 0;
	currentModifiedTime.dwLowDateTime = 0;

	HANDLE hFind;
	WIN32_FIND_DATAA findFileData;

	hFind = FindFirstFileA(searchPath, &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		//still have the default in the main directory
		printf("Invalid handle value (%d)\n", GetLastError());
		return currentModifiedTime;
	}
	do
	{
		if ((findFileData.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			continue;

		if (strlen(findFileData.cFileName) > 0 && findFileData.cFileName[0] == '.')
			continue;

		if (first_file)
		{
			lastModified = findFileData.ftLastWriteTime;
			first_file = false;
		}
		else
		{
			// First file time is earlier than second file time.
			if (CompareFileTime(&lastModified, &findFileData.ftLastWriteTime) == -1)
			{
				/* You can uncomment these if you want to see what the actual times are.
				SYSTEMTIME st;
				FileTimeToLocalFileTime(&lastModified, &lastModified);
				FileTimeToSystemTime(&lastModified, &st);
				GetDateFormatA(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL,
				szLocalDate, 255);
				GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, szLocalTime, 255);
				printf("%s %s - ", szLocalDate, szLocalTime);

				FileTimeToLocalFileTime(&findFileData.ftLastWriteTime, &findFileData.ftLastWriteTime);
				FileTimeToSystemTime(&findFileData.ftLastWriteTime, &st);
				GetDateFormatA(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL,
				szLocalDate, 255);
				GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &st, NULL, szLocalTime, 255);
				printf("%s %s\n", szLocalDate, szLocalTime);
				*/

				lastModified = findFileData.ftLastWriteTime;
			}
		}

	} while (FindNextFileA(hFind, &findFileData));

	FindClose(hFind);


	return lastModified;
}