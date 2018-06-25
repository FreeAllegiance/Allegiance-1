#include "pch.h"
#include "mdl.h"
#include "trekmdl.h"
//#include "console.h"
#ifndef MARKCU
#include "consoledata.h"
#endif 
// KGJV #62
#include "mappreview.h"
//////////////////////////////////////////////////////////////////////////////
//
// SwitchValuePane
//
//////////////////////////////////////////////////////////////////////////////

class SwitchValuePane : public ValuePane
{
protected:
    TMap<int, TRef<Pane> >  m_mapPanes;
    TRef<Pane>              m_pPaneCurrent;
    TRef<Pane>              m_pPaneDefault;
    int                     m_nValueCurrent;
    
    void UpdateLayout()
    {
        DefaultUpdateLayout();
    }
  
public:
       
    SwitchValuePane(Number* pvalue) :
        ValuePane(pvalue),
        m_pPaneDefault(NULL)
    {
        pvalue->Update();
        NeedLayout();
        NeedPaint();
        m_nValueCurrent = (int)Number::Cast(pvalue)->GetValue();
    }

    ZString GetFunctionName() { return "SwitchPane"; }

    void InsertPane(Pane* pPane, int nPaneID)
    {
        ZAssert(nPaneID != -1); // reserved

        m_mapPanes.Set(nPaneID, pPane);
        UpdatePane();
    }

    void SetDefaultPane(Pane* pPane)
    {
        m_pPaneDefault = pPane;
        UpdatePane();
    }

private:

    void UpdatePane()
    {
        TRef<Pane> pPane;
        
        if (!m_mapPanes.Find(m_nValueCurrent, pPane))
            pPane = m_pPaneDefault;

        if (pPane != m_pPaneCurrent)
        {
            if (m_pPaneCurrent)
                RemoveChild(m_pPaneCurrent);
            if (pPane)
                InsertAtBottom(pPane);
            m_pPaneCurrent = pPane;
        }
    }

    void ChildChanged(Value* pvalue, Value* pvalueNew)
    {
        ZAssert(NULL == pvalueNew);
        ZAssert(GetChild(0) == pvalue);
        pvalue->Update();

        m_nValueCurrent = (int)Number::Cast(pvalue)->GetValue();
        UpdatePane();
    }
};


//////////////////////////////////////////////////////////////////////////////
//
// ImageValuePane
//
//////////////////////////////////////////////////////////////////////////////

class ImageValuePane : public ValuePane
{
public:
    ImageValuePane(Image* pimage) :
        ValuePane(pimage)
    {
    }

    Image* GetImage() { return Image::Cast((Value*)GetChild(0)); }
    void   SetImage(Image* pimage) { SetChild(0, pimage); }

private:

    void ChildChanged(Value* pvalue, Value* pvalueNew)
    {
        Value::ChildChanged(pvalue, pvalueNew);

        NeedPaint();
        NeedLayout();
    }

    void UpdateLayout()
    {
        Update();

        InternalSetSize(
            WinPoint::Cast(GetImage()->GetBounds().GetRect().Size())
        );
        Pane::UpdateLayout();
    }

    void Paint(Surface* psurface)
    {
        psurface->BitBlt(WinPoint(0, 0), GetImage()->GetSurface());
    }

};

//////////////////////////////////////////////////////////////////////////////
//
// StringValuePane
//
//////////////////////////////////////////////////////////////////////////////

class StringValuePane : public Value, public StringPane
{
protected:
    TRef<ColorValue> m_pcolor;
    
public:
       
    StringValuePane(StringValue* pvalue, ColorValue* pcolor, IEngineFont* pfont, WinPoint ptSize, 
            Justification justify) :
        Value(pvalue, pcolor),
        StringPane((pvalue->Update(), pvalue->GetValue()), pfont, ptSize, justify),
        m_pcolor(pcolor)
    {
        pcolor->Update();
        SetTextColor(pcolor->GetValue());
        StringPane::SetString(pvalue->GetValue());
    }

    StringValuePane(StringValue* pvalue, ColorValue* pcolor, IEngineFont* pfont) :
        Value(pvalue, pcolor),
        StringPane((pvalue->Update(), pvalue->GetValue()), pfont),
        m_pcolor(pcolor)
    {
        pcolor->Update();
        SetTextColor(pcolor->GetValue());
        StringPane::SetString(pvalue->GetValue());
    }

    ZString GetFunctionName() { return "StringPane"; }

    virtual void SetString(const ZString& str)
    {
        StringPane::SetString(str);
        SetChild(0, new StringValue(str));
    }

private:

    void ChildChanged(Value* pvalue, Value* pvalueNew)
    {
        ZAssert(NULL == pvalueNew);
        
        pvalue->Update();
        if (pvalue == m_pcolor)
            {
            ZAssert(pvalue == GetChild(1));
            SetTextColor(m_pcolor->GetValue());
            }
        else
            {
            ZAssert(pvalue == GetChild(0));
            StringPane::SetString(StringValue::Cast(pvalue)->GetValue());
            }
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// GaugeValuePane
//
//////////////////////////////////////////////////////////////////////////////

class GaugeValuePane : public ValuePane
{
private:
    TRef<Surface>   m_psurface;

    float           m_minValue;
    float           m_maxValue;

    int             m_value;
    int             m_valueOld;
    int             m_valueFlash;

    float           m_timeLastChange;

    Color           m_colorFlash;
    Color           m_colorEmpty;

  
public:

    GaugeValuePane(Surface* psurface, 
                Number* pnumber, Number* ptime,
                const Color& colorFlash,
                const Color& colorEmpty) :
        ValuePane(pnumber, ptime),
        m_psurface(psurface),
        m_colorFlash(colorFlash),
        m_colorEmpty(colorEmpty),
        m_minValue(0.0f),
        m_maxValue(1.0f),
        m_value(0),
        m_valueOld(0),
        m_valueFlash(0),
        m_timeLastChange(ptime->GetValue())
    {
        pnumber->Update();
        SetValue(pnumber->GetValue(), true);
        assert(m_psurface);
        assert(m_minValue < m_maxValue);

        InternalSetSize(m_psurface->GetSize());
        NeedPaint();
    }

protected:

    void Paint(Surface* psurface)
    {
        if (m_value != 0) {
            psurface->BitBlt(
                WinPoint(0, 0),
                m_psurface,
                WinRect(0, 0, m_value, YSize())
            );
        }

        if (m_value < m_valueFlash) {
            psurface->FillRect(
                WinRect(
                    m_value,
                    0,
                    m_valueFlash,
                    YSize()
                ),
                m_colorFlash
            );
        }
    }

    void SetValue(float v, bool fFlash)
    {
        m_value =
            (int)bound(
                (v - m_minValue) * ((float)XSize()) / (m_maxValue - m_minValue),
                0.0f,
                (float)XSize()
            );
    }

    void Update(float time)
    {
        if (m_value != m_valueOld) {
            if (m_value < m_valueOld) {
                m_valueFlash = m_valueOld;
            } else {
                m_valueFlash = m_value;
            }

            m_timeLastChange = time;
            m_valueOld = m_value;

            NeedPaint();
        }

        if (m_value != m_valueFlash && time - m_timeLastChange > 0.25f) {
            m_valueFlash = m_value;
            NeedPaint();
        }
    }

private:

    void ChildChanged(Value* pvalue, Value* pvalueNew)
    {
        ZAssert(NULL == pvalueNew);

        if (pvalue == GetChild(0)) // number changed
            {
            pvalue->Update();
            SetValue(Number::Cast(pvalue)->GetValue(), true);
            }
       else // time changed
            {
            ZAssert(GetChild(1) == pvalue);
            pvalue->Update();
            Update(Number::Cast(pvalue)->GetValue());
            }
    }


};


//////////////////////////////////////////////////////////////////////////////
//
//  Factories
//
//////////////////////////////////////////////////////////////////////////////

class SwitchPaneFactory : public IFunction
{
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Number>      pnumber; CastTo(pnumber, (IObject*)stack.Pop());
        TRef<IObjectList> plist;   CastTo(plist,   (IObject*)stack.Pop());

        TRef<SwitchValuePane> ppane = new SwitchValuePane(pnumber);

        plist->GetFirst();
        while (plist->GetCurrent() != NULL)  {
            IObjectPair* ppair; CastTo(ppair, plist->GetCurrent());

            TRef<Pane>   ppaneChild;  CastTo(ppaneChild, (Pane*)ppair->GetFirst());
            TRef<Number> pnumberChildID = Number::Cast(ppair->GetSecond());

            ppane->InsertPane(ppaneChild, (int)pnumberChildID->GetValue());

            plist->GetNext();
        }

        if (stack.GetCount() > 0) {
            TRef<Pane>   ppaneDefault;  CastTo(ppaneDefault, (Pane*)(IObject*)stack.Pop());

            ppane->SetDefaultPane(ppaneDefault);
        }

        return (Pane*)ppane;
    }

};

class StringPaneFactory : public IFunction
{
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<StringValue> pstring; CastTo(pstring, (IObject*)stack.Pop());
        TRef<ColorValue>  pcolor;  CastTo(pcolor, (IObject*)stack.Pop());

        TRef<PointValue> ppointSize;
        WinPoint ptSize;
        if (stack.GetCount() > 0)
            {
            CastTo(ppointSize, (IObject*)stack.Pop());
            ptSize = WinPoint(
                (int)ppointSize->GetValue().X(),
                (int)ppointSize->GetValue().Y()
                );
            }

        Justification justification = JustifyLeft();
        if (stack.GetCount() > 0)
        {
            TRef<Number> pjustify = Number::Cast((IObject*)stack.Pop());
            justification.SetWord((DWORD)pjustify->GetValue());
        }

        TRef<IEngineFont> pfont = TrekResources::SmallFont();

        if (stack.GetCount() > 0)
        {
            TRef<FontValue> pfontLocal;  CastTo(pfontLocal,  (IObject*)stack.Pop());
            pfont = pfontLocal->GetValue();
        }

        bool bRightClip = false;
        if (stack.GetCount() > 0)
            bRightClip = GetBoolean((IObject*)stack.Pop());

        TRef<StringValuePane> ppane;

        if (ppointSize)
        {
            if (bRightClip)
                justification = JustifyLeftClipRight();

            ppane = new StringValuePane(pstring, pcolor, pfont, ptSize, justification);
        }
        else
            ppane = new StringValuePane(pstring, pcolor, pfont);

        return (Pane*)ppane;
    }
};

class GaugePaneFactory : public IFunction
{
public:

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>  pimage     = Image::Cast((Value*)(IObject*)stack.Pop());
        TRef<Number> pnumber;     CastTo(pnumber, (IObject*)stack.Pop());
        Color        colorFlash = GetColor((IObject*)stack.Pop());
        Color        colorEmpty = GetColor((IObject*)stack.Pop());

        TRef<GaugeValuePane> ppane =
            new GaugeValuePane(
                pimage->GetSurface(), 
                pnumber,
                GetWindow()->GetTime(),
                colorFlash,
                colorEmpty
            );

        return (Pane*)ppane;
    }
};



//////////////////////////////////////////////////////////////////////////////
//
// ButtonPane
//
//////////////////////////////////////////////////////////////////////////////

namespace 
{
    class SoundIDEventSink : public IEventSink, public IIntegerEventSink
    {
    private:
        SoundID m_soundId;

    public:

        SoundIDEventSink(SoundID soundId)
        {
            m_soundId = soundId;
        };

        bool OnEvent(IIntegerEventSource* pevent, int nCmd)
        {
            return OnEvent(pevent);
        }

        bool OnEvent(IEventSource* pevent)
        {
            GetWindow()->StartSound(m_soundId);

            // HACK: force the sound to start _now_ 
            // (some buttons cause events that take 1/5 of a second or so to
            // happen, and the delay in the sonic feedback for the mouse click
            // is annoying).
            GetWindow()->SoundEngineUpdate();

            return true;
        }
    };
};

TRef<ButtonPane> CreateTrekButton(
    ButtonFacePane* ppane,
    bool  bToggle,
    SoundID soundClick,
    float repeatRate,
    float repeatDelay
)
{
    TRef<ButtonPane> pbutton = CreateButton(ppane, bToggle, repeatRate, repeatDelay);
    
    pbutton->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
    pbutton->GetEventSource()->AddSink(new SoundIDEventSink(soundClick));

    return pbutton;
}

TRef<ButtonPane> CreateTrekButton(
	Pane* ppaneUp, 
	Pane* ppaneDown, 
    bool  bToggle,
    SoundID soundClick,
    float repeatRate,
    float repeatDelay
)
{
    TRef<ButtonPane> pbutton = CreateButton(ppaneUp, ppaneDown, bToggle, repeatRate, repeatDelay);
    
    pbutton->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
    pbutton->GetEventSource()->AddSink(new SoundIDEventSink(soundClick));

    return pbutton;
}

class ButtonPaneFactory : public IFunction 
{
private:
    TRef<Modeler> m_pmodeler;

public:
    ButtonPaneFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>        pimage;       CastTo(pimage, (Value*)(IObject*)stack.Pop());
        TRef<Number>       pnumberFaces; CastTo(pnumberFaces, (IObject*)stack.Pop());
        TRef<Boolean>      pboolToggle;  CastTo(pboolToggle,  (IObject*)stack.Pop());

        DWORD dwFaces = (DWORD)pnumberFaces->GetValue();

        return
            CreateTrekButton(
                CreateButtonFacePane(
                    pimage->GetSurface(),
                    dwFaces
                ),
                pboolToggle->GetValue(),
                (dwFaces & ButtonFaceCheckedDown) ? mouseclickSound : positiveButtonClickSound
            );
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// SoundButtonPane
//
//////////////////////////////////////////////////////////////////////////////
/*
namespace 
{
    class SoundEventSink : public IEventSink, public IIntegerEventSink
    {
    private:
        TRef<ISoundTemplate> m_ptemplate;

    public:

        SoundEventSink(ISoundTemplate* ptemplate)
            : m_ptemplate(ptemplate)
        {
        };

        bool OnEvent(IIntegerEventSource* pevent, int nCmd)
        {
            return OnEvent(pevent);
        }

        bool OnEvent(IEventSource* pevent)
        {
            GetWindow()->StartSound(m_ptemplate);

            // HACK: force the sound to start _now_ 
            // (some buttons cause events that take 1/5 of a second or so to
            // happen, and the delay in the sonic feedback for the mouse click
            // is annoying).
            GetWindow()->SoundEngineUpdate();

            return true;
        }
    };
};
*/
class SoundButtonPaneFactory : public IFunction 
{
private:
    TRef<Modeler> m_pmodeler;

public:
    SoundButtonPaneFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>        pimage;       CastTo(pimage, (Value*)(IObject*)stack.Pop());
        TRef<Number>       pnumberFaces; CastTo(pnumberFaces, (IObject*)stack.Pop());
        TRef<Boolean>      pboolToggle;  CastTo(pboolToggle,  (IObject*)stack.Pop());
        TRef<Number>       pnumberClickSoundId; CastTo(pnumberClickSoundId, (IObject*)stack.Pop());

        TRef<ButtonPane> pbutton = 
            CreateButton(
                CreateButtonFacePane(
                    pimage->GetSurface(),
                    (DWORD)pnumberFaces->GetValue()
                ),
                pboolToggle->GetValue()
            );
    
        pbutton->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
        pbutton->GetEventSource()->AddSink(new SoundIDEventSink((SoundID)pnumberClickSoundId->GetValue()));
		//pbutton->GetRightEventSource()->AddSink(new SoundIDEventSink((SoundID)pnumberClickSoundId->GetValue())); //Imago NYI 7/10/09

        return pbutton;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// ButtonBarPane
//
//////////////////////////////////////////////////////////////////////////////

class ButtonBarPaneFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;

public:
    ButtonBarPaneFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>       pimage;        CastTo(pimage,        (Value*)(IObject*)stack.Pop());
        TRef<Number>      pnumberFaces;  CastTo(pnumberFaces,  (IObject*)stack.Pop());
        bool              bActAsTabs = false;

        TVector<int> m_vecColumns;
        ParseIntVector((IObject*)stack.Pop(), m_vecColumns);

        if (stack.GetCount() > 0)
        {
            TRef<Boolean>  pbooleanActAsTabs;  CastTo(pbooleanActAsTabs, (IObject*)stack.Pop());
            bActAsTabs = pbooleanActAsTabs->GetValue();
        }

        TRef<ButtonBarPane> pbuttonbar = 
            CreateButtonBarPane(
                pimage->GetSurface(),
                (DWORD)pnumberFaces->GetValue(),
                m_vecColumns,
                bActAsTabs
            );
    
        pbuttonbar->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
        pbuttonbar->GetEventSource()->AddSink(new SoundIDEventSink(mouseclickSound));

        return pbuttonbar;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// ImageComboPane
//
//////////////////////////////////////////////////////////////////////////////

class ImageComboPaneFactory : public IFunction {
private:
    TRef<Modeler>         m_pmodeler;
    TRef<IPopupContainer> m_ppopupContainer;
    TRef<ModifiablePointValue> m_pMousePosition;

public:
    ImageComboPaneFactory(
        Modeler*         pmodeler,
        IPopupContainer* ppopupContainer,
        ModifiablePointValue* pmousePosition
    ) :
        m_pmodeler(pmodeler),
        m_ppopupContainer(ppopupContainer),
        m_pMousePosition(pmousePosition)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<Image>     pimage; CastTo(pimage, (Value*)(IObject*)stack.Pop());
        TRef<FontValue> pfont;  CastTo(pfont,  (IObject*)stack.Pop());

        TRef<ComboPane> pcombo =
            CreateComboPane(
                m_pmodeler,
                m_ppopupContainer,
                pfont->GetValue(),
                WinPoint(0, 0),
                CreateImageComboFacePane(pimage),
                m_pMousePosition
            );

        pcombo->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
        pcombo->GetMenuSelectEventSource()->AddSink(new SoundIDEventSink(mouseclickSound));

        return pcombo;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// StringComboPane
//
//////////////////////////////////////////////////////////////////////////////

class StringComboPaneFactory : public IFunction {
private:
    TRef<Modeler>         m_pmodeler;
    TRef<IPopupContainer> m_ppopupContainer;
    TRef<ModifiablePointValue> m_pMousePosition;

public:
    StringComboPaneFactory(
        Modeler*         pmodeler,
        IPopupContainer* ppopupContainer,
        ModifiablePointValue* pmousePosition
    ) :
        m_pmodeler(pmodeler),
        m_ppopupContainer(ppopupContainer),
        m_pMousePosition(pmousePosition)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<PointValue> ppointFace; CastTo(ppointFace, (IObject*)stack.Pop());
        TRef<PointValue> ppoint;     CastTo(ppoint,     (IObject*)stack.Pop());
        TRef<FontValue>  pfont;      CastTo(pfont,      (IObject*)stack.Pop());
        TRef<ColorValue> pcolor;     CastTo(pcolor,     (IObject*)stack.Pop());

        TRef<ComboPane> pcombo =
            CreateComboPane(
                m_pmodeler,
                m_ppopupContainer,
                pfont->GetValue(),
                WinPoint::Cast(ppoint->GetValue()),
                CreateStringComboFacePane(
                    WinPoint::Cast(ppointFace->GetValue()),
                    pfont->GetValue(),
                    pcolor->GetValue(),
                    false
                ),
                m_pMousePosition
            );

        pcombo->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
        pcombo->GetMenuSelectEventSource()->AddSink(new SoundIDEventSink(mouseclickSound));

        return pcombo;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// StringColorComboPane
//
//////////////////////////////////////////////////////////////////////////////

class StringColorComboPaneFactory : public IFunction {
private:
    TRef<Modeler>         m_pmodeler;
    TRef<IPopupContainer> m_ppopupContainer;
    TRef<ModifiablePointValue> m_pMousePosition;

public:
    StringColorComboPaneFactory(
        Modeler*         pmodeler,
        IPopupContainer* ppopupContainer,
        ModifiablePointValue* pmousePosition
    ) :
        m_pmodeler(pmodeler),
        m_ppopupContainer(ppopupContainer),
        m_pMousePosition(pmousePosition)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TRef<PointValue> ppointFace; CastTo(ppointFace, (IObject*)stack.Pop());
        TRef<PointValue> ppoint;     CastTo(ppoint,     (IObject*)stack.Pop());
        TRef<FontValue>  pfont;      CastTo(pfont,      (IObject*)stack.Pop());
        TRef<ColorValue> pcolor;     CastTo(pcolor,     (IObject*)stack.Pop());

        TRef<ComboPane> pcombo =
            CreateComboPane(
                m_pmodeler,
                m_ppopupContainer,
                pfont->GetValue(),
                WinPoint::Cast(ppoint->GetValue()),
                CreateStringComboFacePane(
                    WinPoint::Cast(ppointFace->GetValue()),
                    pfont->GetValue(),
                    pcolor->GetValue(),
                    true
                ),
                m_pMousePosition
            );

        pcombo->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
        pcombo->GetMenuSelectEventSource()->AddSink(new SoundIDEventSink(mouseclickSound));

        return pcombo;
    }
};

//////////////////////////////////////////////////////////////////////////////
// KGJV #62 - MapPreviewPane
// MDL syntax:
//  MapPreviewPane(image, bshowdetails, bshowside)
//     image = background image on which the preview will be rendered. So image also sets the size of the pane.
//     bshowdetails = bool, if true displays map details whern the mouse is over the pane
//     bshowside = bool, if true displays the side arrow on the map

class MapPreviewPaneFactory : public IFunction {
private:
    TRef<Modeler>         m_pmodeler;
public:
    MapPreviewPaneFactory(
        Modeler*         pmodeler
    ) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        //TRef<RectValue> prect  = RectValue::Cast((IObject*)stack.Pop());
		TRef<Image>   pimage               = Image::Cast((Value*)(IObject*)stack.Pop());
		TRef<Boolean> pbooleanShowDetails  = Boolean::Cast((IObject*)stack.Pop());
		TRef<Boolean> pbooleanShowSide     = Boolean::Cast((IObject*)stack.Pop());

        return (Pane*)
            new MapPreviewPane(m_pmodeler,pimage,pbooleanShowDetails->GetValue(),pbooleanShowSide->GetValue());
    }
};

//////////////////////////////////////////////////////////////////////////////
// KGJV #114
// TextButtonBarPane
//  serverColumns       = [-1,-1,-1];
//  serverHeaderColumns = ["Name","Location","Ping"];
//  serverListHeader    = TextButtonBarPane(serverColumns, serverHeaderColumns, Button3State, false);
//////////////////////////////////////////////////////////////////////////////

class TextButtonBarPaneFactory : public IFunction {
private:
    TRef<Modeler> m_pmodeler;

public:
    TextButtonBarPaneFactory(Modeler* pmodeler) :
        m_pmodeler(pmodeler)
    {
    }

    TRef<IObject> Apply(ObjectStack& stack)
    {
        TVector<int> m_vecColumns;
        ParseIntVector((IObject*)stack.Pop(), m_vecColumns);
		
		TVector<ZString> m_vecColumnsNames;
        ParseStringVector((IObject*)stack.Pop(), m_vecColumnsNames);

        TRef<Number>      pnumberFaces;  CastTo(pnumberFaces,  (IObject*)stack.Pop());
        bool              bActAsTabs = false;

        if (stack.GetCount() > 0)
        {
            TRef<Boolean>  pbooleanActAsTabs;  CastTo(pbooleanActAsTabs, (IObject*)stack.Pop());
            bActAsTabs = pbooleanActAsTabs->GetValue();
        }

        TRef<ButtonBarPane> pbuttonbar = 
            CreateButtonBarPane(bActAsTabs,false);

		int count = m_vecColumns.GetCount();
		int xprev = 0;

		for (int index = 0; index < count; index++) {
			int x = m_vecColumns[index];

			WinPoint p = TrekResources::SmallFont()->GetTextExtent(m_vecColumnsNames[index]);

			TRef<StringPane> s1 = new StringPane(
									m_vecColumnsNames[index],
									TrekResources::SmallFont(),
									WinPoint(x-xprev,p.Y()),
									JustifyLeft());
			TRef<StringPane> s2 = new StringPane(
									m_vecColumnsNames[index],
									TrekResources::SmallBoldFont(),
									WinPoint(x-xprev,p.Y()),
									JustifyLeft());
			s1->SetOpaque(true);
			s1->SetTextColor(Color::White());
			s2->SetOpaque(true);
			s2->SetTextColor(Color::Yellow());

			pbuttonbar->InsertButton(CreateButton(CreateButtonFacePane(s1,s2)),index);

			xprev = x;
		}
    
        pbuttonbar->GetMouseEnterWhileEnabledEventSource()->AddSink(new SoundIDEventSink(mouseoverSound));
        pbuttonbar->GetEventSource()->AddSink(new SoundIDEventSink(mouseclickSound));

        return pbuttonbar;
    }
};

//////////////////////////////////////////////////////////////////////////////
//
// Misc
//
//////////////////////////////////////////////////////////////////////////////

void ExportPaneFactories(INameSpace* pns)
{
    //
    // Display sides
    //

    pns->AddMember("SideLeft",        new Number((float)ATLeft       ));
    pns->AddMember("SideTopLeft",     new Number((float)ATTopLeft    ));
    pns->AddMember("SideTop",         new Number((float)ATTop        ));
    pns->AddMember("SideTopRight",    new Number((float)ATTopRight   ));
    pns->AddMember("SideRight",       new Number((float)ATRight      ));
    pns->AddMember("SideBottomRight", new Number((float)ATBottomRight));
    pns->AddMember("SideBottom",      new Number((float)ATBottom     ));
    pns->AddMember("SideBottomLeft",  new Number((float)ATBottomLeft ));
    pns->AddMember("SideCenter",      new Number((float)ATCenter     ));

    // add our 'primitive' panes
    pns->AddMember("StringPane",   new StringPaneFactory());
    pns->AddMember("GaugePane",    new GaugePaneFactory());
    pns->AddMember("SwitchPane",   new SwitchPaneFactory());
    pns->AddMember("ButtonPane",   new ButtonPaneFactory(GetModeler()));
    pns->AddMember("SoundButtonPane",   new SoundButtonPaneFactory(GetModeler()));
    pns->AddMember("ButtonBarPane",new ButtonBarPaneFactory(GetModeler()));
    pns->AddMember("ImageComboPane", new ImageComboPaneFactory(GetModeler(), GetWindow()->GetPopupContainer(), GetEngineWindow()->GetMousePositionModifiable()));
    pns->AddMember("StringComboPane", new StringComboPaneFactory(GetModeler(), GetWindow()->GetPopupContainer(), GetEngineWindow()->GetMousePositionModifiable()));
    pns->AddMember("StringColorComboPane", new StringColorComboPaneFactory(GetModeler(), GetWindow()->GetPopupContainer(), GetEngineWindow()->GetMousePositionModifiable()));

    // add our 'specialty' panes
#ifndef MARKCU1
    pns->AddMember("SectorMapPane",   new SectorMapPaneFactory());
    pns->AddMember("SectorInfoPane",  new SectorInfoPaneFactory());
    pns->AddMember("PartInfoPane",  new PartInfoPaneFactory());
    pns->AddMember("PurchasesPane",  new PurchasesPaneFactory());
    pns->AddMember("SelectionPane",  new SelectionPaneFactory());
    pns->AddMember("SelectModelPane",  new SelectModelPaneFactory());
    pns->AddMember("ChatListPane",  new ChatListPaneFactory());
    pns->AddMember("InventoryImage",  new InventoryImageFactory());
#endif

	// KGJV #62 - Map preview pane
	pns->AddMember("MapPreviewPane", new MapPreviewPaneFactory(GetModeler()));

	// KGJV #114 - TextButtonBarPane
    pns->AddMember("TextButtonBarPane",new TextButtonBarPaneFactory(GetModeler()));

}
