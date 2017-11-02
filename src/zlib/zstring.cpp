#include "zstring.h"
#include "zassert.h"

//////////////////////////////////////////////////////////////////////////////
//
// ZString::ZStringData
//
//////////////////////////////////////////////////////////////////////////////

ZString::ZStringData::ZStringData(PCC pcc)
{
    m_length = pcc ? strlen(pcc) : 0;
    m_pch    = new char[m_length + 1]; //Fix memory leak -Imago 8/2/09
    m_pcc    = m_pch;
    memcpy(m_pch, pcc ? pcc : "", m_length + 1);
}

ZString::ZStringData::ZStringData(PCC pcc, bool bStatic)
{
    ZAssert(bStatic);

    m_length = strlen(pcc);
    m_pch    = NULL;
    m_pcc    = pcc;
}

ZString::ZStringData::ZStringData(PCC pcc, int length)
{
    m_length = length;
    m_pch    = new char[m_length + 1];
    m_pcc    = m_pch;
    memcpy(m_pch, pcc, m_length);
    m_pch[m_length] = 0;
}

ZString::ZStringData::ZStringData(char ch, int length)
{
    m_length = length;
    m_pch    = new char[m_length + 1];
    m_pcc    = m_pch;
    memset(m_pch, ch, m_length);
    m_pch[m_length] = 0;
}

ZString::ZStringData::ZStringData(const ZStringData* pdata1, const ZStringData* pdata2) :
    m_length(pdata1->GetLength() + pdata2->GetLength())
{
    m_pch = new char[m_length + 1];
    m_pcc = m_pch;
    memcpy(m_pch, pdata1->m_pcc, pdata1->GetLength());
    memcpy(m_pch + pdata1->m_length, pdata2->m_pcc, pdata2->GetLength() + 1);
}

ZString::ZStringData::~ZStringData()
{
    if (m_pch) {
        delete[] m_pch;
    }
}

void ZString::ZStringData::Set(int index, char ch)
{
    ZAssert(
           m_pch != NULL
        && index < m_length
        && GetCount() == 1
    );

    m_pch[index] = ch;
}

int ZString::ZStringData::Find(const ZStringData* pdata) const
{
	int index2;
    for(int index = 0; index < m_length - pdata->m_length + 1; index++) {
        for (index2 = 0; index2 < pdata->m_length; index2++) {
            if (m_pcc[index + index2] != pdata->m_pcc[index2]) {
                break;
            }
        }
        if (index2 == pdata->m_length) {
            return index;
        }
    }

    return -1;
}

int ZString::ZStringData::ReverseFind(const ZStringData* pdata) const
{
	int index2;
    for(int index = m_length - pdata->m_length + 1; index >=0 ; index--) {
        for (index2 = 0; index2 < pdata->m_length; index2++) {
            if (m_pcc[index + index2] != pdata->m_pcc[index2]) {
                break;
            }
        }
        if (index2 == pdata->m_length) {
            return index;
        }
    }

    return -1;
}

int ZString::ZStringData::Find(char ch, int index) const
{
    for(; index < m_length; index++) {
        if (m_pcc[index] == ch) {
            return index;
        }
    }

    return -1;
}

int ZString::ZStringData::ReverseFind(char ch, int index) const
{
    index = m_length - index;

    for(; index >= 0; index--) {
        if (m_pcc[index] == ch) {
            return index;
        }
    }

    return -1;
}

int ZString::ZStringData::FindAny(const ZStringData* pdata, int index) const
{
    int length = pdata->GetLength();

    for(; index < m_length; index++) {
        for (int index2 = 0; index2 < length; index2++) {
            if (m_pcc[index] == pdata->m_pcc[index2]) {
                return index;
            }
        }
    }

    return -1;
}

int ZString::ZStringData::ReverseFindAny(const ZStringData* pdata, int index) const
{
    int length = pdata->GetLength();

    index = m_length - index;

    for(; index >= 0; index--) {
        for (int index2 = 0; index2 < length; index2++) {
            if (m_pcc[index] == pdata->m_pcc[index2]) {
                return index;
            }
        }
    }

    return -1;
}

/*
BSTR ZString::ZStringData::MakeBSTR() const
{
    if (m_length != 0) {
        WCHAR* pszw = new WCHAR[m_length];
        MultiByteToWideChar(CP_ACP, 0, m_pcc, -1, pszw, m_length);
        BSTR bstr = SysAllocStringLen(pszw, m_length);
        delete[] pszw;
        return bstr;
    } else {
        return SysAllocStringLen(L"", 0);
    }
}
*/

//////////////////////////////////////////////////////////////////////////////
//
// ZString Constructors
//
//////////////////////////////////////////////////////////////////////////////

TRef<ZString::ZStringData> ZString::s_pemptyData = new ZString::ZStringData("", true);

ZString::ZString() :
    m_pdata(s_pemptyData)
{
}

ZString::ZString(PCC pcc) :
    m_pdata(new ZStringData(pcc)) //Fix memory leak -Imago 8/2/09
{
}

ZString::ZString(PCC pcc, bool bStatic) :
    m_pdata(new ZStringData(pcc, bStatic))
{
}

ZString::ZString(PCC pcc, int length)
{
    if (length == 0) {
        m_pdata = s_pemptyData;
    } else {
        m_pdata = new ZStringData(pcc, length);
    }
}

ZString::ZString(char ch, int length)
{
    if (length == 0) {
        m_pdata = s_pemptyData;
    } else {
        m_pdata = new ZStringData(ch, length);
    }
}

ZString::ZString(PCC pcc, int length, bool bStatic)
{
    if (length == 0) {
        m_pdata = s_pemptyData;
    } else {
        m_pdata = new ZStringData(pcc, length);
    }
}

ZString::ZString(const ZString& str) :
    m_pdata(str.m_pdata)
{
}

ZString::ZString(ZStringData* pdata) :
    m_pdata(pdata)
{
}

ZString::ZString(int value)
{
    char buf[32];

    _itoa_s(value,buf,32,10);

    m_pdata = new ZStringData(buf);
}

ZString::ZString(float value, int total, int precision)
{
    char format[32];
    char buf[32];

    sprintf_s(format, 32, "%%%d.%df", total, precision);
    sprintf_s(buf, 32, format, value);

    m_pdata = new ZStringData(buf);
}

ZString::ZString(float value)
{
        char buf[32];
        sprintf_s(buf, 32, "%g", value);
        m_pdata = new ZStringData(buf);
}

ZString::ZString(bool b)
{
    if (b) {
        m_pdata = new ZStringData("true");
    } else {
        m_pdata = new ZStringData("false");
    }
}

ZString operator+(const ZString& str1, const ZString& str2)
{
    return ZString(new ZString::ZStringData(str1.m_pdata, str2.m_pdata));
}

ZString ZString::GetProfileString(const ZString& strSection, const ZString& strKey)
{
    char buf[256];

    uint32_t dw = ::GetProfileString(strSection, strKey, "", buf, 256);

    return ZString(buf, (int)dw);
}

//////////////////////////////////////////////////////////////////////////////
//
// ZString Operations
//
//////////////////////////////////////////////////////////////////////////////

void ZString::UniqueData()
{
    if (m_pdata->GetCount() != 1) {
        m_pdata = new ZStringData(m_pdata->GetPointer());
    }
}

void ZString::SetEmpty()
{
    m_pdata = s_pemptyData;
}

void ZString::Set(int index, char ch)
{
    if (index < GetLength()) {
        if (m_pdata->GetPointer()[index] != ch) {
            UniqueData();
            m_pdata->Set(index, ch);
        }
    }
}

ZString& ZString::operator+=(const ZString& str)
{
    m_pdata = new ZStringData(m_pdata, str.m_pdata);

    return *this;
}

ZString& ZString::operator=(const ZString& str)
{
    m_pdata = str.m_pdata;

    return *this;
}

void ZString::ReplaceAll(const ZString& str, char ch)
{
    UniqueData();

    int length = str.GetLength();

    for(int index = 0; index < GetLength(); index++) {
        for (int index2 = 0; index2 < length; index2++) {
            if (m_pdata->GetPointer()[index] == str[index2]) {
                m_pdata->Set(index, ch);
                break;
            }
        }
    }
}

void ZString::RemoveAll(char chToRemove)
{
    int length = GetLength();
    char* cbNew = (char*)_alloca(length + 1);
    int nLengthNew = 0;

    for (int indexOld = 0; indexOld < GetLength(); indexOld++) {
        char chCurrent = m_pdata->GetPointer()[indexOld];

        if (chCurrent != chToRemove) {
            cbNew[nLengthNew] = chCurrent;
            ++nLengthNew;            
        }
    }
    cbNew[nLengthNew] = '\0';

    m_pdata = new ZStringData(cbNew, nLengthNew);
}

//////////////////////////////////////////////////////////////////////////////
//
// ZString Accessors
//
//////////////////////////////////////////////////////////////////////////////

bool operator==(const ZString& str1, const ZString& str2)
{
    return
           str1.GetLength() == str2.GetLength()
        && (strncmp(str1, str2, str1.GetLength()) == 0);
}

bool operator==(const ZString& str, PCC pcc)
{
    ZAssert(pcc != NULL);

    return
           str.GetLength() == (int)strlen(pcc)
        && (strncmp(str, pcc, str.GetLength()) == 0);
}

bool operator==(PCC pcc, const ZString& str)
{
    ZAssert(pcc != NULL);

    return
           str.GetLength() == (int)strlen(pcc)
        && (strncmp(str, pcc, str.GetLength()) == 0);
}

bool operator!=(const ZString& str1, const ZString& str2)
{
    return
           str1.GetLength() != str2.GetLength()
        || (strncmp(str1, str2, str1.GetLength()) != 0);
}

bool operator!=(const ZString& str, PCC pcc)
{
    ZAssert(pcc != NULL);

    return
           str.GetLength() != (int)strlen(pcc)
        || (strncmp(str, pcc, str.GetLength()) != 0);
}

bool operator!=(PCC pcc, const ZString& str)
{
    ZAssert(pcc != NULL);

    return 
           str.GetLength() != (int)strlen(pcc)
        || (strncmp(str, pcc, str.GetLength()) != 0);
}

bool operator<(const ZString& str1, const ZString& str2)
{
    return strcmp(str1, str2) < 0;
}

bool operator<(const ZString& str, PCC pcc)
{
    return strcmp(str, pcc) < 0;
}

bool operator<(PCC pcc, const ZString& str)
{
    return strcmp(pcc, str) < 0;
}

/*
BSTR ZString::MakeBSTR() const 
{ 
    return m_pdata->MakeBSTR(); 
}
*/

//////////////////////////////////////////////////////////////////////////////
//
// Find functions
//
//////////////////////////////////////////////////////////////////////////////

int ZString::Find(const ZString& str) const
{
    return m_pdata->Find(str.m_pdata);
}

int ZString::ReverseFind(const ZString& str) const
{
    return m_pdata->Find(str.m_pdata);
}

int ZString::Find(char ch, int index) const
{
    return m_pdata->Find(ch, index);
}

int ZString::ReverseFind(char ch, int index) const
{
    return m_pdata->ReverseFind(ch, index);
}

int ZString::FindAny(const ZString& str, int index) const
{
    return m_pdata->FindAny(str.m_pdata, index);
}

int ZString::ReverseFindAny(const ZString& str, int index) const
{
    return m_pdata->ReverseFindAny(str.m_pdata, index);
}

//////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
//////////////////////////////////////////////////////////////////////////////

int ZString::GetInteger() const
{
    return atoi(m_pdata->GetPointer());
}

ZString ZString::Left(int index) const
{
    return ZString(m_pdata->GetPointer(), index);
}

ZString ZString::Right(int index) const
{
    return ZString(m_pdata->GetPointer() + GetLength() - index, index);
}

ZString ZString::Middle(int index, int length) const
{
    return ZString(m_pdata->GetPointer() + index, length);
}

ZString ZString::LeftOf(int index) const
{
    return Left(GetLength() - index);
}

ZString ZString::RightOf(int index) const
{
    int size = GetLength() - index;
    if (size > 0) {
        return Right(size);
    } else {
        return ZString();
    }
}

ZString ZString::LeftOf(const ZString& str) const
{
    int index = FindAny(str);

    if (index == -1) {
        return str;
    } else {
        return Left(index);
    }
}

ZString ZString::RightOf(const ZString& str) const
{
    int index = FindAny(str);

    if (index == -1) {
        return ZString();
    } else {
        return Right(GetLength() - index - 1);
    }
}

ZString ZString::ToLower() const
{
    char* cbResult = (char*)_alloca(GetLength() + 1);

    ZToLower(cbResult, (*this));

    return cbResult;
}

ZString ZString::ToUpper() const
{
    char* cbResult = (char*)_alloca(GetLength() + 1);

    ZToUpper(cbResult, (*this));

    return cbResult;
}

void ZToLower(char* szDest, PCC szSource)
{
	int index;
    for (index = 0; szSource[index] != '\0'; ++index) {
        char ch = szSource[index];
        if (ch >= 'A' && ch <= 'Z')
            szDest[index] = (ch - ('A' - 'a'));
        else
            szDest[index] = ch;
    }
    szDest[index] = '\0';
}

void ZToUpper(char* szDest, PCC szSource)
{
	int index;
    for (index = 0; szSource[index] != '\0'; ++index) {
        char ch = szSource[index];
        if (ch >= 'a' && ch <= 'z')
            szDest[index] = (ch - ('a' - 'A'));
        else
            szDest[index] = ch;
    }
    szDest[index] = '\0';
}

//////////////////////////////////////////////////////////////////////////////
//
// Command line
//
//////////////////////////////////////////////////////////////////////////////

ZString ZString::GetToken()
{
    ZString strToken;
    int length = GetLength();
    int index;

    if (m_pdata->GetPointer()[0] == '"')
    {
        index = 1;

        while (index < length && m_pdata->GetPointer()[index] != '"') 
            index++;

        if (index < length)
            strToken = Middle(1, index++ - 1);
        else
            strToken = *this;
    }
    else
    {
        index = 0;

        while (index < length && m_pdata->GetPointer()[index] != ' ')
            index++;

        strToken = Left(index);
    }

    while (index < length && m_pdata->GetPointer()[index] == ' ')
        index++;

    *this = Right(length - index);

    return strToken;
}

//////////////////////////////////////////////////////////////////////////////
//
// FilenameString
//
// foo.ext
//
//////////////////////////////////////////////////////////////////////////////

ZString FilenameString::GetExtension() const
{
    return RightOf(".");
}

ZString FilenameString::GetName() const
{
    int index = Find('.', 0);

    if (index == -1) {
        return *this;
    } else {
        return Left(index);
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// PathString
//
// foo
// ./foo
// ../foo
// /foo
//
// d:
// d:foo
// d:/foo
// //server/share
//
//  , current code doesn't handle c:\ correctly
//
//////////////////////////////////////////////////////////////////////////////


const char* g_szDirSep = "/";


PathString::PathString(PCC pcc) :
    ZString(pcc)
{
    //  : make sure str is valid

	ReplaceAll("\\", '/');
}

PathString PathString::GetCurrentDirectory()
{
    int size = ::GetCurrentDirectory(0, NULL);
    char* pch = new char[size];
    ::GetCurrentDirectory(size, pch);
    PathString str(pch);
    delete pch;

    return str;
}

PathString PathString::GetModulePath()
{
    char ch[128];
    GetModuleFileNameA(NULL, ch, sizeof(ch) / sizeof(*ch));
    return PathString(ch);
}

FilenameString PathString::GetFilename() const
{
    int index = ReverseFindAny("/\\:");

    if (index == -1) {
        return *this;
    } else {
        return Right(GetLength() - index - 1);
    }
}

ZString PathString::GetExtension() const
{
    return GetFilename().GetExtension();
}

ZString PathString::GetName() const
{
    return GetFilename().GetName();
}

PathString PathString::GetDirectory() const
{
    int index = ReverseFindAny("/\\:");

    if (index == -1) {
        return PathString();
    } else {
        return Left(index);
    }
}

PathString PathString::GetDrive() const
{
    char ch0 = (*this)[0];
    char ch1 = (*this)[1];

    if (   (ch0 ==  '/' && ch1 ==  '/')
        || (ch0 == '\\' && ch1 == '\\')
    ) {
        // network path

        return Left(FindAny("/\\", FindAny("/\\", 2) + 1) - 1);
    } else if (ch1 == ':') {
        // drive letter

        return Left(2);
    }

    return PathString();
}

PathString PathString::operator+(const PathString& strRelativeArg) const
{
    PathString strAbsolute = *this;
    ZString strRelative = strRelativeArg;

    int length = strRelative.GetLength();
    char ch0 = strRelative[0];
    char ch1 = length > 0 ? strRelative[1] : 0;
    char ch2 = length > 1 ? strRelative[2] : 0;

    if (ch0 == '/' || ch0 == '\\') {
        if (ch1 == ch0) {
            // absolute network path

            return strRelative;
        } else {
            // Just use the drive from strAbsolute;

            return ZString(strAbsolute.GetDrive()) + g_szDirSep + strRelative;
        }
    } else if (ch1 == ':') {
        // The relative path has a drive letter

        if (ch2 == '/' || ch2 == '\\') {
            return strRelative;
        } else {
            ZError("Current directory relative paths not supported.");
            return strAbsolute;
        }
    } else {
        // tack the relative path onto the end of the absolute path

        while (true) {
            ch0 = strRelative[0];

            if (ch0 == '.') {
                int length = strRelative.GetLength();

                if (strRelative[1] == '.') {
                    strAbsolute = strAbsolute.GetDirectory();
                    strRelative = strRelative.Right(strRelative.GetLength() - 2);
                } else {
                    strRelative = strRelative.Right(strRelative.GetLength() - 1);
                }
            } else {
                int index = strRelative.FindAny("/\\");

                if (index == -1) {
                    index = strRelative.GetLength();
                }

                char chLast = strAbsolute[strAbsolute.GetLength() - 1];

                if (chLast == '/' || chLast == '\\') {
                    strAbsolute = ZString(strAbsolute) + strRelative.Left(index);
                } else {
                    strAbsolute = ZString(strAbsolute) + g_szDirSep + strRelative.Left(index);
                }

                strRelative = strRelative.Right(strRelative.GetLength() - index);
            }

            ch0 = strRelative[0];

            if (ch0 == '/' || ch0 == '\\') {
                strRelative = strRelative.Right(strRelative.GetLength() - 1);
            } else {
                return strAbsolute;
            }
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// Encryption Stuff
//
//////////////////////////////////////////////////////////////////////////////

static const unsigned char vcScramble[256] = 
{
      0,  26,  45,  66, 155,  83, 137, 133, 
     71, 158,   4, 128, 124, 106,  59,  32, 
    244,   2, 100, 193,  42,  25, 228,  76, 
    110, 116,  72, 163, 214,  97,  61,  11, 
     75, 225,  33,  10,  48,  60,  14,   8, 
    101,  67, 179,   7,  82,  69, 165, 152, 
    222,   6,  44,  23,  35, 246, 180, 120, 
    129, 248, 105,  55,  24, 173, 112, 234, 
      9, 171,  99,  27,  28,  54,  22, 135, 
    102,  51, 113,  18,   3,  29,  80, 191, 
    169, 107,  46,  73,  31, 218, 177,  34, 
    176, 186,  93,  36, 153, 147, 178, 160, 
    148,  43, 182,  84, 154, 141, 164,  77, 
    233, 136,  12, 103, 156,  50,  58, 223, 
    118, 209,  13, 119, 204, 188,  52,  41, 
     21, 189, 241, 151, 192, 172, 104,  57, 
     38, 131,   5,  91, 181,  64, 219,  39, 
    205, 130,  63,  94, 109,  81, 254, 255, 
    174, 220, 142, 161, 162, 132, 211,  49, 
    202, 249, 123, 114,  95,  78, 140,  74, 
     86, 115, 251,  15,   1,  65,  62, 231, 
    125, 194, 201,  40, 143,  30, 159,  96, 
    145, 245, 200,  56,  17, 199, 183,  16, 
    134, 229, 121,  87, 149, 235, 122,  92, 
     37, 166, 157, 215, 144, 213, 126,  89, 
    187,  53, 232, 212,  19,  20, 175, 210, 
    207, 168, 150, 203, 216, 197, 252,  68, 
    247,  70, 117, 138, 146, 224, 139, 190, 
    253,  90, 185, 238, 111, 196, 206, 167, 
    108,  85, 226, 221, 208, 243, 170, 195, 
    236, 240, 242,  79, 184, 237, 239,  47, 
    217, 250, 230,  98,  88, 198, 127, 227
};

static const unsigned char vcUnscramble[256] = 
{
      0, 164,  17,  76,  10, 130,  49,  43, 
     39,  64,  35,  31, 106, 114,  38, 163, 
    183, 180,  75, 204, 205, 120,  70,  51, 
     60,  21,   1,  67,  68,  77, 173,  84, 
     15,  34,  87,  52,  91, 192, 128, 135, 
    171, 119,  20,  97,  50,   2,  82, 247, 
     36, 151, 109,  73, 118, 201,  69,  59, 
    179, 127, 110,  14,  37,  30, 166, 138, 
    133, 165,   3,  41, 215,  45, 217,   8, 
     26,  83, 159,  32,  23, 103, 157, 243, 
     78, 141,  44,   5,  99, 233, 160, 187, 
    252, 199, 225, 131, 191,  90, 139, 156, 
    175,  29, 251,  66,  18,  40,  72, 107, 
    126,  58,  13,  81, 232, 140,  24, 228, 
     62,  74, 155, 161,  25, 218, 112, 115, 
     55, 186, 190, 154,  12, 168, 198, 254, 
     11,  56, 137, 129, 149,   7, 184,  71, 
    105,   6, 219, 222, 158, 101, 146, 172, 
    196, 176, 220,  93,  96, 188, 210, 123, 
     47,  92, 100,   4, 108, 194,   9, 174, 
     95, 147, 148,  27, 102,  46, 193, 231, 
    209,  80, 238,  65, 125,  61, 144, 206, 
     88,  86,  94,  42,  54, 132,  98, 182, 
    244, 226,  89, 200, 117, 121, 223,  79, 
    124,  19, 169, 239, 229, 213, 253, 181, 
    178, 170, 152, 211, 116, 136, 230, 208, 
    236, 113, 207, 150, 203, 197,  28, 195, 
    212, 248,  85, 134, 145, 235,  48, 111, 
    221,  33, 234, 255,  22, 185, 250, 167, 
    202, 104,  63, 189, 240, 245, 227, 246, 
    241, 122, 242, 237,  16, 177,  53, 216, 
     57, 153, 249, 162, 214, 224, 142, 143
};

static unsigned char ScrambleMunge(unsigned char cbStart, unsigned char cbKey)
{
    unsigned char c = cbStart;
    
    ZAssert(c != 0);
    ZAssert(cbKey != 0);

    // (note: beware XOR, since that could generate a NULL character which 
    // would truncate the string). 

    // do an add mod 255 + 1
    //c = (unsigned char)((short(c) + short(cbKey) - 2) % 255 + 1);
    int n = (int(c) + int(cbKey));
    c = (unsigned char)(n - ((n <= 256) ? 1 : 256));

    // do a substitution
    c = vcScramble[c];

    // do a rotation  
    c = (c << (cbKey & 7)) | (c >> (8 - (cbKey & 7)));

    ZAssert(c != 0);

    return c;
}

static unsigned char ScrambleUnmunge(unsigned char cbStart, unsigned char cbKey)
{
    unsigned char c = cbStart;

    ZAssert(c != 0);
    ZAssert(cbKey > 0);

    // (note: beware XOR, since that could generate a NULL character which 
    // would truncate the string). 

    // undo the rotation  
    c = (c >> (cbKey & 7)) | (c << (8 - (cbKey & 7)));

    // undo the substitution
    c = vcUnscramble[c];

    // undo the add mod 255 + 1
    //c = (unsigned char)((short(c) - short(cbKey) + 255) % 255 + 1);
    int n = (int(c) - int(cbKey));
    c = (unsigned char)(n + ((n >= 0) ? 1 : 256));

    ZAssert(c != 0);

    return c;
}

const int c_cScrambleRounds = 12;

void ZScramble(char* szDest, PCC szSource, PCC szKey)
{
    int length = strlen(szSource);
    int lengthKey = strlen(szKey);

    memcpy(szDest, szSource, length + 1);

    for (int nRound = 0; nRound < c_cScrambleRounds; nRound++)
    {
        int index;

        // do a pass to unscramble it by the key
        for (index = 0; index < length; index++) 
            szDest[index] = (char)(ScrambleMunge((unsigned char)szDest[index], 
                (unsigned char)szKey[(index + 3*nRound) % lengthKey]));

        // cypher block chaining forwards
        for (index = 1; index < length; index++) 
            szDest[index] = (char)(ScrambleMunge((unsigned char)szDest[index], 
                (unsigned char)szDest[index-1]));
        
        // cypher block chaining backwards
        for (index = length - 2; index >= 0; index--) 
            szDest[index] = (char)(ScrambleMunge((unsigned char)szDest[index], 
                (unsigned char)szDest[index+1]));
    }
}

void ZUnscramble(char* szDest, PCC szSource, PCC szKey)
{
    int length = strlen(szSource);
    int lengthKey = strlen(szKey);

    memcpy(szDest, szSource, length + 1);

    for (int nRound = c_cScrambleRounds - 1; nRound >= 0; nRound--)
    {
        int index;

        // undo the cypher block chaining backwards
        for (index = 0; index < length - 1; index++) 
            szDest[index] = (char)(ScrambleUnmunge((unsigned char)szDest[index], 
                (unsigned char)szDest[index+1]));

        // undo the cypher block chaining forwards
        for (index = length - 1; index > 0; index--) 
            szDest[index] = (char)(ScrambleUnmunge((unsigned char)szDest[index], 
                (unsigned char)szDest[index-1]));

        // do a pass to unscramble it by the key
        for (index = 0; index < length; index++) 
            szDest[index] = (char)(ScrambleUnmunge((unsigned char)szDest[index], 
                (unsigned char)szKey[(index + 3*nRound) % lengthKey]));
    }
}

ZString ZString::Scramble(const ZString& strKey) const
{
    char* cbResult = (char*)_alloca(GetLength() + 1);

    ZScramble(cbResult, (*this), strKey);

    return cbResult;
}

ZString ZString::Unscramble(const ZString& strKey) const
{
    char* cbResult = (char*)_alloca(GetLength() + 1);

    ZUnscramble(cbResult, (*this), strKey);

    return cbResult;
}
