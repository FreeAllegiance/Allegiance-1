//////////////////////////////////////////////////////////////////////////////
//
// Rectangles
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _rect_h_
#define _rect_h_

#include "point.h"

//////////////////////////////////////////////////////////////////////////////
//
// Template for rectangles
//
//////////////////////////////////////////////////////////////////////////////

template<class RectType>
class TRect : public RectType {
public:
    TRect() {}

    TRect(const typename RectType::PointType min, const typename RectType::PointType max) :
        RectType(min, max)
    {}

    TRect(typename RectType::Number xmin, typename RectType::Number ymin, typename RectType::Number xmax, typename RectType::Number ymax) :
        RectType(RectType::PointType(xmin, ymin), RectType::PointType(xmax, ymax))
    {}

    template<class Type>
    static TRect Cast(const TRect<Type>& rect)
    {
        return
            TRect(
                (RectType::Number)rect.XMin(),
                (RectType::Number)rect.YMin(),
                (RectType::Number)rect.XMax(),
                (RectType::Number)rect.YMax()
            );
    }

    TRect(const RECT& rect) :
        RectType(
            RectType::PointType((RectType::Number)rect.left,  (RectType::Number)rect.top   ),
            RectType::PointType((RectType::Number)rect.right, (RectType::Number)rect.bottom)
        )
    {}

    static TRect GetZero() { return TRect(0, 0, 0, 0); }

    typename RectType::Number XSize() const { return XMax() - XMin(); }
    typename RectType::Number YSize() const { return YMax() - YMin(); }
    typename RectType::PointType Size() const { return Max() - Min(); }

    typename RectType::PointType Center() const
    { 
        return (Max() + Min()) / 2;
    }

    bool IsEmpty() const
    {
        return XMin() >= XMax() || YMin() >= YMax();
    }

    bool Inside(const typename RectType::PointType& point) const
    {
        return
                (point.X() >= XMin())
            &&  (point.X() <  XMax())
            &&  (point.Y() >= YMin())
            &&  (point.Y() <  YMax());
    }

    bool Inside(const RectType& rect) const
    {
        return
                (rect.XMin() >= XMin())
            &&  (rect.XMax() <= XMax())
            &&  (rect.YMin() >= YMin())
            &&  (rect.YMax() <= YMax());
    }

    // operators

    void SetXSize(typename RectType::Number x) { SetXMax(XMin() + x); }
    void SetYSize(typename RectType::Number y) { SetYMax(YMin() + y); }
    void SetSize(const typename RectType::PointType size) { SetMax(Min() + size); }

    void Intersect(const TRect& rect)
    {
        SetXMin(max(XMin(), rect.XMin()));
        SetXMax(min(XMax(), rect.XMax()));
        SetYMin(max(YMin(), rect.YMin()));
        SetYMax(min(YMax(), rect.YMax()));
    }

    void Accumulate(const TRect& rect)
    {
        SetXMin(min(XMin(), rect.XMin()));
        SetXMax(max(XMax(), rect.XMax()));
        SetYMin(min(YMin(), rect.YMin()));
        SetYMax(max(YMax(), rect.YMax()));
    }

    void Offset(const typename RectType::PointType& pt)
    {
        SetMin(Min() + pt);
        SetMax(Max() + pt);
    }

    void MoveTo(const typename RectType::PointType& pt) {
        SetMax(pt + Size());
        SetMin(pt);
    }

    void Expand(typename RectType::Number size)
    {
        SetXMin(XMin() - size);
        SetXMax(XMax() + size);
        SetYMin(YMin() - size);
        SetYMax(YMax() + size);
    }

    typename RectType::PointType TransformNDCToImage(const typename RectType::PointType& point) const
    {
        return Center() + point * (RectType::Number)((float)XSize() * 0.5f);
    }

    friend bool operator==(const TRect<RectType>& rect1, const TRect<RectType>& rect2)
    {
        return
               rect1.Min() == rect2.Min()
            && rect1.Max() == rect2.Max();
    }

    friend bool operator!=(const TRect& rect1, const TRect& rect2)
    {
        return
               rect1.Min() != rect2.Min()
            || rect1.Max() != rect2.Max();
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Windows Rectangles
//
//////////////////////////////////////////////////////////////////////////////

class WinRectCore : public RECT {
protected:
    typedef int Number;
    typedef WinPoint PointType;

public:
    WinRectCore() {}
    WinRectCore(Number minX,
                Number minY,
                Number maxX,
                Number maxY)
    {
        left   = minX;
        top    = minY;
        right  = maxX;
        bottom = maxY;
    }

    WinRectCore(const PointType& min, const PointType& max)
    {
        left   = min.X();
        top    = min.Y();
        right  = max.X();
        bottom = max.Y();
    }

    Number XMin() const { return left; }
    Number XMax() const { return right; }
    Number YMin() const { return top; }
    Number YMax() const { return bottom; }

    PointType Min() const { return PointType(XMin(), YMin()); }
    PointType Max() const { return PointType(XMax(), YMax()); }

    void SetXMin(Number x) { left   = x; }
    void SetXMax(Number x) { right  = x; }
    void SetYMin(Number y) { top    = y; }
    void SetYMax(Number y) { bottom = y; }

    void SetMin(const PointType& min) { left = min.X(); top = min.Y(); }
    void SetMax(const PointType& max) { right = max.X(); bottom = max.Y(); }
};

typedef TRect<WinRectCore> WinRect;

//////////////////////////////////////////////////////////////////////////////
//
// floating point Rectangles
//
//////////////////////////////////////////////////////////////////////////////

class RectCore {
private:
    Point m_min;
    Point m_max;

protected:
    typedef float Number;
    typedef Point PointType;

public:
    RectCore() {}

    RectCore(const PointType& min, const PointType& max) :
        m_min(min),
        m_max(max)
    {}

    float XMin() const { return m_min.X(); }
    float XMax() const { return m_max.X(); }
    float YMin() const { return m_min.Y(); }
    float YMax() const { return m_max.Y(); }
    Point Min() const { return m_min; }
    Point Max() const { return m_max; }

    void SetXMin(float x) { m_min.SetX(x); }
    void SetXMax(float x) { m_max.SetX(x); }
    void SetYMin(float y) { m_min.SetY(y); }
    void SetYMax(float y) { m_max.SetY(y); }
    void SetMin(const Point& min) { m_min = min; }
    void SetMax(const Point& max) { m_max = max; }

    operator RECT()
    {
        RECT rc;

        rc.left   = (int)XMin();
        rc.right  = (int)XMax();
        rc.top    = (int)YMin();
        rc.bottom = (int)YMax();

        return rc;
    }
};

typedef TRect<RectCore> Rect;

#endif
