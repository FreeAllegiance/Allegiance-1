#ifndef _TopPane_H_
#define _TopPane_H_

/////////////////////////////////////////////////////////////////////////////
//
// Top Pane Site
//
/////////////////////////////////////////////////////////////////////////////

class TopPaneSite : public IObject {
public:
    virtual Point TransformLocalToImage(const Point& point) = 0;
    virtual void  SurfaceChanged() = 0;
    virtual void  SizeChanged()    = 0;
};

/////////////////////////////////////////////////////////////////////////////
//
// Top Pane
//
/////////////////////////////////////////////////////////////////////////////

class TopPane : public Pane {
protected:
    TRef<TopPaneSite> m_psite;
    TRef<Engine>      m_pengine;
    TRef<Surface>     m_psurface;
    bool              m_bColorKey;
    bool              m_bNeedLayout;

    void HandleOnTimer(Pane* ppane);
    void UpdateBits();
    Point GetPanePoint(const Point& point);

    //
    // pane members
    //

    Point TransformLocalToImage(const WinPoint& point);
    void  UpdateLayout();
    void  NeedPaintInternal();
    void  NeedLayout();
    void  Paint(Surface* psurface);

public:
    TopPane(Engine* pengine, bool bColorKey, TopPaneSite* psite, Pane* pchild = NULL);

    //
    // Top Pane members
    //

    void Evaluate();
    Surface* GetSurface();
    const WinPoint& GetSurfaceSize();
    void RepaintSurface();

    //
    // IMouseInput methods
    //

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured);
    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown);
    void MouseEnter(IInputProvider* pprovider, const Point& point);
};

/////////////////////////////////////////////////////////////////////////////
//
// TTopPaneSite
//
/////////////////////////////////////////////////////////////////////////////

template<class Type>
class TTopPaneSite : public TopPaneSite {
private:
    Type* m_ptype;

public:
    TTopPaneSite(Type *ptype) :
        m_ptype(ptype)
    {
    }

    Point TransformLocalToImage(const WinPoint& point)
    {
        return m_ptype->TransformLocalToImage(point);
    }

    void SurfaceChanged()
    {
        m_ptype->SurfaceChanged();
    }

    void SizeChanged()
    {
        m_ptype->SizeChanged();
    }
};

#endif
