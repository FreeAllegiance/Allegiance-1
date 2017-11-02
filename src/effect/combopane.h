#ifndef _combopane_h_
#define _combopane_h_

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ComboFacePane : public Pane {
public:
    virtual void SetString(const ZString& str) = 0;
    virtual void SetInside(bool bInside)       = 0;
    virtual void SetColor(const Color& color)  = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////////////

class ComboPane : public Pane {
public:
    virtual TRef<IMenuItem>      AddItem(const ZString& str, int id) = 0;
    virtual TRef<IMenuItem>      AddItem(const ZString& str, int id, const Color& color) = 0;
    virtual void                 ClearContents()      = 0;
    virtual int                  GetSelection()       = 0;
    virtual const ZString *      GetSelectionString() = 0;
    virtual void                 SetSelection(int id) = 0;
    virtual IIntegerEventSource* GetEventSource()     = 0;
    virtual void                 SetEnabled(bool bEnabled) = 0;
    virtual IEventSource*        GetMouseEnterWhileEnabledEventSource() = 0;
    virtual IEventSource*        GetMenuSelectEventSource() = 0;
};

//////////////////////////////////////////////////////////////////////////////
//
// Constructors
//
//////////////////////////////////////////////////////////////////////////////

TRef<ComboFacePane> CreateImageComboFacePane(Image* pimage);

TRef<ComboFacePane> CreateStringComboFacePane(
    const WinPoint&    size,
          IEngineFont* pfont,
    const Color&       color,
          bool         bBackgroundColor
);

TRef<ComboPane> CreateComboPane(
    Modeler*         pmodeler,
    IPopupContainer* ppopupContainer,
    IEngineFont*     pfont,
    const WinPoint&  size,
    ComboFacePane*   pface,
    ModifiablePointValue* pmousePosition
);

#endif
