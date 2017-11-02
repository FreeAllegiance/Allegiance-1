#include "pch.h"

/////////////////////////////////////////////////////////////////////////////
//
// IPopup
//
/////////////////////////////////////////////////////////////////////////////

void IPopup::SetContainer(IPopupContainer* pcontainer)
{
    m_pcontainer = pcontainer;
}

void IPopup::SetOwner(IPopup* ppopupOwner)
{
    m_ppopupOwner = ppopupOwner;
}

void IPopup::OnClose()
{
}

void IPopup::ClosePopup(IPopup* ppopup)
{
    m_pcontainer->ClosePopup(ppopup);
}

TRef<Image> IPopup::GetImage(Engine* pengine)
{
    return CreatePaneImage(pengine, true, GetPane());
}

Rect IPopup::GetRect()
{
    TRef<Pane> ppane = GetPane();
    ppane->UpdateLayout();
    return Rect(Point(0, 0), Point::Cast(ppane->GetSize()));
}

//////////////////////////////////////////////////////////////////////////////
//
// PanePopup
//
//////////////////////////////////////////////////////////////////////////////

class PanePopup : public IPopup {
private:
    TRef<Pane> m_ppane;

public:
    PanePopup(Pane* ppane) :
        m_ppane(ppane)
    {
    }

    //
    // IPopup methods
    //

    Pane* GetPane()
    {
        return m_ppane;
    }
};

TRef<IPopup> CreatePanePopup(Pane* ppane)
{
    return new PanePopup(ppane);
}

/////////////////////////////////////////////////////////////////////////////
//
// IPopupContainerImpl
//
/////////////////////////////////////////////////////////////////////////////

class PopupContainerImpl :
    public IPopupContainerPrivate,
    public WrapImage
{
private:
    /////////////////////////////////////////////////////////////////////////////
    //
    // Types
    //
    /////////////////////////////////////////////////////////////////////////////

    class PickImage : public Image {
    private:
        PopupContainerImpl* m_pppic;

    public:
        PickImage(PopupContainerImpl* pppic) :
            m_pppic(pppic)
        {
        }

        MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
        {
            return m_pppic->HitTest();
        }

        MouseResult Button(
            IInputProvider* pprovider,
            const Point& point,
            int button,
            bool bCaptured,
            bool bInside,
            bool bDown
        ) {
            if (bDown) {
                m_pppic->Picked();
            }

            return MouseResult();
        }

        ZString GetFunctionName() { return "ImagePopupContainer"; }
    };

    class PopupData : public IObject {
    public:
        TRef<PopupData> m_pdataNext;
        TRef<IPopup>    m_ppopup;
        TRef<Image>     m_pimage;
        TRef<Image>     m_ppickImage;
        bool            m_bCloseAll;
    };

    /////////////////////////////////////////////////////////////////////////////
    //
    // Members
    //
    /////////////////////////////////////////////////////////////////////////////

    TRef<PopupData>  m_pdata;
    TRef<Engine>     m_pengine;
    TRef<GroupImage> m_pgroup;
    TRef<RectValue>  m_prectValue;
    bool             m_bCascadeRight;

    /////////////////////////////////////////////////////////////////////////////
    //
    // Methods
    //
    /////////////////////////////////////////////////////////////////////////////

public:
    PopupContainerImpl() :
        WrapImage(Image::GetEmpty()),
        m_bCascadeRight(true)
    {
        m_pgroup = new GroupImage();
        SetImage(m_pgroup);
    }

    MouseResult HitTest()
    {
        return (m_pdata != NULL) ? MouseResultHit() : MouseResult();
    }

    void Picked()
    {
        //
        // Picked outside close all the popups
        //

        while (m_pdata && m_pdata->m_bCloseAll) {
            ClosePopup(m_pdata->m_ppopup);
        }

        EngineWindow::DoHitTest();
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // IPopupContainerPrivate methods
    //
    /////////////////////////////////////////////////////////////////////////////

    void Initialize(
        Engine*    pengine,
        RectValue* prectValue
    ) {
        m_pengine    = pengine;
        m_prectValue = prectValue;
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // IPopupContainer
    //
    /////////////////////////////////////////////////////////////////////////////

    void OpenPopup(IPopup* ppopup, const Point& point, bool bCloseAll, IPopup* ppopupOwner)
    {
        OpenPopup(ppopup, new PointValue(point), bCloseAll, ppopupOwner);
    }

    void OpenPopup(IPopup* ppopup, PointValue* ppoint, bool bCloseAll, IPopup* ppopupOwner)
    {
        //
        // Open the popup
        //

        TRef<PopupData> pdata = new PopupData();

        pdata->m_ppopup    = ppopup;
        pdata->m_bCloseAll = bCloseAll;
        pdata->m_pimage    =
            new TransformImage(
                ppopup->GetImage(m_pengine),
                new TranslateTransform2(
                    ppoint
                )
            );

        pdata->m_pdataNext = m_pdata;
        m_pdata = pdata;

        if (ppopupOwner == NULL) {
            pdata->m_ppickImage = new PickImage(this);
            m_pgroup->AddImageToTop(pdata->m_ppickImage);
        }
        m_pgroup->AddImageToTop(pdata->m_pimage);

        ppopup->SetContainer(this);

        if (ppopupOwner != NULL) {
            ppopup->SetOwner(ppopupOwner);
        }

        //EngineWindow::DoHitTest(); // TE: Commented to prevent mouse from triggering quickchats prematurely
    }

    void OpenPopup(IPopup* ppopup, const Rect& rect, bool bCloseAll, bool bCascadeDown, IPopup* ppopupOwner)
    {
        //
        // Figure out where the popup should go
        //

        Rect rectPopup     = ppopup->GetRect();
        Rect rectContainer = m_prectValue->GetValue();

        Point point;

        //
        // x position
        //

        if (m_bCascadeRight) {
            if (rect.XMax() + rectPopup.XSize() > rectContainer.XMax()) {
                point.SetX(rect.XMin() - rectPopup.XSize());
            } else {
                point.SetX(rect.XMax());
            }
        } else {
            if (rect.XMin() - rectPopup.XSize() < 0) {
                point.SetX(rect.XMax());
            } else {
                point.SetX(rect.XMin() - rectPopup.XSize());
            }
        }

        //
        // Make sure we actually stay on the screen
        //

        if (point.X() + rectPopup.XSize() > rectContainer.XMax()) {
            point.SetX(rectContainer.XMax() - rectPopup.XSize());
        }

        if (point.X() < 0) {
            point.SetX(0);
        }

        //
        // Check if going up or down would go off the screen
        //

        if (bCascadeDown) {
            if (rect.YMax() - rectPopup.YSize() < 0) {
                bCascadeDown = false;
            }
        } else {
            if (rect.YMin() + rectPopup.YSize() > rectContainer.YMax()) {
                bCascadeDown = true;
            }
        }

        //
        // y position
        //

        if (bCascadeDown) {
            if (rect.YMax() - rectPopup.YSize() < 0) {
                point.SetY(0);
            } else {
                point.SetY(rect.YMax() - rectPopup.YSize());
            }
        } else {
            if (rect.YMin() + rectPopup.YSize() > rectContainer.YMax()) {
                point.SetY(rectContainer.YMax() - rectPopup.YSize());
            } else {
                point.SetY(rect.YMin());
            }
        }

        //
        // Open the popup
        //

        OpenPopup(ppopup, point, bCloseAll, ppopupOwner);
    }

    class CenterRectPoint : public PointValue {
    private:
        const Rect& GetContainerRect() { return RectValue::Cast(GetChild(0))->GetValue(); }
        const Rect& GetRect()          { return RectValue::Cast(GetChild(1))->GetValue(); }

    public:
        CenterRectPoint(RectValue* prectContainer, RectValue* prect) :
            PointValue(prectContainer, prect)
        {
        }

        void Evaluate()
        {
            GetValueInternal() = (GetContainerRect().Size() - GetRect().Size()) / 2;
        }
    };

    void OpenPopup(IPopup* ppopup, bool bCloseAll, IPopup* ppopupOwner)
    {
        OpenPopup(
            ppopup,
            new CenterRectPoint(m_prectValue, new RectValue(ppopup->GetRect())),
            bCloseAll,
            ppopupOwner
        );
    }

    void ClosePopup(IPopup* ppopup)
    {
        while (m_pdata) {
            m_pgroup->RemoveImage(m_pdata->m_pimage);
            m_pgroup->RemoveImage(m_pdata->m_ppickImage);
            m_pdata->m_ppopup->OnClose();

            if (m_pdata->m_ppopup == ppopup) {
                m_pdata = m_pdata->m_pdataNext;
                break;
            }

            m_pdata = m_pdata->m_pdataNext;
        }

        //
        // If this is the first popup start cascading to the right
        //

        if (m_pdata == NULL) {
            m_bCascadeRight = true;
        }

        EngineWindow::DoHitTest();
    }

    bool IsEmpty()
	{
		return (m_pdata == NULL);
	}

    Image* GetImage()
    {
        return this;
    }

    /////////////////////////////////////////////////////////////////////////////
    //
    // IKeyboardInput
    //
    /////////////////////////////////////////////////////////////////////////////

    bool OnChar(IInputProvider* pprovider, const KeyState& ks)
    {
        if (m_pdata != NULL) {
            m_pdata->m_ppopup->OnChar(pprovider, ks);
            return true;
        }

        return false;
    }

    bool OnKey(IInputProvider* pprovider, const KeyState& ks, bool& fForceTranslate)
    {
        if (m_pdata != NULL) {
            m_pdata->m_ppopup->OnKey(pprovider, ks, fForceTranslate);

            //
            // The popup container eats all of the keyboard
            // messages if a popup is up.
            //

            return true;
        }

        return false;
    }
};

TRef<IPopupContainerPrivate> CreatePopupContainer()
{
    return new PopupContainerImpl();
}
