//////////////////////////////////////////////////////////////////////////////
//
// Rectangles
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _rect_h_
#define _rect_h_

#include <algorithm>

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
        RectType(typename RectType::PointType(xmin, ymin), typename RectType::PointType(xmax, ymax))
    {}

    template<class Type>
    static TRect Cast(const TRect<Type>& rect)
    {
        return
            TRect(
                (typename RectType::Number)rect.XMin(),
                (typename RectType::Number)rect.YMin(),
                (typename RectType::Number)rect.XMax(),
                (typename RectType::Number)rect.YMax()
            );
    }

    TRect(const RECT& rect) :
        RectType(
            typename RectType::PointType((typename RectType::Number)rect.left,  (typename RectType::Number)rect.top   ),
            typename RectType::PointType((typename RectType::Number)rect.right, (typename RectType::Number)rect.bottom)
        )
    {}

    static TRect GetZero() { return TRect(0, 0, 0, 0); }

    ZString GetString() const {
        return "(min=" + RectType::Min().GetString() + ", max=" + RectType::Max().GetString() + ")";
    }

    typename RectType::Number XSize() const { return RectType::XMax() - RectType::XMin(); }
    typename RectType::Number YSize() const { return RectType::YMax() - RectType::YMin(); }
    typename RectType::PointType Size() const { return RectType::Max() - RectType::Min(); }

    typename RectType::PointType Center() const
    { 
        return (RectType::Max() + RectType::Min()) / 2;
    }

    bool IsEmpty() const
    {
        return RectType::XMin() >= RectType::XMax() || RectType::YMin() >= RectType::YMax();
    }

    bool Inside(const typename RectType::PointType& point) const
    {
        return
                (point.X() >= RectType::XMin())
            &&  (point.X() <  RectType::XMax())
            &&  (point.Y() >= RectType::YMin())
            &&  (point.Y() <  RectType::YMax());
    }

    bool Inside(const RectType& rect) const
    {
        return
                (rect.XMin() >= RectType::XMin())
            &&  (rect.XMax() <= RectType::XMax())
            &&  (rect.YMin() >= RectType::YMin())
            &&  (rect.YMax() <= RectType::YMax());
    }

    // operators

    void SetXSize(typename RectType::Number x) { SetXMax(RectType::XMin() + x); }
    void SetYSize(typename RectType::Number y) { SetYMax(RectType::YMin() + y); }
    void SetSize(const typename RectType::PointType size) { RectType::SetMax(RectType::Min() + size); }

    void Intersect(const TRect& rect)
    {
		RectType::SetXMin(std::max(RectType::XMin(), rect.XMin()));
		RectType::SetXMax(std::min(RectType::XMax(), rect.XMax()));
		RectType::SetYMin(std::max(RectType::YMin(), rect.YMin()));
		RectType::SetYMax(std::min(RectType::YMax(), rect.YMax()));
    }

    void Accumulate(const TRect& rect)
    {
		RectType::SetXMin(std::min(RectType::XMin(), rect.XMin()));
        RectType::SetXMax(std::max(RectType::XMax(), rect.XMax()));
        RectType::SetYMin(std::min(RectType::YMin(), rect.YMin()));
        RectType::SetYMax(std::max(RectType::YMax(), rect.YMax()));
    }

    void Offset(const typename RectType::PointType& pt)
    {
        RectType::SetMin(RectType::Min() + pt);
        RectType::SetMax(RectType::Max() + pt);
    }

    void MoveTo(const typename RectType::PointType& pt) {
        RectType::SetMax(pt + Size());
        RectType::SetMin(pt);
    }

    void Expand(typename RectType::Number size)
    {
        RectType::SetXMin(RectType::XMin() - size);
        RectType::SetXMax(RectType::XMax() + size);
        RectType::SetYMin(RectType::YMin() - size);
        RectType::SetYMax(RectType::YMax() + size);
    }

    typename RectType::PointType TransformNDCToImage(const typename RectType::PointType& point) const
    {
        return Center() + point * (typename RectType::Number)((float)XSize() * 0.5f);
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
