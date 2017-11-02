#include "pch.h"

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class StringComboFacePane : public ComboFacePane {
private:
    TRef<StringPane> m_pstringPane;
    TRef<BorderPane> m_pborder;

public:
    StringComboFacePane(
        const WinPoint&    size,
              IEngineFont* pfont,
        const Color&       color,
              bool         bBackgroundColor
    ) {
        InternalSetSize(size);

        m_pstringPane =
            new StringPane(
                ZString(),
                pfont,
                size
            );

        m_pstringPane->SetTextColor(color);

        if (bBackgroundColor) {
            InsertAtBottom(
                m_pborder = new BorderPane(
                    0,
                    Color(0.25, 0.25, 0.25),
                    m_pstringPane
                )
            );
        } else {
            InsertAtBottom(m_pstringPane);
        }
    }

    void SetString(const ZString& str)
    {
        m_pstringPane->SetString(str);
    }

    void SetInside(bool bInside)
    {
    }

    void SetColor(const Color& color)
    {
        if (m_pborder) {
            m_pborder->SetColor(color);
        }
    }
};

TRef<ComboFacePane> CreateStringComboFacePane(
    const WinPoint&    size,
          IEngineFont* pfont,
    const Color&       color,
          bool         bBackgroundColor
) {
    return new StringComboFacePane(size, pfont, color, bBackgroundColor);
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ImageComboFacePane : public ComboFacePane {
private:
    TRef<ButtonFacePane> m_pfacePane;

public:
    ImageComboFacePane(Image* pimage)
    {
        m_pfacePane =
            CreateButtonFacePane(
                pimage->GetSurface(),
                ButtonFaceUp | ButtonFaceInside
            );

        InsertAtBottom(m_pfacePane);
    }

    void SetString(const ZString& str)
    {
    }

    void SetColor(const Color& color)
    {
    }

    void SetInside(bool bInside)
    {
        m_pfacePane->SetInside(bInside);
    }

    void UpdateLayout()
    {
        DefaultUpdateLayout();
    }
};

TRef<ComboFacePane> CreateImageComboFacePane(Image* pimage)
{
    return new ImageComboFacePane(pimage);
}

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ComboPaneImpl :
    public ComboPane,
    public IMenuCommandSink
{
private:
    TRef<IntegerEventSourceImpl> m_peventSource;
    TRef<EventSourceImpl>        m_peventMouseEnterWhileEnabledSource;
    TRef<EventSourceImpl>        m_peventMenuSelectSource;
    TRef<IMenu>                  m_pmenu;
    TRef<Modeler>                m_pmodeler;
    TRef<IEngineFont>            m_pfont;
    TRef<ComboFacePane>          m_pface;
    TRef<IPopupContainer>        m_ppopupContainer;
    TRef<ModifiablePointValue> m_pMousePosition;
    bool                         m_bFirstItem;
    TVector<ZString>             m_vstrItems;
    int                          m_idSelection;
    bool                         m_bEnabled;
    bool                         m_bInside;

public:
    ComboPaneImpl(
        Modeler*         pmodeler,
        IPopupContainer* ppopupContainer,
        IEngineFont*     pfont,
        const WinPoint&  size,
        ComboFacePane*   pface,
        ModifiablePointValue* pmousePosition
    ) :
        m_peventMouseEnterWhileEnabledSource(new EventSourceImpl()),
        m_peventMenuSelectSource(new EventSourceImpl()),
        m_ppopupContainer(ppopupContainer),
        m_pface(pface),
        m_bFirstItem(true),
        m_idSelection(0),
        m_bInside(false),
        m_bEnabled(true),
        m_pmodeler(pmodeler),
        m_pfont(pfont),
        m_pMousePosition(pmousePosition)
    {
        InsertAtBottom(
            new JustifyPane(
                JustifyPane::Left,
                size,
                pface
            )
        );
        m_peventSource     = new IntegerEventSourceImpl();
        m_pmenu =
            CreateMenu(
                pmodeler,
                pfont,
                IMenuCommandSink::CreateDelegate(this)
            );
    }

    ~ComboPaneImpl()
    {
        m_pmenu->SetMenuCommandSink(NULL);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IEventSink
    //
    //////////////////////////////////////////////////////////////////////////////

    bool OnEvent(IEventSource* peventSource)
    {
        return true;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IMenuCommandSink
    //
    //////////////////////////////////////////////////////////////////////////////

    void OnMenuCommand(IMenuItem* pitem)
    {
        m_pface->SetString(pitem->GetString());
        m_pface->SetColor(pitem->GetColor());
        m_idSelection = pitem->GetID();
        m_peventMenuSelectSource->Trigger();
        m_peventSource->Trigger(pitem->GetID());
        m_ppopupContainer->ClosePopup(m_pmenu);
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // ComboPane methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void ClearContents()
    {
        m_bFirstItem = true;
        m_idSelection = 0;
        m_vstrItems.SetEmpty();

        m_pmenu =
            CreateMenu(
                m_pmodeler,
                m_pfont,
                IMenuCommandSink::CreateDelegate(this)
            );
    }
    
    TRef<IMenuItem> AddItem(const ZString& str, int id)
    {
        TRef<IMenuItem> pitem = m_pmenu->AddMenuItem(id, str);
        if (m_bFirstItem) {
            m_bFirstItem = false;
            m_idSelection = id;
            m_pface->SetString(pitem->GetString());
            m_pface->SetColor(pitem->GetColor());
        }
        m_vstrItems.PushEnd(str);

        return pitem;
    }

    TRef<IMenuItem> AddItem(const ZString& str, int id, const Color& color)
    {
        bool bFirstItem = m_bFirstItem;

        TRef<IMenuItem> pitem = m_pmenu->AddMenuItem(id, str);

        if (bFirstItem) {
            m_pface->SetColor(color);
        }

        return pitem;
    }

    void SetSelection(int id)
    {
        m_idSelection = id;
        IMenuItem* pitem = m_pmenu->FindMenuItem(id);

        if (pitem) {
            m_pface->SetString(pitem->GetString());
            m_pface->SetColor(pitem->GetColor());
            m_peventSource->Trigger(id);
        } else {
            m_pface->SetString("");
            m_peventSource->Trigger(id);
        }
    }

    const ZString * GetSelectionString()
    {
        return &m_vstrItems[m_idSelection];
    }

    int GetSelection()
    {
        return m_idSelection;
    }

    IIntegerEventSource* GetEventSource()
    {
        return m_peventSource;
    }

    void SetEnabled(bool bEnabled)
    {
        if (m_bEnabled != bEnabled)
        {
            m_bEnabled = bEnabled;

            if (bEnabled && m_bInside)
                m_peventMouseEnterWhileEnabledSource->Trigger();
        }
    }

    IEventSource* GetMouseEnterWhileEnabledEventSource()
    {
        return m_peventMouseEnterWhileEnabledSource;
    }

    IEventSource* GetMenuSelectEventSource()
    {
        return m_peventMenuSelectSource;
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // Pane methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void UpdateLayout()
    {
        DefaultUpdateLayout();
    }

    //////////////////////////////////////////////////////////////////////////////
    //
    // IMouseInput methods
    //
    //////////////////////////////////////////////////////////////////////////////

    void MouseEnter(IInputProvider* pprovider, const Point& point)
    {
        m_bInside = true;
        m_pface->SetInside(true);

        if (m_bEnabled)
            m_peventMouseEnterWhileEnabledSource->Trigger();
    }

    void MouseLeave(IInputProvider* pprovider)
    {
        m_bInside = false;
        m_pface->SetInside(false);
    }

    MouseResult HitTest(IInputProvider* pprovider, const Point& point, bool bCaptured)
    {
        bool bInside =
               !IsHidden()
            && point.X() >= 0
            && point.X() < (float)XSize()
            && point.Y() >= 0
            && point.Y() < (float)YSize();

        return bInside ? MouseResultHit() : MouseResult();
    }

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        if (button == 0 && m_bEnabled) {
            if (bDown) {
                Point point = m_pMousePosition->GetValue();
                m_ppopupContainer->OpenPopup(m_pmenu, Rect(point,point), true, true);
            }
        }

        return MouseResult();
    }
};

TRef<ComboPane> CreateComboPane(
    Modeler*         pmodeler,
    IPopupContainer* ppopupContainer,
    IEngineFont*     pfont,
    const WinPoint&  size,
    ComboFacePane*   pface,
    ModifiablePointValue* pmousePosition
) {
    return new ComboPaneImpl(pmodeler, ppopupContainer, pfont, size, pface, pmousePosition);
}
