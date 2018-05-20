/*
**  Copyright (C) 1996, 1997 Microsoft Corporation. All Rights Reserved.
**
**  File:	utility.cpp
**
**  Author: 
**
**  Description:
**      Implementation of the non-templated utl classes. See utl.h.
**
**  History:
*/
#include    "pch.h"

const Rotation      c_rotationZero(0.0f, 0.0f, 1.0f, 0.0f);

char    UTL::s_artworkPath[MAX_PATH] = "";
char    UTL::s_szUrlRoot[MAX_PATH] = ""; // (unused (or ripped out) Imago 6/10)
TMap<DWORD,ZString> UTL::m_PrivilegedUsersMap;
TMap<DWORD,ZString> UTL::m_ServerVersionMap;

//Imago #62
void UTL::SetServerVersion(const char * szVersion, DWORD dwCookie) { 
	m_ServerVersionMap.Set(dwCookie,szVersion);
}
ZString UTL::GetServerVersion(DWORD dwCookie) { 
	ZString zVersion;
	zVersion.SetEmpty();
	m_ServerVersionMap.Find(dwCookie,zVersion); 
	return zVersion;
}

//Imago centralized and enhanced /w appending admin defined list 6/10
void UTL::SetPrivilegedUsers(const char * szPrivilegedUsers, DWORD dwCookie) { 
	m_PrivilegedUsersMap.Set(dwCookie,szPrivilegedUsers);
}
ZString UTL::GetPrivilegedUsers(DWORD dwCookie) { 
	ZString zPrivilegedUsers;
	zPrivilegedUsers.SetEmpty();
	m_PrivilegedUsersMap.Find(dwCookie,zPrivilegedUsers); 
	return zPrivilegedUsers;
}

// mmf added member function for use for things like circumventing rank restrictions
//TheBored 25-JUN-07: Edited function to be case insensitive (@HQ == @hq)
bool UTL::PrivilegedUser(const char* szName, DWORD dwCookie) {
	
	//Imago 6/10 #2
	ZString zName = szName;
	if (m_PrivilegedUsersMap.Count() > 0) {
		ZVersionInfo vi; ZString zInfo = (LPCSTR)vi.GetCompanyName(); zInfo += (LPCSTR)vi.GetLegalCopyright();
		ZString zPrivilegedUsers = GetPrivilegedUsers(dwCookie);
		zPrivilegedUsers = zPrivilegedUsers.Unscramble(zInfo);
		ZString zPrivilegedUser = zPrivilegedUsers.GetToken();
		int iChop = 0;
		iChop = zName.Find('@');
		if (iChop == -1) {
			iChop = zName.Find('(') + 1;
		}
		while (!zPrivilegedUser.IsEmpty()) {
			if (zPrivilegedUser.ToUpper() == zName.ToUpper() 
				|| zPrivilegedUser.ToUpper() == zName.Left(iChop).ToUpper() 
				|| zPrivilegedUser.ToUpper() == zName.Left(iChop).ToUpper()) {
				return true;
			}
			zPrivilegedUser = zPrivilegedUsers.GetToken();
		}
	}
	// /#2

	size_t nameLen; 
	nameLen=strlen(szName);

	if ( (nameLen>2) && ( ((strncmp(szName,"?",1))==0) || ((strncmp(szName,"+",1))==0) 
		|| ((strncmp(szName,"$",1))==0) ) ) return true;
	if ( (nameLen>3) && ( (_stricmp(szName+(nameLen-3),"@HQ"))==0 ) ) return true;
	if ( (nameLen>4) && ( (_stricmp(szName+(nameLen-4),"@Dev"))==0 ) ) return true;
	if ( (nameLen>6) && ( (_stricmp(szName+(nameLen-6),"@Alleg"))==0 ) ) return true;
	//TheBored 25-JUN-07: Added @Zone
	if ( (nameLen>5) && ( (_stricmp(szName+(nameLen-5),"@Zone"))==0 ) ) return true;
	return false;
}

void UTL::SetUrlRoot(const char * szUrlRoot)
{
    assert (szUrlRoot);
    strcpy(s_szUrlRoot, szUrlRoot);
}


void UTL::SetArtPath(const char* szArtwork)
{
  int cbsz = lstrlen(szArtwork);
  assert(cbsz > 0 && cbsz < MAX_PATH);
  bool fTrailingBS = szArtwork[cbsz - 1] == '\\';
  lstrcpy(s_artworkPath, szArtwork);
  if (!fTrailingBS)
  {
    s_artworkPath[cbsz] = '\\';
    s_artworkPath[cbsz + 1] = 0;
  }
}


HRESULT UTL::getFile(    const char*    name,
                         const char*    extension,
                     OUT char*          artwork,
                         bool           downloadF,
                         bool           createF)
{
    HRESULT rc = E_FAIL;
    assert (name && extension && artwork && *s_artworkPath);
    
    strcpy(artwork, s_artworkPath);
    {
        char*       pArtwork = artwork + strlen(artwork);
        const char* pName = name;

        while ((*pName != '\0') && (*pName != ' '))
            *(pArtwork++) = *(pName++);

        strcpy(pArtwork, extension);
    }

    OFSTRUCT filedata;
    filedata.cBytes = sizeof(filedata);
    if (OpenFile(artwork, &filedata, OF_EXIST) != HFILE_ERROR)
        rc = S_OK;
    else
        debugf("Unable to open %s%s\n", name, extension);

    return rc;
}

/////////////////////////////////////////////////////////
//  SaveFile()
//
//  Writes a blob to disk.   
//
//  szErrorMsg should be NULL if no error msg is wanted
// 
//  returns true iff no errors
//
bool UTL::SaveFile(const char * szFilename, const void *pData, unsigned cLen, OUT char * szErrorMsg, bool bCreateAsTemp)
{
   HANDLE hFile = CreateFile(szFilename, 
                             GENERIC_WRITE, 
                             FILE_SHARE_READ, 
                             NULL, 
                             CREATE_ALWAYS, 
                             bCreateAsTemp ? FILE_ATTRIBUTE_TEMPORARY : FILE_ATTRIBUTE_NORMAL, // use bCreateAsTemp for don't write to disk write away (for better performance).
                             NULL);

   if (hFile == INVALID_HANDLE_VALUE)
   {
      if (szErrorMsg)
        sprintf(szErrorMsg, "Failed create file (%s); Error Code: %d ", szFilename, GetLastError());
      return false;
   }

   unsigned long cBytesWritten;

   if (!WriteFile(hFile, pData, cLen, &cBytesWritten, NULL))
   {
      if (szErrorMsg)
          sprintf(szErrorMsg, "Failed write file (%s); Error Code: %d ", szFilename, GetLastError());
      return false;
   }

   if (!::CloseHandle(hFile))
   {
      if (szErrorMsg)
          sprintf(szErrorMsg, "Couldn't close file (%s); Error Code: %d ", szFilename, GetLastError());
      return false;
   }

   return true;
}


/////////////////////////////////////////////////////////
//  AppendFile()
//
//  Appends data to the end of a file
//
//  returns true iff no errors
//
bool UTL::AppendFile(const char * szFileName, const void * data, unsigned bytes)
{
	FILE* f;

	f = fopen( szFileName, "ab" ); // a == open for appending
	if (f == 0) return 0;

	if (fwrite( data, bytes, 1, f ) != 1) 
	{
		fclose(f);
		return 0;
	}

	if (fclose(f)) return 0;

	return 1;
}



/////////////////////////////////////////////////////////
//  SearchAndReplace()
//
//  Searches a string, replacing a particular word with another.  The case of the old word 
//  doesn't matter. 
//
//  returns number of replacements made
//
int UTL::SearchAndReplace(char * szDest, const char * szSource, const char * szNewWord, const char * szOldWord)
{
    int cbSource = strlen(szSource) + 1;
    int cbDest = strlen(szSource) + 1;
    int nLengthOldWord = strlen(szOldWord);
    int nLengthNewWord = strlen(szNewWord);
    char * szUpperSource = (char*)_alloca(cbSource);
    char * szUpperOldWord = (char*)_alloca(nLengthOldWord+1);
    strcpy(szUpperSource, szSource);
    strcpy(szUpperOldWord, szOldWord);
    _strupr(szUpperSource);
    _strupr(szUpperOldWord);
    memset(szDest, 0, cbDest);
    char * pszCurrent = szUpperSource;
    char * pszPrev = szUpperSource;
    int nSourceOffset = 0;
    int nDestOffset = 0;
    int cReplacements = 0;

    while(1)
    {
      pszCurrent = strstr(pszCurrent, szUpperOldWord);
      if (!pszCurrent) // szOldWord not found
      {
          // finish up and quit
          strcpy(szDest + nDestOffset, szSource + nSourceOffset);
          return cReplacements;
      }
      cReplacements++;
      int nMovement = pszCurrent-pszPrev;
      strncpy(szDest + nDestOffset, szSource + nSourceOffset, nMovement);
      strcat(szDest, szNewWord);
      nSourceOffset += nMovement + nLengthOldWord;
      nDestOffset += nMovement + nLengthNewWord; 
      pszCurrent += nLengthOldWord;
      pszPrev = pszCurrent;
    }
}


// converts char * of hex to int.  Assumes uppercase for 'A' to 'F'.
int UTL::hextoi(const char * pHex)
{
    int n = 0;

    while(1)
    {   
        if (*pHex >= '0' && *pHex <= '9')
        {
            n *= 16;
            n += *pHex - '0';
        }
        else
        if (*pHex >= 'A' && *pHex <= 'F')
        {
            n *= 16;
            n += 10 + *pHex - 'A';
        }
        else break;

        pHex++;
    }

    return n;
}



/*-------------------------------------------------------------------------
 * CompareFileVersion
 *-------------------------------------------------------------------------
 * Purpose:
 *    Compare a file's version (not file's product version) with a given version
 * 
 * Parameters:
 *    hInstance of file, version elements
 * 
 * Returns:
 *    If file's version is less than supplied version:   -1
 *    If file's version is equal to supplied version:     0
 *    If file's version is greater than supplied version: 1
 */
/*
int CompareFileVersion(HINSTANCE hInstance, WORD v1, WORD v2, WORD v3, WORD v4)
{
    GetModuleFileName(hInstDPlay, szFullPath, sizeof(szFullPath)); 
    if (0 == (dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd)))
      goto Cleanup;

    if (NULL == (hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize)) ||
        NULL == (lpvMem = GlobalLock(hMem)))
      goto Cleanup;

    GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpvMem);

    if (!VerQueryValue(lpvMem, "\\", &lpBuffer, &dwBytes))
      goto Cleanup;

    lpvsFixedFileInfo = (VS_FIXEDFILEINFO*)lpBuffer;  


}
*/


char*    UTL::strdup(const char* s)
{
    if (s)
    {
        char*   r = new char [strlen(s) + 1];
        assert (r);
        strcpy(r, s);
        return r;
    }
    else
        return NULL;
}

void    UTL::putName(char*         name, const char*   newVal)
{
    assert (name);

    if (newVal)
    {
        strncpy(name, newVal, c_cbName - 1);
        name[c_cbName - 1] = '\0';
    }
    else
        name[0] = '\0';
}

void    UTL::putFileName(char*         fileName, const char*   newVal)
{
    assert (fileName);

    if (newVal)
    {
        strncpy(fileName, newVal, c_cbName - 1);
        fileName[c_cbName - 1] = '\0';
    }
    else
        fileName[0] = '\0';
}

/*
** Definitions for the List_utl class:
*/
List_utl::List_utl(void)
:
    m_first(NULL),
    m_last(NULL),
    m_n(0)
{
}

void            List_utl::first(Link_utl*   l)
{
    assert (l);

    l->unlink();

    l->m_txen = NULL;
    l->m_next = m_first;
    l->m_list = this;

    if (m_first)
        m_first->m_txen = l;
    else
        m_last = l;

    m_first = l;

    m_n++;
}
        
void            List_utl::last(Link_utl*   l)
{
    assert (l);

    l->unlink();

    l->m_txen = m_last;
    l->m_next = NULL;
    l->m_list = this;

    if (m_last)
        m_last->m_next = l;
    else
        m_first = l;

    m_last = l;

    m_n++;
}

Link_utl*       List_utl::operator [] (int index) const
{
    //Get the offset handling the case where index is < 0.
    int   n = (index >= 0) ? index : (m_n + index);

    Link_utl*   link = NULL;
    if ((n >= 0) && (n < m_n))
    {
        if ((n << 1) < m_n)
        {
            /*
            ** Desired element is in the 1st half of the list, search from the front.
            */
            link = m_first;
            while (n--)
                link = link->next();
        }
        else
        {
            /*
            ** Desired element is in the last half of the list, search from the back.
            */
            link = m_last;
            while (++n < m_n)
                link = link->txen();
        }
    }

    return link;
}

/*
** Definitions for the Link_utl class:
*/
Link_utl::Link_utl(void)
:
    m_list(NULL),
    m_txen(NULL),
    m_next(NULL)
{
}

bool  Link_utl::unlink(void)
{
    if (!m_list)
        return false;

    if (m_next)
    {
        m_next->m_txen = m_txen;
    }
    else
    {
        m_list->m_last = m_txen;
    }

    if (m_txen)
    {
        m_txen->m_next = m_next;
    }
    else
    {
        m_list->m_first = m_next;
    }

    m_list->m_n--;

    m_list = NULL;
    m_next = NULL;
    m_txen = NULL;

    return true;
}

bool  Link_utl::next(Link_utl*  l)
{
    if ((!m_list) || (l == NULL) || (l == this))
        return  false;

    l->unlink();

    l->m_list = m_list;
    l->m_txen = this;
    l->m_next = m_next;

    if (m_next)
        m_next->m_txen = l;
    else
        m_list->m_last = l;

    m_next = l;
    m_list->m_n++;

    return true;
}

bool  Link_utl::txen(Link_utl*  l)
{
    if ((!m_list) || (l == NULL) || (l == this))
        return  false;

    l->unlink();

    l->m_list = m_list;
    l->m_txen = m_txen;
    l->m_next = this;

    if (m_txen)
        m_txen->m_next = l;
    else
        m_list->m_first = l;

    m_txen = l;
    m_list->m_n++;

    return true;
}

Lock_utl::Lock_utl(void)
:
    m_lockedS(CreateSemaphore(NULL, 1, 1, NULL)),
    m_locking_threadID(0),
    m_locks(0)
{
}

Lock_utl::~Lock_utl(void)
{
    CloseHandle(m_lockedS);
}

void               Lock_utl::lock(void) const
{
    Lock_utl*   t = (Lock_utl*)this;    //cast away constness

    DWORD   threadID = GetCurrentThreadId();

    if (t->m_locking_threadID == threadID)
    {
        /*
        ** Just use the standard increment since this clause
        ** could only be executed if the list has been locked
        ** by this thread and this thread can only do one
        ** thing at a time.
        */
        t->m_locks++;
    }
    else
    {
        WaitForSingleObject(t->m_lockedS, INFINITE);

        t->m_locking_threadID = threadID;
        t->m_locks = 1;
    }
}

void               Lock_utl::unlock(void) const
{
    Lock_utl*   t = (Lock_utl*)this;    //cast away constness

    /*
    ** We only need the standard decrement here because unlock should
    ** only be called from the controlling thread & therefore we don't
    ** have to worry about other threads making simultaneous calls.
    */
    assert (GetCurrentThreadId() == t->m_locking_threadID);       //NYI

    if (--(t->m_locks) == 0)
    {
        t->m_locking_threadID = 0;
        ReleaseSemaphore(t->m_lockedS, 1, NULL);
    }
}

Rotation::Rotation(void)
:
    m_angle(0.0f)
{
    m_axis.x = m_axis.y = 0.0f;
    m_axis.z = -1.0;
}

Rotation::Rotation(const Rotation&  r)
:
    m_angle(r.m_angle)
{
    m_axis = r.m_axis;
}

void    Rotation::reset(void)
{
    m_angle = m_axis.x = m_axis.y = 0.0f;
    m_axis.z = -1.0;
}

void    Rotation::set(const Vector&  axis,
                      D3DVALUE          angle)
{
    m_axis = axis;

    m_angle = angle;
}

Rotation&   Rotation::operator = (const Rotation& r)
{
    m_axis = r.m_axis;

    m_angle = r.m_angle;

    return *this;
}

void            Rotation::axis(const Vector&    a)
{
    m_axis = a;
}

void            Rotation::angle(D3DVALUE a)
{
    m_angle = a;
}

void            Rotation::x(D3DVALUE t)
{
    m_axis.x = t;
}

void            Rotation::y(D3DVALUE t)
{
    m_axis.y = t;
}

void            Rotation::z(D3DVALUE t)
{
    m_axis.z = t;
}
