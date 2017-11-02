//////////////////////////////////////////////////////////////////////////////
//
// Color
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _color_h_
#define _color_h_

#include "zstring.h"

#undef RGB // TODO: remove this line after all #include <windows.h> are nuked
#define RGB(r,g,b) (uint32_t(((uint8_t(r)|((uint16_t)(uint8_t(g))<<8))|(((uint32_t)uint8_t(b))<<16))))

class Color {
private:
    float m_r, m_g, m_b, m_a;

    static const Color s_colorWhite;
    static const Color s_colorRed;
    static const Color s_colorGreen;
    static const Color s_colorBlue;
    static const Color s_colorYellow;
    static const Color s_colorMagenta;
    static const Color s_colorCyan;
    static const Color s_colorBlack;
    static const Color s_colorGray;
	static const Color s_colorOrange;

public:
    Color() {}
    Color(float r, float g, float b, float a = 1) :
        m_r(r),
        m_g(g),
        m_b(b),
        m_a(a)
    {
    }

    Color(float gray, float a = 1) :
        m_r(gray),
        m_g(gray),
        m_b(gray),
        m_a(a)
    {
    }

    static const Color&   White() { return s_colorWhite;   }
    static const Color&     Red() { return s_colorRed;     }
    static const Color&   Green() { return s_colorGreen;   }
    static const Color&    Blue() { return s_colorBlue;    }
    static const Color&  Yellow() { return s_colorYellow;  }
    static const Color& Magenta() { return s_colorMagenta; }
    static const Color&    Cyan() { return s_colorCyan;    }
    static const Color&   Black() { return s_colorBlack;   }
    static const Color&    Gray() { return s_colorGray;    }
	static const Color&  Orange() { return s_colorOrange;  } // KG- added

    void SetHSBA(float hue, float saturation, float brightness, float alpha = 1);
    void SetRGBA(float r, float g, float b, float a = 1)
    {
        m_r = r;
        m_g = g;
        m_b = b;
        m_a = a;
    }

    float GetRed()   const { return m_r; }
    float GetGreen() const { return m_g; }
    float GetBlue()  const { return m_b; }
    float GetAlpha() const { return m_a; }
    void GetHSB(float& h, float& s, float& b);

    float R() const { return m_r; }
    float G() const { return m_g; }
    float B() const { return m_b; }
    float A() const { return m_a; }

    void SetRed(float value)   { m_r = value; }
    void SetGreen(float value) { m_g = value; }
    void SetBlue(float value)  { m_b = value; }
    void SetAlpha(float value) { m_a = value; }

    uint32_t MakeCOLORREF() const
    { 
        return RGB(min(m_r * 255, 255), min(m_g * 255, 255), min(m_b * 255, 255)); 
    }

    Color AdjustAlpha(float scale)
    {
        return
            Color(
                scale * m_r,
                scale * m_g,
                scale * m_b,
                scale * m_a
            );
    }

    friend Color operator*(const Color& color, float scale)
    {
        return 
            Color(
                scale * color.m_r,
                scale * color.m_g,
                scale * color.m_b,
                color.m_a
            );
    }

    friend Color operator*(float scale, const Color& color)
    {
        return color * scale;
    }

    friend Color operator+(const Color& color1, const Color& color2)
    {
        return
            Color(
                color1.GetRed()   + color2.GetRed(),
                color1.GetGreen() + color2.GetGreen(),
                color1.GetBlue()  + color2.GetBlue()
            );
    }

    friend bool operator==(const Color& color1, const Color& color2)
    {
        return
            (color1.GetRed()   == color2.GetRed()) &&
            (color1.GetGreen() == color2.GetGreen()) &&
            (color1.GetBlue()  == color2.GetBlue());
    }

    friend bool operator!=(const Color& color1, const Color& color2)
    {
        return
            (color1.GetRed()   != color2.GetRed()) ||
            (color1.GetGreen() != color2.GetGreen()) ||
            (color1.GetBlue()  != color2.GetBlue());
    }
};

class HSBColor : public Color {
public:
    HSBColor(float hue, float saturation, float brightness, float alpha = 1)
    {
        SetHSBA(hue, saturation, brightness, alpha);
    }
};

inline Color MakeColorFromCOLORREF(COLORREF color)
{
    return 
        Color(
            (float)((color >> 16) & 0xff) / 255.0f,
            (float)((color >>  8) & 0xff) / 255.0f,
            (float)((color >>  0) & 0xff) / 255.0f,
            (float)((color >> 24) & 0xff) / 255.0f
        );
}

// moved from engine/font.h
//////////////////////////////////////////////////////////////////////////////
//
// Color Codes
//
//////////////////////////////////////////////////////////////////////////////
#define START_COLOR_CODE    0x81
#define END_COLOR_CODE      0x82
#define START_COLOR_STRING  "\x81 %s"
#define END_COLOR_STRING    "\x82 "

ZString ConvertColorToString (const Color& color);
// move end

#endif
