#ifndef _efpane_H_
#define _efpane_H_

#include <pane.h>
#include <tref.h>
#include <value.h>
#include <ztime.h>

//////////////////////////////////////////////////////////////////////////////
//
// effect panes
//
//////////////////////////////////////////////////////////////////////////////

class Modeler;
class IPopupContainer;
void AddPaneFactories(
    INameSpace*      pns,
    Modeler*         pmodeler,
    IPopupContainer* ppopupContainer,
    Number*          ptime
);

//////////////////////////////////////////////////////////////////////////////
//
// Black background 
//
//////////////////////////////////////////////////////////////////////////////

TRef<Pane> CreateBlackPane(Pane* ppane);

//////////////////////////////////////////////////////////////////////////////
//
// Hover Site
//
//////////////////////////////////////////////////////////////////////////////

class HoverSite : public IObject {
public:
    virtual void    Enter(float id) = 0;
    virtual void    Leave(float id) = 0;
    virtual Number* GetID()         = 0;
};

TRef<HoverSite> CreateHoverSite(float id);
TRef<Pane>      CreateHoverPane(HoverSite* psite, float id, Pane* ppane);

//////////////////////////////////////////////////////////////////////////////
//
// ThumbPane
//
//////////////////////////////////////////////////////////////////////////////
class Image;
class ThumbPane : public Pane {
public:
    static TRef<ThumbPane> Create(Modeler* pmodeler, 
        bool bHorizontal,        // false == vertical
        TRef<Image> pImageThumb // Set this to NULL for default image
        );
};

//////////////////////////////////////////////////////////////////////////////
//
// Trek Scroll Bar
//
//////////////////////////////////////////////////////////////////////////////

class IIntegerEventSource;
class ScrollPane;
TRef<Pane> CreateTrekScrollPane(
    unsigned height,
    Modeler* pmodeler,
    TRef<IIntegerEventSource>& pevent,
    TRef<ScrollPane>& pscrollPane
);

/////////////////////////////////////////////////////////////////////////////
//
// Gauge Pane
//
/////////////////////////////////////////////////////////////////////////////

class Surface;
class GaugePane : public Pane {
private:
    TRef<Surface>   m_psurface;

    float           m_minValue;
    float           m_maxValue;

    int             m_value;
    int             m_valueOld;
    int             m_valueFlash;

    Time            m_timeLastChange;

    Color           m_colorFlash;
    Color           m_colorEmpty;

    void Paint(Surface* psurface);

public:
    GaugePane(Surface*         psurface,
              const Color&     colorFlash = Color::Red(),
              float            minValue = 0.0f,
              float            maxValue = 1.0f,
              const Color&     colorEmpty = Color::Black());

    void SetValue(float v, bool fFlash = true);
    void Update(Time time);
};

void ParseIntVector(IObject* pobject, TVector<int>& vec);
void ParseFloatVector(IObject* pobject, TVector<int>& vec);
void ParseStringVector(IObject* pobject, TVector<ZString>& vec);

#endif
