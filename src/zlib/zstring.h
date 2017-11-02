
#ifndef _ZString_H_
#define _ZString_H_

#include "tref.h"

typedef const char* PCC;

//////////////////////////////////////////////////////////////////////////////
//
// ZString
//
//////////////////////////////////////////////////////////////////////////////

class StringMDLType;

class ZString {
private:
    friend class StringMDLType;
    class ZStringData : public IObjectSingle {
    protected:
        PCC   m_pcc;
        char* m_pch;
        int   m_length;

    public:
        ZStringData(PCC pch);
        ZStringData(PCC pch, bool bStatic);
        ZStringData(PCC pch, int length);
        ZStringData(char ch, int length);
        ZStringData(const ZStringData* pdata1, const ZStringData* pdata2);
        ~ZStringData();

        void Set(int index, char ch);
        PCC  GetPointer() { return m_pcc; }

        int GetLength() const
        {
            return m_length;
        }

        int        Find(const ZStringData* pdata) const;
        int ReverseFind(const ZStringData* pdata) const;

        int        Find(char ch, int index) const;
        int ReverseFind(char ch, int index) const;

        int        FindAny(const ZStringData* pdata, int index) const;
        int ReverseFindAny(const ZStringData* pdata, int index) const;

        //BSTR MakeBSTR() const;
    };

    static TRef<ZStringData> s_pemptyData;

    TRef<ZStringData> m_pdata;

    void UniqueData();

    ZString(ZStringData*);

public:
    ZString();
    ZString(PCC);
    ZString(PCC, bool bStatic);
    ZString(PCC, int length);
    ZString(PCC, int length, bool bStatic);
    ZString(char ch, int length);
    ZString(const ZString&);
    ZString(int);
    ZString(float value);
    ZString(float value, int total, int precision);
    ZString(bool);

    static ZString GetProfileString(const ZString& strSection, const ZString& strKey);

    const char& operator[](int index) const { return m_pdata->GetPointer()[index]; }
    int GetLength() const { return m_pdata->GetLength(); }
    operator PCC() const { return m_pdata->GetPointer(); }
    bool IsEmpty() const { return GetLength() == 0; }
    BSTR MakeBSTR() const;

    int Find(char ch, int index = 0) const;
    int Find(const ZString& str) const;
    int ReverseFind(char ch, int index = 0) const;
    int ReverseFind(const ZString& str) const;
    int FindAny(const ZString& str, int index = 0) const;
    int ReverseFindAny(const ZString& str, int index = 0) const;

    int GetInteger() const;

    ZString ToLower() const;
    ZString ToUpper() const;

    ZString Scramble(const ZString& strKey) const;
    ZString Unscramble(const ZString& strKey) const;

    ZString Left(int index) const;
    ZString Right(int index) const;
    ZString Middle(int index, int length) const;

    ZString LeftOf(int index) const;
    ZString RightOf(int index) const;

    ZString LeftOf(const ZString& str) const;
    ZString RightOf(const ZString& str) const;

    friend ZString operator+(const ZString&, const ZString&);

    friend bool operator==(const ZString&, const ZString&);
    friend bool operator==(const ZString&, PCC);
    friend bool operator==(           PCC, const ZString&);
    friend bool operator!=(const ZString&, const ZString&);
    friend bool operator!=(const ZString&, PCC);
    friend bool operator!=(           PCC, const ZString&);
    friend bool operator<(const ZString&, const ZString&);
    friend bool operator<(const ZString&, PCC);
    friend bool operator<(           PCC, const ZString&);

    // define the other comparison operators in terms of less than.
    inline friend bool operator>(const ZString& str1, const ZString& str2)   { return str2 < str1; }
    inline friend bool operator>(const ZString& str, PCC pcc)    { return pcc < str; }
    inline friend bool operator>(           PCC pcc, const ZString& str)    { return str < pcc; }
    inline friend bool operator<=(const ZString& str1, const ZString& str2)     { return !(str2 < str1); }
    inline friend bool operator<=(const ZString& str, PCC pcc)   { return !(pcc < str); }
    inline friend bool operator<=(           PCC pcc, const ZString& str)     { return !(str < pcc); }
    inline friend bool operator>=(const ZString& str1, const ZString& str2)     { return !(str1 < str2); }
    inline friend bool operator>=(const ZString& str, PCC pcc)     { return !(str < pcc); }
    inline friend bool operator>=(           PCC pcc, const ZString& str)     { return !(pcc < str); }

    void ReplaceAll(const ZString& str, char ch);
    void RemoveAll(char chToRemove);
    ZString& operator=(const ZString&);
    ZString& operator+=(const ZString&);
    void Set(int index, char ch);
    void SetEmpty();
    ZString GetToken();
};

//////////////////////////////////////////////////////////////////////////////
//
// FilenameString
//
//////////////////////////////////////////////////////////////////////////////

class FilenameString;
class PathString;
class URLString;

class FilenameString : public ZString {
protected:
    FilenameString(const ZString& str) : ZString(str)
    {
    }

    friend PathString;

public:
    FilenameString() : ZString() {}

    ZString GetExtension() const;
    ZString GetName() const;
};

//////////////////////////////////////////////////////////////////////////////
//
// PathString
//
//////////////////////////////////////////////////////////////////////////////

class PathString : public ZString {
public:
    PathString() : ZString() {}
    PathString(PCC pcc);
    PathString(const ZString& str) : ZString(str) {}

    static PathString GetCurrentDirectory();
    static PathString GetModulePath();

    PathString operator+(const PathString&) const;

    FilenameString GetFilename()  const;
    ZString        GetExtension() const;
    ZString        GetName()      const;
    PathString     GetDirectory() const;
    PathString     GetDrive()     const;
};

//////////////////////////////////////////////////////////////////////////////
//
// URLString
//
//////////////////////////////////////////////////////////////////////////////

class URLString : public ZString {
public:
    URLString() : ZString() {}
    URLString(const PathString& pathStr) : ZString("file://" + pathStr) {}
};

//////////////////////////////////////////////////////////////////////////////
//
// Fast encryption functions (separated from ZString to prevent heap 
// fragmentation in the lobby).
//
//////////////////////////////////////////////////////////////////////////////

void ZScramble(char* szDest, PCC szSource, PCC szKey);
void ZUnscramble(char* szDest, PCC szSource, PCC szKey);

void ZToUpper(char* szDest, PCC szSource);
void ZToLower(char* szDest, PCC szSource);

#endif
