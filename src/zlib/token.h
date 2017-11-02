#ifndef _token_h_
#define _token_h_

#include "tmap.h"
#include "zstring.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Translate escape sequences \?
//
//////////////////////////////////////////////////////////////////////////////

int TranslateCharacter(const char*& pcc);
int ReadHexNumber(const char*& pcc, int max = -1);

//////////////////////////////////////////////////////////////////////////////
//
//  Token
//
//////////////////////////////////////////////////////////////////////////////

class TextTokenImpl {
public:
    const int Null  ;
    const int String;
    const int Number;
    const int Symbol;

private:
    TMap<ZString, int> m_symbolMap;
    TMap<int, ZString> m_tokenMap;

    int     m_idFree;
    int     m_type;
    ZString m_string;
    float   m_numberValue;
    int     m_line;

    ZString m_strError;

    PCC  m_pcc;
    PCC  m_pccEnd;
    PCC  m_pccLine;
    PCC  m_pccStart;
    PCC  m_pccExpressionStart;

    bool ReadNumber(bool bExponent = false);
    bool ReadExponent();
    bool ReadString(char);
    bool ReadSymbol();
    bool SkipWhite();

    int GetLineNumber();
    int GetLineCharNumber();
    int GetCharNumber();

protected:
    TextTokenImpl(PCC pcc, int length);

    int AddSymbol(const ZString& str);
    int AddToken(const ZString& str);

    virtual bool IsSymbolChar(char ch);
    virtual int ParseToken(PCC& pcc);

public:
    ~TextTokenImpl();

    //
    // IToken members
    //

    void SetError(const ZString& szerror);
    void SetErrorUnexpected();
    bool Error();

    void Next();
    bool MoreTokens();
    bool Is(int id, bool bError);
    bool IsSymbol(ZString& str, bool bError);
    bool IsString(ZString& str, bool bError);
    bool IsNumber(float& number, bool bError);
};

//////////////////////////////////////////////////////////////////////////////
//
//  CommandLineToken
//
//////////////////////////////////////////////////////////////////////////////

class CommandLineToken {
private:
    PCC  m_pcc;
    PCC  m_pccEnd;

    void SkipWhite();
    bool ReadExponent(float& number);
    bool ReadNumber(float& number, bool bExponent = false);

public:
    CommandLineToken(PCC& pcc, int length);

    bool MoreTokens();
    bool IsMinus(ZString& str);
    bool IsString(ZString& str);
    bool IsNumber(float& number);
    void Skip();
};

#endif
