#ifndef _font_h_
#define _font_h_

//////////////////////////////////////////////////////////////////////////////
//
// IEngineFont
//
//////////////////////////////////////////////////////////////////////////////

class IEngineFont : public IObject {
public:
    virtual int      GetMaxTextLength(const ZString& str, int maxWidth, bool bLeft) = 0;
    virtual WinPoint GetTextExtent(const ZString& str)                              = 0;
    virtual WinPoint GetTextExtent(const char* sz)                                  = 0;
    virtual int      GetHeight()                                                    = 0;

    virtual void DrawString(
        const WinPoint& point,
        const ZString&  str, 
        const Color&    color
    ) = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Color Codes
//
//////////////////////////////////////////////////////////////////////////////
// moved to zlib\color.h

//////////////////////////////////////////////////////////////////////////////
//
// FontValue
//
//////////////////////////////////////////////////////////////////////////////

ZString GetString(int indent, TRef<IEngineFont> pfont);
void Write(IMDLBinaryFile* pmdlFile, TRef<IEngineFont> pfont);
ZString GetFunctionName(const TRef<IEngineFont>& value);

typedef TStaticValue<TRef<IEngineFont> > FontValue;

#endif
