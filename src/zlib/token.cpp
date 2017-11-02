#include "token.h"
#include "zassert.h"

//////////////////////////////////////////////////////////////////////////////
//
//  Token
//
//////////////////////////////////////////////////////////////////////////////

void TextTokenImpl::SetError(const ZString& strError)
{
    ZError("Parsing error - " + strError);
    m_strError = strError;
}

void TextTokenImpl::SetErrorUnexpected()
{
    if (m_type == Symbol) {
        SetError("Unexpected symbol " + m_string);
    } else if (m_type == String) {
        SetError("Unexpected string " + m_string);
    } else if (m_type == Number) {
        SetError("Unexpected number " + ZString(m_numberValue));
    } else if (m_type == NULL) {
        SetError("Unexpected end of file");
    } else {
        ZString str;
        m_tokenMap.Find(m_type, str);

        SetError("Unexpected symbol " + str);
    }
}

bool TextTokenImpl::Error()
{
    return !m_strError.IsEmpty();
}

//////////////////////////////////////////////////////////////////////////////
//
//  TextTokenImpl
//
//////////////////////////////////////////////////////////////////////////////

const int TextTokenImplNull   = 0;
const int TextTokenImplString = 1;
const int TextTokenImplNumber = 2;
const int TextTokenImplSymbol = 3;

inline bool isDigit(char ch)
{
    return (ch >= '0' && ch <= '9');
}

inline bool isAlpha(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

inline bool isNumber(char ch)
{
    return isDigit(ch) || ch == '-' || ch == '.';
}

inline bool isSymbol(const char* pcc)
{
    return isAlpha(pcc[0]) || pcc[0] == '_' || pcc[0] == '\'';
}

inline bool IsWhite(const char* pcc)
{
    return
        (
               pcc[0] == ' '
            || pcc[0] == '\t'
            || pcc[0] == '\n'
            || pcc[0] == 0xd
            || pcc[0] == '#'
            || (pcc[0] == '/' && (pcc[1] == '/' || pcc[1] == '*'))
        );
}

//////////////////////////////////////////////////////////////////////////////
//
//  Lexer
//
//////////////////////////////////////////////////////////////////////////////

int ReadHexNumber(const char*& pcc, int max)
{
    int number = 0;

    while (true) {
        if (isDigit(pcc[0])) {
            number = number * 16 + pcc[0] - '0';
        } else if (pcc[0] >= 'a' && pcc[0] <= 'f') {
            number = number * 16 + pcc[0] - 'a' + 10;
        } else if (pcc[0] >= 'A' && pcc[0] <= 'F') {
            number = number * 16 + pcc[0] - 'A' + 10;
        } else {
            return number;
        }

        pcc++;
        if (max != -1) {
            max--;
            if (max == 0) {
                return number;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

int TranslateCharacter(const char*& pcc)
{
    if (pcc[0] == '\\') {
        pcc += 2;
        switch (pcc[-1]) {
            case '\\': return '\\';
            case '\'': return '\'';
            case '"': return '"';
            case 'n': return '\n';
            case 't': return '\t';
            case '0': return 0;
            case 'x': return ReadHexNumber(pcc, 2);

        }
    }

    pcc++;
    return pcc[-1];
}

//////////////////////////////////////////////////////////////////////////////

bool TextTokenImpl::SkipWhite()
{
    while (m_pcc < m_pccEnd && IsWhite(m_pcc)) {
        if (m_pcc[0] == '#') {
            while (m_pcc < m_pccEnd && m_pcc[0] != '\n') m_pcc++;
            if (m_pcc[-1] == '\n') {
                m_line++;
                m_pccLine = m_pcc;
            }
        } else if (m_pcc[0] == '/' && (m_pcc[1] == '/' || m_pcc[1] == '*')) {
            if (m_pcc[1] == '/') {
                while (m_pcc < m_pccEnd && m_pcc[0] != '\n') m_pcc++;
                if (m_pcc[-1] == '\n') {
                    m_line++;
                    m_pccLine = m_pcc;
                }
            } else {
                while (m_pcc < m_pccEnd && (m_pcc[0] != '*' || m_pcc[1] != '/')) {
                    m_pcc++;
                    if (m_pcc[-1] == '\n') {
                        m_line++;
                        m_pccLine = m_pcc;
                    }
                }
                m_pcc+=2;
           }
        } else if (m_pcc[0] == '\n') {
            m_pcc++;
            m_line++;
            m_pccLine = m_pcc;
        } else {
            m_pcc++;
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////////

bool TextTokenImpl::ReadExponent()
{
    m_pcc++;
    float mantisa = m_numberValue;

    if (!ReadNumber(true)) {
        return false;
    }

    m_numberValue = mantisa * pow((float)10, m_numberValue);
    return true;
}

bool TextTokenImpl::ReadNumber(bool bExponent)
{
    float multiplier = 0.1f;
    float sign       = 1;

    m_numberValue = 0;

    if (m_pcc[0] == '-') {
        sign = -1;
        m_pcc++;
    } else if (m_pcc[0] == '+') {
        m_pcc++;
    }

    while (true) {
        if (m_pcc[0] == '.') {
            m_pcc++;
            while (isDigit(m_pcc[0])) {
                m_numberValue += multiplier * (m_pcc[0] - '0');
                m_pcc++;
                multiplier /= 10;
                if ((!bExponent) && (m_pcc[0] == 'e' || m_pcc[0] == 'E')) {
                    m_numberValue *= sign;
                    return ReadExponent();
                }
            }
            m_numberValue *= sign;
            return true;
        } else if ((!bExponent) && (m_pcc[0] == 'e' || m_pcc[0] == 'E')) {
            m_numberValue *= sign;
            return ReadExponent();
        } else if (isDigit(m_pcc[0])) {
            m_numberValue = m_numberValue * 10 + m_pcc[0] - '0';
            m_pcc++;
        } else {
            m_numberValue *= sign;
            return true;
        }
    }
}

bool TextTokenImpl::ReadString(char chEnd)
{
    m_string.SetEmpty();

    //
    // Skip initial quote
    //

    m_pcc++;

    //
    // Read 256 characters at a time
    //

    char  buf[256];
    char* pccDest = buf;

    while (m_pcc < m_pccEnd) {
        //
        // Read 256 characters at a time
        //

        if ((pccDest - buf) == 256) {
            m_string += ZString(buf, 256);
            pccDest = buf;
        } else if (m_pcc[0] == '"') {
            m_string += ZString(buf, (int)(pccDest - buf));  //imago x64
            
            //
            // See if there is another string
            //

            m_pcc++;
            SkipWhite();
            if (m_pcc[0] != '\"') {
                return true;
            }

            //
            // Another string
            //

            m_pcc++;
            pccDest = buf;
        } else {
            *pccDest = TranslateCharacter(m_pcc);
            pccDest++;
        }
    }

    return false;
}

bool TextTokenImpl::IsSymbolChar(char ch)
{
    return
           isAlpha(ch)
        || isDigit(ch)
        || ch == '_';
}

bool TextTokenImpl::ReadSymbol()
{
    char buf[1000];
    char* pccDest = buf;

    if (m_pcc[0] == '\'') {
        *pccDest = m_pcc[0];
        pccDest++;
        m_pcc++;
    }

    while (m_pcc < m_pccEnd && IsSymbolChar(m_pcc[0])) {
        *pccDest = m_pcc[0];
        pccDest++;
        m_pcc++;
    }

    *pccDest = 0;
    m_string = ZString(buf);

    int type;

    if (m_symbolMap.Find(m_string, type)) {
        m_type = type;
    } else {
        m_type = Symbol;
    }

    return true;
}

void TextTokenImpl::Next()
{
    m_pccStart = m_pcc;
    m_type     = Null;

    if (m_pcc < m_pccEnd) {
        if (m_pcc[0] == '"') {
            // it's a string

            m_type = String;
            ReadString('"');
        } else if (m_pcc[0] == '0' && m_pcc[1] == 'x') {
            // hex number

            m_pcc += 2;
            PCC pccSave = m_pcc;
            m_numberValue = float(ReadHexNumber(m_pcc));
            if (m_pcc != pccSave) {
                m_type = Number;
            }
        } else if (
               (m_pcc[0] >= '0' && m_pcc[0] <= '9')
            || (m_pcc[0] == '-' && m_pcc[1] >= '0' && m_pcc[1] <= '9')
            || m_pcc[0] == '.'
        ) {
            // it's a number

            m_type = Number;
            ReadNumber();
        } else if (m_pcc[0] == '\'') {
            // it's a character

            m_pcc++;
            m_type = Number;
            m_numberValue = (float)TranslateCharacter(m_pcc);
            ZAssert(m_pcc[0] == '\'');
            m_pcc++;
        } else if (isSymbol(m_pcc)) {
            // it's a symbol

            ReadSymbol();
        } else {
            m_type = ParseToken(m_pcc);
        }
        SkipWhite();
    }
}

int TextTokenImpl::AddSymbol(const ZString& str)
{
    m_symbolMap.Set(str, m_idFree);
    return m_idFree++;
}

int TextTokenImpl::AddToken(const ZString& str)
{
    m_tokenMap.Set(m_idFree, str);
    return m_idFree++;
}

int TextTokenImpl::ParseToken(PCC& pcc)
{
    return 0;
}

TextTokenImpl::TextTokenImpl(PCC pcc, int length) :
    m_pcc(pcc),
    m_pccEnd(pcc + length),
    m_pccExpressionStart(pcc),
    m_pccLine(pcc),
    m_line(1),
    m_string(ZString()),
    m_idFree(0),
    Null(0),
    String(0),
    Number(0),
    Symbol(0)
{
    (*(int*)&Null)   = AddToken("end of text");
    (*(int*)&String) = AddToken("string");
    (*(int*)&Number) = AddSymbol("number");
    (*(int*)&Symbol) = AddSymbol("symbol");

    SkipWhite();
}

TextTokenImpl::~TextTokenImpl()
{
}

bool TextTokenImpl::MoreTokens()
{
    return m_type != Null;
}

int TextTokenImpl::GetLineNumber()
{
    return m_line;
}

int TextTokenImpl::GetLineCharNumber()
{
    return m_pcc - m_pccLine + 1;
}

int TextTokenImpl::GetCharNumber()
{
    return m_pcc - m_pccExpressionStart + 1;
}

bool TextTokenImpl::IsSymbol(ZString& str, bool bError)

{
    if (m_type == Symbol) {
        str = m_string;
        Next();
        return true;
    } else if (bError) {
        SetError("Symbol expected");
    }

    return false;
}

bool TextTokenImpl::IsString(ZString& str, bool bError)
{
    if (m_type == String) {
        str = m_string;
        Next();
        return true;
    } else if (bError) {
        SetError("String expected");
    }

    return false;
}

bool TextTokenImpl::IsNumber(float& number, bool bError)
{
    if (m_type == Number) {
        number = m_numberValue;
        Next();
        return true;
    } else if (bError) {
        SetError("Number expected");
    }

    return false;
}

bool TextTokenImpl::Is(int id, bool bError)
{
    if (m_type == id) {
        Next();
        return true;
    } else if (bError) {
        ZString str;
        m_tokenMap.Find(m_type, str);

        SetError("Unexpected " + str);
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CommandLineToken
//
//////////////////////////////////////////////////////////////////////////////

CommandLineToken::CommandLineToken(PCC& pcc, int length) :
    m_pcc(pcc),
    m_pccEnd(pcc + length)
{
}

bool CommandLineToken::MoreTokens()
{
    return m_pcc != m_pccEnd;
}

bool CommandLineToken::IsMinus(ZString& str)
{
    if (m_pcc[0] == '-') {
        m_pcc++;
        return IsString(str);
    }

    return false;
}

void CommandLineToken::SkipWhite()
{
   while (m_pcc < m_pccEnd && m_pcc[0] == ' ') {
       m_pcc++;
   }
}

bool CommandLineToken::IsString(ZString& str)
{
    PCC pccStart = m_pcc;

    if (m_pcc[0] == '"') {
        m_pcc++;
        while (m_pcc < m_pccEnd && m_pcc[0] != '"') {
            m_pcc++;
        }

        if (m_pcc < m_pccEnd) {
            m_pcc++;
            str = ZString(pccStart + 1, (int)(m_pcc - pccStart - 2));  //imago x64 
        } else {
            str = ZString(pccStart + 1, (int)(m_pcc - pccStart - 1)); //imago x64 
        }
    } else {
        while (m_pcc < m_pccEnd && m_pcc[0] != ' ') {
            m_pcc++;
        }

        if (m_pcc - pccStart != 0) {
            str = ZString(pccStart, (int)(m_pcc - pccStart));  //imago x64
        } else {
            return false;
        }
    }

    SkipWhite();

    return true;
}

bool CommandLineToken::ReadExponent(float& number)
{
    m_pcc++;
    float mantisa = number;
    float exponent;

    if (!ReadNumber(exponent, true)) {
        return false;
    }

    number = mantisa * pow((float)10, number);
    return true;
}

bool CommandLineToken::ReadNumber(float& number, bool bExponent)
{
    float multiplier = 0.1f;
    float sign       = 1;

    number = 0;

    if (m_pcc[0] == '-') {
        sign = -1;
        m_pcc++;
    } else if (m_pcc[0] == '+') {
        m_pcc++;
    }

    while (true) {
        if (m_pcc[0] == '.') {
            m_pcc++;
            while (isDigit(m_pcc[0])) {
                number += multiplier * (m_pcc[0] - '0');
                m_pcc++;
                multiplier /= 10;
                if ((!bExponent) && (m_pcc[0] == 'e' || m_pcc[0] == 'E')) {
                    number *= sign;
                    return ReadExponent(number);
                }
            }
            number *= sign;
            return true;
        } else if ((!bExponent) && (m_pcc[0] == 'e' || m_pcc[0] == 'E')) {
            number *= sign;
            return ReadExponent(number);
        } else if (isDigit(m_pcc[0])) {
            number = number * 10 + m_pcc[0] - '0';
            m_pcc++;
        } else {
            number *= sign;
            return true;
        }
    }
}

bool CommandLineToken::IsNumber(float& number)
{
    if (
           (m_pcc[0] >= '0' && m_pcc[0] <= '9')
        || (m_pcc[0] == '-' && m_pcc[1] >= '0' && m_pcc[1] <= '9')
        || m_pcc[0] == '.'
    ) {
        // it's a number

        ReadNumber(number, false);
        SkipWhite();
        return true;
    }

    return false;
}

void CommandLineToken::Skip()
{
    ZString str;

    if (!IsMinus(str)) {
        if (!IsString(str)) {
            float num;
            IsNumber(num);
        }
    }
}
